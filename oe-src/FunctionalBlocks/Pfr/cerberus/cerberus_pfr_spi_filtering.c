#if CONFIG_CERBERUS_PFR_SUPPORT

#include <stdint.h>
#include "Common.h"
#include "cerberus_pfr_definitions.h"

#define	ADDR_MASK_0	GENMASK(7, 0)
#define	ADDR_MASK_1	GENMASK(15, 8)
#define	ADDR_SHIFT_1	8
#define	ADDR_MASK_2	GENMASK(23, 16)
#define	ADDR_SHIFT_2	16
#define	ADDR_MASK_3	GENMASK(31, 24)
#define	ADDR_SHIFT_3	24

void init_SPI_RW_region(int spi_device_id)
{
	int status = 0;

	status = SpiFilterInit (getSpiFilterEngineWrapper());
	struct SpiEngine *spi_flash = getSpiEngineWrapper();
	struct SpiFilterEngine *spi_filter = getSpiFilterEngineWrapper();

	spi_flash->spi.device_id[0] = spi_device_id;

	int region_id = 1;
	int region_length;
	int toc_element_length;
	int allowable_fw_list_header_to_alignment_length;

	uint32_t pfm_read_address = BMC_MANIFEST_OFFSET;
	uint32_t toc_elements_list_start = pfm_read_address + TOC_ELEMENTS_LIST_START;
	toc_elements_list_start = toc_elements_list_start + TOC_ELEMENT_LENGTH_OFFSET;
	uint32_t pfm_rw_flash_region_start = pfm_read_address + PLATFORM_HEADER_START;
	uint32_t rw_count_location;
	uint32_t version_length_location;
	uint32_t region_start_address;
	uint32_t region_end_address;
	uint8_t region_record[PFM_RW_FLASH_REGION_LENGTH];
	uint8_t rw_count[1];
	uint8_t version_length[1];
	uint8_t toc_elements_list_length[2];

	/*
	if (spi_device_id == 0) {
		get_provision_data_in_flash(BMC_ACTIVE_PFM_OFFSET, &pfm_read_address, sizeof(pfm_read_address));

	} else if (spi_device_id == 1) {
		get_provision_data_in_flash(PCH_ACTIVE_PFM_OFFSET, &pfm_read_address, sizeof(pfm_read_address));
	}
	*/
	for (int i = 0; i < 3; i++) // platform_header, flash_device, fw_elements
	{
		spi_flash->spi.base.read(&spi_flash->spi, toc_elements_list_start, toc_elements_list_length, sizeof(toc_elements_list_length));
		
 		toc_element_length = (toc_elements_list_length[0] & ADDR_MASK_0) | 
								(toc_elements_list_length[1] << ADDR_SHIFT_1 & ADDR_MASK_1);
		toc_elements_list_start = toc_elements_list_start + 8; // next toc element
		pfm_rw_flash_region_start = pfm_rw_flash_region_start + toc_element_length;
	}

	rw_count_location = pfm_rw_flash_region_start + 1; 
	spi_flash->spi.base.read(&spi_flash->spi, rw_count_location, rw_count, sizeof(rw_count)); 
	version_length_location = rw_count_location + 1;
	spi_flash->spi.base.read(&spi_flash->spi, version_length_location, version_length, sizeof(version_length)); 
	pfm_rw_flash_region_start = pfm_rw_flash_region_start + 
								ALLOWABLE_FW_LIST_HEADER_LENGTH + 
								ALLOWABLE_FW_LIST_VERSION_ADDR_LENGTH + 
								version_length[0] + 
								ALLOWABLE_FW_LIST_ALLIGNMENT_LENGTH;

	for (int i = 0; i < rw_count[0]; i++)
	{
		spi_flash->spi.base.read(&spi_flash->spi, pfm_rw_flash_region_start, region_record, sizeof(region_record));
		region_start_address = (region_record[4] & ADDR_MASK_0) | 
								(region_record[5] << ADDR_SHIFT_1 & ADDR_MASK_1) | 
								(region_record[6] << ADDR_SHIFT_2 & ADDR_MASK_2) | 
								(region_record[7] << ADDR_SHIFT_3 & ADDR_MASK_3);
		region_end_address = (region_record[8] & ADDR_MASK_0) | 
								(region_record[9] << ADDR_SHIFT_1 & ADDR_MASK_1) | 
								(region_record[10] << ADDR_SHIFT_2 & ADDR_MASK_2) | 
								(region_record[11] << ADDR_SHIFT_3 & ADDR_MASK_3);     
		region_end_address = region_end_address + 1;
		region_length = region_end_address - region_start_address;

		spi_filter->base.set_filter_rw_region(&spi_filter->base, region_id, region_start_address, region_end_address);
		pfm_rw_flash_region_start = pfm_rw_flash_region_start + 12;
		region_id++;
	}
	spi_filter->base.enable_filter(spi_filter, true);
}

#endif
