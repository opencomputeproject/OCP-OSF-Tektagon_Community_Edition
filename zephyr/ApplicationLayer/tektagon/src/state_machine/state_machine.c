//***********************************************************************//
//*                                                                     *//
//*                      Copyright © 2022 AMI                           *//
//*                                                                     *//
//*        All rights reserved. Subject to AMI licensing agreement.     *//
//*                                                                     *//
//***********************************************************************//

#include <zephyr.h>
#include <smf.h>

#include "state_machine.h"
#include "StateMachineAction/StateMachineActions.h"
#include "include/SmbusMailBoxCom.h"
#include "Smbus_mailbox/Smbus_mailbox.h"
#include "logging/debug_log.h"// State Machine log saving

K_FIFO_DEFINE(evt_q);

/* Forward declaration of HRoT state table */
static const struct smf_state hrot_states[];
static struct hrot_smc_context context = { 0 };

int StartHrotStateMachine(void)
{
	int32_t ret = 0;

	smf_set_initial(SMF_CTX(&context), &hrot_states[IDLE]);
	struct _smc_fifo_event *initial_event = (struct _smc_fifo_event *)k_malloc(sizeof(struct _smc_fifo_event));
	if (initial_event == NULL) {
		return ENOMEM;
	}

	initial_event->new_event_state = INITIALIZE;
	initial_event->new_event_ctx = NULL;
	initial_event->new_sm_static_data = NULL;
	k_fifo_put(&evt_q, initial_event);

	/* Run the state machine */
	do {

		/* State machine terminates if a non-zero value is returned */
		ret = smf_run_state(SMF_CTX(&context));
		if (ret) {
			/* handle return code and terminate state machine */
			break;
		}
		k_msleep(10);
	} while (true);

	if (initial_event) {
		k_free(initial_event);
	}

	return ret;
}

static void run_idle(void *context)
{
	// printk("Executing run_idle\r\n");

	if (!k_fifo_is_empty(&evt_q)) {
		int new_state = 0;
		struct _smc_fifo_event *hrot_event = k_fifo_get(&evt_q, K_NO_WAIT);
		struct hrot_smc_context *sm_context = (struct hrot_smc_context *)context;

		sm_context->sm_static_data = hrot_event->new_sm_static_data;
		sm_context->event_ctx = hrot_event->new_event_ctx;

		switch (hrot_event->new_event_state) {
		case INITIALIZE:
			PublishInitialEvents();
			break;
		case VERIFY:
			smf_set_state(SMF_CTX(sm_context), &hrot_states[VERIFY]);
			break;
		case RECOVERY:
			smf_set_state(SMF_CTX(sm_context), &hrot_states[RECOVERY]);
			break;
		case UPDATE:
			smf_set_state(SMF_CTX(sm_context), &hrot_states[UPDATE]);
			break;
		case I2C:
			smf_set_state(SMF_CTX(sm_context), &hrot_states[I2C]);
			break;
		case LOCKDOWN:
			smf_set_state(SMF_CTX(sm_context), &hrot_states[LOCKDOWN]);
			break;
		default:
			break;
		}

		k_free(hrot_event);
		
	}
}

/* I2C State Handlers */
static void i2c_entry(void *context)
{
	// printk("Executing i2c_entry\r\n");
	// printk("Leaving i2c_entry\r\n");
}

static void run_i2c(void *context)
{
	// printk("Executing run_i2c\r\n");
	struct hrot_smc_context *sm_context = (struct hrot_smc_context *)context;
	void *sm_static_data = sm_context->sm_static_data;
	void *event_ctx = sm_context->event_ctx;

	// Data
	// Event
	process_i2c_command(sm_static_data, event_ctx);
	// PchBmcCommands(event_ctx, 0);


	// printk("Leaving run_i2c\r\n");
}

static void i2c_exit(void *context)
{
	// printk("Executing i2c_exit\r\n");
	// printk("Leaving i2c_exit\r\n");
}

/* verify State Handlers */
static void verify_entry(void *context)
{
	debug_log_create_entry(DEBUG_LOG_SEVERITY_INFO, DEBUG_LOG_COMPONENT_VERIFY, VERIFY_LOG_COMPONENT_ENTRY_START, 0, 0);
	debug_log_flush();// State Machine log saving to SPI

	struct hrot_smc_context *sm_context = (struct hrot_smc_context *)context;
	handleVerifyEntryState(sm_context->sm_static_data, sm_context->event_ctx);
	handleVerifyInitState(sm_context->sm_static_data);
}

static void run_verify(void *context)
{
	int status = 0;

	struct hrot_smc_context *sm_context = (struct hrot_smc_context *)context;
	void *sm_static_data = sm_context->sm_static_data;
	void *event_ctx = sm_context->event_ctx;

	debug_log_create_entry(DEBUG_LOG_SEVERITY_INFO, DEBUG_LOG_COMPONENT_VERIFY, VERIFY_LOG_COMPONENT_RUN_START, 0, 0);
	debug_log_flush();// State Machine log saving to SPI

	//printk("Executing run_verify\r\n");
	status = handleImageVerification(sm_static_data, event_ctx);

	//printk("Leaving run_verify\r\n");
}

static void verify_exit(void *context)
{
	struct hrot_smc_context *sm_context = (struct hrot_smc_context *)context;
	void *sm_static_data = sm_context->sm_static_data;
	handleVerifyExitState(sm_static_data);
}

/* recovery State Handlers */
static void recovery_entry(void *context)
{
	struct hrot_smc_context *sm_context = (struct hrot_smc_context *)context;
	void *sm_static_data = sm_context->sm_static_data;

	debug_log_create_entry(DEBUG_LOG_SEVERITY_INFO, DEBUG_LOG_COMPONENT_RECOVERY, RECOVERY_LOG_COMPONENT_ENTRY_START, 0, 0);
	debug_log_flush();// State Machine log saving to SPI

	printk("Executing recovery_entry\r\n");
	handleRecoveryEntryState(sm_static_data);
	handleRecoveryInitState(sm_static_data);
	printk("Leaving recovery_entry\r\n");
}

static void run_recovery(void *context)
{
	int status = 0;
	int imageType;
	struct hrot_smc_context *sm_context = (struct hrot_smc_context *)context;
	void *sm_static_data = sm_context->sm_static_data;
	void *event_ctx = sm_context->event_ctx;

	debug_log_create_entry(DEBUG_LOG_SEVERITY_INFO, DEBUG_LOG_COMPONENT_RECOVERY, RECOVERY_LOG_COMPONENT_RUN_START, 0, 0);
	debug_log_flush();// State Machine log saving to SPI

	printk("Executing run_recovery\r\n");
	status = handleRecoveryAction(sm_static_data, event_ctx);
	AO_DATA *ActiveObjectData = (AO_DATA *)sm_static_data;
	imageType = ActiveObjectData->type;
	if (status == Success) {
		debug_log_create_entry(DEBUG_LOG_SEVERITY_INFO, DEBUG_LOG_COMPONENT_RECOVERY, RECOVERY_LOG_COMPONENT_VERIFY_RECOVERY_SUCCESS, 0, 0);
		debug_log_flush();// State Machine log saving to SPI

		handlePostRecoverySuccess(sm_static_data, event_ctx);
	} else if (status == Failure) {
		debug_log_create_entry(DEBUG_LOG_SEVERITY_ERROR, DEBUG_LOG_COMPONENT_RECOVERY, RECOVERY_LOG_COMPONENT_VERIFY_FAIL, 0, 0);
		debug_log_flush();// State Machine log saving to SPI

		SetMajorErrorCode(imageType == BMC_EVENT ? BMC_AUTH_FAIL : PCH_AUTH_FAIL);
		SetMinorErrorCode(ACTIVE_RECOVERY_AUTH_FAIL);
		handlePostRecoveryFailure(sm_static_data, event_ctx);
	}

	printk("Leaving run_recovery\r\n");
}

static void recovery_exit(void *context)
{
	struct hrot_smc_context *sm_context = (struct hrot_smc_context *)context;
	void *sm_static_data = sm_context->sm_static_data;

	// printk("Executing recovery_exit\r\n");
	handleRecoveryExitState(sm_static_data);
	// printk("Leaving recovery_exit\r\n");
}

/* update State Handlers */
static void update_entry(void *context)
{
	struct hrot_smc_context *sm_context = (struct hrot_smc_context *)context;
	void *sm_static_data = sm_context->sm_static_data;
	void *event_ctx = sm_context->event_ctx;
	unsigned int type = ((EVENT_CONTEXT *)event_ctx)->image;

	debug_log_create_entry(DEBUG_LOG_SEVERITY_INFO, DEBUG_LOG_COMPONENT_UPDATE, UPDATE_LOG_COMPONENT_ENTRY_START, 0, 0);
	debug_log_flush();// State Machine log saving to SPI

	// printk("Executing update_entry\r\n");
	handleUpdateEntryState((int)type, sm_static_data);
	handleUpdateInitState((int)type, sm_static_data);
	// printk("Leaving update_entry\r\n");
}

static void run_update(void *context)
{
	int status = 0;
	struct hrot_smc_context *sm_context = (struct hrot_smc_context *)context;
	void *sm_static_data = sm_context->sm_static_data;
	void *event_ctx = sm_context->event_ctx;
	unsigned int type = ((EVENT_CONTEXT *)event_ctx)->image;

	debug_log_create_entry(DEBUG_LOG_SEVERITY_INFO, DEBUG_LOG_COMPONENT_UPDATE, UPDATE_LOG_COMPONENT_RUN_START, 0, 0);
	debug_log_flush();// State Machine log saving to SPI

	// printk("Executing run_update\r\n");
	status = handleUpdateImageAction(sm_static_data, event_ctx);
	if (status == Success) {
		debug_log_create_entry(DEBUG_LOG_SEVERITY_INFO, DEBUG_LOG_COMPONENT_UPDATE, UPDATE_LOG_COMPONENT_UPDATE_SUCCESS, 0, 0);
		debug_log_flush();// State Machine log saving to SPI

		handlePostUpdateSuccess(sm_static_data);
	} else if (status ==  Failure) {
		debug_log_create_entry(DEBUG_LOG_SEVERITY_ERROR, DEBUG_LOG_COMPONENT_UPDATE, UPDATE_LOG_COMPONENT_UPDATE_FAIL, 0, 0);
		debug_log_flush();// State Machine log saving to SPI

		handlePostUpdateFailure(sm_static_data);
	}
	// printk("Leaving run_update\r\n");
}

static void update_exit(void *context)
{
	struct hrot_smc_context *sm_context = (struct hrot_smc_context *)context;
	void *sm_static_data = sm_context->sm_static_data;
	void *event_ctx = sm_context->event_ctx;
	unsigned int type = ((EVENT_CONTEXT *)event_ctx)->image;

	// printk("Executing update_exit\r\n");
	handleUpdateExitState((int)type, sm_static_data);
	// printk("Leaving update_exit\r\n");
}

/* Lockdown State Handlers */
static void run_lockdown(void *context)
{
	struct hrot_smc_context *sm_context = (struct hrot_smc_context *)context;
	void *sm_static_data = sm_context->sm_static_data;

	// printk("Executing run_lockdown\r\n");
	handleLockDownState(sm_static_data);
	// printk("Leaving run_lockdown\r\n");
}

int post_smc_action(int new_state, void *static_data, void *event)
{
	struct _smc_fifo_event *smc_fifo_entry = (struct _smc_fifo_event *)k_malloc(sizeof(struct _smc_fifo_event));

	if (smc_fifo_entry == NULL) {
		return ENOMEM;
	}

	smc_fifo_entry->new_event_state = new_state;
	smc_fifo_entry->new_event_ctx = event;
	smc_fifo_entry->new_sm_static_data = static_data;
	k_fifo_put(&evt_q, smc_fifo_entry);

	return 0;
}

int execute_next_smc_action(int new_state, void *static_data, void *event_ctx)
{
	context.event_ctx = event_ctx;
	context.sm_static_data = static_data;

	smf_set_state(SMF_CTX(&context), &hrot_states[new_state]);
	return 0;
}

static const struct smf_state hrot_states[] = {
	/* Idle State: Root state with no Parent and has only RUN fn*/
	[IDLE] = SMF_CREATE_STATE(NULL, run_idle, NULL, NULL),
	/* State S2 does not have a Parent */
	[I2C] = SMF_CREATE_STATE(i2c_entry, run_i2c, i2c_exit, &hrot_states[IDLE]),
	/* Idle State: Root state with no Parent*/
	[VERIFY] = SMF_CREATE_STATE(verify_entry, run_verify, verify_exit, &hrot_states[IDLE]),
	/* State S1 does not have an entry action */
	[RECOVERY] = SMF_CREATE_STATE(recovery_entry, run_recovery, recovery_exit, &hrot_states[IDLE]),
	/* State S2 does not have an exit action */
	[UPDATE] = SMF_CREATE_STATE(update_entry, run_update, update_exit, &hrot_states[IDLE]),
	/* State S2 does not have an exit action */
	[LOCKDOWN] = SMF_CREATE_STATE(NULL, run_lockdown, NULL, NULL),
};
