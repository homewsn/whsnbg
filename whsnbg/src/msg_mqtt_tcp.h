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

#ifndef __MSG_MQTT_TCP_H__
#define __MSG_MQTT_TCP_H__

#include "msgs.h"

//--------------------------------------------
typedef struct msg_mqtt_tcp
{
	msg_t msg;
	unsigned char *msg_buf;
	size_t msg_cnt;
	struct sockaddr_in addr;
	unsigned char close;
} msg_mqtt_tcp_t;

//--------------------------------------------
void msg_mqtt_tcp_init(void);
msg_mqtt_tcp_t *msg_mqtt_tcp_new(void);
void msg_mqtt_tcp_add(msg_mqtt_tcp_t *ms);
void msg_mqtt_tcp_remove(msg_mqtt_tcp_t *ms);
msg_mqtt_tcp_t *msg_mqtt_tcp_get_first(void);
#define msg_mqtt_tcp_close() msg_close(a)
void msg_mqtt_tcp_destroy(void);

void msg_mqtt_tcp_add_close_conn(struct sockaddr_in *addr);
void msg_mqtt_tcp_add_packet(struct sockaddr_in *addr, unsigned char *buf, size_t size);

#endif /* __MSG_MQTT_TCP_H__ */
