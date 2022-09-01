/**
 * @file
 * @brief JTAG public API header file.
 */

/*
 * Copyright (c) 2021 Aspeed Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_JTAG_H_
#define ZEPHYR_INCLUDE_DRIVERS_JTAG_H_

#include <device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief JTAG driver APIs
 * @{
 */

/**
 * Defines JTAG Test Access Port states.
 *
 * These definitions were gleaned from the ARM7TDMI-S Technical
 * Reference Manual and validated against several other ARM core
 * technical manuals.
 *
 * FIXME some interfaces require specific numbers be used, as they
 * are handed-off directly to their hardware implementations.
 * Fix those drivers to map as appropriate ... then pick some
 * sane set of numbers here (where 0/uninitialized == INVALID).
 */
enum tap_state {
	TAP_INVALID = -1,
	/* Proper ARM recommended numbers */
	TAP_DREXIT2 = 0x0,
	TAP_DREXIT1 = 0x1,
	TAP_DRSHIFT = 0x2,
	TAP_DRPAUSE = 0x3,
	TAP_IRSELECT = 0x4,
	TAP_DRUPDATE = 0x5,
	TAP_DRCAPTURE = 0x6,
	TAP_DRSELECT = 0x7,
	TAP_IREXIT2 = 0x8,
	TAP_IREXIT1 = 0x9,
	TAP_IRSHIFT = 0xa,
	TAP_IRPAUSE = 0xb,
	TAP_IDLE = 0xc,
	TAP_IRUPDATE = 0xd,
	TAP_IRCAPTURE = 0xe,
	TAP_RESET = 0x0f,
};

enum jtag_pin {
	JTAG_TDI = 0x01,
	JTAG_TCK = 0x02,
	JTAG_TMS = 0x04,
	JTAG_ENABLE = 0x08,
	JTAG_TRST = 0x10,
};

/**
 * This structure defines a single scan field in the scan. It provides
 * fields for the field's width and pointers to scan input and output
 * values.
 */
struct scan_field_s {
	/** The number of bits this field specifies */
	int num_bits;
	/** A pointer to value to be scanned into the device */
	const uint8_t *out_value;
	/** A pointer to a 32-bit memory location for data scanned out */
	uint8_t *in_value;
};

/**
 * The scan_command provide a means of encapsulating a set of scan_field_s
 * structures that should be scanned in/out to the device.
 */
struct scan_command_s {
	/** instruction/not data scan */
	bool ir_scan;
	/** data scan fields */
	struct scan_field_s fields;
	/** state in which JTAG commands should finish */
	enum tap_state end_state;
};

/**
 * @brief Type definition of ADC API function for getting frequency.
 * See jtag_freq_get() for argument descriptions.
 */
typedef int (*jtag_api_freq_get)(const struct device *dev, uint32_t *freq);

/**
 * @brief Type definition of ADC API function for setting frequency.
 * See jtag_freq_set() for argument descriptions.
 */
typedef int (*jtag_api_freq_set)(const struct device *dev, uint32_t freq);

/**
 * @brief Type definition of JTAG API function for setting tap state.
 * See jtag_tap_set() for argument descriptions.
 */
typedef int (*jtag_api_tap_set)(const struct device *dev, enum tap_state state);

/**
 * @brief Type definition of JTAG API function for getting tap state.
 * See jtag_tap_get() for argument descriptions.
 */
typedef int (*jtag_api_tap_get)(const struct device *dev,
				enum tap_state *state);

/**
 * @brief Type definition of JTAG API function for running tck cycles.
 * See jtag_tck_run() for argument descriptions.
 */
typedef int (*jtag_api_tck_run)(const struct device *dev, uint32_t run_count);

/**
 * @brief Type definition of JTAG API function for scanning.
 */
typedef int (*jtag_api_xfer)(const struct device *dev,
			     struct scan_command_s *scan);

/**
 * @brief Type definition of JTAG API function for software mode.
 */
typedef int (*jtag_api_sw_xfer)(const struct device *dev, enum jtag_pin pin,
				uint8_t value);

/**
 * @brief Type definition of JTAG API function for getting tdo value.
 */
typedef int (*jtag_api_tdo_get)(const struct device *dev, uint8_t *value);


/**
 * @brief JTAG driver API
 *
 * This is the mandatory API any JTAG driver needs to expose.
 */
__subsystem struct jtag_driver_api {
	jtag_api_freq_get freq_get;
	jtag_api_freq_set freq_set;
	jtag_api_tap_get tap_get;
	jtag_api_tap_set tap_set;
	jtag_api_tck_run tck_run;
	jtag_api_xfer xfer;
	jtag_api_sw_xfer sw_xfer;
	jtag_api_tdo_get tdo_get;
};

__syscall int jtag_freq_get(const struct device *dev, uint32_t *freq);

static inline int z_impl_jtag_freq_get(const struct device *dev, uint32_t *freq)
{
	const struct jtag_driver_api *api =
		(const struct jtag_driver_api *)dev->api;

	return api->freq_get(dev, freq);
}

__syscall int jtag_freq_set(const struct device *dev, uint32_t freq);

static inline int z_impl_jtag_freq_set(const struct device *dev, uint32_t freq)
{
	const struct jtag_driver_api *api =
		(const struct jtag_driver_api *)dev->api;

	return api->freq_set(dev, freq);
}

__syscall int jtag_tap_get(const struct device *dev, enum tap_state *state);

static inline int z_impl_jtag_tap_get(const struct device *dev,
				      enum tap_state *state)
{
	const struct jtag_driver_api *api =
		(const struct jtag_driver_api *)dev->api;

	return api->tap_get(dev, state);
}

__syscall int jtag_tap_set(const struct device *dev, enum tap_state state);

static inline int z_impl_jtag_tap_set(const struct device *dev,
				      enum tap_state state)
{
	const struct jtag_driver_api *api =
		(const struct jtag_driver_api *)dev->api;

	return api->tap_set(dev, state);
}

__syscall int jtag_tck_run(const struct device *dev, uint32_t run_count);

static inline int z_impl_jtag_tck_run(const struct device *dev,
				      uint32_t run_count)
{
	const struct jtag_driver_api *api =
		(const struct jtag_driver_api *)dev->api;

	return api->tck_run(dev, run_count);
}

__syscall int jtag_ir_scan(const struct device *dev, int num_bits,
			   const uint8_t *out_value, uint8_t *in_value,
			   enum tap_state state);

static inline int z_impl_jtag_ir_scan(const struct device *dev, int num_bits,
				      const uint8_t *out_value,
				      uint8_t *in_value, enum tap_state state)
{
	const struct jtag_driver_api *api =
		(const struct jtag_driver_api *)dev->api;

	struct scan_command_s scan;

	scan.ir_scan = 1;
	scan.end_state = state;
	scan.fields.num_bits = num_bits;
	scan.fields.out_value = out_value;
	scan.fields.in_value = in_value;

	return api->xfer(dev, &scan);
}

__syscall int jtag_dr_scan(const struct device *dev, int num_bits,
			   const uint8_t *out_value, uint8_t *in_value,
			   enum tap_state state);

static inline int z_impl_jtag_dr_scan(const struct device *dev, int num_bits,
				      const uint8_t *out_value,
				      uint8_t *in_value, enum tap_state state)
{
	const struct jtag_driver_api *api =
		(const struct jtag_driver_api *)dev->api;

	struct scan_command_s scan;

	scan.ir_scan = 0;
	scan.end_state = state;
	scan.fields.num_bits = num_bits;
	scan.fields.out_value = out_value;
	scan.fields.in_value = in_value;

	return api->xfer(dev, &scan);
}

__syscall int jtag_sw_xfer(const struct device *dev, enum jtag_pin pin,
			   uint8_t value);

static inline int z_impl_jtag_sw_xfer(const struct device *dev, enum jtag_pin pin,
			   uint8_t value)
{
	const struct jtag_driver_api *api =
		(const struct jtag_driver_api *)dev->api;

	return api->sw_xfer(dev, pin, value);
}

__syscall int jtag_tdo_get(const struct device *dev, uint8_t *value);

static inline int z_impl_jtag_tdo_get(const struct device *dev, uint8_t *value)
{
	const struct jtag_driver_api *api =
		(const struct jtag_driver_api *)dev->api;

	return api->tdo_get(dev, value);
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#include <syscalls/jtag.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_JTAG_H_ */
