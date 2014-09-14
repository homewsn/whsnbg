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

#ifndef __MSG_RULES_MQTT_H__
#define __MSG_RULES_MQTT_H__

#include "msgs.h"
#include "lists.h"

//--------------------------------------------
void msggap_rules_mqtt_init(void);
void msggap_rules_mqtt_request(list_data_t *list);
void msggap_rules_mqtt_clear_request(void);
list_data_t *msggap_rules_mqtt_get_request(void);
void msggap_rules_mqtt_reply_request(void);
void msggap_rules_mqtt_close(void);
void msggap_rules_mqtt_destroy(void);

//--------------------------------------------
typedef struct msg_rules_mqtt
{
	msg_t msg;
	list_data_t *list;
	uint8_t retain;
} msg_rules_mqtt_t;

//--------------------------------------------
void msg_rules_mqtt_init(void);
msg_rules_mqtt_t *msg_rules_mqtt_new(void);
void msg_rules_mqtt_add(msg_rules_mqtt_t *msg);
void msg_rules_mqtt_remove(msg_rules_mqtt_t *msg);
msg_rules_mqtt_t* msg_rules_mqtt_get_first(void);
#define msg_rules_mqtt_close() msg_close(a)
void msg_rules_mqtt_destroy(void);

void msg_rules_mqtt_add_packet(list_data_t *list, uint8_t retain);

#endif /* __MSG_RULES_MQTT_H__ */
