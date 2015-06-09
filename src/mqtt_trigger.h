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

#ifndef MQTT_TRIGGER_H_
#define MQTT_TRIGGER_H_

#include "list.h"

#ifdef _MSC_VER
#include "stdint_msc.h"	/* uint8_t ... uint64_t */
#else
#include <stdint.h>		/* uint8_t ... uint64_t */
#include <stddef.h>		/* size_t */
#endif


//--------------------------------------------
typedef struct mqtt_trigger
{
	list_t next;
	uint8_t *name;
	uint16_t name_len;
	size_t next_id;
} mqtt_trigger_t;

mqtt_trigger_t *mqtt_trigger_add_new(mqtt_trigger_t **list, const char *name, size_t next_id);
#define mqtt_trigger_head(a) (mqtt_trigger_t *)list_head((list_t **)a)
#define mqtt_trigger_next(a) (mqtt_trigger_t *)list_next((list_t *)a)
void mqtt_trigger_remove_all(mqtt_trigger_t **list);

#endif /* MQTT_TRIGGER_H_ */
