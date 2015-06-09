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

#include "rules.h"
#include "os_port.h"
#include "msg_trigger_rules.h"
#include "thread_state.h"


//--------------------------------------------
static rules_function_t *rules_functions = NULL;
static rules_node_t *rules_nodes = NULL;
static volatile thread_state_t thread_state;


//--------------------------------------------
static void msg_trigger_rules_handle(msg_trigger_rules_t *ms)
{
	node_execute(&rules_functions, &rules_nodes, ms->next_id);
}


//--------------------------------------------
//** main thread

//--------------------------------------------
static void thread_run(void *param)
{
	for (;;)
	{
		msg_trigger_rules_t *msg;

		if ((msg = msg_trigger_rules_get_first()) != NULL)
		{
			msg_trigger_rules_handle(msg);
			msg_trigger_rules_remove(msg);
		}
		if (thread_state == THREAD_STAYING)
			break;
		sleep(10);
	}

	rules_function_remove_all(&rules_functions);
	rules_node_remove_all(&rules_nodes);
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
void thread_rules_start(void)
{
	pthread_t thread;
	void *param = NULL;

	rules_functions_init(&rules_functions);

	thread_state = THREAD_RUNNING;
	thread_begin(thread_launcher, param, &thread);
}

//--------------------------------------------
void thread_rules_stop(void)
{
	if (thread_state == THREAD_RUNNING)
		thread_state = THREAD_STAYING;
	while (thread_state != THREAD_STOPPED)
		sleep(10);
}

//--------------------------------------------
void thread_rules_add_node(uint16_t id, rf_type_t type, void *param)
{
	rules_node_add(&rules_nodes, id, type, param);
}

//--------------------------------------------
void thread_rules_remove_all(void)
{
	rules_node_remove_all(&rules_nodes);
}
