/*
 * Copyright (c) 2021 ASPEED
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DRIVERS_TIMER_ASPEED_TIMER_H
#define ZEPHYR_INCLUDE_DRIVERS_TIMER_ASPEED_TIMER_H

#define ASPEED_TIMER_TYPE_ONE_SHOT	0	/* one-shot timer */
#define ASPEED_TIMER_TYPE_PERIODIC	1	/* periodic timer */

/**
 * @brief structure for timer configuration
 *
 * @param millisec millisecond to be waiting for
 * @param callback callback function when timer expired
 * @param user_data context of the callback function
 * @param timer_type ASPEED_TIMER_TYPE_ONE_SHOT or ASPEED_TIMER_TYPE_PERIODIC
 *
 * This structure shall be filled by the timer user before calling @ref timer_aspeed_start
 */
struct aspeed_timer_user_config {
	uint32_t millisec;
	void (*callback)(void *user_data);
	void *user_data;
	int timer_type;
};
/**
 * @brief start the timer
 *
 * @param dev specify the timer device
 * @param user_config pointer to the user config
 * @return int 0 if success
 */
int timer_aspeed_start(const struct device *dev, struct aspeed_timer_user_config *user_config);

/**
 * @brief stop the timer
 *
 * @param dev specify the timer device
 * @return int 0 if success
 */
int timer_aspeed_stop(const struct device *dev);

/**
 * @brief query the timer counter
 *
 * @param dev specify the timer device
 * @return int current timer counter, 0 indicaties the timer is expired
 */
int timer_aspeed_query(const struct device *dev);
#endif /* ZEPHYR_INCLUDE_DRIVERS_TIMER_ASPEED_TIMER_H */
