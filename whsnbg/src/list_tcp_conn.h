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

#ifndef __LIST_TCP_CONN_H__
#define __LIST_TCP_CONN_H__

#include "lists.h"
#include "os_port.h"
#include "tls.h"

#ifdef _MSC_VER
#include "stdint_msc.h"	/* uint8_t ... uint64_t */
#else
#include <stdint.h>		/* uint8_t ... uint64_t */
#include <stddef.h>		/* size_t */
#endif

//--------------------------------------------
typedef enum
{
	WS_OPCODE_CONTINUATION		= 0x00,
	WS_OPCODE_TEXT				= 0x01,
	WS_OPCODE_BINARY			= 0x02,
	WS_OPCODE_CONNECTION_CLOSE	= 0x08,
	WS_OPCODE_PING				= 0x09,
	WS_OPCODE_PONG				= 0x0A
} ws_opcode_t;

//--------------------------------------------
typedef struct ws_frame
{
	ws_opcode_t opcode;
	unsigned char mask;
	unsigned char fin;
	unsigned char key[4];
	size_t hdr_size;
	size_t pld_size;
	unsigned char hdr_buf[14];
	size_t hdr_cnt;
} ws_frame_t;

//--------------------------------------------
typedef enum
{
	HANDLE_NOTHING = 0,
	HANDLE_HTTP_HEADER,
	HANDLE_WS_FRAME,
	HANDLE_MQTT_MESSAGE
} conn_state_t;

//--------------------------------------------
typedef struct list_tcp_conn
{
	list_t next;
	SOCKET sock;
	struct sockaddr_in addr;
	SSL *ssl;
	conn_state_t state;
	unsigned char *recv_buf;
	size_t recv_cnt;
	ws_frame_t ws_frame;
} list_tcp_conn_t;

//--------------------------------------------
#define list_tcp_conn_add(a, b) (list_tcp_conn_t *)list_add_item((list_t **)a, (list_t *)b)
#define list_tcp_conn_next(a) (list_tcp_conn_t *)list_next((list_t *)a)
list_tcp_conn_t *list_tcp_conn_add_new(list_tcp_conn_t **list, struct sockaddr_in *addr);
list_tcp_conn_t *list_tcp_conn_find_addr(list_tcp_conn_t **list, struct sockaddr_in *addr);
list_tcp_conn_t *list_tcp_conn_remove(list_tcp_conn_t **list, list_tcp_conn_t *item);
void list_tcp_conn_remove_all(list_tcp_conn_t **list);

#endif /* __LIST_TCP_CONN_H__ */
