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

#include "msg_tcp_mqtt.h"
#include <string.h>		/* memset, memcpy */

static msgqueue_t queue;

//--------------------------------------------
void msg_tcp_mqtt_init(void)
{
	msg_init(&queue);
}

//--------------------------------------------
msg_tcp_mqtt_t *msg_tcp_mqtt_new(void)
{
	msg_tcp_mqtt_t *ms;
	ms = (msg_tcp_mqtt_t *)malloc(sizeof(msg_tcp_mqtt_t));
	memset(ms, 0, sizeof(msg_tcp_mqtt_t));
	return ms;
}

//--------------------------------------------
void msg_tcp_mqtt_add(msg_tcp_mqtt_t *ms)
{
	msg_add(&queue, (msg_t *)ms);
}

//--------------------------------------------
void msg_tcp_mqtt_remove(msg_tcp_mqtt_t *ms)
{
	msg_remove(&queue, (msg_t *)ms);
	if (ms->msg_buf != NULL)
		free(ms->msg_buf);
	free(ms);
}

//--------------------------------------------
msg_tcp_mqtt_t *msg_tcp_mqtt_get_first(void)
{
	return (msg_tcp_mqtt_t *)msg_get_first(&queue);
}

//--------------------------------------------
void msg_tcp_mqtt_destroy(void)
{
	msg_destroy(&queue);
}

//--------------------------------------------
void msg_tcp_mqtt_add_close_conn(struct sockaddr_in *addr)
{
	msg_tcp_mqtt_t *ms = msg_tcp_mqtt_new();
	memcpy(&ms->addr, addr, sizeof(struct sockaddr_in));
	ms->close = 1;
	msg_tcp_mqtt_add(ms);
}

//--------------------------------------------
void msg_tcp_mqtt_add_packet(struct sockaddr_in *addr, unsigned char *buf, size_t size)
{
	msg_tcp_mqtt_t *ms = msg_tcp_mqtt_new();
	memcpy(&ms->addr, addr, sizeof(struct sockaddr_in));
	ms->msg_buf = (unsigned char *)malloc(size);
	ms->msg_cnt = size;
	memcpy(ms->msg_buf, buf, size);
	msg_tcp_mqtt_add(ms);
}
