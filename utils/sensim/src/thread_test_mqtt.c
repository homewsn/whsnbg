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
#include <stdio.h>		/* printf */
#include "os_port.h"
#include "msg_tcp_mqtt.h"
#include "thread_state.h"
#include "tick_counter.h"
#include "mqtt.h"
#include "test_mqtt.h"
#include "thread_test_mqttsn.h"

#ifdef _MSC_VER
#pragma warning (disable:4996) // This function may be unsafe.
#endif

#ifndef NDPRINTF
#define dprintf(...) printf(__VA_ARGS__)
#else
#define dprintf(...)
#endif

//--------------------------------------------
static int cancel_flag = 0;
static int exitmsg_flag = 0;
static volatile thread_state_t thread_state;

//--------------------------------------------
static void thread_run(void *param)
{
	alarm_timer_t test_timer = { 0 };
	size_t cnt;
	msg_tcp_mqtt_t *ms;
	mqtt_fixed_header_t fixhdr;
	int test_failed = 0;
	test_t *test;

	for (cnt = 0; (cnt < test_mqtt_list_get_size()) && (!test_failed); cnt++)
	{
		test = test_mqtt_list_get_test(cnt);
		if (test->delay)
		{
			printf("MQTT: delay %d sec before sending the next command.\n\n", test->delay);
			tick_counter_set_mqtt_mode(1);
			while (!alarm_timer_check(&test_timer, test->delay))
			{
				if (thread_state == THREAD_STAYING)
				{
					goto test_stop;
				}
				if (cancel_flag)
				{
					goto test_cancel;
				}
				sched_yield();
			}
			tick_counter_set_mqtt_mode(0);
		}
		if (test->command)
		{
			test->command(test->to_send);
		}
		for (;;)
		{
			tick_counter_set_mqtt_mode(1);
			if (alarm_timer_check(&test_timer, test->timeout))
			{
				test_failed = test->noresponse();
				break;
			}
			if ((ms = msg_tcp_mqtt_get_first()) != NULL)
			{
				if (mqtt_fixed_header_decode(&fixhdr, (unsigned char *)ms->msg_buf, ms->msg_cnt) == 0)
				{
					test_failed = test->response(&fixhdr, test->expected);
				}
				else
				{
					printf("The received packet is corrupted.\n");
					test_failed = 1;
				}
				msg_tcp_mqtt_remove(ms);
				break;
			}
			if (test_failed)
			{
				break;
			}
			if (thread_state == THREAD_STAYING)
			{
				goto test_stop;
			}
			if (cancel_flag)
			{
				goto test_cancel;
			}
			sched_yield();
		}
		tick_counter_set_mqtt_mode(0);
	}

	printf("\n--------------------------------------------\n");
	if (test_failed)
	{
		thread_test_mqttsn_cancel();
		printf("MQTT tests are failed.\n");
	}
	else
	{
		printf("MQTT tests passed successfully.\n");
	}
	thread_test_mqttsn_exitmsg();
	printf("--------------------------------------------\n\n");

test_cancel:
	if (cancel_flag)
	{
		tick_counter_set_mqtt_mode(0);
		printf("MQTT tests are cancelled.\n");
	}
	if (exitmsg_flag)
	{
		printf("Press any key to exit...\n");
	}

	for (;;)
	{
		if (thread_state == THREAD_STAYING)
		{
			break;
		}
		sched_yield();
	}

test_stop:
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
void thread_test_mqtt_start(void)
{
	pthread_t thread;
	void *param = NULL;

	thread_state = THREAD_RUNNING;
	thread_begin(thread_launcher, param, &thread);
}

//--------------------------------------------
void thread_test_mqtt_stop(void)
{
	if (thread_state == THREAD_RUNNING)
		thread_state = THREAD_STAYING;
	while (thread_state != THREAD_STOPPED)
		sleep(10);
}

//--------------------------------------------
void thread_test_mqtt_cancel(void)
{
	cancel_flag = 1;
}

//--------------------------------------------
void thread_test_mqtt_exitmsg(void)
{
	exitmsg_flag = 1;
}
