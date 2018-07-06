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

#include "time.h"
#include <assert.h>		/* assert */
#include <stdio.h>		/* printf */
#include "os_port.h"
#include "thread_state.h"
#include "tick_counter.h"

#ifdef _MSC_VER
#pragma warning (disable:4996) // This function may be unsafe.
#endif

#ifndef NDPRINTF
#define dprintf(...) printf(__VA_ARGS__)
#else
#define dprintf(...)
#endif

//--------------------------------------------
static volatile thread_state_t thread_state;

//--------------------------------------------
static void thread_run(void *param)
{
	clock_t ticks_start;
	clock_t ticks_now;
	uint32_t counter;

	ticks_start = clock();
	for (;;)
	{
		if ((ticks_now = clock()) - ticks_start > TICK_TIME)
		{
			ticks_start = ticks_now;
			tick_counter_increment();
			if (tick_counter_get_mode())
			{
				counter = tick_counter_get();
				if (!(counter % TICKS_IN_SECOND))
				{
					printf("secs: %d\n", counter / TICKS_IN_SECOND);
				}
			}
		}
		if (thread_state == THREAD_STAYING)
		{
			break;
		}
		sched_yield();
	}

	thread_state = THREAD_STOPPED;
}

//--------------------------------------------
#ifdef WIN32
static unsigned int __stdcall thread_launcher(void *param)
{
	thread_run(param);
	return 0;
}
#else
static void *thread_launcher(void *param)
{
	thread_run(param);
	return NULL;
}
#endif

//--------------------------------------------
void thread_tick_start(void)
{
	pthread_t thread;
	void *param = NULL;

	thread_state = THREAD_RUNNING;
	thread_begin(thread_launcher, param, &thread);
}

//--------------------------------------------
void thread_tick_stop(void)
{
	if (thread_state == THREAD_RUNNING)
		thread_state = THREAD_STAYING;
	while (thread_state != THREAD_STOPPED)
		sleep(10);
}
