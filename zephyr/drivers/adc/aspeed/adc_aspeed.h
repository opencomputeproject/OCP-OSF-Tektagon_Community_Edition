/*
 * Copyright (c) 2020-2021 Aspeed Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef _ADC_ASPEED_H_
#define _ADC_ASPEED_H_

/**********************************************************
 * ADC register fields
 *********************************************************/
union adc_engine_control_s {
	volatile uint32_t value;
	struct {
		volatile uint32_t engine_enable : 1;                    /*[0-0]*/
		volatile uint32_t adc_operation_mode : 3;               /*[1-3]*/
		volatile uint32_t compensating_sensing_mode : 1;        /*[4-4]*/
		volatile uint32_t auto_compensating_sensing_mode : 1;   /*[5-5]*/
		volatile uint32_t reference_voltage_selection : 2;      /*[6-7]*/
		volatile uint32_t initial_sequence_complete : 1;        /*[8-8]*/
		volatile uint32_t reserved0: 3;                                  /*[9-11]*/
		volatile uint32_t channel_7_selection : 1;              /*[12-12]*/
		volatile uint32_t enable_battery_sensing : 1;           /*[13-13]*/
		volatile uint32_t reserved1: 2;                                  /*[14-15]*/
		volatile uint32_t channel_enable : 8;                   /*[16-23]*/
		volatile uint32_t reserved2: 8;                                  /*[24-31]*/

	} fields;
}; /* 00000000 */

union adc_interrupt_control_s {
	volatile uint32_t value;
	struct {
		volatile uint32_t interrupt_status : 8; /*[0-7]*/
		volatile uint32_t reserved0: 8;                  /*[8-15]*/
		volatile uint32_t interrupt_enable : 8; /*[16-23]*/
		volatile uint32_t reserved1: 8;                  /*[24-31]*/

	} fields;
}; /* 00000004 */

union adc_clock_control_s {
	volatile uint32_t value;
	struct {
		volatile uint32_t divisor_of_adc_clock : 16;    /*[0-15]*/
		volatile uint32_t reserved0: 16;                         /*[16-31]*/

	} fields;
}; /* 0000000c */

union adc_data_s {
	volatile uint32_t value;
	struct {
		volatile uint32_t data_odd : 10;        /*[0-9]*/
		volatile uint32_t reserved0: 6;                  /*[10-15]*/
		volatile uint32_t data_even : 10;       /*[16-25]*/
		volatile uint32_t reserved1: 6;                  /*[26-31]*/

	} fields;
}; /* 00000010~0000001c */

union adc_interrupt_bound_s {
	volatile uint32_t value;
	struct {
		volatile uint32_t lower_bound : 10;     /*[0-9]*/
		volatile uint32_t reserved0: 6;                  /*[10-15]*/
		volatile uint32_t upper_bound : 10;     /*[16-25]*/
		volatile uint32_t reserved1: 6;                  /*[26-31]*/

	} fields;
}; /* 00000030~0000004c */


union adc_hysteresis_control_s {
	volatile uint32_t value;
	struct {
		volatile uint32_t lower_bound : 10;             /*[0-9]*/
		volatile uint32_t reserved0: 6;                          /*[10-15]*/
		volatile uint32_t upper_bound : 10;             /*[16-25]*/
		volatile uint32_t reserved1: 5;                          /*[26-30]*/
		volatile uint32_t hysteresis_control : 1;       /*[31-31]*/

	} fields;
}; /* 00000070~0000008c */

union adc_interrupt_source_sel_s {
	volatile uint32_t value;
	struct {
		volatile uint32_t interrupt_source : 8; /*[0-7]*/
		volatile uint32_t reserved0: 24;                 /*[8-31]*/

	} fields;
}; /* 000000c0 */

union adc_compensating_trimming_s {
	volatile uint32_t value;
	struct {
		volatile uint32_t trimming_value : 4;           /*[0-3]*/
		volatile uint32_t reserved0: 12;                         /*[4-15]*/
		volatile uint32_t compensating_value : 10;      /*[16-25]*/
		volatile uint32_t reserved1: 6;                          /*[26-31]*/

	} fields;
}; /* 000000c4 */

union adc_global_interrupt_s {
	volatile uint32_t value;
	struct {
		volatile uint32_t adc_interrupt_status : 2;     /*[0-1]*/
		volatile uint32_t reserved0: 30;                         /*[2-31]*/

	} fields;
}; /* 000000cc */


struct adc_register_s {
	union adc_engine_control_s engine_ctrl;         /* 00000000 */
	union adc_interrupt_control_s int_ctrl;         /* 00000004 */
	uint32_t reserved0[1];                          /* 00000008 */
	union adc_clock_control_s adc_clk_ctrl;         /* 0000000c */
	union adc_data_s adc_data[4];                   /* 00000010~0000001c */
	uint32_t reserved1[4];                          /* 00000020~0000002c*/
	union adc_interrupt_bound_s int_bound[8];       /* 00000030~0000004c */
	uint32_t reserved2[8];                          /* 00000050~0000006c*/
	union adc_hysteresis_control_s hys_ctrl[8];     /* 00000070~0000008c */
	uint32_t reserved3[12];                         /* 00000090~000000bc*/
	union adc_interrupt_source_sel_s int_src_sel;   /* 000000c0 */
	union adc_compensating_trimming_s comp_trim;    /* 000000c4 */
	uint32_t reserved4[1];                          /* 000000c8 */
	union adc_global_interrupt_s g_int_status;      /* 000000cc */
};

/**********************************************************
 * ADC feature define
 *********************************************************/
#define ASPEED_ADC_CH_NUMBER            7
#define ASPEED_RESOLUTION_BITS          10
#define ASPEED_CLOCKS_PER_SAMPLE        12

/**********************************************************
 * ADC macro
 *********************************************************/
/* adc_operation_mode */
#define ADC_POWER_DOWN          0
#define ADC_STANDBY             1
#define ADC_NORMAL              7

/* reference_voltage_selection */
#define REF_VOLTAGE_2500mV      0
#define REF_VOLTAGE_1200mV      1
#define REF_VOLTAGE_EXT_HIGH    2
#define REF_VOLTAGE_EXT_LOW     3

/* reference_voltage_selection & 1 */
#define BATTERY_DIVIDE_2_3      0
#define BATTERY_DIVIDE_1_3      1

/* channel_7_selection */
#define Ch7_NORMAL_MODE         0
#define Ch7_BATTERY_MODE        1

/**********************************************************
 * Software setting
 *********************************************************/
#define ASPEED_CLOCK_FREQ_DEFAULT       3000000
#define ASPEED_ADC_INIT_TIMEOUT         500000
#define ASPEED_CV_SAMPLE_TIMES          10

/**********************************************************
 * ADC private data
 *********************************************************/
struct aspeed_adc_priv_s {
	uint32_t ref_voltage_mv;
	uint32_t clk_freq;
	uint32_t sampling_rate;
	int cv;
	void *parent;
	uint8_t battery_en;
	uint8_t battery_div;
	uint8_t do_calibration;
	uint8_t index;
};

#endif
