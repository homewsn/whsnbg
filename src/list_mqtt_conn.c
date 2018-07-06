/*
* Copyright (c) 2013-2015, 2018 Vladimir Alemasov
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

#include "list_mqtt_conn.h"
#include <assert.h>		/* assert */
#include <stdlib.h>		/* malloc */
#include <string.h>		/* memcpy, memset, memcmp */

#ifdef _MSC_VER
#pragma warning (disable:4996) // This function may be unsafe.
#endif


//--------------------------------------------
list_mqtt_conn_t *list_mqtt_conn_add_new(list_mqtt_conn_t **list, struct sockaddr_in *addr)
{
	list_mqtt_conn_t *item;

	assert(list != NULL);

	item = (list_mqtt_conn_t *)malloc(sizeof(list_mqtt_conn_t));
	memset(item, 0, sizeof(list_mqtt_conn_t));
	memcpy(&item->addr, addr, sizeof(struct sockaddr_in));

	list_mqtt_conn_add(list, item);
	return item;
}

//--------------------------------------------
list_mqtt_conn_t *list_mqtt_conn_find_addr(list_mqtt_conn_t **list, struct sockaddr_in *addr)
{
	list_mqtt_conn_t *item;

	assert(list != NULL);

	item = *list;
	while (item != NULL)
	{
		if (memcmp(&item->addr, addr, sizeof(struct sockaddr_in)) == 0)
			return item;
		item = list_mqtt_conn_next(item);
	}
	return NULL;
}

#if 0
//--------------------------------------------
list_mqtt_conn_t *list_mqtt_conn_find_client_id(list_mqtt_conn_t **list, const char *client_id, size_t client_id_length)
{
	list_mqtt_conn_t *item;

	assert(list != NULL);

	item = *list;
	while (item != NULL)
	{
		if (item->client_id_length == client_id_length &&
			memcmp(item->client_id, client_id, client_id_length) == 0)
			return item;
		item = list_mqtt_conn_next(item);
	}
	return NULL;
}

//--------------------------------------------
void list_mqtt_conn_set_client_id(list_mqtt_conn_t *item, const char *client_id, size_t client_id_length)
{
	assert(item != NULL);

	if (item->client_id != NULL)
		free(item->client_id);
	item->client_id_length = client_id_length;
	item->client_id = (char *)malloc(client_id_length);
	memcpy(item->client_id, client_id, client_id_length);
}
#endif

//--------------------------------------------
void list_mqtt_conn_reset_remainsec(list_mqtt_conn_t *item)
{
	assert(item != NULL);

	item->remainsec = item->keepalivesec;
}

//--------------------------------------------
void list_mqtt_conn_topics_clear(list_mqtt_conn_t *item)
{
	list_sub_remove_all(&item->sub_list);
	will_remove_all(&item->will);
}

//--------------------------------------------
list_mqtt_conn_t *list_mqtt_conn_remove(list_mqtt_conn_t **list, list_mqtt_conn_t *item)
{
	list_mqtt_conn_t *next;

	assert(list != NULL);
	assert(item != NULL);

#if 0
	if (item->client_id != NULL)
		free(item->client_id);
#endif
	list_mqtt_conn_topics_clear(item);
	next = (list_mqtt_conn_t *)list_remove((list_t **)list, (list_t *)item);
	free(item);
	return next;
}

//--------------------------------------------
void list_mqtt_conn_remove_all(list_mqtt_conn_t **list)
{
	list_mqtt_conn_t *next;
	list_mqtt_conn_t *item;

	item = *list;
	while (item != NULL)
	{
		next = list_mqtt_conn_next(item);
#if 0
		if (item->client_id != NULL)
			free(item->client_id);
#endif
		list_mqtt_conn_topics_clear(item);
		free(item);
		item = next;
	}
	*list = NULL;
}
