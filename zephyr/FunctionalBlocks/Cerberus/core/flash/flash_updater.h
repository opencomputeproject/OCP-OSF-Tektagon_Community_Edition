// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

#ifndef FLASH_UPDATER_H_
#define FLASH_UPDATER_H_

#include <stdint.h>
#include <stddef.h>
#include "status/rot_status.h"
#include "flash.h"


/**
 * Manage writing updates to flash.  The updates will be received in pieces that will be written
 * to sequential locations in flash.
 */
struct flash_updater {
	struct flash *flash;							/**< The flash to manage for updates. */
	uint32_t base_addr;								/**< The start of the update region. */
	size_t max_size;								/**< The maximum size of an update. */
	int update_size;								/**< Expected size of the current update. */
	uint32_t write_offset;							/**< Offset from the base for the next write. */
	int (*erase) (struct flash*, uint32_t, size_t);	/**< Function for erasing flash. */
};


int flash_updater_init (struct flash_updater *updater, struct flash *flash, uint32_t base_addr,
	size_t max_size);
int flash_updater_init_sector (struct flash_updater *updater, struct flash *flash,
	uint32_t base_addr, size_t max_size);
void flash_updater_release (struct flash_updater *updater);

void flash_updater_apply_update_offset (struct flash_updater *updater, uint32_t offset);

int flash_updater_check_update_size (struct flash_updater *updater, size_t total_length);
int flash_updater_prepare_for_update (struct flash_updater *updater, size_t total_length);
int flash_updater_prepare_for_update_erase_all (struct flash_updater *updater, size_t total_length);

int flash_updater_write_update_data (struct flash_updater *updater, const uint8_t *data,
	size_t length);

size_t flash_updater_get_bytes_written (struct flash_updater *updater);
int flash_updater_get_remaining_bytes (struct flash_updater *updater);


#define	FLASH_UPDATER_ERROR(code)		ROT_ERROR (ROT_MODULE_FLASH_UPDATER, code)

/**
 * Error codes that can be generated by a manager for flash updates.
 */
enum {
	FLASH_UPDATER_INVALID_ARGUMENT = FLASH_UPDATER_ERROR (0x00),	/**< Input parameter is null or not valid. */
	FLASH_UPDATER_NO_MEMORY = FLASH_UPDATER_ERROR (0x01),			/**< Memory allocation failed. */
	FLASH_UPDATER_TOO_LARGE = FLASH_UPDATER_ERROR (0x02),			/**< The update is too large for the allocated region. */
	FLASH_UPDATER_INCOMPLETE_WRITE = FLASH_UPDATER_ERROR (0x03),	/**< All update data was not written to flash. */
	FLASH_UPDATER_OUT_OF_SPACE = FLASH_UPDATER_ERROR (0x04),		/**< Update data would extend beyond region boundaries. */
};


#endif /* FLASH_UPDATER_H_ */
