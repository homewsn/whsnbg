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

#include "msg_udp_mqtt.h"
#include <string.h>		/* memset, memcpy */

static msgqueue_t queue;

//--------------------------------------------
void msg_udp_mqtt_init(void)
{
	msg_init(&queue);
}

//--------------------------------------------
msg_udp_mqtt_t *msg_udp_mqtt_new(void)
{
	msg_udp_mqtt_t *msg;

	msg = (msg_udp_mqtt_t *)malloc(sizeof(msg_udp_mqtt_t));
	memset(msg, 0, sizeof(msg_udp_mqtt_t));
	return msg;
}

//--------------------------------------------
void msg_udp_mqtt_add(msg_udp_mqtt_t *msg)
{
	msg_add(&queue, (msg_t *)msg);
}

//--------------------------------------------
void msg_udp_mqtt_remove(msg_udp_mqtt_t *msg)
{
	msg_remove(&queue, (msg_t *)msg);
	if (msg->msg_buf != NULL)
		free(msg->msg_buf);
	free(msg);
}

//--------------------------------------------
msg_udp_mqtt_t *msg_udp_mqtt_get_first(void)
{
	return (msg_udp_mqtt_t *)msg_get_first(&queue);
}

//--------------------------------------------
void msg_udp_mqtt_destroy(void)
{
	msg_destroy(&queue);
}

//--------------------------------------------
void msg_udp_mqtt_add_packet(struct sockaddr_in *addr, unsigned char *buf, size_t size)
{
	msg_udp_mqtt_t *msg;

	msg = msg_udp_mqtt_new();
	memcpy(&msg->addr, addr, sizeof(struct sockaddr_in));
	msg->msg_buf = (unsigned char *)malloc(size);
	msg->msg_cnt = size;
	memcpy(msg->msg_buf, buf, size);
	msg_udp_mqtt_add(msg);
}
