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

#include "msg_mqtt_udp.h"
#include <string.h>		/* memset */

static msgqueue_t queue;

//--------------------------------------------
void msg_mqtt_udp_init(void)
{
	msg_init(&queue);
}

//--------------------------------------------
msg_mqtt_udp_t *msg_mqtt_udp_new(void)
{
	msg_mqtt_udp_t *ms;
	ms = (msg_mqtt_udp_t *)malloc(sizeof(msg_mqtt_udp_t));
	memset(ms, 0, sizeof(msg_mqtt_udp_t));
	return ms;
}

//--------------------------------------------
void msg_mqtt_udp_add(msg_mqtt_udp_t *ms)
{
	msg_add(&queue, (msg_t *)ms);
}

//--------------------------------------------
void msg_mqtt_udp_remove(msg_mqtt_udp_t *ms)
{
	msg_remove(&queue, (msg_t *)ms);
	if (ms->msg_buf != NULL)
		free(ms->msg_buf);
	free(ms);
}

//--------------------------------------------
msg_mqtt_udp_t* msg_mqtt_udp_get_first(void)
{
	return (msg_mqtt_udp_t *)msg_get_first(&queue);
}

//--------------------------------------------
void msg_mqtt_udp_destroy(void)
{
	msg_destroy(&queue);
}

//--------------------------------------------
void msg_mqtt_udp_add_packet(struct sockaddr_in *addr, unsigned char *buf, size_t size)
{
	msg_mqtt_udp_t *ms = msg_mqtt_udp_new();
	memcpy(&ms->addr, addr, sizeof(struct sockaddr_in));
	ms->msg_buf = buf;	// external malloc
	ms->msg_cnt = size;
	msg_mqtt_udp_add(ms);
}
