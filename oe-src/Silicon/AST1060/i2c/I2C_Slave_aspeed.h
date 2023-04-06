#ifndef ZEPHYR_INCLUDE_I2C_SLAVE_DEVICE_API_MIDLEYER_H_
#define ZEPHYR_INCLUDE_I2C_SLAVE_DEVICE_API_MIDLEYER_H_
#include <zephyr/types.h>
#include <stddef.h>
#include <device.h>
#include <drivers/clock_control.h>

#define I2C_SLAVE_BUF_SIZE		256
enum i2c_xfer_mode {
	DMA_MODE = 0,
	BUFF_MODE,
	BYTE_MODE,
};

struct i2c_aspeed_config {
	uint32_t global_reg;
	uintptr_t base;
	/* Buffer mode */
	uintptr_t buf_base;
	size_t buf_size;
	uint32_t bitrate;
	int multi_master;
	int smbus_alert;
	const struct device *clock_dev;
	const clock_control_subsys_t clk_id;
	uint32_t ac_timing;
	void (*irq_config_func)(const struct device *dev);
	uint32_t clk_src;
	int clk_div_mode;       /* 0: old mode, 1: new mode */
	enum i2c_xfer_mode mode;
};

struct i2c_aspeed_data {
	struct k_sem sync_sem;
	struct k_mutex trans_mutex;

	int alert_enable;

	uint32_t bus_recover;

	struct i2c_msg          *msgs;  /* cur xfer msgs */
	uint16_t addr;
	int buf_index;                  /*buffer mode idx */
	int msgs_index;                 /* cur xfer msgs index */
	int msgs_count;                 /* total msgs */

	int master_xfer_cnt;            /* total xfer count */

	bool slave_attached;

	int xfer_complete;

	uint32_t bus_frequency;
	/* master configuration */
	bool multi_master;
	int cmd_err;
	uint16_t flags;
	uint8_t         *buf;
	int len;                        /* master xfer count */

	/* slave configuration */
	uint8_t slave_addr;
	uint32_t slave_xfer_len;
	uint32_t slave_xfer_cnt;

	/* byte mode check re-start */
	uint8_t slave_addr_last;

#ifdef CONFIG_I2C_SLAVE
	unsigned char slave_dma_buf[I2C_SLAVE_BUF_SIZE];
	struct i2c_slave_config *slave_cfg;
#endif
};

typedef struct _I2C_Slave_Process {
	uint8_t InProcess;
	uint8_t operation;
	uint8_t DataBuf[2];
} I2C_Slave_Process;

enum {
	SLAVE_IDLE,
	MASTER_CMD_READ,
	MASTER_DATA_READ_SLAVE_DATA_SEND,
};

#define SLAVE_BUF_INDEX0                        0x00
#define SLAVE_BUF_INDEX1                        0x01

#define SlaveDataReceiveComplete        0x02
#define I2CInProcess_Flag                       0x01

int ast_i2c_slave_dev_init(const struct device *dev, uint8_t slave_addr);
// void PchBmcProcessCommands(unsigned char *CipherText);

#define SLAVE_BUF_DMA_MODE 0
#define SLAVE_BUF_BUFF_MODE 1
#define SLAVE_BUF_BYTE_MODE 2
#define SLAVE_BUF_B_size 0x80
#endif