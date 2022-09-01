/* reset_control.h - public reset controller driver API */

/*
 * Copyright (c) 2021 ASPEED
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public Reset Control APIs
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_RESET_CONTROL_H_
#define ZEPHYR_INCLUDE_DRIVERS_RESET_CONTROL_H_

/**
 * @brief Reset Control Interface
 * @defgroup reset_control_interface Reset Control Interface
 * @ingroup io_interfaces
 * @{
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <device.h>
#include <sys/__assert.h>
#include <sys/slist.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Reset control API */

/* Used to select all subsystem of a reset controller */
#define RESET_CONTROL_SUBSYS_ALL	NULL

/**
 * @brief Current reset status.
 */
enum reset_control_status {
	RESET_CONTROL_STATUS_ASSERT,
	RESET_CONTROL_STATUS_DEASSERT,
	RESET_CONTROL_STATUS_UNKNOWN
};

/**
 * reset_control_subsys_t is a type to identify a reset controller sub-system.
 * Such data pointed is opaque and relevant only to the reset controller
 * driver instance being used.
 */
typedef void *reset_control_subsys_t;

typedef int (*reset_control)(const struct device *dev,
			     reset_control_subsys_t sys);


typedef enum reset_control_status (*reset_control_get_status_fn)(
						    const struct device *dev,
						    reset_control_subsys_t sys);

struct reset_control_driver_api {
	reset_control			deassert;
	reset_control			assert;
	reset_control_get_status_fn	get_status;
};

/**
 * @brief De-assert a reset controlled by the device
 *
 * On success, the reset is de-assert and ready when this function
 * returns.
 *
 * @param dev Device structure whose driver controls the reset.
 * @param sys Opaque data representing the reset.
 * @return 0 on success, negative errno on failure.
 */
static inline int reset_control_deassert(const struct device *dev,
				   reset_control_subsys_t sys)
{
	const struct reset_control_driver_api *api =
		(const struct reset_control_driver_api *)dev->api;

	return api->deassert(dev, sys);
}

/**
 * @brief Assert a reset controlled by the device
 *
 * On success, the reset is asserted when this function returns.
 *
 * @param dev Device structure whose driver controls the reset
 * @param sys Opaque data representing the reset
 * @return 0 on success, negative errno on failure.
 */
static inline int reset_control_assert(const struct device *dev,
				    reset_control_subsys_t sys)
{
	const struct reset_control_driver_api *api =
		(const struct reset_control_driver_api *)dev->api;

	return api->assert(dev, sys);
}

/**
 * @brief Get reset status.
 *
 * @param dev Device.
 * @param sys A pointer to an opaque data representing the sub-system.
 *
 * @return Status.
 */
static inline enum reset_control_status reset_control_get_status(const struct device *dev,
								 reset_control_subsys_t sys)
{
	const struct reset_control_driver_api *api =
		(const struct reset_control_driver_api *)dev->api;

	if (!api->get_status) {
		return RESET_CONTROL_STATUS_UNKNOWN;
	}

	return api->get_status(dev, sys);
}


#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_RESET_CONTROL_H_ */
