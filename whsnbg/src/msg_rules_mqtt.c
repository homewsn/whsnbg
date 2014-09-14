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

#include "msg_rules_mqtt.h"
#include <string.h>		/* memset */

static msggap_t gap;
static msgqueue_t queue;


//--------------------------------------------
void msggap_rules_mqtt_init(void)
{
	msggap_init(&gap);
}

//--------------------------------------------
void msggap_rules_mqtt_request(list_data_t *list)
{
	gap.msg = (void *)list;
	msggap_request(&gap);
}

//--------------------------------------------
void msggap_rules_mqtt_clear_request(void)
{
	gap.msg = NULL;
}

//--------------------------------------------
list_data_t *msggap_rules_mqtt_get_request(void)
{
	if (msggap_get_request(&gap) == 1)
		return (list_data_t *)gap.msg;
	else
		return NULL;
}

//--------------------------------------------
void msggap_rules_mqtt_reply_request(void)
{
	msggap_reply(&gap);
}

//--------------------------------------------
void msggap_rules_mqtt_close(void)
{
	msggap_close(&gap);
}

//--------------------------------------------
void msggap_rules_mqtt_destroy(void)
{
	msggap_destroy(&gap);
}




//--------------------------------------------
void msg_rules_mqtt_init(void)
{
	msg_init(&queue);
}

//--------------------------------------------
msg_rules_mqtt_t *msg_rules_mqtt_new(void)
{
	msg_rules_mqtt_t *msg;

	msg = (msg_rules_mqtt_t *)malloc(sizeof(msg_rules_mqtt_t));
	memset(msg, 0, sizeof(msg_rules_mqtt_t));
	return msg;
}

//--------------------------------------------
void msg_rules_mqtt_add(msg_rules_mqtt_t *msg)
{
	msg_add(&queue, (msg_t *)msg);
}

//--------------------------------------------
void msg_rules_mqtt_remove(msg_rules_mqtt_t *msg)
{
	msg_remove(&queue, (msg_t *)msg);
	list_data_remove_all(&msg->list);
	free(msg);
}

//--------------------------------------------
msg_rules_mqtt_t* msg_rules_mqtt_get_first(void)
{
	return (msg_rules_mqtt_t *)msg_get_first(&queue);
}

//--------------------------------------------
void msg_rules_mqtt_destroy(void)
{
	msg_destroy(&queue);
}

//--------------------------------------------
void msg_rules_mqtt_add_packet(list_data_t *list, uint8_t retain)
{
	msg_rules_mqtt_t *msg = msg_rules_mqtt_new();
	msg->list = list;
	msg->retain = retain;
	msg_rules_mqtt_add(msg);
}
