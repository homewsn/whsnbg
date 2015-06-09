/*
* Copyright (c) 2013-2015 Vladimir Alemasov
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

#include "cron_trigger.h"
#include "os_port.h"
#include "msg_trigger_rules.h"
#include "thread_state.h"

//--------------------------------------------
static cron_trigger_t *cron_triggers = NULL;
static volatile thread_state_t thread_state;


//--------------------------------------------
static unsigned int sleep_500ms(void)
{
	size_t cnt;

	for (cnt = 0; cnt < 5; cnt++)
	{
		sleep(100);
		if (thread_state == THREAD_STAYING)
			return 1;
	}
	return 0;
}


//--------------------------------------------
//** main thread

//--------------------------------------------
static void thread_run(void *param)
{
	cron_trigger_t *item;

	for (;;)
	{
		if (time(NULL) % 60 == 0)
		{
			item = cron_trigger_head(&cron_triggers);
			while (item != NULL)
			{
				if (flag_starting_jobs(&item->cl) == 1)
					msg_cron_rules_add_packet(item);
				item = cron_trigger_next(item);
			}
			if (sleep_500ms() == 1)
				break;
		}
		if (sleep_500ms() == 1)
			break;
	}

	cron_trigger_remove_all(&cron_triggers);
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
void thread_cron_start(void)
{
	pthread_t thread;
	void *param = NULL;

	thread_state = THREAD_RUNNING;
	thread_begin(thread_launcher, param, &thread);
}

//--------------------------------------------
void thread_cron_stop(void)
{
	if (thread_state == THREAD_RUNNING)
		thread_state = THREAD_STAYING;
	while (thread_state != THREAD_STOPPED)
		sleep(10);
}

//--------------------------------------------
void thread_cron_trigger_add(const char *cronstr, int next_id)
{
	cron_trigger_add_new(&cron_triggers, cronstr, next_id);
}

//--------------------------------------------
void thread_cron_remove_all(void)
{
	cron_trigger_remove_all(&cron_triggers);
}
