//***********************************************************************//
//*                                                                     *//
//*                      Copyright © 2022 AMI                           *//
//*                                                                     *//
//*        All rights reserved. Subject to AMI licensing agreement.     *//
//*                                                                     *//
//***********************************************************************//

#include <drivers/i2c.h>

#include <../../Silicon/AST1060/i2c/I2C_Slave_aspeed.h>
#ifdef CONFIG_INTEL_PFR_SUPPORT
#include "intel_2.0/intel_pfr_verification.h"
#include "intel_2.0/intel_pfr_provision.h"
#endif
#ifdef CONFIG_CERBERUS_PFR_SUPPORT
#include "cerberus/cerberus_pfr_verification.h"
#include "cerberus/cerberus_pfr_provision.h"
#endif
// extern struct i2c_slave_callbacks i2c_1060_callbacks_pch;
extern I2C_Slave_Process gI2cSlaveProcess;
extern uint8_t gBmcFlag;


/*	* i2c_1060_slave_cb1_write_requested
 * callback function for I2C_1 write requested
 *
 * @param config i2c_slave_config
 *
 * @return 0
 */
int i2c_1060_slave_pch_write_requested(struct i2c_slave_config *config)
{
	gI2cSlaveProcess.operation = MASTER_CMD_READ;
	printk("i2c_1060_slave_cb1_write_requested write_requested\n");
	return 0;
}

/*	* i2c_1060_slave_cb1_read_requested
 * callback function for I2C_1 read requested
 *
 * @param config i2c_slave_config
 *               val    pointer to store mailbox register vaule
 *
 * @return 0
 */
int i2c_1060_slave_pch_read_requested(struct i2c_slave_config *config,
				      uint8_t *val)
{
	if (gI2cSlaveProcess.operation == MASTER_DATA_READ_SLAVE_DATA_SEND) {
		gBmcFlag = FALSE;
		*val = PchBmcCommands(&gI2cSlaveProcess.DataBuf[SLAVE_BUF_INDEX0]);
	}
	printk("i2c_1060_slave_cb1_read_requested read_requested *val = %x\n", *val);
	ClearI2cSlaveProcessData();
	return *val;
}

/*	* i2c_1060_slave_cb1_write_received
 * callback function for I2C_1 write received
 *
 * @param config i2c_slave_config
 *               val    value which receive from master device
 *
 * @return 0
 */
int i2c_1060_slave_pch_write_received(struct i2c_slave_config *config,
				      uint8_t val)
{
	printk("i2c_1060_slave_cb1_write_received write_received val = %x\n", val);
	// read protocol to read master command
	if (gI2cSlaveProcess.operation == MASTER_CMD_READ) {
		gI2cSlaveProcess.DataBuf[SLAVE_BUF_INDEX0] = val;
		gI2cSlaveProcess.operation = MASTER_DATA_READ_SLAVE_DATA_SEND;
	}

	// write protocol to read master data after read master command
	if (gI2cSlaveProcess.operation == MASTER_DATA_READ_SLAVE_DATA_SEND) {
		gI2cSlaveProcess.DataBuf[SLAVE_BUF_INDEX1] = val;
		gI2cSlaveProcess.InProcess = I2CInProcess_Flag;
		gBmcFlag = FALSE;
		PchBmcCommands(&gI2cSlaveProcess.DataBuf[SLAVE_BUF_INDEX0]);
	}
	return 0;
}

/*	* i2c_1060_slave_cb1_read_processed
 * callback function for I2C_1 read processed
 *
 * @param config i2c_slave_config
 *               val    value send to master device
 *
 * @return 0
 */
int i2c_1060_slave_pch_read_processed(struct i2c_slave_config *config,
				      uint8_t *val)
{
	printk("i2c_1060_slave_cb1_read_processed read_processed *val = %x\n", val);
	return 0;
}

/*	* i2c_1060_slave_cb1_stop
 * callback function for I2C_1 stop
 *
 * @param config i2c_slave_config
 *
 * @return 0
 */
int i2c_1060_slave_pch_stop(struct i2c_slave_config *config)
{
	printk("i2c_1060_slave_cb1_stop stop\n");
	return 0;
}

/*	* i2c_1060_callbacks_1
 * callback function for I2C_1
 */
struct i2c_slave_callbacks i2c_1060_callbacks_pch = {
	.write_requested = i2c_1060_slave_pch_write_requested,
	.read_requested = i2c_1060_slave_pch_read_requested,
	.write_received = i2c_1060_slave_pch_write_received,
	.read_processed = i2c_1060_slave_pch_read_processed,
	.stop = i2c_1060_slave_pch_stop,
};
