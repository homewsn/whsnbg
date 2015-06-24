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

#include "config.h"
#include "tls.h"
#include "list_tcp_conn.h"
#include <assert.h>		/* assert */
#include <stdlib.h>		/* malloc */
#include <string.h>		/* memcpy, memset, memcmp */

#ifdef _MSC_VER
#pragma warning (disable:4996) // This function may be unsafe.
#endif


//--------------------------------------------
list_tcp_conn_t *list_tcp_conn_add_new(list_tcp_conn_t **list, struct sockaddr_in *addr)
{
	list_tcp_conn_t *item;

	assert(list != NULL);

	item = (list_tcp_conn_t *)malloc(sizeof(list_tcp_conn_t));
	memset(item, 0, sizeof(list_tcp_conn_t));
	memcpy(&item->addr, addr, sizeof(struct sockaddr_in));

	list_tcp_conn_add(list, item);
	return item;
}

//--------------------------------------------
list_tcp_conn_t *list_tcp_conn_find_addr(list_tcp_conn_t **list, struct sockaddr_in *addr)
{
	list_tcp_conn_t *item;

	assert(list != NULL);

	item = *list;
	while (item != NULL)
	{
		if (memcmp(&item->addr, addr, sizeof(struct sockaddr_in)) == 0)
			return item;
		item = list_tcp_conn_next(item);
	}
	return NULL;
}

//--------------------------------------------
list_tcp_conn_t *list_tcp_conn_remove(list_tcp_conn_t **list, list_tcp_conn_t *item)
{
	list_tcp_conn_t *next;

	assert(list != NULL);
	assert(item != NULL);

	next = (list_tcp_conn_t *)list_remove((list_t **)list, (list_t *)item);
	free(item);
	return next;
}

//--------------------------------------------
void list_tcp_conn_remove_all(list_tcp_conn_t **list)
{
	list_tcp_conn_t *next;
	list_tcp_conn_t *item;

	item = *list;
	while (item != NULL)
	{
		next = list_tcp_conn_next(item);
		free(item);
		item = next;
	}
	*list = NULL;
}
