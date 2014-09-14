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

#include <stdio.h>		/* sscanf */
#include <string.h>		/* memset, memcmp, memcpy */
#include <stdlib.h>		/* malloc, strtol */
#include <ctype.h>		/* isdigit */
#include <assert.h>		/* assert */
#include "mqtt_trigger.h"
#include "os_port.h"

#ifdef _MSC_VER
#pragma warning (disable:4996) // This function may be unsafe.
#endif



//--------------------------------------------
mqtt_trigger_t *mqtt_trigger_add_new(mqtt_trigger_t **list, const char *name, size_t next_id)
{
	mqtt_trigger_t *item;

	assert(list != NULL);
	assert(name != NULL);
	assert(next_id != 0);

	item = (mqtt_trigger_t *)malloc(sizeof(mqtt_trigger_t));
	memset(item, 0, sizeof(mqtt_trigger_t));
	item->name_len = (uint16_t)strlen(name);
	item->name = (uint8_t *)malloc(item->name_len);
	memcpy(item->name, name, item->name_len);
	item->next_id = next_id;
	list_add((list_t **)list, (list_t *)item);
	return item;
}

//--------------------------------------------
void mqtt_trigger_remove_all(mqtt_trigger_t **list)
{
	mqtt_trigger_t *next;
	mqtt_trigger_t *item;

	assert(list != NULL);

	item = *list;
	while (item != NULL)
	{
		next = mqtt_trigger_next(item);
		free(item);
		item = next;
	}
	*list = NULL;
}

