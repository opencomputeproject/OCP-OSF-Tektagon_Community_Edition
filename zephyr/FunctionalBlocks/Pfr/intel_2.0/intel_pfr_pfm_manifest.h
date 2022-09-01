//***********************************************************************//
//*                                                                     *//
//*                      Copyright © 2022 AMI                           *//
//*                                                                     *//
//*        All rights reserved. Subject to AMI licensing agreement.     *//
//*                                                                     *//
//***********************************************************************//

#ifndef INTEL_PFR_PFM_VERIFICATION_H_
#define INTEL_PFR_PFM_VERIFICATION_H_

#include <stdint.h>

#pragma pack(1)

#define NUM_WHITESPACE 8

typedef struct PFMSPIDEFINITION
{
    uint8_t PFMDefinitionType;
    struct
    {
        uint8_t ReadAllowed : 1;
        uint8_t WriteAllowed :1;
        uint8_t RecoverOnFirstRecovery : 1;
        uint8_t RecoverOnSecondRecovery : 1;
        uint8_t RecoverOnThirdRecovery : 1;
        uint8_t Reserved : 3;
    }ProtectLevelMask;
    struct
    {
        uint16_t SHA256HashPresent : 1;
        uint16_t SHA384HashPresent :1;
        uint16_t Reserved : 14;
    }HashAlgorithmInfo;
    uint32_t Reserved;
    uint32_t RegionStartAddress;
    uint32_t RegionEndAddress;
}PFM_SPI_DEFINITION;

typedef enum {
    manifest_success,
    manifest_failure,
    manifest_unsupported
}Manifest_Status;

typedef struct _PFM_SPI_REGION
{
    uint8_t PfmDefType;
    uint8_t ProtectLevelMask;
    struct {
        uint16_t Sha256Present :1;
        uint16_t Sha384Present :1;
        uint16_t Reserved :14;
    }HashAlgorithmInfo;
    uint32_t Reserved;
    uint32_t StartOffset;
    uint32_t EndOffset;
}PFM_SPI_REGION;



typedef struct _PFM_STRUCTURE_1
{
    uint32_t PfmTag;
    uint8_t  SVN;
    uint8_t  BkcVersion;
    uint16_t PfmRevision;
    uint32_t Reserved;
    uint8_t  OemSpecificData[16];
    uint32_t Length;
}PFM_STRUCTURE_1;

typedef struct _FVM_STRUCTURE
{
	uint32_t FvmTag;
	uint8_t  SVN;
	uint8_t  Reserved;
	uint16_t FvmRevision;
	uint16_t Reserved1;
	uint16_t FvType;
	uint8_t  OemSpecificData[16];
	uint32_t Length;
}FVM_STRUCTURE;

typedef struct _PFM_SMBUS_RULE{
	uint8_t PFMDefinitionType;
	uint32_t Reserved;
	uint8_t BusId;
	uint8_t RuleID;
	uint8_t DeviceAddress;
	uint8_t CmdPasslist[32];
}PFM_SMBUS_RULE;

typedef struct _PFM_FVM_ADDRESS_DEFINITION
{
	uint8_t PFMDefinitionType;
	uint16_t FVType;
	uint8_t Reserved[5];
	uint32_t FVMAddress;
}PFM_FVM_ADDRESS_DEFINITION;

typedef struct _FVM_CAPABILITIES
{
	uint8_t FvmDefinition;
	uint16_t  Reserved1;
	uint8_t  Revision;
	uint16_t Size;
	uint32_t PckgVersion;
	uint32_t LayoutId;
	struct {
		uint32_t Reboot : 1;
		uint32_t Reserved : 31;
	}UpdateAction;
	uint8_t  Reserved2[26];
	uint8_t Description[20];
}FVM_CAPABLITIES;

#define SHA384_SIZE 48
#define SHA256_SIZE 32

#define BIOS1_BIOS2 0x00
#define ME_SPS		0x01
#define Microcode1	0x02
#define Microcode2 	0x03

#define SPI_REGION 0x1
#define SMBUS_RULE 0x2
#define SIZE_OF_PCH_SMBUS_RULE 40
#define PCH_PFM_SPI_REGION 0x01
#define ACTIVE_PFM_SMBUS_RULE 0x02
#define PCH_PFM_FVM_ADDRESS_DEFINITION 0x03

#define PCH_FVM_SPI_REGION 0x01
#define PCH_FVM_Capabilities 0x04

typedef struct{
	uint8_t Calculated : 1;
	uint8_t Count : 2;
	uint8_t RecoveredCount : 2;
	uint8_t DynamicEraseTriggered : 1;
	uint8_t Reserved : 2;
}ProtectLevelMask;

extern uint32_t g_manifest_length;
extern uint32_t g_fvm_manifest_length;

extern ProtectLevelMask pch_protect_level_mask_count;
extern ProtectLevelMask bmc_protect_level_mask_count;

// int pfm_spi_region_verification(struct pfr_manifest *manifest);
// Manifest_Status get_pfm_manifest_data(struct pfr_manifest *manifest, uint32_t *position,void *spi_definition, uint8_t *pfm_spi_hash, uint8_t pfm_definition);

#pragma pack()


#endif /*INTEL_PFR_PFM_VERIFICATION_H_*/