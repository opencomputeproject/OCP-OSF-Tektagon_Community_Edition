//***********************************************************************//
//*                                                                     *//
//*                      Copyright © 2022 AMI                           *//
//*                                                                     *//
//*        All rights reserved. Subject to AMI licensing agreement.     *//
//*                                                                     *//
//***********************************************************************//

#include "CommonFlash/CommonFlash.h"
#include "state_machine/common_smc.h"
#include "Definition.h"
#ifdef CONFIG_INTEL_PFR_SUPPORT
#include "intel_2.0/intel_pfr_definitions.h"
#endif
#ifdef CONFIG_CERBERUS_PFR_SUPPORT
#include "cerberus/cerberus_pfr_definitions.h"
#endif
int get_cpld_status(uint8_t *data, uint32_t data_length){
    
    int status;
	struct SpiEngine *spi_flash = getSpiEngineWrapper();
	spi_flash->spi.device_id[0] = ROT_INTERNAL_STATE; //Internal UFM SPI
	status = spi_flash->spi.base.read(&spi_flash->spi,0,data,data_length);

    return Success;
}

int set_cpld_status(uint8_t *data,uint32_t data_length){

    int status;
	struct SpiEngine *spi_flash = getSpiEngineWrapper();
	spi_flash->spi.device_id[0] = ROT_INTERNAL_STATE; //Internal UFM SPI
    status  = pfr_spi_erase_4k(ROT_INTERNAL_STATE, 0);
    status = spi_flash->spi.base.write(&spi_flash->spi,0,data,data_length);

    return Success;
}

int ufm_read(uint32_t ufm_id, uint32_t offset, uint8_t *data, uint32_t data_length){
    
    if(ufm_id == PROVISION_UFM)
        return get_provision_data_in_flash(offset, data, data_length);
    else if (ufm_id == UPDATE_STATUS_UFM)
        return get_cpld_status(data, data_length);
    else
        return Failure;

    return Success;
}

int ufm_write(uint32_t ufm_id, uint32_t offset, uint8_t *data, uint32_t data_length){
   
    if(ufm_id == PROVISION_UFM)
        return set_provision_data_in_flash(offset, data, data_length);
    else if (ufm_id == UPDATE_STATUS_UFM)
        return set_cpld_status(data, data_length);
    else
        return Failure;

    return Success;
    
}

int ufm_erase(uint32_t ufm_id){
    if(ufm_id == PROVISION_UFM)
        return pfr_spi_erase_4k(ROT_INTERNAL_INTEL_STATE, 0);
    else if(ufm_id == UPDATE_STATUS_UFM)
        return pfr_spi_erase_4k(ROT_INTERNAL_STATE, 0);
    else
        return Failure;

    return Success;
}



