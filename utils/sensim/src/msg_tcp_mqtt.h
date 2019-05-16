/*
* Copyright (c) 2013-2015, 2019 Vladimir Alemasov
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

#ifndef MSG_TCP_MQTT_H_
#define MSG_TCP_MQTT_H_

#include "msgs.h"

//--------------------------------------------
typedef struct msg_tcp_mqtt
{
	msg_t msg;
	char *msg_buf;
	size_t msg_cnt;
	size_t proc_msg_cnt;
	struct sockaddr_in addr;
	unsigned char close;
} msg_tcp_mqtt_t;

//--------------------------------------------
void msg_tcp_mqtt_init(void);
msg_tcp_mqtt_t *msg_tcp_mqtt_new(void);
void msg_tcp_mqtt_add(msg_tcp_mqtt_t *ms);
void msg_tcp_mqtt_remove(msg_tcp_mqtt_t *ms);
msg_tcp_mqtt_t *msg_tcp_mqtt_get_first(void);
#define msg_tcp_mqtt_close() msg_close(a)
void msg_tcp_mqtt_destroy(void);

void msg_tcp_mqtt_add_close_conn(struct sockaddr_in *addr);
void msg_tcp_mqtt_add_packet(struct sockaddr_in *addr, unsigned char *buf, size_t size);

#endif /* MSG_TCP_MQTT_H_ */
