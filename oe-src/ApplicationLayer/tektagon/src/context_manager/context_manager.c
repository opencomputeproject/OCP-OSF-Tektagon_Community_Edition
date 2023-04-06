//***********************************************************************//
//*                                                                     *//
//*                      Copyright © 2022 AMI                           *//
//*                                                                     *//
//*        All rights reserved. Subject to AMI licensing agreement.     *//
//*                                                                     *//
//***********************************************************************//

#include <Common.h>
#include "context_manager.h"
#include "state_machine/common_smc.h"
#include "Definition.h"


static struct Context_Manager *context_manager;
static struct app_context _app_context;

struct Context_Manager *get_Context_Manager(void)
{
	return &context_manager;
}

void set_Context_Manager(struct Context_Manager *new_context)
{
	if (new_context) {
		memcpy(context_manager, new_context, sizeof(struct Context_Manager));
	}
}

unsigned char erase_context_data_flash(void)
{
	int status;
	struct SpiEngine *spi_flash = getSpiEngineWrapper();

	spi_flash->spi.device_id[0] = ROT_INTERNAL_STATE;
	status = spi_flash->spi.base.sector_erase(&spi_flash->spi, 0);
	return status;
}

void get_context_data_in_flash(uint32_t addr, uint8_t *DataBuffer, uint32_t length)
{
	uint8_t status;
	struct SpiEngine *spi_flash = getSpiEngineWrapper();

	spi_flash->spi.device_id[0] = ROT_INTERNAL_STATE; // Internal UFM SPI
	status = spi_flash->spi.base.read(&spi_flash->spi, addr, DataBuffer, length);

}

unsigned char set_context_data_in_flash(uint8_t addr, uint8_t *DataBuffer, uint8_t DataSize)
{
	uint8_t status;
	uint8_t buffer[256];
	struct SpiEngine *spi_flash = getSpiEngineWrapper();
	spi_flash->spi.device_id[0] = ROT_INTERNAL_STATE;

	// Read Intel State
	status = spi_flash->spi.base.read(&spi_flash->spi, 0, buffer, sizeof(buffer) / sizeof(buffer[0]));

	if (status == Success) {
		status = erase_context_data_flash();

		if (status == Success) {
			for (int i = addr; i < DataSize + addr; i++) {
				buffer[i] = DataBuffer[i - addr];
			}

			memcpy(buffer + addr, DataBuffer, DataSize);

			status = spi_flash->spi.base.write(&spi_flash->spi, 0, buffer, sizeof(buffer) / sizeof(buffer[0]));
			if (status == Failure) 
				return Failure;
		}
	}

	spi_flash->spi.device_id[0] = ROT_INTERNAL_INTEL_STATE;
	status = spi_flash->spi.base.read(&spi_flash->spi, 0, buffer, sizeof(buffer) / sizeof(buffer[0]));

	return status;
}
/**
 * Save the current context for the running application.  A reboot after this context has been
 * saved will restore it and skip normal boot-time initializations and checks.
 *
 * @param context The application context instance.
 *
 * @return 0 if the application context has been saved or an error code.
 */
static int save_cpld_context(struct app_context *context)
{
	int status = 0;
	uint8_t Readbuffer[sizeof(context_manager)];

	if (context == NULL) {
		return APP_CONTEXT_INVALID_ARGUMENT;
	}
	// Add code to save the App Context context_manager
	status = set_context_data_in_flash(0, (uint8_t *)&context_manager, sizeof(context_manager));
	// get_context_data_in_flash(0, Readbuffer, sizeof(context_manager));
	// printk("sizeof(context_manager): %d\n",sizeof(context_manager));
	// printk("ReadBuffer: ");
	// for(int i=0;i<sizeof(context_manager);i++)
	// {
	//	printk("%x\n", Readbuffer[i]);
	// }
	// struct Context_Manager *read_context_manager = (struct Context_Manager *)Readbuffer;
	// printk("read_context_manager: %x %x %x\n", read_context_manager->bmc_2_pch_update, //read_context_manager->bmc_2_pch_update_active, read_context_manager->pch_update);


	return status;
}

int app_context_init(struct app_context *context)
{
	if (context == NULL) {
		return APP_CONTEXT_INVALID_ARGUMENT;
	}

	memset(context, 0, sizeof(struct app_context));

	context->save = save_cpld_context;

	return 0;
}

struct app_context *getappcontextInstance(void)
{
	return &_app_context;
}
