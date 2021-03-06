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

#include "list_mqttsn_conn.h"
#include <assert.h>		/* assert */
#include <stdlib.h>		/* malloc */
#include <string.h>		/* memcpy, memset, memcmp */

#ifdef _MSC_VER
#pragma warning (disable:4996) // This function may be unsafe.
#endif


//--------------------------------------------
list_mqttsn_conn_t *list_mqttsn_conn_add_new(list_mqttsn_conn_t **list, struct sockaddr_in *addr)
{
	list_mqttsn_conn_t *item;

	assert(list != NULL);

	item = (list_mqttsn_conn_t *)malloc(sizeof(list_mqttsn_conn_t));
	memset(item, 0, sizeof(list_mqttsn_conn_t));
	memcpy(&item->addr, addr, sizeof(struct sockaddr_in));

	list_mqttsn_conn_add(list, item);
	return item;
}

//--------------------------------------------
list_mqttsn_conn_t *list_mqttsn_conn_find_addr(list_mqttsn_conn_t **list, struct sockaddr_in *addr)
{
	list_mqttsn_conn_t *item;

	assert(list != NULL);

	item = *list;
	while (item != NULL)
	{
		if (memcmp(&item->addr, addr, sizeof(struct sockaddr_in)) == 0)
			return item;
		item = list_mqttsn_conn_next(item);
	}
	return NULL;
}

//--------------------------------------------
list_mqttsn_conn_t *list_mqttsn_conn_find_client_id(list_mqttsn_conn_t **list, const char *client_id, size_t client_id_length)
{
	list_mqttsn_conn_t *item;

	assert(list != NULL);

	item = *list;
	while (item != NULL)
	{
		if (item->client_id_length == client_id_length &&
			memcmp(item->client_id, client_id, client_id_length) == 0)
			return item;
		item = list_mqttsn_conn_next(item);
	}
	return NULL;
}

//--------------------------------------------
void list_mqttsn_conn_set_client_id(list_mqttsn_conn_t *item, const char *client_id, size_t client_id_length)
{
	assert(item != NULL);

	if (item->client_id != NULL)
		free(item->client_id);
	item->client_id_length = client_id_length;
	item->client_id = (char *)malloc(client_id_length);
	memcpy(item->client_id, client_id, client_id_length);
}

//--------------------------------------------
void list_mqttsn_conn_reset_remainsec(list_mqttsn_conn_t *item)
{
	assert(item != NULL);
	assert(item->keepalivesec != 0);

	item->remainsec = item->keepalivesec;
}

//--------------------------------------------
void list_mqttsn_conn_topics_clear(list_mqttsn_conn_t *item)
{
	list_sub_remove_all(&item->sub_list);
	will_remove_all(&item->will);
	list_reg_remove_all(&item->reg_list);
	list_msg_remove_all(&item->reg_msg_list);
	list_msg_remove_all(&item->pub_msg_list);
}

//--------------------------------------------
list_mqttsn_conn_t *list_mqttsn_conn_remove(list_mqttsn_conn_t **list, list_mqttsn_conn_t *item)
{
	list_mqttsn_conn_t *next;

	assert(list != NULL);
	assert(item != NULL);

	if (item->client_id != NULL)
		free(item->client_id);
	list_mqttsn_conn_topics_clear(item);
	next = (list_mqttsn_conn_t *)list_remove((list_t **)list, (list_t *)item);
	free(item);
	return next;
}

//--------------------------------------------
void list_mqttsn_conn_remove_all(list_mqttsn_conn_t **list)
{
	list_mqttsn_conn_t *next;
	list_mqttsn_conn_t *item;

	item = *list;
	while (item != NULL)
	{
		next = list_mqttsn_conn_next(item);
		if (item->client_id != NULL)
			free(item->client_id);
		list_mqttsn_conn_topics_clear(item);
		free(item);
		item = next;
	}
	*list = NULL;
}
