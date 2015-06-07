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

#ifndef __LIST_MQTTSN_CONN_H__
#define __LIST_MQTTSN_CONN_H__

#include "lists.h"
#include "os_port.h"
#include "mqttsn.h"

#ifdef _MSC_VER
#include "stdint_msc.h"	/* uint8_t ... uint64_t */
#else
#include <stdint.h>		/* uint8_t ... uint64_t */
#include <stddef.h>		/* size_t */
#endif

//--------------------------------------------
typedef struct list_mqttsn_conn
{
	list_t next;
	struct sockaddr_in addr;
	char *client_id;
	size_t client_id_length;
	list_sub_t *sub_list;
	will_t will;
	size_t keepalivesec;
	size_t remainsec;
	mqttsn_client_state_t state;
	list_reg_t *reg_list;		// list of the registered topics
	list_msg_t *reg_msg_list;	// list of the REGISTER messages to send
	list_msg_t *pub_msg_list;	// list of the PUBLISH messages to send
} list_mqttsn_conn_t;

//--------------------------------------------
#define list_mqttsn_conn_add(a, b) (list_mqttsn_conn_t *)list_add_item((list_t **)a, (list_t *)b)
#define list_mqttsn_conn_next(a) (list_mqttsn_conn_t *)list_next((list_t *)a)
list_mqttsn_conn_t *list_mqttsn_conn_add_new(list_mqttsn_conn_t **list, struct sockaddr_in *addr);
list_mqttsn_conn_t *list_mqttsn_conn_find_addr(list_mqttsn_conn_t **list, struct sockaddr_in *addr);
list_mqttsn_conn_t *list_mqttsn_conn_find_client_id(list_mqttsn_conn_t **list, const char *client_id, size_t client_id_length);
void list_mqttsn_conn_set_client_id(list_mqttsn_conn_t *item, const char *client_id, size_t client_id_length);
void list_mqttsn_conn_reset_remainsec(list_mqttsn_conn_t *item);
void list_mqttsn_conn_topics_clear(list_mqttsn_conn_t *item);
list_mqttsn_conn_t *list_mqttsn_conn_remove(list_mqttsn_conn_t **list, list_mqttsn_conn_t *item);
void list_mqttsn_conn_remove_all(list_mqttsn_conn_t **list);

//--------------------------------------------
#define mqttsn_conn_registered_topic_id_add(a, b) list_reg_add(&a->reg_list, b)
#define mqttsn_conn_registered_topic_id_find(a, b) list_reg_find_topic_id(&a->reg_list, b)

//--------------------------------------------
#define mqttsn_conn_register_msg_head(a) list_msg_head(&a->reg_msg_list)
#define mqttsn_conn_register_msg_length(a) list_msg_length(&a->reg_msg_list)
#define mqttsn_conn_register_msg_add(a, b, c) list_msg_add(&a->reg_msg_list, b->topic_id, c)
#define mqttsn_conn_register_msg_find(a, b) list_msg_find_msg_id(&a->reg_msg_list, b)
#define mqttsn_conn_register_msg_remove_msg_id(a, b) list_msg_remove_msg_id(&a->reg_msg_list, b)

//--------------------------------------------
#define mqttsn_conn_publish_msg_head(a) list_msg_head(&a->pub_msg_list)
#define mqttsn_conn_publish_msg_length(a) list_msg_length(&a->pub_msg_list)
#define mqttsn_conn_publish_msg_add(a, b, c) list_msg_add(&a->pub_msg_list, b->topic_id, c)
#define mqttsn_conn_publish_msg_remove(a, b) list_msg_remove(&a->pub_msg_list, b)
#define mqttsn_conn_publish_msg_find(a, b) list_msg_find_msg_id(&a->pub_msg_list, b)
#define mqttsn_conn_publish_msg_remove_msg_id(a, b) list_msg_remove_msg_id(&a->pub_msg_list, b)

//--------------------------------------------
#define mqttsn_conn_will_remove(a) will_remove_all(&a->will)
#define mqttsn_conn_will_name_set(a, b, c, d) will_name_replace(&a->will, b, c, d)
#define mqttsn_conn_will_data_set(a, b, c) will_data_replace(&a->will, b, c)


#endif /* __LIST_MQTTSN_CONN_H__ */
