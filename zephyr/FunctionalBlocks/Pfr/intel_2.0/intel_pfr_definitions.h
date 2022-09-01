//***********************************************************************//
//*                                                                     *//
//*                      Copyright © 2022 AMI                           *//
//*                                                                     *//
//*        All rights reserved. Subject to AMI licensing agreement.     *//
//*                                                                     *//
//***********************************************************************//

#ifndef INTEL_PFR_DEFINITIONS_H_
#define INTEL_PFR_DEFINITIONS_H_

#define BMC_FLASH_ID 				0
#define PCH_FLASH_ID 				1

#define BMC_TYPE 0
#define PCH_TYPE 1

#define UFM0         	4
#define UFM0_SIZE       256

#define UFM1 			3

#define FALSE				 0
#define TRUE				 1
#define START				 2

#define PROVISION_UFM UFM0
#define PROVISION_UFM_SIZE UFM0_SIZE

#define UPDATE_STATUS_UFM UFM1
#define UPDATE_STATUS_ADDRESS 0x00

//Debug configuration token
#define PF_STATUS_DEBUG             1
#define PF_VERIFY_DEBUG             1
#define PF_UPDATE_DEBUG             1
#define PFR_AUTHENTICATION_DEBUG    1
#define HROT_STATE_DEBUG            1
#define SMBUS_MAILBOX_DEBUG         1
#define INTEL_MANIFEST_DEBUG        1
#define EVERY_BOOT_SVN_VALIDATION	1

#define BMC_SUPPORT                 1
#define EMULATION_SUPPORT			1
#define LOG_DEBUG					1
#define LOG_ENABLE					1
#define SMBUS_MAILBOX_SUPPORT       1
#define PFR_AUTO_PROVISION 			1
#define UART_ENABLE					1


//HROT FW version
#define CPLD_RELEASE_VERSION	1
#define CPLD_RoT_SVN			1

//BIOS/BMC SPI Region information
#define PCH_CAPSULE_STAGING_ADDRESS	0x007F0000
#define PCH_PFM_ADDRESS				0x02FF0000
#define PCH_FVM_ADDRESS				0x02FF1000
#define PCH_RECOVERY_AREA_ADDRESS	0x01BF0000
#define PCH_STAGING_SIZE 			0x01400000
#define BMC_STAGING_SIZE 			0x02000000
#define BMC_CAPSULE_STAGING_ADDRESS	0x04A00000
#define BMC_CPLD_STAGING_ADDRESS	0x07A00000
#define BMC_CPLD_RECOVERY_ADDRESS	0x07F00000
#define BMC_PFM_ADDRESS				0x029C0000
#define BMC_RECOVERY_AREA_ADDRESS	0x02A00000
#define	BMC_FLASH_SIZE				0x08000000
#define	PCH_FLASH_SIZE	 			0x04000000
#define BMC_CPLD_RECOVERY_SIZE	    0x100000

#define BMC_CAPSULE_PCH_STAGING_ADDRESS	0x06A00000
#define BMC_PCH_STAGING_SIZE 			0x01000000

#define COMPRESSION_TAG             0x5F504243
#define BLOCK0TAG                   0xB6EAFD19
#define BLOCK0_RSA_TAG         		0x35C6B783
#define BLOCK1TAG                   0xF27F28D7
#define BLOCK1_RSA_TAG           	0xD1550984
#define BLOCK1_ROOTENTRY_TAG        0xA757A046
#define BLOCK1_ROOTENTRY_RSA_TAG  	0x6CCDCAD7
#define BLOCK1CSKTAG                0x14711C2F
#define BLOCK1_BLOCK0ENTRYTAG       0x15364367
#define SIGNATURE_SECP256_TAG       0xDE64437D
#define SIGNATURE_SECP384_TAG       0xEA2A50E9
#define SIGNATURE_RSA2K_256_TAG		0xee728a05
#define SIGNATURE_RSA3K_384_TAG		0x9432a93e
#define SIGNATURE_RSA4K_384_TAG		0xF61B70F3
#define SIGNATURE_RSA4K_512_TAG		0x383c5144
#define PUBLIC_SECP256_TAG          0xC7B88C74
#define PUBLIC_SECP384_TAG          0x08F07B47
#define PUBLIC_RSA2K_TAG			0x7FAD5E14
#define PUBLIC_RSA3K_TAG			0x684694E6
#define PUBLIC_RSA4K_TAG			0xB73AA717
#define PEM_TAG                     0x02B3CE1D
#define PFM_SIG_BLOCK_SIZE          1024
#define PFM_SIG_BLOCK_SIZE_3K       3072
#define PFMTAG                      0x02B3CE1D
#define FVMTAG						0xA8E7C2D4
#define PFMTYPE                     0x01
#define UPDATE_CAPSULE              1
#define ACTIVE_PFM                  2
#define HROT_TYPE               3
#define LENGTH                      256
#define DECOMMISSION_PC_SIZE		128

#define  SIGN_PCH_PFM_BIT0          0x00000001
#define  SIGN_PCH_UPDATE_BIT1       0x00000002
#define  SIGN_BMC_PFM_BIT2          0x00000004
#define  SIGN_BMC_UPDATE_BIT3       0x00000008
#define  SIGN_CPLD_UPDATE_BIT4      0x00000010
#define  SIGN_CPLD_UPDATE_BIT9      0x00000200

#define SHA384_SIZE 48
#define SHA256_SIZE 32

#define SHA256_DIGEST_LENGTH    32
#define SHA384_DIGEST_LENGTH    48
#define SHA512_DIGEST_LENGTH    64
#define FOUR_BYTE_ADDR_MODE     1
#define SVN_MAX 				63
#define MAX_READ_SIZE 			0x1000
#define MAX_WRITE_SIZE 			0x1000
#define PAGE_SIZE               0x1000
#define UFM_PAGE_SIZE			16
#define CSK_KEY_SIZE			16
#define ROOT_KEY_SIZE			64
#define ROOT_KEY_X_Y_SIZE_256 		32
#define ROOT_KEY_X_Y_SIZE_384 		48

#define BLOCK_SUPPORT_1KB 1
#define BLOCK_SUPPORT_3KB 0

#define SMBUS_FILTER_IRQ_ENABLE     0x20
#define SMBUS_FILTER_IRQ_DISABLE    0x00
#define SMBUS_FILTER_ENCRYPTED_DATA_SIZE		64
#define NOACK_FLAG			0x3
#define ACTIVE_UFM PROVISION_UFM
#define ACTIVE_UFM_SIZE PROVISION_UFM_SIZE


#define  BIT0_SET          			0x00000001
#define  BIT1_SET			       	0x00000002
#define  BIT2_SET          			0x00000004
#define  BIT3_SET       			0x00000008
#define  BIT4_SET      				0x00000010
#define  BIT5_SET      				0x00000020
#define  BIT6_SET      				0x00000040
#define  BIT7_SET      				0x00000080 //Smbus filter disable/enable Reuest key permission
#define  BIT8_SET      				0x00000100 //Debug Request key permission

#define CSK_KEY \
	{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, \
	0xFF, 0xFF, 0xFF, 0xFF}

#define ROOTKEYHASH \
	{0x6b, 0xa4, 0xa6, 0x98, 0x53, 0x63, 0xf0, 0xe7, \
	 0xe9, 0x98, 0x76, 0x62, 0x7d, 0xe7, 0x12, 0x41, \
	 0xda, 0xab, 0x4b, 0x96, 0xbd, 0x67, 0x99, 0x82, \
	 0x81, 0x40, 0x27, 0x87, 0xa5, 0x10, 0x6e, 0x73}

enum Ecc_Curve {
    secp384r1 = 1,
    secp256r1,
};

typedef struct {
	uint8_t  ActiveRegion;
	uint8_t  Recoveryregion;
}UPD_REGION;

typedef struct{
    uint8_t CpldStatus;
    uint8_t BmcStatus;
    uint8_t PchStatus;
    UPD_REGION Region[3];
    uint8_t DecommissionFlag;
    uint8_t  CpldRecovery;
    uint8_t  BmcToPchStatus;
    uint8_t  Reserved[4];
}CPLD_STATUS;

#endif