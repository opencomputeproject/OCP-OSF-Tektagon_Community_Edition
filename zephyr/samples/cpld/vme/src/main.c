#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr.h>
#include <device.h>
#include <shell/shell.h>
#include <drivers/jtag.h>
#include "vmopcode.h"
#include <portability/cmsis_os2.h>
#include "file/vme_file.h"

/***************************************************************
*
* External variables declared in hardware.c module.
*
***************************************************************/

extern const struct device *jtag_dev;
/***************************************************************
*
* External variables and functions declared in ivm_core.c module.
*
***************************************************************/
extern signed char ispVMCode();
extern void ispVMCalculateCRC32(unsigned char a_ucData);
extern void ispVMStart();
extern void ispVMEnd();
extern unsigned short g_usCalculatedCRC;
extern unsigned short g_usDataType;
extern unsigned char *g_pucOutMaskData,
		     *g_pucInData,
		     *g_pucOutData,
		     *g_pucHIRData,
		     *g_pucTIRData,
		     *g_pucHDRData,
		     *g_pucTDRData,
		     *g_pucOutDMaskData,
		     *g_pucIntelBuffer;
extern unsigned char *g_pucHeapMemory;
extern unsigned short g_iHeapCounter;
extern unsigned short g_iHEAPSize;
extern unsigned short g_usIntelDataIndex;
extern unsigned short g_usIntelBufferSize;
extern LVDSPair *g_pLVDSList;
// 08/28/08 NN Added Calculate checksum support.
extern unsigned long g_usChecksum;
extern unsigned int g_uiChecksumIndex;

/***************************************************************
*
* Functions declared in this main.c module
*
***************************************************************/
unsigned char GetByte(void);
void vme_out_char(unsigned char charOut);
void vme_out_hex(unsigned char hexOut);
void vme_out_string(char *stringOut);
void ispVMMemManager(signed char cTarget, unsigned short usSize);
void ispVMFreeMem(void);
signed char ispVM(void);

unsigned short g_usPreviousSize = 0;
unsigned long read_bytes;
unsigned short expectedCRC;

const char *const g_szSupportedVersions[] = {
	"__VME2.0", "__VME3.0", "____12.0", "____12.1", 0
};

int vme_getc(unsigned char *cData)
{
#if CONFIG_FIXED_VME_FILE
	uint32_t arry_index = read_bytes / 60000;
	uint32_t byte_index = read_bytes % 60000;

	if (arry_index == vmearrays && byte_index > g_pArraySizes[arry_index]) {
		return -1;
	}
	*cData = vmeArray[arry_index][byte_index];
#endif
	return 0;
}
/***************************************************************
*
* GetByte
*
* Returns a byte to the caller. The returned byte depends on the
* g_usDataType register. If the HEAP_IN bit is set, then the byte
* is returned from the HEAP. If the LHEAP_IN bit is set, then
* the byte is returned from the intelligent buffer. Otherwise,
* the byte is returned directly from the VME file.
*
***************************************************************/
unsigned char GetByte(void)
{
	unsigned char ucData;

	if (g_usDataType & HEAP_IN) {

		/*
		 * Get data from repeat buffer.
		 */

		if (g_iHeapCounter > g_iHEAPSize) {

			/*
			 * Data over-run.
			 */

			return 0xFF;
		}

		ucData = g_pucHeapMemory[g_iHeapCounter++];
	} else if (g_usDataType & LHEAP_IN) {

		/*
		 * Get data from intel buffer.
		 */

		if (g_usIntelDataIndex >= g_usIntelBufferSize) {
			return 0xFF;
		}

		ucData = g_pucIntelBuffer[g_usIntelDataIndex++];
	} else {
		if (vme_getc(&ucData)) {
			return 0xFF;
		}
		read_bytes++;

		if (expectedCRC != 0) {
			ispVMCalculateCRC32(ucData);
		}
	}

	return ucData;
}

/***************************************************************
*
* vme_out_char
*
* Send a character out to the output resource if available.
* The monitor is the default output resource.
*
*
***************************************************************/
void vme_out_char(unsigned char charOut)
{
	printf("%c", charOut);
}
/***************************************************************
*
* vme_out_hex
*
* Send a character out as in hex format to the output resource
* if available. The monitor is the default output resource.
*
*
***************************************************************/
void vme_out_hex(unsigned char hexOut)
{
	printf("%.2X", hexOut);
}
/***************************************************************
*
* vme_out_string
*
* Send a text string out to the output resource if available.
* The monitor is the default output resource.
*
*
***************************************************************/
void vme_out_string(char *stringOut)
{
	if (stringOut) {
		printf("%s", stringOut);
	}

}
/***************************************************************
*
* ispVMMemManager
*
* Allocate memory based on cTarget. The memory size is specified
* by usSize.
*
***************************************************************/

void ispVMMemManager(signed char cTarget, unsigned short usSize)
{
	switch (cTarget) {
	case XTDI:
	case TDI:
		if (g_pucInData != NULL) {
			if (g_usPreviousSize == usSize) {  /*memory exist*/
				break;
			} else   {
				free(g_pucInData);
				g_pucInData = NULL;
			}
		}
		g_pucInData = ( unsigned char * ) malloc(usSize / 8 + 2);
		g_usPreviousSize = usSize;
	case XTDO:
	case TDO:
		if (g_pucOutData != NULL) {
			if (g_usPreviousSize == usSize) {   /*already exist*/
				break;
			} else   {
				free(g_pucOutData);
				g_pucOutData = NULL;
			}
		}
		g_pucOutData = ( unsigned char * ) malloc(usSize / 8 + 2);
		g_usPreviousSize = usSize;
		break;
	case MASK:
		if (g_pucOutMaskData != NULL) {
			if (g_usPreviousSize == usSize) {  /*already allocated*/
				break;
			} else   {
				free(g_pucOutMaskData);
				g_pucOutMaskData = NULL;
			}
		}
		g_pucOutMaskData = ( unsigned char * ) malloc(usSize / 8 + 2);
		g_usPreviousSize = usSize;
		break;
	case HIR:
		if (g_pucHIRData != NULL) {
			free(g_pucHIRData);
			g_pucHIRData = NULL;
		}
		g_pucHIRData = ( unsigned char * ) malloc(usSize / 8 + 2);
		break;
	case TIR:
		if (g_pucTIRData != NULL) {
			free(g_pucTIRData);
			g_pucTIRData = NULL;
		}
		g_pucTIRData = ( unsigned char * ) malloc(usSize / 8 + 2);
		break;
	case HDR:
		if (g_pucHDRData != NULL) {
			free(g_pucHDRData);
			g_pucHDRData = NULL;
		}
		g_pucHDRData = ( unsigned char * ) malloc(usSize / 8 + 2);
		break;
	case TDR:
		if (g_pucTDRData != NULL) {
			free(g_pucTDRData);
			g_pucTDRData = NULL;
		}
		g_pucTDRData = ( unsigned char * ) malloc(usSize / 8 + 2);
		break;
	case HEAP:
		if (g_pucHeapMemory != NULL) {
			free(g_pucHeapMemory);
			g_pucHeapMemory = NULL;
		}
		g_pucHeapMemory = ( unsigned char * ) malloc(usSize + 2);
		break;
	case DMASK:
		if (g_pucOutDMaskData != NULL) {
			if (g_usPreviousSize == usSize) {   /*already allocated*/
				break;
			} else   {
				free(g_pucOutDMaskData);
				g_pucOutDMaskData = NULL;
			}
		}
		g_pucOutDMaskData = ( unsigned char * ) malloc(usSize / 8 + 2);
		g_usPreviousSize = usSize;
		break;
	case LHEAP:
		if (g_pucIntelBuffer != NULL) {
			free(g_pucIntelBuffer);
			g_pucIntelBuffer = NULL;
		}
		g_pucIntelBuffer = ( unsigned char * ) malloc(usSize + 2);
		break;
	case LVDS:
		if (g_pLVDSList != NULL) {
			free(g_pLVDSList);
			g_pLVDSList = NULL;
		}
		g_pLVDSList = ( LVDSPair * ) calloc(usSize, sizeof(LVDSPair));
		break;
	default:
		return;
	}
}

/***************************************************************
*
* ispVMFreeMem
*
* Free memory that were dynamically allocated.
*
***************************************************************/

void ispVMFreeMem()
{
	if (g_pucHeapMemory != NULL) {
		free(g_pucHeapMemory);
		g_pucHeapMemory = NULL;
	}

	if (g_pucOutMaskData != NULL) {
		free(g_pucOutMaskData);
		g_pucOutMaskData = NULL;
	}

	if (g_pucInData != NULL) {
		free(g_pucInData);
		g_pucInData = NULL;
	}

	if (g_pucOutData != NULL) {
		free(g_pucOutData);
		g_pucOutData = NULL;
	}

	if (g_pucHIRData != NULL) {
		free(g_pucHIRData);
		g_pucHIRData = NULL;
	}

	if (g_pucTIRData != NULL) {
		free(g_pucTIRData);
		g_pucTIRData = NULL;
	}

	if (g_pucHDRData != NULL) {
		free(g_pucHDRData);
		g_pucHDRData = NULL;
	}

	if (g_pucTDRData != NULL) {
		free(g_pucTDRData);
		g_pucTDRData = NULL;
	}

	if (g_pucOutDMaskData != NULL) {
		free(g_pucOutDMaskData);
		g_pucOutDMaskData = NULL;
	}

	if (g_pucIntelBuffer != NULL) {
		free(g_pucIntelBuffer);
		g_pucIntelBuffer = NULL;
	}

	if (g_pLVDSList != NULL) {
		free(g_pLVDSList);
		g_pLVDSList = NULL;
	}
}


/* size of stack area used by each thread */
#define STACKSIZE 1024

/* scheduling priority used by each thread */
#define PRIORITY 7

signed char ispVM(void)
{
	jtag_dev = device_get_binding("JTAG1");
	char szFileVersion[9] = { 0 };
	signed char cRetCode = 0;
	signed char cIndex = 0;
	signed char cVersionIndex = 0;
	unsigned char ucReadByte = 0;
	unsigned short crc;

	g_pucHeapMemory = NULL;
	g_iHeapCounter = 0;
	g_iHEAPSize = 0;
	g_usIntelDataIndex = 0;
	g_usIntelBufferSize = 0;
	g_usCalculatedCRC = 0;
	expectedCRC = 0;
	ucReadByte = GetByte();
	switch (ucReadByte) {
	case FILE_CRC:
		crc = (unsigned char)GetByte();
		crc <<= 8;
		crc |= GetByte();
		expectedCRC = crc;

		for (cIndex = 0; cIndex < 8; cIndex++)
			szFileVersion[cIndex] = GetByte();

		break;
	default:
		szFileVersion[0] = (signed char) ucReadByte;
		for (cIndex = 1; cIndex < 8; cIndex++)
			szFileVersion[cIndex] = GetByte();

		break;
	}

	/*
	 *
	 * Compare the VME file version against the supported version.
	 *
	 */

	for (cVersionIndex = 0; g_szSupportedVersions[cVersionIndex] != 0;
	     cVersionIndex++) {
		for (cIndex = 0; cIndex < 8; cIndex++) {
			if (szFileVersion[cIndex] !=
			    g_szSupportedVersions[cVersionIndex][cIndex]) {
				cRetCode = VME_VERSION_FAILURE;
				break;
			}
			cRetCode = 0;
		}

		if (cRetCode == 0) {
			break;
		}
	}

	if (cRetCode < 0) {
		return VME_VERSION_FAILURE;
	}

	printf("VME file checked: starting downloading to FPGA\n");

	ispVMStart();

	cRetCode = ispVMCode();

	ispVMEnd();
	ispVMFreeMem();
	puts("\n");

	if (cRetCode == 0 && expectedCRC != 0 &&
	    (expectedCRC != g_usCalculatedCRC)) {
		printf("Expected CRC:   0x%.4X\n", expectedCRC);
		printf("Calculated CRC: 0x%.4X\n", g_usCalculatedCRC);
		return VME_CRC_FAILURE;
	}
	return cRetCode;
}

struct k_sem sem;

void vme_test(void)
{
	signed char ret = 0;

	k_sem_init(&sem, 0, 1);

	while (1) {
		k_sem_take(&sem, K_FOREVER);
		read_bytes = 0;
		ret = ispVM();
		if (ret < 0) {
			printf("VME program FAIL error %d!!\n", ret);
		} else {
			printf("PASS!!\n");
		}
	}
}

K_THREAD_DEFINE(vme_thread, STACKSIZE, vme_test, NULL, NULL, NULL,
		PRIORITY, 0, 0);


static int cmd_vme_run(const struct shell *shell, size_t argc, char **argv)
{
	k_sem_give(&sem);
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	vmd_cmds,
	SHELL_CMD_ARG(run, NULL, NULL, cmd_vme_run, 0,
		      0),
	SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(vme, &vmd_cmds, "vme commands", NULL);
