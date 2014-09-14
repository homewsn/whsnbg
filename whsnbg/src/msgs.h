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

#ifndef __MSGS_H__
#define __MSGS_H__

#include "os_port.h"

//--------------------------------------------
typedef struct msg
{
	struct msg *next;
} msg_t;


//--------------------------------------------
typedef struct msgqueue
{
	msg_t *msglist;
	pthread_mutex_t mutex;
} msgqueue_t;

void msg_init(msgqueue_t *queue);
void msg_add(msgqueue_t *queue, msg_t *msg);
void msg_remove(msgqueue_t *queue, msg_t *msg);
msg_t* msg_get_first(msgqueue_t *queue);
#define msg_close(a)
void msg_destroy(msgqueue_t *queue);


//--------------------------------------------
typedef struct msgqueue_cond
{
	msgqueue_t msgqueue;
	pthread_cond_t cond;
} msgqueue_cond_t;

void msg_cond_init(msgqueue_cond_t *queue);
void msg_cond_add(msgqueue_cond_t *queue, msg_t *msg);
void msg_cond_remove(msgqueue_cond_t *queue, msg_t *msg);
msg_t* msg_cond_get_first(msgqueue_cond_t *queue);
void msg_cond_close(msgqueue_cond_t *queue);
void msg_cond_destroy(msgqueue_cond_t *queue);


//--------------------------------------------
typedef struct msggap
{
	void *msg;
	int request;
	pthread_mutex_t mutex;
	pthread_cond_t cond;
} msggap_t;

void msggap_init(msggap_t *gap);
void msggap_request(msggap_t *gap);
int msggap_get_request(msggap_t *gap);
void msggap_reply(msggap_t *gap);
void msggap_close(msggap_t *gap);
void msggap_destroy(msggap_t *gap);

#endif /* __MSGS_H__ */
