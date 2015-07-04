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

#ifndef MSG_TRIGGER_RULES_H_
#define MSG_TRIGGER_RULES_H_

#include "msgs.h"
#include "cron_trigger.h"
#include "mqtt_trigger.h"

#ifdef _MSC_VER
#include "stdint_msc.h"	/* uint8_t ... uint64_t */
#else
#include <stdint.h>		/* uint8_t ... uint64_t */
#include <stddef.h>		/* size_t */
#endif

//--------------------------------------------
typedef struct msg_trigger_rules
{
	msg_t msg;
	uint32_t next_id;
} msg_trigger_rules_t;

//--------------------------------------------
void msg_trigger_rules_init(void);
msg_trigger_rules_t *msg_trigger_rules_new(void);
void msg_trigger_rules_add(msg_trigger_rules_t *msg);
void msg_trigger_rules_remove(msg_trigger_rules_t *msg);
msg_trigger_rules_t* msg_trigger_rules_get_first(void);
#define msg_trigger_rules_close() msg_close(a)
void msg_trigger_rules_destroy(void);

void msg_cron_rules_add_packet(cron_trigger_t *cron_trigger);
void msg_mqtt_rules_add_packet(mqtt_trigger_t *mqtt_trigger);

#endif /* MSG_TRIGGER_RULES_H_ */
