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

#ifndef __LIST_MQTT_CONN_H__
#define __LIST_MQTT_CONN_H__

#include "lists.h"
#include "os_port.h"
#include "mqtt.h"

#ifdef _MSC_VER
#include "stdint_msc.h"	/* uint8_t ... uint64_t */
#else
#include <stdint.h>		/* uint8_t ... uint64_t */
#include <stddef.h>		/* size_t */
#endif

//--------------------------------------------
typedef struct list_mqtt_conn
{
	list_t next;
	struct sockaddr_in addr;
#if 0
	char *client_id;
	size_t client_id_length;
#endif
	list_sub_t *sub_list;
	will_t will;
	size_t keepalivesec;
	size_t remainsec;
	unsigned char publish_enable;
} list_mqtt_conn_t;

//--------------------------------------------
#define list_mqtt_conn_add(a, b) (list_mqtt_conn_t *)list_add_item((list_t **)a, (list_t *)b)
#define list_mqtt_conn_next(a) (list_mqtt_conn_t *)list_next((list_t *)a)
list_mqtt_conn_t *list_mqtt_conn_add_new(list_mqtt_conn_t **list, struct sockaddr_in *addr);
list_mqtt_conn_t *list_mqtt_conn_find_addr(list_mqtt_conn_t **list, struct sockaddr_in *addr);
#if 0
list_mqtt_conn_t *list_mqttsn_conn_find_client_id(list_mqtt_conn_t **list, const char *client_id, size_t client_id_length);
void list_mqtt_conn_set_client_id(list_mqtt_conn_t *item, const char *client_id, size_t client_id_length);
#endif
void list_mqtt_conn_reset_remainsec(list_mqtt_conn_t *item);
list_mqtt_conn_t *list_mqtt_conn_remove(list_mqtt_conn_t **list, list_mqtt_conn_t *item);
void list_mqtt_conn_remove_all(list_mqtt_conn_t **list);

//--------------------------------------------
#define mqtt_conn_will_remove(a) will_remove_all(&a->will)
#define mqtt_conn_will_set(a, b, c, d, e, f) will_replace(&a->will, b, c, d, e, f)

#endif /* __LIST_MQTT_CONN_H__ */
