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

#include "list_mqtt_user.h"
#include <assert.h>		/* assert */
#include <stdlib.h>		/* malloc */
#include <string.h>		/* memcpy, memset, memcmp */

#ifdef _MSC_VER
#pragma warning (disable:4996) // This function may be unsafe.
#endif


//--------------------------------------------
list_mqtt_user_t *list_mqtt_user_add_new(list_mqtt_user_t **list, const char *user_name, const char *password, unsigned char publish_enable)
{
	list_mqtt_user_t *item;

	assert(list != NULL);
	assert(user_name != NULL);
	assert(password != NULL);

	item = (list_mqtt_user_t *)malloc(sizeof(list_mqtt_user_t));
	memset(item, 0, sizeof(list_mqtt_user_t));
	item->user_name_length = (uint16_t)strlen(user_name);
	item->user_name = (uint8_t *)malloc(item->user_name_length);
	memcpy(item->user_name, user_name, item->user_name_length);
	item->password_length = (uint16_t)strlen(password);
	item->password = (uint8_t *)malloc(item->password_length);
	memcpy(item->password, password, item->password_length);
	item->publish_enable = publish_enable;

	list_mqtt_user_add(list, item);
	return item;
}

//--------------------------------------------
void list_mqtt_user_remove_all(list_mqtt_user_t **list)
{
	list_mqtt_user_t *next;
	list_mqtt_user_t *item;

	assert(list != NULL);

	item = *list;
	while (item != NULL)
	{
		next = list_mqtt_user_next(item);
		if (item->user_name != NULL)
			free(item->user_name);
		if (item->password != NULL)
			free(item->password);
		free(item);
		item = next;
	}
	*list = NULL;
}

//--------------------------------------------
list_mqtt_user_t *list_mqtt_user_find_user(list_mqtt_user_t **list, uint8_t *user_name, uint16_t user_name_length, uint8_t *password, uint16_t password_length)
{
	list_mqtt_user_t *item;

	assert(list != NULL);
	assert(user_name != NULL);
	assert(password != NULL);
	assert(user_name_length != 0);
	assert(password_length != 0);

	item = *list;
	while (item != NULL)
	{
		if ((item->user_name_length == user_name_length && memcmp(item->user_name, user_name, user_name_length) == 0) &&
			(item->password_length == password_length && memcmp(item->password, password, password_length) == 0))
			return item;
		item = list_mqtt_user_next(item);
	}
	return NULL;
}
