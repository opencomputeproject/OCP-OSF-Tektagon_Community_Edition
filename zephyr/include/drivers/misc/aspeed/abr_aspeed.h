/*
 * Copyright (c) 2022 ASPEED Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DRIVERS_MISC_ABR_ASPEED_H_
#define ZEPHYR_INCLUDE_DRIVERS_MISC_ABR_ASPEED_H_

void disable_abr_wdt(void);
void clear_abr_event_count(void);
void clear_abr_indicator(void);

#endif
