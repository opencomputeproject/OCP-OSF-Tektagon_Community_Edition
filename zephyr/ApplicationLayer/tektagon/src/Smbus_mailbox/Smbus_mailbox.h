//***********************************************************************//
//*                                                                     *//
//*                      Copyright © 2022 AMI                           *//
//*                                                                     *//
//*        All rights reserved. Subject to AMI licensing agreement.     *//
//*                                                                     *//
//***********************************************************************//

#ifndef PFR_SMBUS_MAILBOX_H_
#define PFR_SMBUS_MAILBOX_H_
#include <stdbool.h>
#include <stdint.h>
// #include <HrotStateMachine.h>
#include "include/SmbusMailBoxCom.h"
#include "state_machine/common_smc.h"
#include "StateMachineAction/StateMachineActions.h"

extern int systemState;
extern int gEventCount;
extern int gPublishCount;

typedef struct {
	int signal;
	void *context;
} TUPLE;

#pragma pack(1)

// enum HRoT_state { IDLE, I2C, VERIFY, RECOVERY, UPDATE, LOCKDOWN };

typedef char byte;
#define  BIT2     0x00000004
#define ACM_MAXTIMEOUT 50000
#define BMC_MAXTIMEOUT 175000
#define BIOS_MAXTIMEOUT 900000
// BMC I2c Commands
#define  LOGMESSAGE 0x05

/*uint8_t gUfmFifoData[32];
   uint8_t gReadFifoData[32];
   uint8_t gRootKeyHash[33];
   uint8_t gPchOffsets[13];
   uint8_t gBmcOffsets[13];*/
#define READ_ONLY 24
#define WRITE_ONLY 6

typedef struct _SMBUS_MAIL_BOX_ {
	byte CpldIdentifier;
	byte CpldReleaseVersion;
	byte CpldRoTSVN;
	byte PlatformState;
	byte Recoverycount;
	byte LastRecoveryReason;
	byte PanicEventCount;
	byte LastPanicReason;
	byte MajorErrorCode;
	byte MinorErrorCode;
	union {
		struct {
			unsigned char CommandBusy : 1;
			unsigned char CommandDone : 1;
			unsigned char CommandError : 1;
			unsigned char UfmStatusReserved : 1;
			unsigned char UfmLocked : 1;
			unsigned char Ufmprovisioned : 1;
			unsigned char PITlevel1enforced : 1;
			unsigned char PITL2CompleteSuccess : 1;
		};
		byte UfmStatusValue;
	};
	byte UfmCommand;
	union {
		struct {
			unsigned char ExecuteCmd : 1;
			unsigned char FlushWriteFIFO : 1;
			unsigned char FlushReadFIFO : 1;
			byte UfmCmdTriggerReserved : 4;
		};
		byte UfmCmdTriggerValue;
	};
	byte UfmWriteFIFO;
	byte UfmReadFIFO;
	byte BmcCheckpoint;
	byte AcmCheckpoint;
	byte BiosCheckpoint;
	union {
		struct {
			unsigned char PchUpdateIntentPchActive : 1;
			unsigned char PchUpdateIntentPchrecovery : 1;
			unsigned char PchUpdateIntentCpldActive : 1;
			unsigned char PchUpdateIntentBmcActive : 1;
			unsigned char PchUpdateIntentBmcRecovery : 1;
			unsigned char PchUpdateIntentCpldRecovery : 1;
			unsigned char PchUpdateIntentUpdateDynamic : 1;
			unsigned char PchUpdateIntentUpdateAtReset : 1;
		};
		byte PchUpdateIntentValue;
	};
	union {
		struct {
			unsigned char BmcUpdateIntentPchActive : 1;
			unsigned char BmcUpdateIntentPchrecovery : 1;
			unsigned char BmcUpdateIntentCpldActive : 1;
			unsigned char BmcUpdateIntentBmcActive : 1;
			unsigned char BmcUpdateIntentBmcRecovery : 1;
			unsigned char BmcUpdateIntentCpldRecovery : 1;
			unsigned char BmcUpdateIntentUpdateDynamic : 1;
			unsigned char BmcUpdateIntentUpdateAtReset : 1;
		};
		byte BmcUpdateIntentValue;
	};
	byte PchPFMActiveSVN;
	byte PchPFMActiveMajorVersion;
	byte PchPFMActiveMinorVersion;
	byte BmcPFMActiveSVN;
	byte BmcPFMActiveMajorVersion;
	byte BmcPFMActiveMinorVersion;
	byte PchPFMRecoverSVN;
	byte PchPFMRecoverMajorVersion;
	byte PchPFMRecoverMinorVersion;
	byte BmcPFMRecoverSVN;
	byte BmcPFMRecoverMajorVersion;
	byte BmcPFMRecoverMinorVersion;
	byte CpldFPGARoTHash[0x30];
	byte Reserved[0x20];
	byte AcmBiosScratchPad[0x40];
	byte BmcScratchPad[0x40];
} SMBUS_MAIL_BOX;

typedef enum _SMBUS_MAILBOX_RF_ADDRESS_READONLY {
	CpldIdentifier,
	CpldReleaseVersion,
	CpldRoTSVN,
	PlatformState,
	Recoverycount,
	LastRecoveryReason,
	PanicEventCount,
	LastPanicReason,
	MajorErrorCode,
	MinorErrorCode,
	UfmStatusValue,
	UfmCommand,
	UfmCmdTriggerValue,
	UfmWriteFIFO,
	UfmReadFIFO,
	BmcCheckpoint,
	AcmCheckpoint,
	BiosCheckpoint,
	PchUpdateIntentValue,
	BmcUpdateIntentValue,
	PchPFMActiveSVN,
	PchPFMActiveMajorVersion,
	PchPFMActiveMinorVersion,
	BmcPFMActiveSVN,
	BmcPFMActiveMajorVersion,
	BmcPFMActiveMinorVersion,
	PchPFMRecoverSVN,
	PchPFMRecoverMajorVersion,
	PchPFMRecoverMinorVersion,
	BmcPFMRecoverSVN,
	BmcPFMRecoverMajorVersion,
	BmcPFMRecoverMinorVersion,
	CpldFPGARoTHash,
	Reserved                = 0x63,
	AcmBiosScratchPad       = 0x80,
	BmcScratchPad           = 0xc0,
} SMBUS_MAILBOX_RF_ADDRESS;

typedef enum _EXECUTION_CHECKPOINT {
	ExecutionBlockStrat = 0x01,
	NextExeBlockAuthenticationPass,
	NextExeBlockAuthenticationFail,
	ExitingPlatformManufacturerAuthority,
	StartExternalExecutionBlock,
	ReturnedFromExternalExecutionBlock,
	PausingExecutionBlock,
	ResumedExecutionBlock,
	CompletingexecutionBlock,
	EnteredManagementMode,
	LeavingManagementMode,
	ReadToBootOS = 0x80
} EXECUTION_CHECKPOINT;

typedef enum _UPDATE_INTENT {
	PchActiveUpdate                         = 0x01,
	PchRecoveryUpdate,
	PchActiveAndRecoveryUpdate,
	PlatfireActiveUpdte,
	BmcActiveUpdate                         = 0x08,
	BmcRecoveryUpdate                       = 0x10,
	PlatfireRecoveryUpdate                  = 0x20,
	DymanicUpdate                           = 0x40,
	UpdateAtReset                           = 0x80,
	PchActiveAndRecoveryUpdateAtReset       = 0x83,
	PchActiveDynamicUpdate                  = 0x41,
	PchActiveAndDynamicUpdateAtReset        = 0xc1,
	PlatfireActiveAndRecoveryUpdate         = 0x24,
	BmcActiveAndRecoveryUpdate              = 0x18,
	PchActiveAndBmcActiveUpdate             = 0x09,
	PchRecoveryAndBmcRecvoeryUpdate         = 0x12,
	PchFwAndBmcFwUpdate                     = 0x1B,
	PchBmcPlatfireActiveAndRecoveryUpdate   = 0x3f,
	BmcActiveAndDynamicUpdate               = 0x48,
	ExceptBmcActiveUpdate                   = 0x37
} UPDATE_INTENT;

typedef struct _PFM_STRUCTURE {
	uint32_t PfmTag;
	uint8_t SVN;
	uint8_t BkcVersion;
	uint8_t MarjorVersion;
	uint8_t MinorVersion;
	uint32_t Reserved;
	uint8_t OemSpecificData[16];
	uint32_t Length;
} PFM_STRUCTURE;

static SMBUS_MAIL_BOX gSmbusMailboxData = { 0 };

unsigned char set_provision_data_in_flash(uint8_t addr, uint8_t *DataBuffer, uint8_t DataSize);
void get_provision_data_in_flash(uint32_t addr, uint8_t *DataBuffer, uint32_t length);
// void ReadFullUFM(uint32_t UfmId,uint32_t UfmLocation,uint8_t *DataBuffer, uint16_t DataSize);
unsigned char erase_provision_data_in_flash(void);
void GetUpdateStatus(uint8_t *DataBuffer, uint8_t DataSize);
void SetUpdateStatus(uint8_t *DataBuffer, uint8_t DataSize);

void ResetMailBox(void);
void InitializeSmbusMailbox(void);
void SetCpldIdentifier(byte Data);
byte GetCpldIdentifier(void);
void SetCpldReleaseVersion(byte Data);
byte GetCpldReleaseVersion(void);
void SetCpldRotSvn(byte Data);
byte GetCpldRotSvn(void);
byte GetPlatformState(void);
void SetPlatformState(byte PlatformStateData);
byte GetRecoveryCount(void);
void IncRecoveryCount(void);
byte GetLastRecoveryReason(void);
void SetLastRecoveryReason(LAST_RECOVERY_REASON_VALUE LastRecoveryReasonValue);
byte GetPanicEventCount(void);
void IncPanicEventCount(void);
byte GetLastPanicReason(void);
void SetLastPanicReason(LAST_PANIC_REASON_VALUE LastPanicReasonValue);
byte GetMajorErrorCode(void);
void SetMajorErrorCode(MAJOR_ERROR_CODE_VALUE MajorErrorCodeValue);
byte GetMinorErrorCode(void);
void SetMinorErrorCode(MINOR_ERROR_CODE_VALUE MinorErrorCodeValue);
bool IsUfmStatusCommandBusy(void);
bool IsUfmStatusCommandDone(void);
bool IsUfmStatusCommandError(void);
bool IsUfmStatusLocked(void);
bool IsUfmStatusUfmProvisioned(void);
bool IsUfmStatusPitLevel1Enforced(void);
bool IsUfmStatusPITL2CompleteSuccess(void);
byte get_provision_status(void);
void set_provision_status(byte UfmStatus);
byte get_provision_command(void);
void set_provision_command(byte UfmCommandValue);
void set_provision_commandTrigger(byte UfmCommandTrigger);
byte GetBmcCheckPoint(void);
void SetBmcCheckPoint(byte BmcCheckpoint);
byte GetBiosCheckPoint(void);
void SetBiosCheckPoint(byte BiosCheckpoint);
bool IsPchUpdateIntentPCHActive(void);
bool IsPchUpdateIntentPchRecovery(void);
bool IsPchUpdateIntentCpldActive(void);
bool IsPchUpdateIntentCpldRecovery(void);
bool IsPchUpdateIntentBmcActive(void);
bool IsPchUpdateIntentBmcRecovery(void);
bool IsPchUpdateIntentUpdateDynamic(void);
bool IsPchUpdateIntentUpdateAtReset(void);
byte GetPchUpdateIntent(void);
void SetPchUpdateIntent(byte PchUpdateIntent);
bool IsBmcUpdateIntentPchActive(void);
bool IsBmcUpdateIntentPchRecovery(void);
bool IsBmcUpdateIntentCpldActive(void);
bool IsBmcUpdateIntentCpldRecovery(void);
bool IsBmcUpdateIntentBmcActive(void);
bool IsBmcUpdateIntentBmcRecovery(void);
bool IsBmcUpdateIntentUpdateDynamic(void);
bool IsBmcUpdateIntentUpdateAtReset(void);
byte GetBmcUpdateIntent(void);
void SetBmcUpdateIntent(byte BmcUpdateIntent);
byte GetPchPfmActiveSvn(void);
void SetPchPfmActiveSvn(byte ActiveSVN);
byte GetPchPfmActiveMajorVersion(void);
void SetPchPfmActiveMajorVersion(byte ActiveMajorVersion);
byte GetPchPfmActiveMinorVersion(void);
void SetPchPfmActiveMinorVersion(byte ActiveMinorVersion);
byte GetBmcPfmActiveSvn(void);
void SetBmcPfmActiveSvn(byte ActiveSVN);
byte GetBmcPfmActiveMajorVersion(void);
void SetBmcPfmActiveMajorVersion(byte ActiveMajorVersion);
byte GetBmcPfmActiveMinorVersion(void);
void SetBmcPfmActiveMinorVersion(byte ActiveMinorVersion);
byte GetPchPfmRecoverSvn(void);
void SetPchPfmRecoverSvn(byte RecoverSVN);
byte GetPchPfmRecoverMajorVersion(void);
void SetPchPfmRecoverMajorVersion(byte RecoverMajorVersion);
byte GetPchPfmRecoverMinorVersion(void);
void SetPchPfmRecoverMinorVersion(byte RecoverMinorVersion);
byte GetBmcPfmRecoverSvn(void);
void SetBmcPfmRecoverSvn(byte RecoverSVN);
byte GetBmcPfmRecoverMajorVersion(void);
void SetBmcPfmRecoverMajorVersion(byte RecoverMajorVersion);
byte GetBmcPfmRecoverMinorVersion(void);
void SetBmcPfmRecoverMinorVersion(byte RecoverMinorVersion);
byte *GetCpldFpgaRotHash(void);
void SetCpldFpgaRotHash(byte *HashData);
byte *GetAcmBiosScratchPad(void);
void SetAcmBiosScratchPad(byte *AcmBiosScratchPad);
byte *GetBmcScratchPad(void);
void SetBmcScratchPad(byte *BmcScratchPad);
void HandleSmbusMailBoxWrite(unsigned char MailboxAddress, unsigned char ValueToWrite, int ImageType);
void HandleSmbusMailBoxRead(int MailboxOffset, int ImageType);
void process_provision_command(void);
void UpdateBiosCheckpoint(byte Data);
void UpdateBmcCheckpoint(byte Data);
void UpdateIntentHandle(byte Data, uint32_t Source);
bool WatchDogTimer(int ImageType);
uint8_t PchBmcCommands(unsigned char *CipherText, uint8_t ReadFlag);
void get_image_svn(uint8_t image_id, uint32_t address, uint8_t *SVN, uint8_t *MajorVersion, uint8_t *MinorVersion);

#define ROOT_KEY_HASH_PROVISION_FLAG 1
#define PCH_OFFSET_PROVISION_FLAG 2
#define BMC_OFFSET_PROVISION_FLAG 3

extern uint8_t gBiosBootDone;
extern uint8_t gBmcBootDone;

#endif /* PFR_SMBUS_MAILBOX_H_ */
