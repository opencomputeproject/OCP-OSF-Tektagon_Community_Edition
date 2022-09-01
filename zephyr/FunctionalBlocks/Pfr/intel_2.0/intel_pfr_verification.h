//***********************************************************************//
//*                                                                     *//
//*                      Copyright © 2022 AMI                           *//
//*                                                                     *//
//*        All rights reserved. Subject to AMI licensing agreement.     *//
//*                                                                     *//
//***********************************************************************//

#ifndef INTEL_PFR_VERIFICATION_H_
#define INTEL_PFR_VERIFICATION_H

#include <stdint.h>
#include "pfr/pfr_common.h"
#include "crypto/hash.h"
#include "common/signature_verification.h"

#define INTEL_PFR_BLOCK_0_TAG 0xB6EAFD19

#define DECOMMISSION_CAPSULE		 0x200
#define KEY_CANCELLATION_CAPSULE	 0x300

#define FALSE				 0
#define TRUE				 1
#define START				 2
#define BLOCK0_PCTYPE_ADDRESS       8
#define CSK_KEY_ID_ADDRESS          160
#define CSK_KEY_ID_ADDRESS_3K		580
#define CSK_START_ADDRESS          	148
#define CSK_ENTRY_PC_SIZE	        128
#define KEY_SIZE					48
#define KEY_SIZE_3K					512
#define HROT_UPDATE_RESERVED	32
#define MAX_BIOS_BOOT_TIME			300
#define MAX_ERASE_SIZE 				0x1000


#define RANDOM_KEY_MAGIC_TAG					0x52414E44
#define RANDOM_KEY_ADDRESS_IN_UFM 				(PROVISION_UFM_SIZE * 16) - (2*SHA384_SIZE)
#define ROOT_PUB_KEY_LOC_IN_UFM					RANDOM_KEY_ADDRESS_IN_UFM - (2*SHA384_SIZE)
#define MAGIC_TAG_SIZE							4


#pragma pack(1)

typedef struct _PFR_AUTHENTICATION_BLOCK0{
    // Block 0
    uint32_t Block0Tag;
    uint32_t PcLength;
    uint32_t PcType;
    uint32_t Reserved1;
    uint8_t  Sha256Pc[32];
    uint8_t  Sha384Pc[48];
    uint8_t  Reserved2[32];
} PFR_AUTHENTICATION_BLOCK0;

typedef struct _PFR_AUTHENTICATION_BLOCK0_3K{
    // Block 0
    uint32_t Block0Tag;
    uint32_t PcLength;
    uint32_t PcType;
    uint32_t Reserved1;
    uint8_t  Sha256Pc[32];
    uint8_t  Sha384Pc[48];
    uint8_t  Sha512Pc[64];
    uint8_t  Reserved2[96];
} PFR_AUTHENTICATION_BLOCK0_3K;

typedef struct _RootEntry{
    uint32_t Tag;
    uint32_t PubCurveMagic;
    uint32_t KeyPermission;
    uint32_t KeyId;
    uint8_t  PubKeyX[KEY_SIZE];
    uint8_t  PubKeyY[KEY_SIZE];
    uint8_t  Reserved[20];
}KEY_ENTRY;


typedef struct _RootEntry_3k {
	uint32_t Tag;
	uint32_t PubCurveMagic;
	uint32_t KeyPermission;
	uint32_t KeyId;
	union {
		struct {
			uint8_t PubKeyX[KEY_SIZE];
			uint8_t PubKeyY[KEY_SIZE];
			uint8_t Reserved1[416];
		};
		uint8_t Modulus[KEY_SIZE_3K];
	};
	uint32_t PubKeyExp;
	uint8_t Reserved[20];
} KEY_ENTRY_3K;

//CSK Entry
typedef struct _CSKENTRY{
KEY_ENTRY CskEntryInitial;
uint32_t CskSignatureMagic;
uint8_t  CskSignatureR[KEY_SIZE];
uint8_t  CskSignatureS[KEY_SIZE];
}CSKENTRY;

//CSK Entry For 3K Block Structure
typedef struct _CSKENTRY_3K{
KEY_ENTRY_3K CskEntryInitial;
uint32_t CskSignatureMagic;
union {
	struct {
		uint8_t CskSignatureR[KEY_SIZE];
		uint8_t CskSignatureS[KEY_SIZE];
		uint8_t Reserved[416];
	};
	uint8_t Modulus[KEY_SIZE_3K];
};
}CSKENTRY_3K;

// Block 0 Entry
typedef struct _BLOCK0ENTRY{
uint32_t TagBlock0Entry;
uint32_t Block0SignatureMagic;
uint8_t  Block0SignatureR[KEY_SIZE];
uint8_t  Block0SignatureS[KEY_SIZE];
}BLOCK0ENTRY;

// Block 0 Entry for 3K Block Structure
typedef struct _BLOCK0ENTRY_3K{
uint32_t TagBlock0Entry;
uint32_t Block0SignatureMagic;
union {
	struct {
		uint8_t Block0SignatureR[KEY_SIZE];
		uint8_t Block0SignatureS[KEY_SIZE];
		uint8_t Reserved[416];
	};
	uint8_t Modulus[KEY_SIZE_3K];
};
}BLOCK0ENTRY_3K;

typedef struct _PFR_AUTHENTICATION_BLOCK1{
    // Block 1
    uint32_t TagBlock1;
    uint8_t  ReservedBlock1[12];
    //-----------Signature chain---------
    KEY_ENTRY   RootEntry; //132byte
    CSKENTRY    CskEntry;
    BLOCK0ENTRY Block0Entry;

}PFR_AUTHENTICATION_BLOCK1;

typedef struct _PFR_AUTHENTICATION_BLOCK1_3K{
    // Block 1
    uint32_t TagBlock1;
    uint8_t  ReservedBlock1[12];
    //-----------Signature chain---------
    KEY_ENTRY_3K   RootEntry; //132byte
    CSKENTRY_3K    CskEntry;
    BLOCK0ENTRY_3K Block0Entry;

}PFR_AUTHENTICATION_BLOCK1_3K;

struct key_entry_descriptor {
	uint32_t magic;
	uint32_t curve_magic;
	uint32_t permissions;
	uint32_t key_id;
	uint8_t public_key_x[48];
	uint8_t public_key_y[48];
	uint8_t reserved[20];
};

struct csk_entry_descriptor {
	struct key_entry_descriptor csk_entry_descriptor;
	uint32_t signature_magic;
	uint8_t signature_r[48];
	uint8_t signature_s[48];
};

struct block_0_entry {
	uint32_t magic;
	uint32_t signature_magic;
	uint8_t  signature_r[48];
	uint8_t  signature_s[48];
};

enum
{
    PFR_CPLD_UPDATE_CAPSULE = 0x00,
    PFR_PCH_PFM,
    PFR_PCH_UPDATE_CAPSULE,
    PFR_BMC_PFM,
    PFR_BMC_UPDATE_CAPSULE,
    PFR_PCH_CPU_Seamless_Update_Capsule,
    PFR_AFM,
    PFR_CPLD_UPDATE_CAPSULE_DECOMMISSON = 0x200
};

enum rsa_Curve {
	rsa2k = 3,
	rsa3k,
	rsa4k,
	rsa4k384,
	rsa4k512,
};

//Key Cancellation Enum
enum
{
    CPLD_CAPSULE_CANCELLATION = 0x100,
    PCH_PFM_CANCELLATION,
    PCH_CAPSULE_CANCELLATION,
    BMC_PFM_CANCELLATION,
    BMC_CAPSULE_CANCELLATION,
	SEAMLESS_CAPSULE_CANCELLATION
};

struct pfr_authentication{
    int (*validate_pctye)(struct pfr_manifest *manifest, uint32_t pc_type);
    int (*validate_kc)(struct pfr_manifest *manifest);
	int (*block1_block0_entry_verify)(struct pfr_manifest *manifest);
	int (*block1_csk_block0_entry_verify)(struct pfr_manifest *manifest);
    int (*block1_verify)(struct pfr_manifest *manifest);
	int (*block0_verify)(struct pfr_manifest *manifest);
	int (*validate_root_key);
};


int intel_pfr_manifest_verify(struct manifest *manifest, struct hash_engine *hash,
		struct signature_verification *verification, uint8_t *hash_out, uint32_t hash_length);

#endif /*INTEL_PFR_VERIFICATION_H*/