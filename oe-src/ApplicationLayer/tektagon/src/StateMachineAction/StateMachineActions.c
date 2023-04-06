//***********************************************************************//
//*                                                                     *//
//*                      Copyright © 2022 AMI                           *//
//*                                                                     *//
//*        All rights reserved. Subject to AMI licensing agreement.     *//
//*                                                                     *//
//***********************************************************************//

#include <stdlib.h>
#include "drivers/gpio.h"
#include "StateMachineActions.h"
#include "state_machine/common_smc.h"
#include "include/SmbusMailBoxCom.h"
#include "pfr/pfr_verifcation.h"
#include "pfr/pfr_update.h"
#include "flash/flash_aspeed.h"
#include <watchdog/watchdog_aspeed.h>
#include "Smbus_mailbox/Smbus_mailbox.h"
#include "SpiFilter/SpiFilter.h"
#include "logging/debug_log.h"// State Machine log saving
#include <gpio/gpio_aspeed.h>


#ifdef CONFIG_INTEL_PFR_SUPPORT
#include "intel_2.0/intel_pfr_definitions.h"
#include "intel_2.0/intel_pfr_provision.h"
#include "intel_2.0/intel_pfr_authentication.h"
#include "intel_2.0/intel_pfr_definitions.h"
#include "intel_2.0/intel_pfr_spi_filtering.h"
#endif

#ifdef CONFIG_CERBERUS_PFR_SUPPORT
#include "cerberus/cerberus_pfr_definitions.h"
#include "cerberus/cerberus_pfr_provision.h"
#include "cerberus/cerberus_pfr_authentication.h"
#include "cerberus/cerberus_pfr_definitions.h"
#include "cerberus/cerberus_pfr_spi_filtering.h"
#endif

#define RELEASE_PLATFORM 1

#define MAX_BUFFER_CHECK 79
#define MAX_LENGTH 32
#define SMBUS_WRITE 0x45

#if PF_STATUS_DEBUG
#define DEBUG_PRINTF printk
#else
#define DEBUG_PRINTF(...)
#endif

static EVENT_CONTEXT BmcData[2], PchData[2], TemperlateEvent;
AO_DATA BmcActiveObjectData, PchActiveObjectData;
AO_DATA WDT_AOData;
static EVENT_CONTEXT WDT_EventData;

void handlePowerOnFailure(void *AoData, void *EventContext)
{
	AO_DATA *ActiveObjectData = (AO_DATA *) AoData;
	EVENT_CONTEXT *EventData = (EVENT_CONTEXT *) EventContext;

	SetPlatformState(T_MINUS_1_FW_RECOVERY);	
	ActiveObjectData->ProcessNewCommand = 1;
	ActiveObjectData->PreviousState = Initial;
	ActiveObjectData->ActiveImageStatus = Failure;
	EventData->operation = RECOVER_ACTIVE;

	printk("PowerOn Failure post recovery action \n");
	post_smc_action(RECOVERY, ActiveObjectData, EventData);

}

static void wdt_callback_bmc_timeout(void)
{
	printk("enter wdt_callback_bmc_timeout \n");
	
	WDT_AOData.ActiveImageStatus = Failure;	
	WDT_AOData.PreviousState = Initial;
	WDT_AOData.ProcessNewCommand = 1;
	WDT_AOData.type = BMC_EVENT;
	WDT_EventData.image = BMC_EVENT;

	SetLastPanicReason(BMC_WDT_EXPIRE);
	SetMajorErrorCode(BMC_BOOT_FAIL);
	SetMinorErrorCode(BMC_WDT_TIMEOUT);
	SetPlatformState(WDT_TIMEOUT_RECOVERY);
	handlePowerOnFailure(&WDT_AOData, &WDT_EventData);
}

static void wdt_callback_pch_timeout(void)
{
	printk("enter wdt_callback_pch_timeout\n");

	WDT_AOData.ActiveImageStatus = Failure;	
	WDT_AOData.PreviousState = Initial;
	WDT_AOData.ProcessNewCommand = 1;
	WDT_AOData.type = PCH_EVENT;
	WDT_EventData.image = PCH_EVENT;

	SetLastPanicReason(ACM_WDT_EXPIRE);
	SetMajorErrorCode(BIOS_BOOT_FAIL);
	SetMinorErrorCode(ACM_WDT_TIMEOUT);
	SetPlatformState(WDT_TIMEOUT_RECOVERY);
	handlePowerOnFailure(&WDT_AOData, &WDT_EventData);
}

void Tektagon_EnableTimer(int type)
{
	struct watchdog_config *wdt_config = malloc(sizeof(struct watchdog_config));
	const struct device *wdt_dev;
	int ret = 0;
	uint32_t count = 0;
	wdt_config->wdt_cfg.window.min = 0;
	wdt_config->reset_option = WDT_FLAG_RESET_NONE;

	if (type == BMC_EVENT) {
		DEBUG_PRINTF("---------------------------------------\r\n");
		DEBUG_PRINTF("     Start BMC Timer\r\n");
		DEBUG_PRINTF("---------------------------------------\r\n");
		wdt_config->wdt_cfg.window.max = BMC_MAXTIMEOUT;
		wdt_config->wdt_cfg.callback = wdt_callback_bmc_timeout;
		wdt_dev = device_get_binding(WDT_Devices_List[0]);
		if (!wdt_dev) {
			printk("wdt_timer_err: cannot find wdt device.\n");
			free(wdt_config);
			return;
		}
		ret = WatchDogInit(wdt_dev, wdt_config);

		WatchDogFeed(wdt_dev, 0);

	} else if (type == PCH_EVENT) {
		DEBUG_PRINTF("---------------------------------------\r\n");
		DEBUG_PRINTF("     Start PCH Timer\r\n");
		DEBUG_PRINTF("---------------------------------------\r\n");
		wdt_config->wdt_cfg.window.max = BIOS_MAXTIMEOUT;
		wdt_config->wdt_cfg.callback = wdt_callback_pch_timeout;
		wdt_dev = device_get_binding(WDT_Devices_List[1]);
		if (!wdt_dev) {
			printk("wdt_timer_err: cannot find wdt device.\n");
			free(wdt_config);
			return;
		}
		ret = WatchDogInit(wdt_dev, wdt_config);

		WatchDogFeed(wdt_dev, 0);
	}
	
	free(wdt_config);
}

void Tektagon_DisableTimer(int type)
{
	const struct device *wdt_dev;
	if (type == BMC_EVENT) {
		DEBUG_PRINTF("---------------------------------------\r\n");
		DEBUG_PRINTF("     Disable BMC Timer\r\n");
		DEBUG_PRINTF("---------------------------------------\r\n");
		wdt_dev = device_get_binding(WDT_Devices_List[0]);
		WatchDogDisable(wdt_dev);
	} else if (type == PCH_EVENT) {
		DEBUG_PRINTF("---------------------------------------\r\n");
		DEBUG_PRINTF("     Disable PCH Timer\r\n");
		DEBUG_PRINTF("---------------------------------------\r\n");
		wdt_dev = device_get_binding(WDT_Devices_List[1]);
		WatchDogDisable(wdt_dev);
	}
	
}

/**
 * Start BMC Active Object and Invoke Initialize Signal
 *
 * @return
 */
int StartBmcAOWithEvent(void)
{
	return true;
}

/**
 * Start PCH Active Object and Invoke Initialize Signal
 *
 * @return
 */
int StartPchAOWithEvent(void)
{
	return false;
}

/**
    Function to Update the BMC data with verify signal to
    publish the signal

    @Param NIL.

    @retval NILL
 **/
void PublishBmcEvents(void)
{
	// Assigning default value
	BmcActiveObjectData.ActiveImageStatus = 1;
	BmcActiveObjectData.RecoveryImageStatus = 1;
	BmcActiveObjectData.PreviousState = Initial;
	BmcActiveObjectData.ProcessNewCommand = 1;
	BmcActiveObjectData.type = BMC_EVENT;

	BmcData[0].operation = VERIFY_BACKUP;
	BmcData[0].image = 1;
	BmcData[0].flash = SECONDARY_FLASH_REGION;

	post_smc_action(VERIFY, &BmcActiveObjectData, &BmcData[0]);

	BmcData[1].operation = VERIFY_ACTIVE;
	BmcData[1].image = 1;
	BmcData[1].flash = PRIMARY_FLASH_REGION;

	post_smc_action(VERIFY, &BmcActiveObjectData, &BmcData[1]);
}

/**
    Function to Update the PCH data with verify signal to
    publish the signal

    @Param NIL.

    @retval NILL
 **/
void PublishPchEvents(void)
{
	PchActiveObjectData.ActiveImageStatus = 1;
	PchActiveObjectData.RecoveryImageStatus = 1;
	PchActiveObjectData.PreviousState = Initial;
	PchActiveObjectData.ProcessNewCommand = 1;
	PchActiveObjectData.type = PCH_EVENT;

	PchData[0].operation = VERIFY_BACKUP;
	PchData[0].image = 2;
	PchData[0].flash = SECONDARY_FLASH_REGION;
	post_smc_action(VERIFY, &PchActiveObjectData, &PchData[0]);

	PchData[1].operation = VERIFY_ACTIVE;
	PchData[1].image = 2;
	PchData[1].flash = PRIMARY_FLASH_REGION;
	post_smc_action(VERIFY, &PchActiveObjectData, &PchData[1]);
}

void PublishInitialEvents(void)
{
	byte provision_state = get_provision_status();

	if (provision_state == UFM_PROVISIONED) {
		check_staging_area();
#if BMC_SUPPORT
		PublishBmcEvents();
#else
		PublishPchEvents();
#endif
	} else {
		// T0
		int releaseBmc = 1;
		int releasePCH = 1;
		Set_SPI_Filter_RW_Region("spi_m1", SPI_FILTER_WRITE_PRIV, SPI_FILTER_PRIV_ENABLE, 0x0, 0x08000000);
		T0Transition(releaseBmc, releasePCH);
	}
}
/**
 * Function to Update LockDown State
 */
void handleLockDownState(void *AoData)
{
	((AO_DATA *)AoData)->InLockdown = 1; // Indicate lock down status
	#if SMBUS_MAILBOX_SUPPORT
	SetPlatformState(LOCKDOWN_ON_AUTH_FAIL);
	#endif
	// Perform Any Other Cleanup
}

/**
 * Function to send I2C deny command
 * Framework will use this function to send a deny response
 */
void denyI2CCommand(void *I2CData)
{
}

/**
 * Function to perform PFR check list before releasing the Platform
 *
 * @param ImageType
 * @param AoData
 */
void CheckAndReleasePlatform(void *AoData, void *EventContext)
{
	AO_DATA *ActiveObjectData = (AO_DATA *) AoData;
	EVENT_CONTEXT *EventData = (EVENT_CONTEXT *) EventContext;
	ActiveObjectData->ProcessNewCommand = 0;
	int release_bmc = 0;
	int release_pch = 0;
	if (EventData->image == BMC_EVENT) {
		release_bmc = 1;
		DEBUG_PRINTF("---------------------------------------\r\n");
		DEBUG_PRINTF("     BMC authentication success\r\n");
		DEBUG_PRINTF("---------------------------------------\r\n");
		PublishPchEvents();
		//T0Transition(release_bmc, release_pch);
	}
	if (EventData->image == PCH_EVENT) {
		release_pch = 1;
		release_bmc = 1;
		DEBUG_PRINTF("---------------------------------------\r\n");
		DEBUG_PRINTF("     PCH authentication success\r\n");
		DEBUG_PRINTF("---------------------------------------\r\n");
		T0Transition(release_bmc, release_pch);
	}
}

/**
 * Function to handle verify success
 *
 * @param ImageType
 * @param AoData
 */
void handlePostVerifySuccess(void *AoData, void *EventContext)
{
	AO_DATA *ActiveObjectData = (AO_DATA *) AoData;
	EVENT_CONTEXT *EventData = (EVENT_CONTEXT *) EventContext;

	if ((ActiveObjectData->ActiveImageStatus == Success) &&
	    (ActiveObjectData->RecoveryImageStatus == Success)) {
		ActiveObjectData->RestrictActiveUpdate = 0;
		CheckAndReleasePlatform(AoData, EventContext);
	}

	// Starting Timer
	// Tektagon_EnableTimer(((AO_DATA *)AoData)->type);
}

/**
 * Function to handle verify failure
 *
 * @param ImageType
 * @param AoData
 */
void handlePostVerifyFailure(void *AoData, void *EventContext)
{
	AO_DATA *ActiveObjectData = (AO_DATA *) AoData;
	EVENT_CONTEXT *EventData = (EVENT_CONTEXT *) EventContext;

	if (ActiveObjectData->ProcessNewCommand == 1) {
		ActiveObjectData->PreviousState = Verify;
		EventData->operation = RECOVER_ACTIVE;

		post_smc_action(RECOVERY, ActiveObjectData, EventData);
	}
}
/**
 * Function to handle Verify Entry
 * Perform any verify library initialization if required here
 *
 * @param type
 * @param data
 */
void handleVerifyEntryState(void *data, void *event_context)
{
	// Stops WDT Timer;
	AO_DATA *ao_data = (AO_DATA *)data;
	int type = ao_data->type;

	// Tektagon_DisableTimer(type);
	#if SMBUS_MAILBOX_SUPPORT
	SetPlatformState(type == BMC_EVENT ? BMC_FLASH_AUTH : PCH_FLASH_AUTH);
	#endif
}

/**
 * Function to Handle Verify Initialize
 * Perform any data initialization here.
 * Initialize required engines needed to
 * verify the Manifest and Active Region.
 * @param type
 * @param data
 */
void handleVerifyInitState(int type, void *data)
{

}

/**
    Function to perform image verification

    @Param int          Image type from AO
    @Param void*        Active object Data from the Framework
    @Param void*        Event Data from the Framework

    @retval int         Returns the verification status
						1 - Verification success
						0 - Verification failed
 **/
int handleImageVerification(void *AoData, void *EventContext)
{
	AO_DATA *ActiveObjectData = (AO_DATA *) AoData;
	EVENT_CONTEXT *EventData = (EVENT_CONTEXT *) EventContext;

	if (ActiveObjectData->ProcessNewCommand == 1) {
		int status = Success;
		int imageType;
		if (ActiveObjectData->PreviousState == Initial && EventData->operation == VERIFY_ACTIVE) {
			printk("Power Reset to BMCBootHold for Verify\n");
			BMCBootHold();
		 	PCHBootHold();
		}
		status = authentication_image(AoData, EventContext);
		imageType = ActiveObjectData->type;

		if (status == Success) {
			if (EventData->operation == VERIFY_ACTIVE) {
				ActiveObjectData->ActiveImageStatus = Success;
				debug_log_create_entry(DEBUG_LOG_SEVERITY_INFO, DEBUG_LOG_COMPONENT_VERIFY, VERIFY_LOG_COMPONENT_RUN_AUTHEN_ACTIVE_SUCCESS, 0, 0);
				debug_log_flush();// State Machine log saving to SPI
			} else {
				ActiveObjectData->RecoveryImageStatus = Success;
				debug_log_create_entry(DEBUG_LOG_SEVERITY_INFO, DEBUG_LOG_COMPONENT_VERIFY, VERIFY_LOG_COMPONENT_RUN_AUTHEN_RECOVERY_SUCCESS, 0, 0);
				debug_log_flush();// State Machine log saving to SPI
			}
			handlePostVerifySuccess(ActiveObjectData, EventContext);
		} else if (status == Failure) {
			if (EventData->operation == VERIFY_ACTIVE) {
				ActiveObjectData->ActiveImageStatus = Failure;
				debug_log_create_entry(DEBUG_LOG_SEVERITY_ERROR, DEBUG_LOG_COMPONENT_VERIFY, VERIFY_LOG_COMPONENT_RUN_AUTHEN_ACTIVE_FAIL, 0, 0);
				debug_log_flush();// State Machine log saving to SPI
			} else {
				ActiveObjectData->RecoveryImageStatus = Failure;
				debug_log_create_entry(DEBUG_LOG_SEVERITY_ERROR, DEBUG_LOG_COMPONENT_VERIFY, VERIFY_LOG_COMPONENT_RUN_AUTHEN_RECOVERY_FAIL, 0, 0);
				debug_log_flush();// State Machine log saving to SPI
			}
			SetMajorErrorCode(imageType == BMC_EVENT ? BMC_AUTH_FAIL : PCH_AUTH_FAIL);
			SetMinorErrorCode(ACTIVE_AUTH_FAIL);
			handlePostVerifyFailure(ActiveObjectData, EventContext);
		}


	}
	return 0;
	// return processPfmFlashManifest();
}

/**
 * Function to handle Verify Exit
 * Unload libraries or free any memory used
 *
 * @param type
 * @param data
 */
void handleVerifyExitState(int type, void *data)
{
}

/**
 * Function to handle Recover Entry
 * Perform any recover library initialization if requred here
 */

int lastRecoveryReason(int ImageType, void *AoData)
{
	if (ImageType == BMC_EVENT) {
		if (((AO_DATA *)AoData)->ActiveImageStatus == Failure) {
			return BMC_ACTIVE_FAIL;
		} else if (((AO_DATA *)AoData)->RecoveryImageStatus == Failure) {
			return BMC_RECOVERY_FAIL;
		}

	} else if (ImageType == PCH_EVENT) {
		if (((AO_DATA *)AoData)->ActiveImageStatus == Failure) {
			return PCH_ACTIVE_FAIL;
		} else if (((AO_DATA *)AoData)->RecoveryImageStatus == Failure) {
			return PCH_RECOVERY_FAIL;
		}
	}

	return 0;
}
void handleRecoveryEntryState(void *data)
{
	// Stops WDT Timer;
	AO_DATA *ao_data = (AO_DATA *)data;
	int type = ao_data->type;

	// Tektagon_DisableTimer(type);
	#if SMBUS_MAILBOX_SUPPORT
	SetPlatformState(T_MINUS_1_FW_RECOVERY);
	SetLastRecoveryReason(lastRecoveryReason(type, data));
	IncRecoveryCount();
	#endif
}

/**
 * Function to Handle Recover Initialize
 * Perform any data initialization here
 */
void handleRecoveryInitState(int type, void *data)
{

}

/**
 * Function to handle Recovery success
 *
 * @param ImageType
 * @param AoData
 */
void handlePostRecoverySuccess(void *AoData, void *EventContext)
{
	// Perform Post Recovery Success
	AO_DATA *ActiveObjectData = (AO_DATA *) AoData;
	EVENT_CONTEXT *EventData = (EVENT_CONTEXT *) EventContext;

	ActiveObjectData->PreviousState = Recovery;
	if (EventData->flash == SECONDARY_FLASH_REGION) {
		EventData->operation = VERIFY_BACKUP;
	} else {
		EventData->operation = VERIFY_ACTIVE;
	}

	post_smc_action(VERIFY, ActiveObjectData, EventData);

}

/**
 * Function to handle verify failure
 *
 * @param ImageType
 * @param AoData
 */
void handlePostRecoveryFailure(void *AoData, void *EventContext)
{
	// Perform Post Recovery Failure
	AO_DATA *ActiveObjectData = (AO_DATA *) AoData;
	EVENT_CONTEXT *EventData = (EVENT_CONTEXT *) EventContext;

	if (ActiveObjectData->ActiveImageStatus != Success &&
	    ActiveObjectData->RecoveryImageStatus != Success) {
		ActiveObjectData->InLockdown = 1;
		LockDownPlatform(AoData);
	}

	if (ActiveObjectData->ActiveImageStatus != Success) {
		if (ActiveObjectData->RecoveryImageStatus == Success) {
			ActiveObjectData->InLockdown = 1;
			LockDownPlatform(AoData);
		}
	}

	if (ActiveObjectData->ActiveImageStatus == Success) {
		if (ActiveObjectData->RecoveryImageStatus != Success) {
			CheckAndReleasePlatform(AoData, EventData);
		}
	}

}

/**
    Function to perform image recovery verify

    @Param int          Image type from AO
    @Param void*        Active object Data from the Framework
    @Param void*        Event Data from the Framework

    @retval int         Returns the recovery verification status
						0 - Recovery image Verification success
						1 - Recovery image Verification failed
 **/
int handleRecoveryAction(void *AoData, void *EventContext)
{
	int status = 0;
	AO_DATA *ActiveObjectData = (AO_DATA *) AoData;
	EVENT_CONTEXT *EventData = (EVENT_CONTEXT *) EventContext;

	if (ActiveObjectData->ProcessNewCommand == 1) {
		if (ActiveObjectData->PreviousState == Initial && EventData->operation == RECOVER_ACTIVE) {
			printk("PowerOn Timeout to BMCBootHold for Recovery\n");
			BMCBootHold();
		 	PCHBootHold();
		}
		status = recover_image(AoData, EventContext);

		if (status == VerifyActive) {
			((AO_DATA *)AoData)->PreviousState = Recovery;
			// Posting the verify signal
			EventData->flash = PRIMARY_FLASH_REGION;
			EventData->operation = VERIFY_ACTIVE;
			post_smc_action(VERIFY, ActiveObjectData, EventData);

			return Success;
		} else if (status == VerifyRecovery) {

			((AO_DATA *)AoData)->PreviousState = Recovery;
			// Posting the verify signal
			EventData->flash = SECONDARY_FLASH_REGION;
			EventData->operation = VERIFY_BACKUP;
			post_smc_action(VERIFY, ActiveObjectData, EventData);

			return Success;
		}

	}

	return status;
}

/**
 * Function to handle Recover Exit
 * Unload libraries or free any memory used
 */
void handleRecoveryExitState(int type, void *data)
{

}
int lastPanicReason(int ImageType)
{
	if (ImageType == BMC_EVENT) {
		return BMC_UPDATE_INTENT;
	} else if (ImageType == PCH_EVENT) {
		return PCH_UPDATE_INTENT;
	}
	return 0;
}
/**
 * Function to Handle Update Initialize
 * Perform any data initialization here
 */
void handleUpdateEntryState(void *data)
{
	// Stops WDT Timer;
	AO_DATA *ao_data = (AO_DATA *)data;
	int type = ao_data->type;

	// Tektagon_DisableTimer(type);
	#if SMBUS_MAILBOX_SUPPORT
	SetPlatformState(type == BMC_EVENT ? BMC_FW_UPDATE : (PCH_EVENT ? PCH_FW_UPDATE : CPLD_FW_UPDATE));
	if (type != CPLD_FW_UPDATE) {
		SetLastPanicReason(lastPanicReason(type));
		IncPanicEventCount();
	}
	#endif
}

/**
 * Function to Handle Update Initialize
 * Perform any data initialization here
 */
void handleUpdateInitState(int type, void *data)
{
}

/**
 * Function to handle Update Exit
 * Unload libraries or free any memory used
 */
void handleUpdateExitState(int type, void *data)
{
}

/**
    Function to perform image Update

    @Param int          Image type from AO
    @Param void*        Active object Data from the Framework
    @Param void*        Event Data from the Framework


    @retval int         Returns the Update status
						1 - Update success
						0 - update failed
 **/

int handleUpdateImageAction(void *AoData, void *EventContext)
{
	uint8_t status = 0;
	uint32_t image_type = 0;
	AO_DATA *ActiveObjectData = (AO_DATA *) AoData;
	EVENT_CONTEXT *EventData = (EVENT_CONTEXT *) EventContext;

	if (ActiveObjectData->ProcessNewCommand == 1) {
		if (EventData->image == BMC_EVENT) {
			image_type = BMC_TYPE;
		} else if (EventData->image == PCH_EVENT) {
			image_type = PCH_TYPE;
		} else {
			image_type = HROT_TYPE;
		}

		status = handle_update_image_action(image_type, EventContext);

		ActiveObjectData->ProcessNewCommand = 0;
	}
	return status;
}
/**
    Function that indicates current I2C command status

    @Param int*

    @retval NULL
 **/
void handlePostUpdateSuccess(void *AoData)
{
	// if (IsBmcUpdateIntentCpldActive() || IsPchUpdateIntentCpldActive())
	//              pfr_cpld_update_reboot();
	int status = 0;
	AO_DATA *ActiveObjectData = (AO_DATA *) AoData;

	if (ActiveObjectData->type == BMC_EVENT || ActiveObjectData->type == PCH_EVENT) {
		PublishInitialEvents();
	}

	if (ActiveObjectData->type == HROT_TYPE) {
		pfr_cpld_update_reboot();
	}

}

void handlePostUpdateFailure(void *AoData)
{
	PublishInitialEvents();
}

// detect bmc reset
static struct gpio_callback bmc_rstind_cb_data;

void platform_monitor_remove(void)
{
	struct gpio_dt_spec bmc_rstind =
		GPIO_DT_SPEC_GET_BY_IDX(DT_INST(0, demo_gpio_basic_api), bmc_rst_ind_in_gpios, 0);
	gpio_pin_interrupt_configure_dt(&bmc_rstind, GPIO_INT_DISABLE);
	gpio_remove_callback(bmc_rstind.port, &bmc_rstind_cb_data);
	printk("platform_monitor_remove \n");
}


void bmc_rstind_handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	uint8_t gpio_pin = 31 - __builtin_clz(pins);
	int ret = gpio_pin_get(dev, gpio_pin);
	printk("[BMC->PFR] RSTIND[%s %d] = %d \n", dev->name, gpio_pin, ret);

	platform_monitor_remove();
	SetLastPanicReason(BMC_RESET_DETECT);
	SetPlatformState(T_MINUS_1_FW_RECOVERY);
	BmcActiveObjectData.ActiveImageStatus = Failure;
	BmcActiveObjectData.PreviousState = Initial;
	BmcActiveObjectData.ProcessNewCommand = 1;
	BmcActiveObjectData.type = BMC_EVENT;
	BmcData[1].operation = VERIFY_ACTIVE;
	BmcData[1].image = BMC_EVENT;	
	post_smc_action(VERIFY, &BmcActiveObjectData, &BmcData[1]);

}

/* Monitor BMC Reset Status */
void platform_monitor_init(void)
{
	int ret;
	struct gpio_dt_spec bmc_rstind =
		GPIO_DT_SPEC_GET_BY_IDX(DT_INST(0, demo_gpio_basic_api), bmc_rst_ind_in_gpios, 0);
	ret = gpio_pin_configure_dt(&bmc_rstind, GPIO_INPUT);
	printk("BMC: gpio_pin_configure_dt[%s %d] = %d \n", bmc_rstind.port->name, bmc_rstind.pin, ret);
	ret = gpio_pin_interrupt_configure_dt(&bmc_rstind, GPIO_INT_EDGE_FALLING);
	printk("BMC: gpio_pin_interrupt_configure_dt = %d \n", ret);
	gpio_init_callback(&bmc_rstind_cb_data, bmc_rstind_handler, BIT(bmc_rstind.pin));
	ret = gpio_add_callback(bmc_rstind.port, &bmc_rstind_cb_data);
	printk("BMC: gpio_add_callback = %d \n", ret);

}

// DOTO
// Enable SPI FIltering when UFM PROVISIONED
void T0Transition(int releaseBmc, int releasePCH)
{
	int provision_status;

	SetPlatformState(ENTER_T0);
	
	provision_status = get_provision_status();
	if (provision_status == UFM_PROVISIONED) {
		platform_monitor_init();
		// enable spi filtering
		if (releaseBmc) {
			init_SPI_RW_region(0);
			Tektagon_EnableTimer(BMC_EVENT);
		}
		if (releasePCH) {
			init_SPI_RW_region(1);
			Tektagon_EnableTimer(PCH_EVENT);
		}
	}
	if (releaseBmc) {
		BMCBootRelease();
	}

	if (releasePCH) {
		PCHBootRelease();
	}
}

/**
 * Function to LockDown the Platform
 *
 * @param ImageType
 */
void LockDownPlatform(void *AoData)
{
	AO_DATA *ao_data = (AO_DATA *)AoData;
	ao_data->ProcessNewCommand = 0;
	int image_type = ao_data->type;
	if (image_type == BMC_EVENT) {
		DEBUG_PRINTF("---------------------------------------\r\n");
		DEBUG_PRINTF("     BMC authentication failed\r\n");
		DEBUG_PRINTF("---------------------------------------\r\n");
		execute_next_smc_action(LOCKDOWN, AoData, NULL);
	} else if (image_type == PCH_EVENT) {
		DEBUG_PRINTF("---------------------------------------\r\n");
		DEBUG_PRINTF("     PCH authentication failed\r\n");
		DEBUG_PRINTF("---------------------------------------\r\n");
		DEBUG_PRINTF("ao_data->InLockdown: %d\r\n", ao_data->InLockdown);
		if (ao_data->InLockdown == 1) {
			uint8_t ReleasePch = 0, ReleaseBmc = RELEASE_PLATFORM;

			// BMC authentication success
			T0Transition(ReleaseBmc, ReleasePch);
		}
	}
}

int process_i2c_command(void *static_data, void *event_context)
{
	AO_DATA *I2CActiveObjectData = (AO_DATA *) static_data;
	EVENT_CONTEXT *I2CData = (EVENT_CONTEXT *) event_context;
	if (I2CActiveObjectData->ProcessNewCommand) {
		// printk("I2CData->i2c_data[0]: %x, I2CData->i2c_data[1]: %x\n", I2CData->i2c_data[0], I2CData->i2c_data[1]);
		PchBmcCommands(I2CData->i2c_data, 0);
		I2CActiveObjectData->ProcessNewCommand = 0;
	}
	return 0;
}
