/*
* Copyright (c) 2018 Vladimir Alemasov
* All rights reserved
*
* This program and the accompanying materials are distributed under 
* the terms of GNU General Public License version 2 
* as published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*/

#ifdef _MSC_VER
#include "stdint_msc.h"	/* uint8_t ... uint64_t */
#else
#include <stdint.h>		/* uint8_t ... uint64_t */
#endif

#include <assert.h>		/* assert */
#include "time.h"
#include "os_port.h"
#include "tick_counter.h"

#ifdef _MSC_VER
#pragma warning (disable:4996) // This function may be unsafe.
#endif

#ifndef NDPRINTF
#include <stdio.h>
#define dprintf(...) printf(__VA_ARGS__)
#else
#define dprintf(...)
#endif

//--------------------------------------------
static struct tick_counter
{
	pthread_mutex_t mutex;
	uint32_t counter;
	int mqtt_mode;
	int mqttsn_mode;
} tick_counter;

//--------------------------------------------
void tick_counter_init(void)
{
	pthread_mutex_init(&tick_counter.mutex, NULL);
	tick_counter.counter = 0;
	tick_counter.mqtt_mode = 0;
	tick_counter.mqttsn_mode = 0;
}

//--------------------------------------------
void tick_counter_increment(void)
{
	pthread_mutex_lock(&tick_counter.mutex);
	++tick_counter.counter;
	pthread_mutex_unlock(&tick_counter.mutex);
}

//--------------------------------------------
uint32_t tick_counter_get(void)
{
	uint32_t counter;
	pthread_mutex_lock(&tick_counter.mutex);
	counter = tick_counter.counter;
	pthread_mutex_unlock(&tick_counter.mutex);
	return counter;
}

//--------------------------------------------
void tick_counter_set_mqtt_mode(int mode)
{
	pthread_mutex_lock(&tick_counter.mutex);
	tick_counter.mqtt_mode = mode;
//	printf("mode=%d, tick_counter.mode=%d\n", mode, tick_counter.mode);
	pthread_mutex_unlock(&tick_counter.mutex);
}

//--------------------------------------------
void tick_counter_set_mqttsn_mode(int mode)
{
	pthread_mutex_lock(&tick_counter.mutex);
	tick_counter.mqttsn_mode = mode;
	//	printf("mode=%d, tick_counter.mode=%d\n", mode, tick_counter.mode);
	pthread_mutex_unlock(&tick_counter.mutex);
}

//--------------------------------------------
int tick_counter_get_mode(void)
{
	int mode;
	pthread_mutex_lock(&tick_counter.mutex);
	mode = tick_counter.mqtt_mode + tick_counter.mqttsn_mode;
	pthread_mutex_unlock(&tick_counter.mutex);
	return mode;
}

//--------------------------------------------
void tick_counter_destroy(void)
{
	pthread_mutex_destroy(&tick_counter.mutex);
}

//--------------------------------------------
uint32_t alarm_timer_check(alarm_timer_t *alarm_timer, uint32_t interval)
{
	uint32_t current_counter;

	current_counter = tick_counter_get();
	if (alarm_timer->running == 0)
	{
		alarm_timer->running = 1;
		alarm_timer->start_counter = current_counter;
	}
	else
	{
		if ((current_counter - alarm_timer->start_counter) >= (interval * TICKS_IN_SECOND))
		{
			alarm_timer->running = 0;
			return 1;
		}
	}
	return 0;
}
