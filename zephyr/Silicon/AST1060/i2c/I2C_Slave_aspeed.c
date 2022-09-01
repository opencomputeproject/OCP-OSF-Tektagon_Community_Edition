#include "I2C_Slave_aspeed.h"
#include <drivers/clock_control.h>
#include <drivers/i2c.h>

// static SMBUS_MAIL_BOX gSmbusMailboxData = {0};
I2C_Slave_Process gI2cSlaveProcess = { 0 };

/*	* ClearI2cSlaveProcessData
 * function to clear structure "I2C_Slave_Process" data and flag
 *
 * @param None
 *
 * @return None
 */
void ClearI2cSlaveProcessData(void)
{
	gI2cSlaveProcess.InProcess = 0;
	gI2cSlaveProcess.operation = SLAVE_IDLE;
	gI2cSlaveProcess.DataBuf[0] = 0;
	gI2cSlaveProcess.DataBuf[1] = 0;
}


/*	* SetCpldIdentifier
 * function to update I2C mailbox variable "CpldIdentifier"
 *
 * @param Data data to update mailbox
 *
 * @return None
 */
/*
void SetCpldIdentifier(uint8_t Data)
{
     gSmbusMailboxData.CpldIdentifier = Data;
     //	UpdateMailboxRegisterFile(CpldIdentifier,(uint8_t)gSmbusMailboxData.CpldIdentifier);
}

/*	* GetCpldIdentifier
 * function to get I2C mailbox variable "CpldIdentifier"
 *
 * @param None
 *
 * @return mabilbox register value
 */
/*
uint8_t GetCpldIdentifier(void)
{
     return gSmbusMailboxData.CpldIdentifier;
}

/* ProcessCommands
 * function to process mailbox command, since I2c slave device receive data is using interrupt.
 * for command which only use for read mailbox register value,it can work in interrupt.
 * But for other commnad which need to process,recommand work after interrupt finished it's service.
 *
 */

/* PchBmcProcessCommands
 * function to process mailbox command after interrupt finished it's service
 *
 * @param *CipherText command which send by I2C master device
 *
 * @return None
 */
void PchBmcProcessCommands(unsigned char *CipherText)
{
	printk("PchBmcProcessCommands CipherText[0] =%x\n", CipherText[0]);
	printk("PchBmcProcessCommands CipherText[1] =%x\n", CipherText[1]);
	ClearI2cSlaveProcessData();
}

/*	* PchBmcProcessCommands
 * function to process mailbox read command in interrupt
 *
 * @param *CipherText mailbox register index
 *
 * @return mailbox register data
 */
/*
uint8_t PchBmcReadCommands(unsigned char *CipherText)
{
     uint8_t DataToSend = 0;
     printk("PchBmcReadCommands CipherText[0] = %x\n",CipherText[0]);
     switch(CipherText[0]) {
     case CpldIdentifier:
          DataToSend = GetCpldIdentifier();
          break;
     }
     return DataToSend;
}
*/

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
/*
int i2c_1060_slave_cb2_write_requested(struct i2c_slave_config *config)
{
     gI2cSlaveProcess.operation = MASTER_CMD_READ;
     printk("i2c_1060_slave_cb2_write_requested write_requested\n");
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
/*int i2c_1060_slave_cb2_read_requested(struct i2c_slave_config *config,
                                        uint8_t *val)
   {
        if(gI2cSlaveProcess.operation == MASTER_DATA_READ_SLAVE_DATA_SEND)
        {
 * val = PchBmcReadCommands(&gI2cSlaveProcess.DataBuf[SLAVE_BUF_INDEX0]);
        }
        printk("i2c_1060_slave_cb2_read_requested read_requested *val = %x\n",*val);
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
/*int i2c_1060_slave_cb2_write_received(struct i2c_slave_config *config,
                                       uint8_t val)
   {
        printk("i2c_1060_slave_cb2_write_received write_received val = %x\n",val);
        //read protocol to read master command
        if(gI2cSlaveProcess.operation == MASTER_CMD_READ)
        {
                gI2cSlaveProcess.DataBuf[SLAVE_BUF_INDEX0] = val;
                gI2cSlaveProcess.operation = MASTER_DATA_READ_SLAVE_DATA_SEND;
        }

        //write protocol to read master data after read master command
        if(gI2cSlaveProcess.operation == MASTER_DATA_READ_SLAVE_DATA_SEND)
        {
                gI2cSlaveProcess.DataBuf[SLAVE_BUF_INDEX1] = val;
                gI2cSlaveProcess.InProcess = I2CInProcess_Flag;
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
/*int i2c_1060_slave_cb2_read_processed(struct i2c_slave_config *config,
                                       uint8_t *val)
   {
        printk("i2c_1060_slave_cb2_read_processed read_processed *val = %x\n",val);
        return 0;
   }

   /*	* i2c_1060_slave_cb2_stop
 * callback function for I2C_2 stop
 *
 * @param config i2c_slave_config
 *
 * @return 0
 */
/*int i2c_1060_slave_cb2_stop(struct i2c_slave_config *config)
   {
        printk("i2c_1060_slave_cb2_stop stop\n");
        return 0;
   }

   /*	* i2c_1060_callbacks_2
 * callback function for I2C_2
 */
/*const struct i2c_slave_callbacks i2c_1060_callbacks_2 =
   {
        .write_requested = i2c_1060_slave_cb2_write_requested,
        .read_requested = i2c_1060_slave_cb2_read_requested,
        .write_received =i2c_1060_slave_cb2_write_received,
        .read_processed = i2c_1060_slave_cb2_read_processed,
        .stop = i2c_1060_slave_cb2_stop,
   };

   /*	* i2c_1060_slave_cb1_write_requested
 * callback function for I2C_1 write requested
 *
 * @param config i2c_slave_config
 *
 * @return 0
 */
/*int i2c_1060_slave_cb1_write_requested(struct i2c_slave_config *config)
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
/*int i2c_1060_slave_cb1_read_requested(struct i2c_slave_config *config,
                                       uint8_t *val)
   {
        if(gI2cSlaveProcess.operation == MASTER_DATA_READ_SLAVE_DATA_SEND)
        {
 * val = PchBmcReadCommands(&gI2cSlaveProcess.DataBuf[SLAVE_BUF_INDEX0]);
        }
        printk("i2c_1060_slave_cb1_read_requested read_requested *val = %x\n",*val);
        ClearI2cSlaveProcessData();
        return 0;
   }

   /*	* i2c_1060_slave_cb1_write_received
 * callback function for I2C_1 write received
 *
 * @param config i2c_slave_config
 *               val    value which receive from master device
 *
 * @return 0
 */
/*int i2c_1060_slave_cb1_write_received(struct i2c_slave_config *config,
                                       uint8_t val)
   {
        printk("i2c_1060_slave_cb1_write_received write_received val = %x\n",val);
        //read protocol to read master command
        if(gI2cSlaveProcess.operation == MASTER_CMD_READ)
        {
                gI2cSlaveProcess.DataBuf[SLAVE_BUF_INDEX0] = val;
                gI2cSlaveProcess.operation = MASTER_DATA_READ_SLAVE_DATA_SEND;
        }

        //write protocol to read master data after read master command
        if(gI2cSlaveProcess.operation == MASTER_DATA_READ_SLAVE_DATA_SEND)
        {
                gI2cSlaveProcess.DataBuf[SLAVE_BUF_INDEX1] = val;
                gI2cSlaveProcess.InProcess = I2CInProcess_Flag;
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
/*int i2c_1060_slave_cb1_read_processed(struct i2c_slave_config *config,
                                       uint8_t *val)
   {
        printk("i2c_1060_slave_cb1_read_processed read_processed *val = %x\n",val);
        return 0;
   }

   /*	* i2c_1060_slave_cb1_stop
 * callback function for I2C_1 stop
 *
 * @param config i2c_slave_config
 *
 * @return 0
 */
/*int i2c_1060_slave_cb1_stop(struct i2c_slave_config *config)
   {
        printk("i2c_1060_slave_cb1_stop stop\n");
        return 0;
   }

   /*	* i2c_1060_callbacks_1
 * callback function for I2C_1
 */
/*const struct i2c_slave_callbacks i2c_1060_callbacks_1 =
   {
        .write_requested = i2c_1060_slave_cb1_write_requested,
        .read_requested = i2c_1060_slave_cb1_read_requested,
        .write_received =i2c_1060_slave_cb1_write_received,
        .read_processed = i2c_1060_slave_cb1_read_processed,
        .stop = i2c_1060_slave_cb1_stop,
   };

   /*	* ast_i2c_slave_dev_init
 * Initial I2C device to slave device,only support I2C_1 and I2C_2 so far
 *
 * @param dev I2C device (EX:I2C_1 or I2C_2)
 * @param slave_addr I2C slave device address to setting
 *
 * @return Transfer status, 0 if success or an error code.
 */
#define DEV_CFG(dev) \
	((struct i2c_aspeed_config *)(dev)->config)
#define DEV_DATA(dev) \
	((struct i2c_aspeed_data *)(dev)->data)

extern struct i2c_slave_callbacks i2c_1060_callbacks_bmc;
extern struct i2c_slave_callbacks i2c_1060_callbacks_pch;
static struct i2c_slave_config slave_cfg_temp[2];

int ast_i2c_slave_dev_init(const struct device *dev, uint8_t slave_addr)
{
	struct i2c_slave_config slave_cfg;

	printk("ast_i2c_slave_dev_init\n");
	slave_cfg.address = slave_addr;

     struct i2c_aspeed_config *config = DEV_CFG(dev);
	struct i2c_aspeed_data *data = DEV_DATA(dev);

     if (!strcmp(dev->name, "I2C_1"))
	{
		data->slave_cfg = &slave_cfg_temp[0];
		data->slave_cfg->callbacks = &i2c_1060_callbacks_bmc;
	}
	else if(!strcmp(dev->name, "I2C_2"))
	{
		data->slave_cfg = &slave_cfg_temp[1];
		data->slave_cfg->callbacks = &i2c_1060_callbacks_pch;
	}

	if (i2c_slave_register(dev, &slave_cfg)) {
		printk("I2C: Slave Device driver %s not found.", dev->name);
	}

	SetCpldIdentifier(0xDE);        // initial CpldIdentifier to 0xDE for test
	gI2cSlaveProcess.operation = SLAVE_IDLE;
	return 0;
}