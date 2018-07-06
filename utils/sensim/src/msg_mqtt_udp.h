/*
* Copyright (c) 2013-2018 Vladimir Alemasov
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

#ifndef MSG_MQTT_UDP_H_
#define MSG_MQTT_UDP_H_

#include "msgs.h"


//--------------------------------------------
typedef struct msg_mqtt_udp
{
	msg_t msg;
	unsigned char *msg_buf;
	size_t msg_cnt;
	struct sockaddr_in addr;
} msg_mqtt_udp_t;

#ifdef __cplusplus
extern "C" {
#endif
//--------------------------------------------
void msg_mqtt_udp_init(void);
msg_mqtt_udp_t *msg_mqtt_udp_new(void);
void msg_mqtt_udp_add(msg_mqtt_udp_t *ms);
void msg_mqtt_udp_remove(msg_mqtt_udp_t *ms);
msg_mqtt_udp_t *msg_mqtt_udp_get_first(void);
#define msg_mqtt_udp_close() msg_close(a)
void msg_mqtt_udp_destroy(void);

void msg_mqtt_udp_add_packet(struct sockaddr_in *addr, unsigned char *buf, size_t size);
#ifdef __cplusplus
}
#endif

#endif /* MSG_MQTT_UDP_H_ */
