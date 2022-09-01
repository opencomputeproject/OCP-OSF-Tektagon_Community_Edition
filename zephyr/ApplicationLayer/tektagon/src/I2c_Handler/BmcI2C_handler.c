//***********************************************************************//
//*                                                                     *//
//*                      Copyright © 2022 AMI                           *//
//*                                                                     *//
//*        All rights reserved. Subject to AMI licensing agreement.     *//
//*                                                                     *//
//***********************************************************************//

#include <drivers/i2c.h>
#include "Smbus_mailbox/Smbus_mailbox.h"
#ifdef CONFIG_INTEL_PFR_SUPPORT
#include "intel_2.0/intel_pfr_verification.h"
#include "intel_2.0/intel_pfr_provision.h"
#endif
#ifdef CONFIG_CERBERUS_PFR_SUPPORT
#include "cerberus/cerberus_pfr_verification.h"
#include "cerberus/cerberus_pfr_provision.h"
#endif
#include <../../Silicon/AST1060/i2c/I2C_Slave_aspeed.h>
// extern struct i2c_slave_callbacks i2c_1060_callbacks_pch;
extern I2C_Slave_Process gI2cSlaveProcess;
// extern struct i2c_slave_callbacks i2c_1060_callbacks_bmc;
extern uint8_t gBmcFlag;
extern uint8_t gProvisinDoneFlag;
uint8_t gI2CReadFlag;
uint8_t gDataReceived;
EVENT_CONTEXT I2CData;
AO_DATA I2CActiveObjectData;
/*	* I2c slave device callback function.
 * there are 5 callback function need to creat and link into I2c slave device when initial I2c device as slave device
 * and callback function structure is
 * i2c_slave_write_requested_cb_t write_requested;
 * i2c_slave_read_requested_cb_t read_requested;
 * i2c_slave_write_received_cb_t write_received;
 * i2c_slave_read_processed_cb_t read_processed;
 * i2c_slave_stop_cb_t stop;
 *
 * in I2C read byte protcol, the slave callback function will in the following order
 * write_requested->->write_receive(receive mailbox register address)->read_requested(return mailbox register value)
 *
 * in I2C write byte protcol, the slave callback function will in the following order
 * write_requested->write_receive(receive mailbox register address)->write_receive(receive mailbox command)
 */

/*	* i2c_1060_slave_cb2_write_requested
 * callback function for I2C_2 write requested
 *
 * @param config i2c_slave_config
 *
 * @return 0
 */
int i2c_1060_slave_bmc_write_requested(struct i2c_slave_config *config)
{
	gI2cSlaveProcess.operation = MASTER_CMD_READ;

	return 0;
}

/*	* i2c_1060_slave_cb2_read_requested
 * callback function for I2C_2 read requested
 *
 * @param config i2c_slave_config
 *               val    pointer to store mailbox register vaule
 *
 * @return 0
 */
int i2c_1060_slave_bmc_read_requested(struct i2c_slave_config *config,
				      uint8_t *val)
{
	if (gI2cSlaveProcess.operation == MASTER_DATA_READ_SLAVE_DATA_SEND) {
		gI2CReadFlag = TRUE;
		gBmcFlag = TRUE;
		*val = PchBmcCommands(gI2cSlaveProcess.DataBuf, gI2CReadFlag);
		printk("read command:%x, return value:%x \n", gI2cSlaveProcess.DataBuf[0], *val);
	}
	gDataReceived = 0;
	ClearI2cSlaveProcessData();
	return 0;
}

/*	* i2c_1060_slave_cb2_write_received
 * callback function for I2C_2 write received
 *
 * @param config i2c_slave_config
 *               val    value which receive from master device
 *
 * @return 0
 */
int i2c_1060_slave_bmc_write_received(struct i2c_slave_config *config,
				      uint8_t val)
{
	// read protocol to read master command
	if (gI2cSlaveProcess.operation == MASTER_CMD_READ) {
		gI2cSlaveProcess.DataBuf[SLAVE_BUF_INDEX0] = val;
		gI2cSlaveProcess.operation = MASTER_DATA_READ_SLAVE_DATA_SEND;
	}

	// write protocol to read master data after read master command
	if (gI2cSlaveProcess.operation == MASTER_DATA_READ_SLAVE_DATA_SEND) {
		gI2cSlaveProcess.DataBuf[SLAVE_BUF_INDEX1] = val;
		gI2cSlaveProcess.InProcess = I2CInProcess_Flag;
		gBmcFlag = TRUE;
		gDataReceived = gDataReceived + 1;
		// printk("write command :%x %x\n", gI2cSlaveProcess.DataBuf[0],gI2cSlaveProcess.DataBuf[1]);
	}

	if (gDataReceived == 2) {

		I2CActiveObjectData.ProcessNewCommand = 1;
		I2CActiveObjectData.type = I2C_EVENT;

		I2CData.operation = I2C_HANDLE;
		I2CData.i2c_data = gI2cSlaveProcess.DataBuf;
		post_smc_action(I2C, &I2CActiveObjectData, &I2CData);
		printk("write command :%x %x\n", gI2cSlaveProcess.DataBuf[0],gI2cSlaveProcess.DataBuf[1]);
		gDataReceived = 0;
	}

	return 0;
}

/*	* i2c_1060_slave_cb2_read_processed
 * callback function for I2C_2 read processed
 *
 * @param config i2c_slave_config
 *               val    value send to master device
 *
 * @return 0
 */
int i2c_1060_slave_bmc_read_processed(struct i2c_slave_config *config,
				      uint8_t *val)
{
	// printk("i2c_1060_slave_cb2_read_processed read_processed *val = %x\n",val);
	return 0;
}

/*	* i2c_1060_slave_cb2_stop
 * callback function for I2C_2 stop
 *
 * @param config i2c_slave_config
 *
 * @return 0
 */
int i2c_1060_slave_bmc_stop(struct i2c_slave_config *config)
{
	// printk("i2c_1060_slave_cb2_stop stop\n");

	return 0;
}

/*	* i2c_1060_callbacks_2
 * callback function for I2C_2
 */
struct i2c_slave_callbacks i2c_1060_callbacks_bmc = {
	.write_requested = i2c_1060_slave_bmc_write_requested,
	.read_requested = i2c_1060_slave_bmc_read_requested,
	.write_received = i2c_1060_slave_bmc_write_received,
	.read_processed = i2c_1060_slave_bmc_read_processed,
	.stop = i2c_1060_slave_bmc_stop,
};
