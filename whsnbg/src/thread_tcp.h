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

#ifndef __THREAD_TCP_H__
#define __THREAD_TCP_H__

#include "parse_conf.h"

typedef struct thread_tcp_options
{
	long mqtt_port;
	long mqtt_tls_port;
	long mqtt_ws_port;
	long mqtt_ws_tls_port;
	char mqtt_iface[MAX_CONF_VALUE_SIZE + 1];
} thread_tcp_options_t;

int thread_tcp_start(void);
void thread_tcp_stop(void);
void thread_tcp_serv_setup(thread_tcp_options_t *options);

#endif /* __THREAD_TCP_H__ */
