/*
* Copyright (c) 2013-2014 Vladimir Alemasov
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

#include "msg_trigger_rules.h"
#include <string.h>		/* memset */

static msgqueue_t queue;

//--------------------------------------------
void msg_trigger_rules_init(void)
{
	msg_init(&queue);
}

//--------------------------------------------
msg_trigger_rules_t *msg_trigger_rules_new(void)
{
	msg_trigger_rules_t *msg;

	msg = (msg_trigger_rules_t *)malloc(sizeof(msg_trigger_rules_t));
	memset(msg, 0, sizeof(msg_trigger_rules_t));
	return msg;
}

//--------------------------------------------
void msg_trigger_rules_add(msg_trigger_rules_t *msg)
{
	msg_add(&queue, (msg_t *)msg);
}

//--------------------------------------------
void msg_trigger_rules_remove(msg_trigger_rules_t *msg)
{
	msg_remove(&queue, (msg_t *)msg);
	free(msg);
}

//--------------------------------------------
msg_trigger_rules_t* msg_trigger_rules_get_first(void)
{
	return (msg_trigger_rules_t *)msg_get_first(&queue);
}

//--------------------------------------------
void msg_trigger_rules_destroy(void)
{
	msg_destroy(&queue);
}

//--------------------------------------------
void msg_cron_rules_add_packet(cron_trigger_t *cron_trigger)
{
	msg_trigger_rules_t *msg = msg_trigger_rules_new();
	msg->next_id = cron_trigger->next_id;
	msg_trigger_rules_add(msg);
}

//--------------------------------------------
void msg_mqtt_rules_add_packet(mqtt_trigger_t *mqtt_trigger)
{
	msg_trigger_rules_t *msg = msg_trigger_rules_new();
	msg->next_id = mqtt_trigger->next_id;
	msg_trigger_rules_add(msg);
}
