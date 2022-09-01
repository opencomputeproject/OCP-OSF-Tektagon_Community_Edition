/*
 * Copyright (c) 2021 Aspeed Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef _JTAG_ASPEED_H_
#define _JTAG_ASPEED_H_

/**********************************************************
 * Jtag register fields
 *********************************************************/
union data_port_register_s {
	volatile uint32_t value;
	struct {
		volatile uint32_t data : 32; /*[0-31]*/

	} fields;
}; /* 00000000-00000004 */

union mode_1_control_s {
	volatile uint32_t value;
	struct {
		volatile uint32_t dr_xfer_en : 1;               /*[0-0]*/
		volatile uint32_t ir_xfer_en : 1;               /*[1-1]*/
		volatile uint32_t reserved0 : 2;                /*[2-3]*/
		volatile uint32_t last_xfer : 1;                /*[4-4]*/
		volatile uint32_t terminating_xfer : 1;         /*[5-5]*/
		volatile uint32_t msb_first : 1;                /*[6-6]*/
		volatile uint32_t reserved1 : 1;                /*[7-7]*/
		volatile uint32_t xfer_len : 10;                /*[8-17]*/
		volatile uint32_t reserved2 : 2;                /*[18-19]*/
		volatile uint32_t internal_fifo_mode : 1;       /*[20-20]*/
		volatile uint32_t reset_internal_fifo : 1;      /*[21-21]*/
		volatile uint32_t reserved3 : 7;                /*[22-28]*/
		volatile uint32_t reset_to_tlr : 1;             /*[29-29]*/
		volatile uint32_t engine_output_enable : 1;     /*[30-30]*/
		volatile uint32_t engine_enable : 1;            /*[31-31]*/

	} fields;
}; /* 00000008 */

union mode_1_int_ctrl_s {
	volatile uint32_t value;
	struct {
		volatile uint32_t enable_of_data_xfer_completed : 1;    /*[0-0]*/
		volatile uint32_t enable_of_data_xfer_pause : 1;        /*[1-1]*/
		volatile uint32_t enable_of_instr_xfer_completed : 1;   /*[2-2]*/
		volatile uint32_t enable_of_instr_xfer_pause : 1;       /*[3-3]*/
		volatile uint32_t reserved0 : 12;                       /*[4-15]*/
		volatile uint32_t data_xfer_completed : 1;              /*[16-16]*/
		volatile uint32_t data_xfer_pause : 1;                  /*[17-17]*/
		volatile uint32_t instr_xfer_completed : 1;             /*[18-18]*/
		volatile uint32_t instr_xfer_pause : 1;                 /*[19-19]*/
		volatile uint32_t reserved1 : 12;                       /*[20-31]*/

	} fields;
}; /* 0000000c */

union software_mode_and_status_s {
	volatile uint32_t value;
	struct {
		volatile uint32_t engine_idle : 1;              /*[0-0]*/
		volatile uint32_t data_xfer_pause : 1;          /*[1-1]*/
		volatile uint32_t instr_xfer_pause : 1;         /*[2-2]*/
		volatile uint32_t reserved0 : 13;               /*[3-15]*/
		volatile uint32_t software_tdi_and_tdo : 1;     /*[16-16]*/
		volatile uint32_t software_tms : 1;             /*[17-17]*/
		volatile uint32_t software_tck : 1;             /*[18-18]*/
		volatile uint32_t software_mode_enable : 1;     /*[19-19]*/
		volatile uint32_t reserved1 : 12;               /*[20-31]*/

	} fields;
}; /* 00000010 */

union tck_control_s {
	volatile uint32_t value;
	struct {
		volatile uint32_t tck_divisor : 11;     /*[0-10]*/
		volatile uint32_t reserved0 : 20;       /*[11-30]*/
		volatile uint32_t tck_inverse : 1;      /*[31-31]*/

	} fields;
}; /* 00000014 */

union engine_control_1_s {
	volatile uint32_t value;
	struct {
		volatile uint32_t to_irt_state : 1;     /*[0-0]*/
		volatile uint32_t reserved0 : 30;       /*[1-30]*/
		volatile uint32_t control_of_trstn : 1; /*[31-31]*/

	} fields;
}; /* 00000018 */

union reserved_s {
	volatile uint32_t value;
	struct {
		volatile uint32_t reserved0 : 32; /*[0-31]*/

	} fields;
}; /* 0000001c */

union padding_control_0_s {
	volatile uint32_t value;
	struct {
		volatile uint32_t pre_padding_number : 9;       /*[0-8]*/
		volatile uint32_t reserved0 : 3;                /*[9-11]*/
		volatile uint32_t post_padding_number : 9;      /*[12-20]*/
		volatile uint32_t reserved1 : 3;                /*[21-23]*/
		volatile uint32_t padding_data : 1;             /*[24-24]*/
		volatile uint32_t reserved2 : 7;                /*[25-31]*/

	} fields;
}; /* 00000028 */

union padding_control_1_s {
	volatile uint32_t value;
	struct {
		volatile uint32_t pre_padding_number : 9;       /*[0-8]*/
		volatile uint32_t reserved0 : 3;                /*[9-11]*/
		volatile uint32_t post_padding_number : 9;      /*[12-20]*/
		volatile uint32_t reserved1 : 3;                /*[21-23]*/
		volatile uint32_t padding_data : 1;             /*[24-24]*/
		volatile uint32_t reserved2 : 7;                /*[25-31]*/

	} fields;
}; /* 0000002c */

union shift_control_s {
	volatile uint32_t value;
	struct {
		volatile uint32_t lower_data_shift_number : 7;  /*[0-6]*/
		volatile uint32_t start_of_shift : 1;           /*[7-7]*/
		volatile uint32_t end_of_shift : 1;             /*[8-8]*/
		volatile uint32_t paddign_selection : 1;        /*[9-9]*/
		volatile uint32_t pre_tms_shift_number : 3;     /*[10-12]*/
		volatile uint32_t post_tms_shift_number : 3;    /*[13-15]*/
		volatile uint32_t tms_value : 14;               /*[16-29]*/
		volatile uint32_t enable_static_shift : 1;      /*[30-30]*/
		volatile uint32_t enable_free_run_tck : 1;      /*[31-31]*/

	} fields;
}; /* 00000030 */

union mode_2_control_s {
	volatile uint32_t value;
	struct {
		volatile uint32_t clock_divisor : 12;           /*[0-11]*/
		volatile uint32_t reserved0 : 3;                /*[12-14]*/
		volatile uint32_t trst_value : 1;               /*[15-15]*/
		volatile uint32_t static_shift_value : 1;       /*[16-16]*/
		volatile uint32_t reserved1 : 3;                /*[17-19]*/
		volatile uint32_t upper_data_shift_number : 3;  /*[20-22]*/
		volatile uint32_t reserved2 : 1;                /*[23-23]*/
		volatile uint32_t internal_fifo_mode : 1;       /*[24-24]*/
		volatile uint32_t reset_internal_fifo : 1;      /*[25-25]*/
		volatile uint32_t reserved3 : 3;                /*[26-28]*/
		volatile uint32_t reset_to_tlr : 1;             /*[29-29]*/
		volatile uint32_t engine_output_enable : 1;     /*[30-30]*/
		volatile uint32_t engine_enable : 1;            /*[31-31]*/

	} fields;
}; /* 00000034 */

union mode_2_int_ctrl_s {
	volatile uint32_t value;
	struct {
		volatile uint32_t shift_complete_interrupt_status : 1;  /*[0-0]*/
		volatile uint32_t reserved0 : 15;                       /*[1-15]*/
		volatile uint32_t shift_complete_interrupt_enable : 1;  /*[16-16]*/
		volatile uint32_t reserved1 : 15;                       /*[17-31]*/

	} fields;
}; /* 00000038 */

union fsm_status_s {
	volatile uint32_t value;
	struct {
		volatile uint32_t engine_idle : 1;      /*[0-0]*/
		volatile uint32_t reserved0 : 31;       /*[1-31]*/

	} fields;
}; /* 0000003c */

struct jtag_register_s {
	union data_port_register_s data_for_hw_mode_1[2];               /* 00000000, 00000004 */
	union mode_1_control_s mode_1_control;                          /* 00000008 */
	union mode_1_int_ctrl_s mode_1_int_ctrl;                        /* 0000000c */
	union software_mode_and_status_s software_mode_and_status;      /* 00000010 */
	union tck_control_s tck_control;                                /* 00000014 */
	union engine_control_1_s engine_control_1;                      /* 00000018 */
	union reserved_s reserved0[1];                                  /* 0000001c~0000001c*/
	union data_port_register_s data_for_hw_mode_2[2];               /* 00000020, 00000024 */
	union padding_control_0_s padding_control_0;                    /* 00000028 */
	union padding_control_1_s padding_control_1;                    /* 0000002c */
	union shift_control_s shift_control;                            /* 00000030 */
	union mode_2_control_s mode_2_control;                          /* 00000034 */
	union mode_2_int_ctrl_s mode_2_int_ctrl;                        /* 00000038 */
	union fsm_status_s fsm_status;                                  /* 0000003c */
};


#define JTAG_ASPEED_INST_PAUSE                  BIT(19)
#define JTAG_ASPEED_INST_COMPLETE               BIT(18)
#define JTAG_ASPEED_DATA_PAUSE                  BIT(17)
#define JTAG_ASPEED_DATA_COMPLETE               BIT(16)
#define JTAG_ASPEED_INT_PEND_MASK               GENMASK(19, 16)

#define JTAG_ASPEED_MAX_FREQUENCY 10000000

#endif /* end of "#ifndef _JTAG_ASPEED_H_" */
