//***********************************************************************//
//*                                                                     *//
//*                      Copyright © 2022 AMI                           *//
//*                                                                     *//
//*        All rights reserved. Subject to AMI licensing agreement.     *//
//*                                                                     *//
//***********************************************************************//
#include "Smbus_mailbox.h"
#include <Common.h>
#include "Definition.h"
#ifdef CONFIG_INTEL_PFR_SUPPORT
#include "intel_2.0/intel_pfr_pfm_manifest.h"
#include "intel_2.0/intel_pfr_definitions.h"
#include "intel_2.0/intel_pfr_provision.h"
#endif
#ifdef CONFIG_CERBERUS_PFR_SUPPORT
#include "cerberus/cerberus_pfr_pfm_manifest.h"
#include "cerberus/cerberus_pfr_definitions.h"
#include "cerberus/cerberus_pfr_provision.h"
#endif

#if SMBUS_MAILBOX_DEBUG
#define DEBUG_PRINTF printk
#else
#define DEBUG_PRINTF(...)
#endif

#define READ_ONLY_RF_COUNT  20
#define READ_WRITE_RF_COUNT 06

#define PRIMARY_FLASH_REGION    1
#define SECONDARY_FLASH_REGION  2

uint8_t gReadOnlyRfAddress[READ_ONLY_RF_COUNT] = { 0x1, 0x2, 0x3, 0x04, 0x05, 0x06, 0x07, 0x0A, 0x14, 0x15, 0x16, 0x17,
						   0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F };
uint8_t gReadAndWriteRfAddress[READ_WRITE_RF_COUNT] = { 0x08, 0x09, 0x0B, 0x0C, 0x0D, 0x0E };
extern struct st_pfr_instance pfr_instance;
EVENT_CONTEXT DataContext;

uint8_t gUfmFifoData[64];
uint8_t gReadFifoData[64];
uint8_t gRootKeyHash[32];
uint8_t gPchOffsets[12];
uint8_t gBmcOffsets[12];
uint8_t gUfmFifoLength;
uint8_t gbmcactivesvn;
uint8_t gbmcactiveMajorVersion;
uint8_t gbmcActiveMinorVersion;
uint8_t gAcmBootDone = FALSE;
uint8_t gBiosBootDone = FALSE;
uint8_t gBmcBootDone = FALSE;
uint8_t gObbBootDone = FALSE;
uint8_t gWDTUpdate;
uint8_t gProvisinDoneFlag = FALSE;
// uint8_t MailboxBuffer[256]={0};
extern bool gBootCheckpointReceived;
extern uint32_t gMaxTimeout;
extern int gBMCWatchDogTimer;
extern int gPCHWatchDogTimer;
uint8_t gProvisionCount;
uint8_t gFifoData;
uint8_t gBmcFlag;
uint8_t gDataCount;
uint8_t gProvisionData;
CPLD_STATUS cpld_update_status;

EVENT_CONTEXT UpdateEventData;
AO_DATA UpdateActiveObject;


void ResetMailBox(void)
{
	memset(&gSmbusMailboxData, 0, sizeof(gSmbusMailboxData));
	set_provision_status(COMMAND_DONE);   // reset ufm status
	set_provision_commandTrigger(0x00);
}
/**
    Function to Erase th UFM

    @Param  NULL

    @retval NULL
**/
unsigned char erase_provision_flash(void)
{
	int status;
	struct SpiEngine *spi_flash = getSpiEngineWrapper();
	spi_flash->spi.device_id[0] = ROT_INTERNAL_INTEL_STATE;
	status = spi_flash->spi.base.sector_erase(&spi_flash->spi, 0);
	return status;
}
/**
    Function to Initialize Smbus Mailbox with default value

    @Param  NULL
    @retval NULL
 **/
void get_provision_data_in_flash(uint32_t addr, uint8_t *DataBuffer, uint32_t length)
{
	uint8_t status;
	struct SpiEngine *spi_flash = getSpiEngineWrapper();

	spi_flash->spi.device_id[0] = ROT_INTERNAL_INTEL_STATE; // Internal UFM SPI
	status = spi_flash->spi.base.read(&spi_flash->spi, addr, DataBuffer, length);

}

unsigned char set_provision_data_in_flash(uint8_t addr, uint8_t *DataBuffer, uint8_t DataSize)
{
	uint8_t status;
	uint8_t buffer[1024];
	struct SpiEngine *spi_flash = getSpiEngineWrapper();
	spi_flash->spi.device_id[0] = ROT_INTERNAL_INTEL_STATE;

	// Read Intel State
	status = spi_flash->spi.base.read(&spi_flash->spi, 0, buffer, sizeof(buffer) / sizeof(buffer[0]));

	if (status == Success) {
		status = erase_provision_flash();

		if (status == Success) {
			for (int i = addr; i < DataSize + addr; i++) {
				buffer[i] = DataBuffer[i - addr];
			}

			memcpy(buffer + addr, DataBuffer, DataSize);

			status = spi_flash->spi.base.write(&spi_flash->spi, 0, buffer, sizeof(buffer) / sizeof(buffer[0]));
			if (status != Success) 
				return Failure;
		}
	}

	spi_flash->spi.device_id[0] = ROT_INTERNAL_INTEL_STATE;
	status = spi_flash->spi.base.read(&spi_flash->spi, 0, buffer, sizeof(buffer) / sizeof(buffer[0]));

	return status;
}
void get_image_svn(uint8_t image_id, uint32_t address, uint8_t *SVN, uint8_t *MajorVersion, uint8_t *MinorVersion)
{
	uint8_t status;
	PFM_STRUCTURE Buffer;

	struct SpiEngine *spi_flash = getSpiEngineWrapper();

	spi_flash->spi.device_id[0] = image_id;         // Internal UFM SPI
	status = spi_flash->spi.base.read(&spi_flash->spi, address, &Buffer, sizeof(PFM_STRUCTURE));

	*SVN = Buffer.SVN;
	*MajorVersion = Buffer.MarjorVersion;
	*MinorVersion = Buffer.MinorVersion;
}

void InitializeSmbusMailbox(void)
{
	ResetMailBox();
	SetCpldIdentifier(0xDE);
	SetCpldReleaseVersion(CPLD_RELEASE_VERSION);
	uint8_t CurrentSvn = 0;

	// get root key hash
	get_provision_data_in_flash(ROOT_KEY_HASH, gRootKeyHash, SHA256_DIGEST_LENGTH);
	get_provision_data_in_flash(PCH_ACTIVE_PFM_OFFSET, gPchOffsets, sizeof(gPchOffsets));
	get_provision_data_in_flash(BMC_ACTIVE_PFM_OFFSET, gBmcOffsets, sizeof(gBmcOffsets));

	uint32_t UfmStatus;
	get_provision_data_in_flash(UFM_STATUS, &UfmStatus, sizeof(uint32_t) / sizeof(uint8_t));


	uint8_t bmc_provision_flag = 1 << BMC_OFFSET_PROVISION_FLAG;
	uint8_t pch_provision_flag = 1 << PCH_OFFSET_PROVISION_FLAG;
	uint8_t root_key_provision_flag = 1 << ROOT_KEY_HASH_PROVISION_FLAG;
	uint8_t provision_flag = root_key_provision_flag | bmc_provision_flag | pch_provision_flag;

	if ((UfmStatus & provision_flag) == 0) {
		set_provision_status(UFM_PROVISIONED);
	}

	if ((UfmStatus & pch_provision_flag) == 0) {
		uint8_t PCHActiveMajorVersion, PCHActiveMinorVersion;
		uint8_t PCHActiveSVN;
		uint32_t pch_pfm_address;

		memcpy(&pch_pfm_address, gPchOffsets, 4);
		pch_pfm_address += 1024;
		get_image_svn(PCH_SPI, pch_pfm_address, &PCHActiveSVN, &PCHActiveMajorVersion, &PCHActiveMinorVersion);
		SetPchPfmActiveSvn(PCHActiveSVN);
		SetPchPfmActiveMajorVersion(PCHActiveMajorVersion);
		SetPchPfmActiveMinorVersion(PCHActiveMinorVersion);

		uint8_t PCHRecoveryMajorVersion, PCHRecoveryMinorVersion;
		uint8_t PCHRecoverySVN;
		uint32_t pch_rec_address;
		memcpy(&pch_rec_address, gPchOffsets + 4, 4);
		pch_rec_address += 2048;
		get_image_svn(PCH_SPI, pch_rec_address, &PCHRecoverySVN, &PCHRecoveryMajorVersion, &PCHRecoveryMinorVersion);
		SetPchPfmRecoverSvn(PCHRecoverySVN);
		SetPchPfmRecoverMajorVersion(PCHRecoveryMajorVersion);
		SetPchPfmRecoverMinorVersion(PCHRecoveryMinorVersion);
	}
#if BMC_SUPPORT
	if ((UfmStatus & bmc_provision_flag) == 0) {
		uint8_t BMCActiveMajorVersion, BMCActiveMinorVersion;
		uint8_t BMCActiveSVN;
		uint32_t bmc_pfm_address;

		memcpy(&bmc_pfm_address, gBmcOffsets, 4);
		bmc_pfm_address += 1024;
		get_image_svn(BMC_SPI, bmc_pfm_address, &BMCActiveSVN, &BMCActiveMajorVersion, &BMCActiveMinorVersion);
		SetBmcPfmActiveSvn(BMCActiveSVN);
		SetBmcPfmActiveMajorVersion(BMCActiveMajorVersion);
		SetBmcPfmActiveMinorVersion(BMCActiveMinorVersion);

		uint8_t BMCRecoveryMajorVersion, BMCRecoveryMinorVersion;
		uint8_t BMCRecoverySVN;
		uint32_t bmc_rec_address;
		memcpy(&bmc_rec_address, gBmcOffsets + 4, 4);
		bmc_rec_address += 2048;
		get_image_svn(BMC_SPI, bmc_rec_address, &BMCRecoverySVN, &BMCRecoveryMajorVersion, &BMCRecoveryMinorVersion);
		SetBmcPfmRecoverSvn(BMCRecoverySVN);
		SetBmcPfmRecoverMajorVersion(BMCRecoveryMajorVersion);
		SetBmcPfmRecoverMinorVersion(BMCRecoveryMinorVersion);
	}
#endif

	uint8_t current_svn;
	current_svn = get_ufm_svn(NULL, SVN_POLICY_FOR_CPLD_UPDATE);

	// CurrentSvn = Get_Ufm_SVN_Number(SVN_POLICY_FOR_CPLD_UPDATE);
	SetCpldRotSvn(current_svn);
}

void SetCpldIdentifier(byte Data)
{
	gSmbusMailboxData.CpldIdentifier = Data;
	////UpdateMailboxRegisterFile(CpldIdentifier,(uint8_t)gSmbusMailboxData.CpldIdentifier);
}
byte GetCpldIdentifier(void)
{
	return gSmbusMailboxData.CpldIdentifier;
}

void SetCpldReleaseVersion(byte Data)
{
	gSmbusMailboxData.CpldReleaseVersion = Data;
	////UpdateMailboxRegisterFile(CpldReleaseVersion,(uint8_t)gSmbusMailboxData.CpldReleaseVersion);
}
byte GetCpldReleaseVersion(void)
{
	return gSmbusMailboxData.CpldReleaseVersion;
}

void SetCpldRotSvn(byte Data)
{
	gSmbusMailboxData.CpldRoTSVN = Data;
	////UpdateMailboxRegisterFile(CpldRoTSVN,(uint8_t)gSmbusMailboxData.CpldRoTSVN);
}
byte GetCpldRotSvn(void)
{
	return gSmbusMailboxData.CpldRoTSVN;
}

byte GetPlatformState(void)
{
	return gSmbusMailboxData.PlatformState;
}

void SetPlatformState(byte PlatformStateData)
{
	gSmbusMailboxData.PlatformState = PlatformStateData;
	////UpdateMailboxRegisterFile(PlatformState, (uint8_t)gSmbusMailboxData.PlatformState);
}

byte GetRecoveryCount(void)
{
	return gSmbusMailboxData.Recoverycount;
}

void IncRecoveryCount(void)
{
	gSmbusMailboxData.Recoverycount++;
	////UpdateMailboxRegisterFile(Recoverycount, (uint8_t)gSmbusMailboxData.Recoverycount);
}

byte GetLastRecoveryReason(void)
{
	return gSmbusMailboxData.LastRecoveryReason;
}

void SetLastRecoveryReason(LAST_RECOVERY_REASON_VALUE LastRecoveryReasonValue)
{
	gSmbusMailboxData.LastRecoveryReason = LastRecoveryReasonValue;
	////UpdateMailboxRegisterFile(LastRecoveryReason, (uint8_t)gSmbusMailboxData.LastRecoveryReason);
}

byte GetPanicEventCount(void)
{
	return gSmbusMailboxData.PanicEventCount;
}

void IncPanicEventCount(void)
{
	gSmbusMailboxData.PanicEventCount++;
	//UpdateMailboxRegisterFile(PanicEventCount, (uint8_t)gSmbusMailboxData.PanicEventCount);
}

byte GetLastPanicReason(void)
{
	return gSmbusMailboxData.LastPanicReason;
}

void SetLastPanicReason(LAST_PANIC_REASON_VALUE LastPanicReasonValue)
{
	gSmbusMailboxData.LastPanicReason = LastPanicReasonValue;
	// UpdateMailboxRegisterFile(LastPanicReason, (uint8_t)gSmbusMailboxData.LastPanicReason);
}

byte GetMajorErrorCode(void)
{
	return gSmbusMailboxData.MajorErrorCode;
}

void SetMajorErrorCode(MAJOR_ERROR_CODE_VALUE MajorErrorCodeValue)
{
	gSmbusMailboxData.MajorErrorCode = MajorErrorCodeValue;
	// UpdateMailboxRegisterFile(MajorErrorCode, (uint8_t)gSmbusMailboxData.MajorErrorCode);
}

byte GetMinorErrorCode(void)
{
	return gSmbusMailboxData.MinorErrorCode;
}

void SetMinorErrorCode(MINOR_ERROR_CODE_VALUE MinorErrorCodeValue)
{
	gSmbusMailboxData.MinorErrorCode = MinorErrorCodeValue;
	// UpdateMailboxRegisterFile(MinorErrorCode,(uint8_t)gSmbusMailboxData.MinorErrorCode);
}

// UFM Status
bool IsUfmStatusCommandBusy(void)
{
	return gSmbusMailboxData.CommandBusy ? true : false;
}

bool IsUfmStatusCommandDone(void)
{
	return gSmbusMailboxData.CommandDone ? true : false;
}

bool IsUfmStatusCommandError(void)
{
	return gSmbusMailboxData.CommandError ? true : false;
}

bool IsUfmStatusLocked(void)
{
	return gSmbusMailboxData.UfmLocked ? true : false;
}

bool IsUfmStatusUfmProvisioned(void)
{
	return gSmbusMailboxData.Ufmprovisioned ? true : false;
}

bool IsUfmStatusPitLevel1Enforced(void)
{
	return gSmbusMailboxData.PITlevel1enforced ? true : false;
}

bool IsUfmStatusPITL2CompleteSuccess(void)
{
	return gSmbusMailboxData.PITL2CompleteSuccess ? true : false;
}

byte get_provision_status(void)
{
	uint8_t UfmStatus = 0;
	UfmStatus = gSmbusMailboxData.UfmStatusValue;
	if (gProvisionCount == 3)
		gProvisinDoneFlag = TRUE;

	return UfmStatus;
}

void set_provision_status(byte UfmStatus)
{
	gSmbusMailboxData.UfmStatusValue = UfmStatus;
	// UpdateMailboxRegisterFile(UfmStatusValue, (uint8_t)gSmbusMailboxData.UfmStatusValue);
}

byte get_provision_command(void)
{
	uint8_t UfmCommandData = 0;

	if (gBmcFlag)
		UfmCommandData = gSmbusMailboxData.UfmCommand;
	/*else
		ReadFromMailbox(UfmCommand, &UfmCommandData);*/

	return UfmCommandData;
}

void set_provision_command(byte UfmCommandValue)
{
	gSmbusMailboxData.UfmCommand = UfmCommandValue;
	// UpdateMailboxRegisterFile(UfmCommand, (uint8_t)gSmbusMailboxData.UfmCommand);
}

void set_provision_commandTrigger(byte UfmCommandTrigger)
{
	gSmbusMailboxData.UfmCmdTriggerValue = UfmCommandTrigger;
	// UpdateMailboxRegisterFile(UfmCmdTriggerValue, (uint8_t)gSmbusMailboxData.UfmCmdTriggerValue);
}

byte get_provision_commandTrigger(void)
{
	return gSmbusMailboxData.UfmCmdTriggerValue;
}

byte GetBmcCheckPoint(void)
{
	return gSmbusMailboxData.BmcCheckpoint;
}

void SetBmcCheckPoint(byte BmcCheckpointData)
{
	// TODO Allow for updating from BMC Reset to Boot Complete
	gSmbusMailboxData.BmcCheckpoint = BmcCheckpointData;
	// UpdateMailboxRegisterFile(BmcCheckpoint, (uint8_t)gSmbusMailboxData.BmcCheckpoint);
}

byte GetBiosCheckPoint(void)
{
	return gSmbusMailboxData.BiosCheckpoint;
}

void SetBiosCheckPoint(byte BiosCheckpointData)
{

	gSmbusMailboxData.BiosCheckpoint = BiosCheckpointData;
	// UpdateMailboxRegisterFile(BiosCheckpoint, (uint8_t)gSmbusMailboxData.BiosCheckpoint);

}

// PCH UpdateIntent
bool IsPchUpdateIntentPCHActive(void)
{
	return gSmbusMailboxData.PchUpdateIntentPchActive ? true : false;
}

bool IsPchUpdateIntentPchRecovery(void)
{
	return gSmbusMailboxData.PchUpdateIntentPchrecovery ? true : false;
}

bool IsPchUpdateIntentCpldActive(void)
{
	return gSmbusMailboxData.PchUpdateIntentCpldActive ? true : false;
}

bool IsPchUpdateIntentCpldRecovery(void)
{
	return gSmbusMailboxData.PchUpdateIntentCpldRecovery ? true : false;
}

bool IsPchUpdateIntentBmcActive(void)
{
	return gSmbusMailboxData.PchUpdateIntentBmcActive ? true : false;
}

bool IsPchUpdateIntentBmcRecovery(void)
{
	return gSmbusMailboxData.PchUpdateIntentBmcRecovery ? true : false;
}

bool IsPchUpdateIntentUpdateDynamic(void)
{
	return gSmbusMailboxData.PchUpdateIntentUpdateDynamic ? true : false;
}

bool IsPchUpdateIntentUpdateAtReset(void)
{
	return gSmbusMailboxData.PchUpdateIntentUpdateAtReset ? true : false;
}

byte GetPchUpdateIntent(void)
{
	return gSmbusMailboxData.PchUpdateIntentValue;
}

void SetPchUpdateIntent(byte PchUpdateIntent)
{
	gSmbusMailboxData.PchUpdateIntentValue = PchUpdateIntent;
	// UpdateMailboxRegisterFile(PchUpdateIntentValue, (uint8_t)gSmbusMailboxData.PchUpdateIntentValue);

}

// BMC UpdateIntent
bool IsBmcUpdateIntentPchActive(void)
{
	return gSmbusMailboxData.BmcUpdateIntentPchActive ? true : false;
}

bool IsBmcUpdateIntentPchRecovery(void)
{
	return gSmbusMailboxData.BmcUpdateIntentPchrecovery ? true : false;
}

bool IsBmcUpdateIntentCpldActive(void)
{
	return gSmbusMailboxData.BmcUpdateIntentCpldActive ? true : false;
}

bool IsBmcUpdateIntentCpldRecovery(void)
{
	return gSmbusMailboxData.BmcUpdateIntentCpldRecovery ? true : false;
}

bool IsBmcUpdateIntentBmcActive(void)
{
	return gSmbusMailboxData.BmcUpdateIntentBmcActive ? true : false;
}

bool IsBmcUpdateIntentBmcRecovery(void)
{
	return gSmbusMailboxData.BmcUpdateIntentBmcRecovery ? true : false;
}

bool IsBmcUpdateIntentUpdateDynamic(void)
{
	return gSmbusMailboxData.BmcUpdateIntentUpdateDynamic ? true : false;
}

bool IsBmcUpdateIntentUpdateAtReset(void)
{
	return gSmbusMailboxData.BmcUpdateIntentUpdateAtReset ? true : false;
}

byte GetBmcUpdateIntent(void)
{
	return gSmbusMailboxData.BmcUpdateIntentValue;
}

void SetBmcUpdateIntent(byte BmcUpdateIntent)
{
	gSmbusMailboxData.BmcUpdateIntentValue = BmcUpdateIntent;
	// UpdateMailboxRegisterFile(BmcUpdateIntentValue, (uint8_t)gSmbusMailboxData.BmcUpdateIntentValue);
}

byte GetPchPfmActiveSvn(void)
{
	return gSmbusMailboxData.PchPFMActiveSVN;
}

void SetPchPfmActiveSvn(byte ActiveSVN)
{
	gSmbusMailboxData.PchPFMActiveSVN = ActiveSVN;
	// UpdateMailboxRegisterFile(PchPFMActiveSVN, (uint8_t)gSmbusMailboxData.PchPFMActiveSVN);
}

byte GetPchPfmActiveMajorVersion(void)
{
	return gSmbusMailboxData.PchPFMActiveMajorVersion;
}

void SetPchPfmActiveMajorVersion(byte ActiveMajorVersion)
{
	gSmbusMailboxData.PchPFMActiveMajorVersion = ActiveMajorVersion;
	// UpdateMailboxRegisterFile(PchPFMActiveMajorVersion, (uint8_t)gSmbusMailboxData.PchPFMActiveMajorVersion);
}

byte GetPchPfmActiveMinorVersion(void)
{
	return gSmbusMailboxData.PchPFMActiveMinorVersion;
}

void SetPchPfmActiveMinorVersion(byte ActiveMinorVersion)
{
	gSmbusMailboxData.PchPFMActiveMinorVersion = ActiveMinorVersion;
	// UpdateMailboxRegisterFile(PchPFMActiveMinorVersion, (uint8_t)gSmbusMailboxData.PchPFMActiveMinorVersion);
}

byte GetBmcPfmActiveSvn(void)
{
	return gSmbusMailboxData.BmcPFMActiveSVN;
}

void SetBmcPfmActiveSvn(byte ActiveSVN)
{
	gSmbusMailboxData.BmcPFMActiveSVN = ActiveSVN;
	// UpdateMailboxRegisterFile(BmcPFMActiveSVN, (uint8_t)gSmbusMailboxData.BmcPFMActiveSVN);
}

byte GetBmcPfmActiveMajorVersion(void)
{
	return gSmbusMailboxData.BmcPFMActiveMajorVersion;
}

void SetBmcPfmActiveMajorVersion(byte ActiveMajorVersion)
{
	gSmbusMailboxData.BmcPFMActiveMajorVersion = ActiveMajorVersion;
	// UpdateMailboxRegisterFile(BmcPFMActiveMajorVersion, (uint8_t)gSmbusMailboxData.BmcPFMActiveMajorVersion);
}

byte GetBmcPfmActiveMinorVersion(void)
{
	return gSmbusMailboxData.BmcPFMActiveMinorVersion;
}

void SetBmcPfmActiveMinorVersion(byte ActiveMinorVersion)
{
	gSmbusMailboxData.BmcPFMActiveMinorVersion = ActiveMinorVersion;
	// UpdateMailboxRegisterFile(BmcPFMActiveMinorVersion, (uint8_t)gSmbusMailboxData.BmcPFMActiveMinorVersion);
}

byte GetPchPfmRecoverSvn(void)
{
	return gSmbusMailboxData.PchPFMRecoverSVN;
}

void SetPchPfmRecoverSvn(byte RecoverSVN)
{
	gSmbusMailboxData.PchPFMRecoverSVN = RecoverSVN;
	// UpdateMailboxRegisterFile(PchPFMRecoverSVN, (uint8_t)gSmbusMailboxData.PchPFMRecoverSVN);
}

byte GetPchPfmRecoverMajorVersion(void)
{
	return gSmbusMailboxData.PchPFMRecoverMajorVersion;
}

void SetPchPfmRecoverMajorVersion(byte RecoverMajorVersion)
{
	gSmbusMailboxData.PchPFMRecoverMajorVersion = RecoverMajorVersion;
	// UpdateMailboxRegisterFile(PchPFMRecoverMajorVersion, (uint8_t)gSmbusMailboxData.PchPFMRecoverMajorVersion);
}

byte GetPchPfmRecoverMinorVersion(void)
{
	return gSmbusMailboxData.PchPFMRecoverMinorVersion;
}

void SetPchPfmRecoverMinorVersion(byte RecoverMinorVersion)
{
	gSmbusMailboxData.PchPFMRecoverMinorVersion = RecoverMinorVersion;
	// UpdateMailboxRegisterFile(PchPFMRecoverMinorVersion, (uint8_t)gSmbusMailboxData.PchPFMRecoverMinorVersion);
}

byte GetBmcPfmRecoverSvn(void)
{
	return gSmbusMailboxData.BmcPFMRecoverSVN;
}

void SetBmcPfmRecoverSvn(byte RecoverSVN)
{
	gSmbusMailboxData.BmcPFMRecoverSVN = RecoverSVN;
	// UpdateMailboxRegisterFile(BmcPFMRecoverSVN, (uint8_t)gSmbusMailboxData.BmcPFMRecoverSVN);
}

byte GetBmcPfmRecoverMajorVersion(void)
{
	return gSmbusMailboxData.BmcPFMRecoverMajorVersion;
}

void SetBmcPfmRecoverMajorVersion(byte RecoverMajorVersion)
{
	gSmbusMailboxData.BmcPFMRecoverMajorVersion = RecoverMajorVersion;
	// UpdateMailboxRegisterFile(BmcPFMRecoverMajorVersion, (uint8_t)gSmbusMailboxData.BmcPFMRecoverMajorVersion);
}

byte GetBmcPfmRecoverMinorVersion(void)
{
	return gSmbusMailboxData.BmcPFMRecoverMinorVersion;
}

void SetBmcPfmRecoverMinorVersion(byte RecoverMinorVersion)
{
	gSmbusMailboxData.BmcPFMRecoverMinorVersion = RecoverMinorVersion;
	// UpdateMailboxRegisterFile(BmcPFMRecoverMinorVersion, (uint8_t)gSmbusMailboxData.BmcPFMRecoverMinorVersion);
}

byte *GetCpldFpgaRotHash(void)
{
	uint8_t HashData[SHA256_DIGEST_LENGTH] = {0};
	memcpy(HashData, gSmbusMailboxData.CpldFPGARoTHash, SHA256_DIGEST_LENGTH);
	// add obb read code for bmc
	return gSmbusMailboxData.CpldFPGARoTHash;
}

void SetCpldFpgaRotHash(byte *HashData)
{
	memcpy(gSmbusMailboxData.CpldFPGARoTHash, HashData, SHA384_DIGEST_LENGTH);
	// UpdateMailboxRegisterFile(CpldFPGARoTHash, (uint8_t)gSmbusMailboxData.CpldFPGARoTHash);
}

byte *GetAcmBiosScratchPad(void)
{
	return gSmbusMailboxData.AcmBiosScratchPad;
}

void SetAcmBiosScratchPad(byte *AcmBiosScratchPad)
{
	memcpy(gSmbusMailboxData.AcmBiosScratchPad, AcmBiosScratchPad, 0x40);
}

#if 1

unsigned char ProvisionRootKeyHash(void)
{
	uint8_t Status;
	uint32_t UfmStatus;

	get_provision_data_in_flash(UFM_STATUS, &UfmStatus, sizeof(uint32_t) / sizeof(uint8_t));
	if (((UfmStatus & 1)) && ((UfmStatus & 2))) {
		Status = set_provision_data_in_flash(ROOT_KEY_HASH, gRootKeyHash, SHA256_DIGEST_LENGTH);
		if (Status == Success) {
			UfmStatus &= 0xFD;
			Status = set_provision_data_in_flash(UFM_STATUS, (uint8_t *)&UfmStatus, sizeof(uint32_t) / sizeof(uint8_t));
			return Success;
		} else {
			return Failure;
		}
	} else {
		printk("Unsupported error\n");
		return UnSupported;
	}
}

unsigned char ProvisionPchOffsets(void)
{
	uint8_t Status;
	uint32_t UfmStatus;

	get_provision_data_in_flash(UFM_STATUS, (uint8_t *)&UfmStatus, sizeof(UfmStatus));

	if (((UfmStatus & 1)) && ((UfmStatus & 4))) {

		Status = set_provision_data_in_flash(PCH_ACTIVE_PFM_OFFSET, gPchOffsets, sizeof(gPchOffsets));
		if (Status == Success) {
			DEBUG_PRINTF("Ps\r\n");
			UfmStatus &= 0xFB;
			Status = set_provision_data_in_flash(UFM_STATUS, (uint8_t *)&UfmStatus, sizeof(uint32_t) / sizeof(uint8_t));
		} else {
			DEBUG_PRINTF("PCH Offsets Provision failed...\r\n");
			erase_provision_flash();
			set_provision_status(COMMAND_ERROR);
		}
		return Success;
	} else {
		return UnSupported;
	}
}

unsigned char ProvisionBmcOffsets(void)
{
	uint8_t Status;
	uint32_t UfmStatus;

	get_provision_data_in_flash(UFM_STATUS, (uint8_t *)&UfmStatus, sizeof(UfmStatus));

	if (((UfmStatus & 1)) && ((UfmStatus & 8))) {

		Status = set_provision_data_in_flash(BMC_ACTIVE_PFM_OFFSET, gBmcOffsets, sizeof(gBmcOffsets));
		if (Status == Success) {
			UfmStatus &= 0xF7;
			Status = set_provision_data_in_flash(UFM_STATUS, (uint8_t *)&UfmStatus, sizeof(UfmStatus));
			DEBUG_PRINTF("Bs\r\n");

		} else {
			DEBUG_PRINTF("BMC Offsets Provision failed...\r\n");
			erase_provision_flash();
			set_provision_status(COMMAND_ERROR);
		}
		return Success;
	} else {
		return UnSupported;
	}
}
#endif

void lock_provision_flash(void)
{
	uint8_t Status;
	uint32_t UfmStatus;

	get_provision_data_in_flash(UFM_STATUS, (uint8_t *)&UfmStatus, sizeof(UfmStatus));
	UfmStatus |= 1;

	set_provision_data_in_flash(UFM_STATUS, (uint8_t *)&UfmStatus, sizeof(UfmStatus));
}
void ReadRootKey(void)
{
	memcpy(gReadFifoData, gRootKeyHash, SHA256_DIGEST_LENGTH);
}

void ReadPchOfsets(void)
{
	memcpy(gReadFifoData, gPchOffsets, sizeof(gPchOffsets));
}

void ReadBmcOffets(void)
{
	memcpy(gReadFifoData, gBmcOffsets, sizeof(gBmcOffsets));
}
/**
    Function to process th UFM command operations

    @Param  NULL

    @retval NULL
 **/
void process_provision_command(void)
{
	byte UfmCommandData;
	byte UfmStatus;
	byte Status = 0;

	UfmStatus = get_provision_status();

	if (UfmStatus & UFM_LOCKED) {
		// Ufm locked
		DEBUG_PRINTF("UFM LOCKED\n\r");
		return;
	}

	UfmCommandData = get_provision_command();
	switch (UfmCommandData) {
	case ERASE_CURRENT:
		set_provision_status(COMMAND_BUSY);
		Status = erase_provision_flash();
		if (Status == Success) {
			gProvisionCount = 0;
			set_provision_status(COMMAND_DONE);
		} else
			set_provision_status(COMMAND_ERROR);
		break;
	case PROVISION_ROOT_KEY:
		set_provision_status(COMMAND_BUSY);
		memcpy(gRootKeyHash, gUfmFifoData, SHA256_DIGEST_LENGTH);
		gProvisionCount++;
		gProvisionData = 1;
		set_provision_status(COMMAND_DONE);

		break;
	case PROVISION_PIT_KEY:
		// Update password to provsioned UFM
		DEBUG_PRINTF("PIT IS NOT SUPPORTED\n\r");
		break;
	case PROVISION_PCH_OFFSET:
		set_provision_status(COMMAND_BUSY);
		memcpy(gPchOffsets, gUfmFifoData, sizeof(gPchOffsets));
		gProvisionCount++;
		gProvisionData = 1;
		set_provision_status(COMMAND_DONE);

		break;
	case PROVISION_BMC_OFFSET:
		set_provision_status(COMMAND_BUSY);
		memcpy(gBmcOffsets, gUfmFifoData, sizeof(gPchOffsets));
		gProvisionCount++;
		gProvisionData = 1;
		set_provision_status(COMMAND_DONE);

		break;
	case LOCK_UFM:
		// lock ufm
		lock_provision_flash();
		set_provision_status(COMMAND_DONE | UFM_LOCKED);
		break;
	case READ_ROOT_KEY:
		ReadRootKey();
		set_provision_status(COMMAND_DONE);
		break;
	case READ_PCH_OFFSET:
		ReadPchOfsets();
		set_provision_status(COMMAND_DONE);
		break;
	case READ_BMC_OFFSET:
		ReadBmcOffets();
		set_provision_status(COMMAND_DONE | UFM_PROVISIONED);
		// set_provision_status(COMMAND_DONE);
		break;

	case ENABLE_PIT_LEVEL_1_PROTECTION:
		// EnablePitLevel1();
		DEBUG_PRINTF("PIT IS NOT SUPPORTED\n\r");
		break;
	case ENABLE_PIT_LEVEL_2_PROTECTION:
		// EnablePitLevel2();
		DEBUG_PRINTF("PIT IS NOT SUPPORTED\n\r");
		break;
	}
	if ((gProvisionCount == 3) && (gProvisionData == 1)) {
		set_provision_status(COMMAND_BUSY);
		// printk("Calling provisioing process..\n");
		gProvisionData = 0;
		Status = ProvisionRootKeyHash();
		if (Status != Success) {
			set_provision_status(COMMAND_ERROR);
			return;
		}

		Status = ProvisionPchOffsets();
		if (Status != Success) {
			set_provision_status(COMMAND_ERROR);
			return;
		}

		Status = ProvisionBmcOffsets();
		if (Status != Success) {
			printk("Status: %x\n", Status);
			set_provision_status(COMMAND_ERROR);
			return;
		}

		set_provision_status(COMMAND_DONE | UFM_PROVISIONED);

		CPLD_STATUS cpld_status;
		ufm_read(UPDATE_STATUS_UFM, UPDATE_STATUS_ADDRESS, &cpld_status, sizeof(CPLD_STATUS));
		if (cpld_status.DecommissionFlag == TRUE) {
			cpld_status.DecommissionFlag = 0;
			ufm_write(UPDATE_STATUS_UFM, UPDATE_STATUS_ADDRESS, &cpld_status, sizeof(CPLD_STATUS));
		}
	}
}

/**
    Function to update the Bmc Checkpoint

    @Param  NULL

    @retval NULL
 **/
void UpdateBmcCheckpoint(byte Data)
{
	if (gBmcBootDone == FALSE) {
		// Start WDT for BMC boot
		gBmcBootDone = START;
		gBMCWatchDogTimer = 0;
		SetBmcCheckPoint(Data);
	} else {
		DEBUG_PRINTF("BMC boot completed.\r\n");
	}
	if (Data == PausingExecutionBlock) {
		printk("Enter PausingExecutionBlock Disable Timer \n");
		Tektagon_DisableTimer(BMC_EVENT);
	}
	if (Data == ResumedExecutionBlock) {
		Tektagon_EnableTimer(BMC_EVENT);
	}

	// BMC boot completed
	if (Data == CompletingexecutionBlock || Data == ReadToBootOS) {
		// If execution completed disable timer
		printk("Enter CompletingexecutionBlock Disable Timer \n");
		Tektagon_DisableTimer(BMC_EVENT);
		gBmcBootDone = TRUE;
		gBMCWatchDogTimer = -1;
		SetPlatformState(T0_BMC_BOOTED);
	}
	if (Data == AUTHENTICATION_FAILED) {
		gBmcBootDone = FALSE;
		gBMCWatchDogTimer = BMC_MAXTIMEOUT;
	}
	if (Data == RESET_BMC) {
		SetLastPanicReason(BMC_RESET_DETECT);
	}
	if (gBmcBootDone == TRUE && gBiosBootDone == TRUE) {
		SetPlatformState(T0_BOOT_COMPLETED);
	}
}

void UpdateBiosCheckpoint(byte Data)
{
	if (gBiosBootDone == TRUE) {
		if (Data == EXECUTION_BLOCK_STARTED) {
			gBiosBootDone = FALSE;
			gObbBootDone = TRUE;
		}
	}
	if (gBiosBootDone == FALSE) {
		if (Data == EXECUTION_BLOCK_STARTED) {
			// Set max time for BIOS boot & starts timer
			gMaxTimeout = BIOS_MAXTIMEOUT;
			gBootCheckpointReceived = false;
			gBiosBootDone = START;
			gPCHWatchDogTimer = 0;
		}
	}
	if (Data == PausingExecutionBlock) {
		Tektagon_DisableTimer(PCH_EVENT);
	}
	if (Data == ResumedExecutionBlock) {
		Tektagon_EnableTimer(PCH_EVENT);
	}
	// BIOS boot completed
	if (Data == CompletingexecutionBlock || Data == ReadToBootOS) {
		Tektagon_DisableTimer(PCH_EVENT);
		gBiosBootDone = TRUE;
		gBootCheckpointReceived = true;
		gPCHWatchDogTimer = -1;
		SetPlatformState(T0_BIOS_BOOTED);
		DEBUG_PRINTF("BIOS boot completed. \r\n");
	}
	if (Data == AUTHENTICATION_FAILED) {
		gBiosBootDone = FALSE;
		gPCHWatchDogTimer = gMaxTimeout;
		gBootCheckpointReceived = false;
		SetLastPanicReason(ACM_IBB_0BB_AUTH_FAIL);
	}
	if (gBmcBootDone == TRUE && gBiosBootDone == TRUE) {
		SetPlatformState(T0_BOOT_COMPLETED);
	}
	SetBiosCheckPoint(Data);
}

void PublishUpdateEvent(uint8_t ImageType, uint8_t FlashRegion)
{
	// Posting the Update signal
	UpdateActiveObject.type = ImageType;
	UpdateActiveObject.ProcessNewCommand = 1;

	UpdateEventData.operation = UPDATE_BACKUP;
	UpdateEventData.flash = FlashRegion;
	UpdateEventData.image = ImageType;

	if (post_smc_action(UPDATE, &UpdateActiveObject, &UpdateEventData)) {
		DEBUG_PRINTF("%s : event queue not available !\n", __FUNCTION__);
		return;
	}
}

void UpdateIntentHandle(byte Data, uint32_t Source)
{
	uint8_t Index;
	uint8_t PchActiveStatus;
	uint8_t BmcActiveStatus;

	DEBUG_PRINTF("\r\n Update Intent = 0x%x\r\n", Data);

	if (Data & UpdateAtReset) {
		// Getting cpld status from UFM

		ufm_read(UPDATE_STATUS_UFM, UPDATE_STATUS_ADDRESS, &cpld_update_status, sizeof(CPLD_STATUS));
		if (Data & PchActiveUpdate) {
			cpld_update_status.PchStatus = 1;
			cpld_update_status.Region[2].ActiveRegion = 1;
		}
		if (Data & PchRecoveryUpdate) {
			cpld_update_status.PchStatus = 1;
			cpld_update_status.Region[2].Recoveryregion = 1;
		}
		if (Data & PlatfireActiveUpdte) {
			cpld_update_status.CpldStatus = 1;
			cpld_update_status.CpldRecovery = 1;
			cpld_update_status.Region[0].ActiveRegion = 1;
		}
		if (Data & BmcActiveUpdate) {
			cpld_update_status.BmcStatus = 1;
			cpld_update_status.Region[1].ActiveRegion = 1;
		}
		if (Data & BmcRecoveryUpdate) {
			cpld_update_status.BmcStatus = 1;
			cpld_update_status.Region[1].Recoveryregion = 1;
		}
		if (Data & PlatfireRecoveryUpdate) {
			DEBUG_PRINTF("PlatfireRecoveryUpdate not supported\r\n");
		}
		if (Data & DymanicUpdate) {
			DEBUG_PRINTF("DymanicUpdate not supported\r\n");
		}
		// Setting updated cpld status to ufm
		ufm_write(UPDATE_STATUS_UFM, UPDATE_STATUS_ADDRESS, (uint8_t *)&cpld_update_status, sizeof(CPLD_STATUS));
	} else {
		if (Data & PchActiveUpdate) {
			if ((Data & PchActiveUpdate) && (Data & PchRecoveryUpdate)) {
				ufm_read(UPDATE_STATUS_UFM, UPDATE_STATUS_ADDRESS, &cpld_update_status, sizeof(CPLD_STATUS));
				cpld_update_status.PchStatus = 1;
				cpld_update_status.Region[2].ActiveRegion = 1;
				cpld_update_status.Region[2].Recoveryregion = 1;
				ufm_write(UPDATE_STATUS_UFM, UPDATE_STATUS_ADDRESS, (uint8_t *)&cpld_update_status, sizeof(CPLD_STATUS));
				PublishUpdateEvent(PCH_EVENT, PRIMARY_FLASH_REGION);
				return;
			} else {

				if (Source == BmcUpdateIntentValue) {
					ufm_read(UPDATE_STATUS_UFM, UPDATE_STATUS_ADDRESS, &cpld_update_status, sizeof(CPLD_STATUS));
					cpld_update_status.BmcToPchStatus = 1;
					ufm_write(UPDATE_STATUS_UFM, UPDATE_STATUS_ADDRESS, (uint8_t *)&cpld_update_status, sizeof(CPLD_STATUS));
				}

				PublishUpdateEvent(PCH_EVENT, PRIMARY_FLASH_REGION);
			}
		}

		if (Data & PchRecoveryUpdate) {
			if (Source == BmcUpdateIntentValue) {
				ufm_read(UPDATE_STATUS_UFM, UPDATE_STATUS_ADDRESS, &cpld_update_status, sizeof(CPLD_STATUS));
				cpld_update_status.BmcToPchStatus = 1;
				ufm_write(UPDATE_STATUS_UFM, UPDATE_STATUS_ADDRESS, &cpld_update_status, sizeof(CPLD_STATUS));
			}
			PublishUpdateEvent(PCH_EVENT, SECONDARY_FLASH_REGION);
		}
		if (Data & PlatfireActiveUpdte) {
			// cpld_update_status.CpldStatus = 0;
			// cpld_update_status.CpldRecovery = 1;
			// cpld_update_status.Region[0].ActiveRegion = 1;
			// set_provision_data_in_flash(UFM3, UFM3_CPLD_OFFSET, &cpld_update_status, sizeof(cpld_update_status));
			PublishUpdateEvent(HROT_TYPE, PRIMARY_FLASH_REGION);
			// RestartPlatFire();
		}
		if (Data & BmcActiveUpdate) {

			if ((Data & BmcActiveUpdate) && (Data & BmcRecoveryUpdate)) {
				// ufm_read(UPDATE_STATUS_UFM, UPDATE_STATUS_ADDRESS, &cpld_update_status, sizeof(CPLD_STATUS));
				// cpld_update_status.BmcStatus = 1;
				// cpld_update_status.Region[1].ActiveRegion = 1;
				// cpld_update_status.Region[1].Recoveryregion = 1;
				// ufm_write(UPDATE_STATUS_UFM, UPDATE_STATUS_ADDRESS, &cpld_update_status, sizeof(CPLD_STATUS));
				PublishUpdateEvent(BMC_EVENT, PRIMARY_FLASH_REGION);
				return;
			} else {
				PublishUpdateEvent(BMC_EVENT, PRIMARY_FLASH_REGION);
			}
		}
		if (Data & BmcRecoveryUpdate) {
			PublishUpdateEvent(BMC_EVENT, SECONDARY_FLASH_REGION);
		}
		if (Data & PlatfireRecoveryUpdate) {
			DEBUG_PRINTF("PlatfireRecoveryUpdate not supported\r\n");
		}
		if (Data & DymanicUpdate) {
			DEBUG_PRINTF("DymanicUpdate not supported\r\n");
		}
	}

}

/**
   Function to Check if system boots fine based on TimerISR
   Returns Status
 */
bool WatchDogTimer(int ImageType)
{
	DEBUG_PRINTF("WDT Update Tiggers\r\n");
	if (ImageType == PCH_EVENT) {
		gPCHWatchDogTimer = 0;
		gBiosBootDone = FALSE;
	}
	if (ImageType == BMC_EVENT) {
		gBMCWatchDogTimer = 0;
		gBmcBootDone = FALSE;
	}

	gBootCheckpointReceived = false;
	gWDTUpdate = 1;

	return true;
}

static unsigned int mailBox_index;
uint8_t PchBmcCommands(unsigned char *CipherText, uint8_t ReadFlag)
{

	byte DataToSend = 0;
	uint8_t i = 0;
	
	switch (CipherText[0]) {
	case CpldIdentifier:
		DataToSend = GetCpldIdentifier();
		break;
	case CpldReleaseVersion:
		DataToSend = GetCpldReleaseVersion();
		break;
	case CpldRoTSVN:
		DataToSend = GetCpldRotSvn();
		break;
	case PlatformState:
		DataToSend = GetPlatformState();
		break;
	case Recoverycount:
		DataToSend = GetRecoveryCount();
		break;
	case LastRecoveryReason:
		DataToSend = GetLastRecoveryReason();
		break;
	case PanicEventCount:
		DataToSend = GetPanicEventCount();
		break;
	case LastPanicReason:
		DataToSend = GetLastPanicReason();
		break;
	case MajorErrorCode:
		DataToSend = GetMajorErrorCode();
		break;
	case MinorErrorCode:
		DataToSend = GetMinorErrorCode();
		break;
	case UfmStatusValue:
		DataToSend = get_provision_status();
		break;
	case UfmCommand:
		if (ReadFlag == TRUE)
			DataToSend = get_provision_command();
		else
			set_provision_command(CipherText[1]);

		break;
	case UfmCmdTriggerValue:
		if (ReadFlag == TRUE) {
			DataToSend = get_provision_commandTrigger();
		} else {
			if (CipherText[1] & EXECUTE_UFM_COMMAND) {// If bit 0 set
				// Execute command specified at UFM/Provisioning Command register
				process_provision_command();
			} else if (CipherText[1] & FLUSH_WRITE_FIFO) {// Flush Write FIFO
				// Need to read UFM Write FIFO offest
				memset(&gUfmFifoData, 0, sizeof(gUfmFifoData));
				gFifoData = 0;
			} else if (CipherText[1] & FLUSH_READ_FIFO) {// flush Read FIFO
				// Need to read UFM Read FIFO offest
				memset(&gReadFifoData, 0, sizeof(gReadFifoData));
				gFifoData = 0;
				mailBox_index = 0;
			}
		}

		break;
	case UfmWriteFIFO:
		if (gFifoData <= MAX_FIFO_LENGTH - 1)
			gUfmFifoData[gFifoData++] = CipherText[1];

		break;
	case UfmReadFIFO:
		if (mailBox_index <= MAX_FIFO_LENGTH - 1) {
			DataToSend = gReadFifoData[mailBox_index];
			mailBox_index++;
		}

		break;
	case BmcCheckpoint:
		UpdateBmcCheckpoint(CipherText[1]);
		break;
	case AcmCheckpoint:
		// UpdateAcmCheckpoint(CipherText[1]);
		break;
	case BiosCheckpoint:
		UpdateBiosCheckpoint(CipherText[1]);
		break;
	case PchUpdateIntentValue:
		if (!ReadFlag) {
			SetPchUpdateIntent(CipherText[1]);
			UpdateIntentHandle(CipherText[1], PchUpdateIntentValue);
		}
		break;
	case BmcUpdateIntentValue:
		if (!ReadFlag) {
			SetBmcUpdateIntent(CipherText[1]);
			UpdateIntentHandle(CipherText[1], BmcUpdateIntentValue);
		}
		break;
	case PchPFMActiveSVN:
		DataToSend = GetPchPfmActiveSvn();
		break;
	case PchPFMActiveMajorVersion:
		DataToSend = GetPchPfmActiveMajorVersion();
		break;
	case PchPFMActiveMinorVersion:
		DataToSend = GetPchPfmActiveMinorVersion();
		break;
	case BmcPFMActiveSVN:
		DataToSend = GetBmcPfmActiveSvn();
		break;
	case BmcPFMActiveMajorVersion:
		DataToSend = GetBmcPfmActiveMajorVersion();
		break;
	case BmcPFMActiveMinorVersion:
		DataToSend = GetBmcPfmActiveMinorVersion();
		break;
	case PchPFMRecoverSVN:
		DataToSend = GetPchPfmRecoverSvn();
		break;
	case PchPFMRecoverMajorVersion:
		DataToSend = GetPchPfmRecoverMajorVersion();
		break;
	case PchPFMRecoverMinorVersion:
		DataToSend = GetPchPfmRecoverMinorVersion();
		break;
	case BmcPFMRecoverSVN:
		DataToSend = GetBmcPfmRecoverSvn();
		break;
	case BmcPFMRecoverMajorVersion:
		DataToSend = GetBmcPfmRecoverMajorVersion();
		break;
	case BmcPFMRecoverMinorVersion:
		DataToSend = GetBmcPfmRecoverMinorVersion();
		break;
	case CpldFPGARoTHash:
		break;
	case AcmBiosScratchPad:
		break;
	case BmcScratchPad:
		break;
	default:
		DEBUG_PRINTF("Mailbox command not found\r\n");
		break;
	}

	return DataToSend;
}

