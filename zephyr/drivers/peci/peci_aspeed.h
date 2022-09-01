/*
 * Copyright (c) 2020-2021 Aspeed Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef _PECI_ASPEED_H_
#define _PECI_ASPEED_H_

/**********************************************************
 * PECI register fields
 *********************************************************/
typedef union {
	volatile uint32_t value;
	struct {
		volatile uint32_t enable_peci_clock : 1;                /*[0-0]*/
		volatile uint32_t enable_64_byte_mode : 1;              /*[1-1]*/
		volatile uint32_t : 2;                                  /*[2-3]*/
		volatile uint32_t enable_peci : 1;                      /*[4-4]*/
		volatile uint32_t enable_bus_contention : 1;            /*[5-5]*/
		volatile uint32_t inverse_peci_input_polarity : 1;      /*[6-6]*/
		volatile uint32_t inverse_peci_output_polarity : 1;     /*[7-7]*/
		volatile uint32_t peci_clock_divider : 3;               /*[8-10]*/
		volatile uint32_t clock_source_selection : 1;           /*[11-11]*/
		volatile uint32_t read_mode_selection : 2;              /*[12-13]*/
		volatile uint32_t : 2;                                  /*[14-15]*/
		volatile uint32_t read_sampling_point_selection : 4;    /*[16-19]*/
		volatile uint32_t io_driving_strength : 4;              /*[20-23]*/
		volatile uint32_t : 8;                                  /*[24-31]*/

	} fields;
} control_register_t; /* 0x00000000 */

typedef union {
	volatile uint32_t value;
	struct {
		volatile uint32_t tbit_a_1 : 8;                 /*[0-7]*/
		volatile uint32_t tbit_a_2_and_tbit_m : 8;      /*[8-15]*/
		volatile uint32_t : 16;                         /*[16-31]*/

	} fields;
} timing_negotiation_register_t; /* 0x00000004 */

typedef union {
	volatile uint32_t value;
	struct {
		volatile uint32_t fire_a_peci_command : 1;      /*[0-0]*/
		volatile uint32_t : 23;                         /*[1-23]*/
		volatile uint32_t peci_controller_state : 4;    /*[24-27]*/
		volatile uint32_t : 2;                          /*[28-29]*/
		volatile uint32_t raw_peci_pin_monitoring: 1;	/*[30-30]*/
		volatile uint32_t peci_pin_monitoring : 1;      /*[31-31]*/

	} fields;
} command_register_t; /* 0x00000008 */

typedef union {
	volatile uint32_t value;
	struct {
		volatile uint32_t target_address : 8;           /*[0-7]*/
		volatile uint32_t write_data_length : 8;        /*[8-15]*/
		volatile uint32_t read_data_length : 8;         /*[16-23]*/
		volatile uint32_t : 7;                          /*[24-30]*/
		volatile uint32_t enable_aw_fcs_cycle : 1;      /*[31-31]*/

	} fields;
} read_write_length_register_t; /* 0x0000000c */

typedef union {
	volatile uint32_t value;
	struct {
		volatile uint32_t expected_write_fcs : 8;               /*[0-7]*/
		volatile uint32_t expected_auto_aw_fcs : 8;             /*[8-15]*/
		volatile uint32_t expected_read_fcs : 8;                /*[16-23]*/
		volatile uint32_t write_data_following_pecibc : 8;      /*[24-31]*/

	} fields;
} expected_fcs_data_register_t; /* 0x00000010 */

typedef union {
	volatile uint32_t value;
	struct {
		volatile uint32_t captured_write_fcs : 8;       /*[0-7]*/
		volatile uint32_t : 8;                          /*[8-15]*/
		volatile uint32_t captured_read_fcs : 8;        /*[16-23]*/
		volatile uint32_t : 8;                          /*[24-31]*/

	} fields;
} captured_fcs_data_register_t; /* 0x00000014 */

typedef union {
	volatile uint32_t value;
	struct {
		volatile uint32_t enable_peci_command_done_interrupt : 1;               /*[0-0]*/
		volatile uint32_t enable_peci_write_fcs_abort_interrupt : 1;            /*[1-1]*/
		volatile uint32_t enable_peci_write_fcs_bad_interrupt : 1;              /*[2-2]*/
		volatile uint32_t enable_peci_bus_contention_interrupt : 1;             /*[3-3]*/
		volatile uint32_t enable_peci_bus_time_out_interrupt : 1;               /*[4-4]*/
		volatile uint32_t : 25;                                                 /*[5-29]*/
		volatile uint32_t selection_of_timing_negotiation_result_bit_1_0 : 2;   /*[30-31]*/

	} fields;
} interrupt_register_t; /* 0x00000018 */

typedef union {
	volatile uint32_t value;
	struct {
		volatile uint32_t peci_done_interrupt_status : 1;               /*[0-0]*/
		volatile uint32_t peci_write_fcs_abort_interrupt_status : 1;    /*[1-1]*/
		volatile uint32_t peci_write_fcs_bad_interrupt_status : 1;      /*[2-2]*/
		volatile uint32_t peci_bus_contention_interrupt_status : 1;     /*[3-3]*/
		volatile uint32_t peci_bus_time_out_interrupt_status : 1;       /*[4-4]*/
		volatile uint32_t : 10;                                         /*[5-14]*/
		volatile uint32_t good1fcs_status : 1;                          /*[15-15]*/
		volatile uint32_t timing_negotiation_result_bit_13_0 : 14;      /*[16-29]*/
		volatile uint32_t : 2;                                          /*[30-31]*/

	} fields;
} interrupt_status_register_t; /* 0x0000001c */

typedef union {
	volatile uint32_t value;
	volatile uint8_t data[4];
} write_data_register_t;

typedef union {
	volatile uint32_t value;
	volatile uint8_t data[4];
} read_data_register_t;

typedef struct {
	control_register_t control;                             /* 0x00000000 */
	timing_negotiation_register_t timing_negotiation;       /* 0x00000004 */
	command_register_t command;                             /* 0x00000008 */
	read_write_length_register_t peci_header;               /* 0x0000000c */
	expected_fcs_data_register_t expected_fcs_data;         /* 0x00000010 */
	captured_fcs_data_register_t captured_fcs_data;         /* 0x00000014 */
	interrupt_register_t interrupt;                         /* 0x00000018 */
	interrupt_status_register_t interrupt_status;           /* 0x0000001c */
	write_data_register_t write_data_32_mode_low[4];
	read_data_register_t read_data_32_mode_low[4];
	write_data_register_t write_data_32_mode_up[4];
	read_data_register_t read_data_32_mode_up[4];
	uint32_t reserved0[8]; /* 00000060~0000007c*/
	write_data_register_t write_data_64_mode[16];
	read_data_register_t read_data_64_mode[16];
} peci_register_t;
/**********************************************************
 * PECI macro
 *********************************************************/
/* ASPEED_PECI_INT_CTRL/STS - 0x18/0x1c : Interrupt Register */
enum {
	PECI_INT_CMD_DONE = 0,
	PECI_INT_W_FCS_ABORT,
	PECI_INT_W_FCS_BAD,
	PECI_INT_CONNECT,
	PECI_INT_TIMEOUT,
	PECI_INT_TYPE_NUM,
};

enum {
	PECI_WFCS = 0,
	PECI_RFCS,
};

enum {
	ASPEED_PECI_CLK_SOURCE_OSC_25M = 0,
	ASPEED_PECI_CLK_SOURCE_HCLK,
};

/* Bus Frequency */
#define ASPEED_PECI_BUS_FREQ_MAX        2000000
#define ASPEED_PECI_BUS_FREQ_MIN        2000
#define ASPEED_PECI_BUS_FREQ_DEFAULT    1000000

#endif /* end of "#ifndef _PECI_ASPEED_H_" */