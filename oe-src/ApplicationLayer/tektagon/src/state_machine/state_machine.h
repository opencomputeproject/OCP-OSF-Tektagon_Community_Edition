//***********************************************************************//
//*                                                                     *//
//*                      Copyright © 2022 AMI                           *//
//*                                                                     *//
//*        All rights reserved. Subject to AMI licensing agreement.     *//
//*                                                                     *//
//***********************************************************************//

#ifndef ZEPHYR_STATE_MACHINE_H
#define ZEPHYR_STATE_MACHINE_H

#include <../../HardwareAbstraction/smf/smf.h>
#include "common_smc.h"

struct _smc_fifo_event {
	void *fifo_reserved;
	int new_event_state;
	void *new_sm_static_data;
	void *new_event_ctx;
};

struct hrot_smc_context {
	struct smf_ctx ctx;
	/* All User Defined Data Follows */
	void *sm_static_data;
	void *event_ctx;
};

int StartHrotStateMachine(void);
int post_smc_action(int new_state, void *static_data, void *event);
int execute_next_smc_action(int new_state, void *static_data, void *event_ctx);

void PublishInitialEvents(void);

#endif // #ifndef ZEPHYR_STATE_MACHINE_H
