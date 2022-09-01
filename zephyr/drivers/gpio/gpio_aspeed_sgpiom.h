#ifndef _GPIO_ASPEED_SGPIOM_H_
#define _GPIO_ASPEED_SGPIOM_H_

/**
 * SGPIOM register fields
 */
union sgpiom_data_value_register_s {
	volatile uint32_t value;
	struct {
		volatile uint8_t data[4];
	} fields;
};

union sgpiom_int_en_register_s {
	volatile uint32_t value;
	struct {
		volatile uint8_t int_en[4];
	} fields;
};

union sgpiom_int_sens_type_register_s {
	volatile uint32_t value;
	struct {
		volatile uint8_t int_sens_type[4];
	} fields;
};

union sgpiom_int_status_register_s {
	volatile uint32_t value;
	struct {
		volatile uint8_t int_status[4];
	} fields;
};

union sgpiom_wdt_reset_tolerant_register_s {
	volatile uint32_t value;
	struct {
		volatile uint8_t wdt_reset_tolerant_en[4];
	} fields;
};


union sgpiom_input_mask_register_s {
	volatile uint32_t value;
	struct {
		volatile uint8_t input_mask[4];
	} fields;
};

union sgpiom_write_latch_register_s {
	volatile uint32_t value;
	struct {
		volatile uint8_t wr_latch[4];
	} fields;
};

union sgpiom_config_register_s {
	volatile uint32_t value;
	struct {
		volatile uint32_t enable : 1;           /*[0-0]*/
		volatile uint32_t reserved0 : 5;        /*[1-5]*/
		volatile uint32_t numbers : 5;          /*[6-10]*/
		volatile uint32_t reserved1 : 5;        /*[10-15]*/
		volatile uint32_t division : 16;        /*[16-31]*/

	} fields;
};

struct sgpiom_gather_register_s {
	union sgpiom_data_value_register_s data;                        /* 00000000 */
	union sgpiom_int_en_register_s int_en;                          /* 00000004 */
	union sgpiom_int_sens_type_register_s int_sens_type[3];         /* 00000008~00000010 */
	union sgpiom_int_status_register_s int_status;                  /* 00000014 */
	union sgpiom_wdt_reset_tolerant_register_s rst_tolerant;        /* 00000018 */
};

struct sgpiom_register_s {
	struct sgpiom_gather_register_s gather0;                /* 00000000~00000018 */
	struct sgpiom_gather_register_s gather1;                /* 0000001c~00000034 */
	struct sgpiom_gather_register_s gather2;                /* 00000038~00000050 */
	union sgpiom_config_register_s config;                  /* 00000054 */
	union sgpiom_input_mask_register_s input_mask[4];       /* 00000058~00000064 */
	uint32_t reserved0[2];                                  /* 00000068~0000006c*/
	union sgpiom_write_latch_register_s wr_latch[4];        /* 00000070~0000007c */
	uint32_t reserved1[4];                                  /* 00000080~0000008c*/
	struct sgpiom_gather_register_s gather3;                /* 00000090~000000a8 */
	uint32_t reserved2[21];                                 /* 000000ac~000000fc*/
};

enum aspeed_sgpiom_int_type_t {
	ASPEED_SGPIOM_FALLING_EDGE = 0,
	ASPEED_SGPIOM_RAISING_EDGE,
	ASPEED_SGPIOM_LEVEL_LOW,
	ASPEED_SGPIOM_LEVEL_HIGH,
	ASPEED_SGPIOM_DUAL_EDGE,
};

#endif /* end of "#ifndef _GPIO_ASPEED_SGPIOM_H_" */
