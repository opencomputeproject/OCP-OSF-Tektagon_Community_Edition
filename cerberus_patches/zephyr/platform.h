// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

#ifndef ZEPHYR_PLATFORM_H_
#define ZEPHYR_PLATFORM_H_

#include <stdint.h>
#include <crypto/rsa.h>
#include <posix/unistd.h>
#include <stdlib.h>
#include <sys_clock.h>
#include <kernel.h>
#include "zephyr.h"
//#include "semphr.h"

#define CERBERUS_PREVIOUS_VERSION 0x1

//typedef void* SemaphoreHandle_t;
typedef struct k_timer TimerHandle_t;
typedef k_ticks_t TickType_t;

#define pvPortMalloc 		malloc
#define vPortFree			free
#define portMAX_DELAY		( k_ticks_t ) 0xffffffffUL

/* FreeRTOS memory management. */
#define	platform_malloc		pvPortMalloc
#define	platform_free		vPortFree

void* platform_calloc (size_t nmemb, size_t size);
void* platform_realloc (void *ptr, size_t size);

/* FreeRTOS internet operations. */
uint32_t platform_htonl (uint32_t host_long);
uint16_t platform_htons (uint16_t host_short);

/* FreeRTOS sleep and system time. */
//#define	platform_msleep(x)	vTaskDelay (pdMS_TO_TICKS (x) + 1)
#define	platform_msleep(x)	sleep (x + 1)

typedef struct {
	TickType_t ticks;
	uint8_t wrap;
} platform_clock;

int platform_init_timeout (uint32_t msec, platform_clock *timeout);
int platform_increase_timeout (uint32_t msec, platform_clock *timeout);
int platform_init_current_tick (platform_clock *currtime);
int platform_has_timeout_expired (platform_clock *timeout);
uint64_t platform_get_time_since_boot (void);
uint32_t platform_get_duration (const platform_clock *start, const platform_clock *end);

/* Zephyr mutex. */
typedef struct k_sem platform_mutex;

int platform_mutex_init (platform_mutex *mutex);
int platform_mutex_free (platform_mutex *mutex);
int platform_mutex_lock (platform_mutex *mutex);
int platform_mutex_unlock (platform_mutex *mutex);

/* FreeRTOS recursive mutex */
int platform_recursive_mutex_init (platform_mutex *mutex);
int platform_recursive_mutex_free (platform_mutex *mutex);
int platform_recursive_mutex_lock (platform_mutex *mutex);
int platform_recursive_mutex_unlock (platform_mutex *mutex);

/* FreeRTOS timer. */
typedef void (*timer_callback) (void *context);
typedef struct {
	TimerHandle_t timer;
	struct k_sem disarm_lock;
	timer_callback callback;
	void *context;
	uint8_t disarm;
} platform_timer;

int platform_timer_create (platform_timer *timer, timer_callback callback, void *context);
int platform_timer_arm_one_shot (platform_timer *timer, uint32_t ms_timeout);
int platform_timer_disarm (platform_timer *timer);
void platform_timer_delete (platform_timer *timer);
#endif /* ZEPHYR_PLATFORM_H_ */
