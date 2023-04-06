//***********************************************************************//
//*                                                                     *//
//*                      Copyright © 2022 AMI                           *//
//*                                                                     *//
//*        All rights reserved. Subject to AMI licensing agreement.     *//
//*                                                                     *//
//***********************************************************************//

#include <stdint.h>
#include "state_machine/common_smc.h"
#include "cerberus_pfr_definitions.h"


#if PF_UPDATE_DEBUG
#define DEBUG_PRINTF printk
#else
#define DEBUG_PRINTF(...)
#endif


/**
    Function Used to Verify whether the compression Tag value is Matched or Not

    @Param uint32_t *           Pointer to Compression Tag
    @Param uint32_t             Read Address
    @Param uint32_t             Total size need to read

    @retval int		Return Status
 **/
int cerberus_is_compression_tag_matched(uint32_t image_type, uint32_t *compression_tag, uint32_t read_address, uint32_t AreaSize)
{
	return Success;
}

/**
 * Function Used to Erase the Active Area based on the BitMap.
 * @param - N - Size
 * @param - ActiveMapAddress - Location od the Bitmap.
 * @return - Success or Failure
 */
/**
    Function Used to Erase the Active Area based on the BitMap

    @Param uint32_t		Size
	@Param uint32_t         Active Bit Map Address

    @retval int		Return Status
 **/
int cerberus_decompression_erasing(uint32_t image_type, uint32_t N, uint32_t active_map_address)
{
	return Success;
}


/**
    Function Used to Write the Compressed Data based on the Bit Map

    @Param uint32_t             Size
    @Param uint32_t             Compression Tag
    @Param uint32_t             Compression Map Address

    @retval int			Return Status
 **/
int cerberus_decompression_write(uint32_t image_type, uint32_t N, uint32_t compression_tag, uint32_t compression_map_address)
{
	return Success;
}

/**
    Function Used to Decompress The Compressed Capsule bin

    @Param uint32_t             Read Address
    @Param uint32_t             Total size need to Decompress

    @retval int		Return Status
 **/
int cerberus_capsule_decompression(uint32_t image_type, uint32_t read_address, uint32_t area_size)
{
	DEBUG_PRINTF("Decompression completed\r\n");
	return Success;
}



