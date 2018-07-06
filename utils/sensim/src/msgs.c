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

#include "msgs.h"

//--------------------------------------------
static void add(msgqueue_t *queue, msg_t *msg)
{
	msg->next = queue->msglist;
	queue->msglist = msg;
}

//--------------------------------------------
static void remove(msgqueue_t *queue, msg_t *msg)
{
	msg_t *item;

	if (msg == queue->msglist)
		queue->msglist = queue->msglist->next;
	else
	{
		for (item = queue->msglist; item != NULL && item->next != msg; item = item->next);
		if (item != NULL)
			item->next = msg->next;
	}
	msg->next = NULL;
}

//--------------------------------------------
static msg_t* get_first(msgqueue_t *queue)
{
	msg_t *m;

	if (queue->msglist == NULL)
		return NULL;
	for (m = queue->msglist; m->next != NULL; m = m->next);
	return m;
}



//--------------------------------------------
//** msgqueue_t functions

//--------------------------------------------
void msg_init(msgqueue_t *queue)
{
	pthread_mutex_init(&queue->mutex, NULL);
}

//--------------------------------------------
void msg_add(msgqueue_t *queue, msg_t *msg)
{
	pthread_mutex_lock(&queue->mutex);
	add(queue, msg);
	pthread_mutex_unlock(&queue->mutex);
}

//--------------------------------------------
void msg_remove(msgqueue_t *queue, msg_t *msg)
{
	pthread_mutex_lock(&queue->mutex);
	remove(queue, msg);
	pthread_mutex_unlock(&queue->mutex);
}

//--------------------------------------------
msg_t *msg_get_first(msgqueue_t *queue)
{
	msg_t* msg;

	pthread_mutex_lock(&queue->mutex);
	msg = get_first(queue);
	pthread_mutex_unlock(&queue->mutex);
	return msg;
}

//--------------------------------------------
void msg_destroy(msgqueue_t *queue)
{
	pthread_mutex_destroy(&queue->mutex);
}



//--------------------------------------------
//** msgqueue_cond_t functions

//--------------------------------------------
void msg_cond_init(msgqueue_cond_t *queue)
{
	pthread_mutex_init(&((msgqueue_t *)queue)->mutex, NULL);
	pthread_cond_init(&queue->cond, NULL);
}

//--------------------------------------------
void msg_cond_add(msgqueue_cond_t *queue, msg_t *msg)
{
	pthread_mutex_lock(&((msgqueue_t *)queue)->mutex);
	add((msgqueue_t *)queue, msg);
	pthread_cond_signal(&queue->cond);
	pthread_mutex_unlock(&((msgqueue_t *)queue)->mutex);
}

//--------------------------------------------
void msg_cond_remove(msgqueue_cond_t *queue, msg_t *msg)
{
	msg_remove((msgqueue_t *)queue, msg);
}

//--------------------------------------------
msg_t *msg_cond_get_first(msgqueue_cond_t *queue)
{
	msg_t *msg;

	pthread_mutex_lock(&((msgqueue_t *)queue)->mutex);
	msg = get_first((msgqueue_t *)queue);
	if (msg == NULL)
		pthread_cond_wait(&queue->cond, &((msgqueue_t *)queue)->mutex);
	pthread_mutex_unlock(&((msgqueue_t *)queue)->mutex);
	return msg;
}

//--------------------------------------------
void msg_cond_close(msgqueue_cond_t *queue)
{
	pthread_cond_signal(&queue->cond);
}

//--------------------------------------------
void msg_cond_destroy(msgqueue_cond_t *queue)
{
	pthread_cond_destroy(&queue->cond);
	pthread_mutex_destroy(&((msgqueue_t *)queue)->mutex);
}



//--------------------------------------------
//** msggap_t functions

//--------------------------------------------
void msggap_init(msggap_t *gap)
{
	gap->request = 0;
	pthread_mutex_init(&gap->mutex, NULL);
	pthread_cond_init(&gap->cond, NULL);
}

//--------------------------------------------
void msggap_request(msggap_t *gap)
{
	pthread_mutex_lock(&gap->mutex);
	gap->request = 1;
	while (gap->request == 1)
		pthread_cond_wait(&gap->cond, &gap->mutex);
	pthread_mutex_unlock(&gap->mutex);
}

//--------------------------------------------
int msggap_get_request(msggap_t *gap)
{
	int request;
	pthread_mutex_lock(&gap->mutex);
	request = gap->request;
	pthread_mutex_unlock(&gap->mutex);
	return request;
}

//--------------------------------------------
void msggap_reply(msggap_t *gap)
{
	pthread_mutex_lock(&gap->mutex);
	gap->request = 0;
	pthread_cond_signal(&gap->cond);
	pthread_mutex_unlock(&gap->mutex);
}

//--------------------------------------------
void msggap_close(msggap_t *gap)
{
	gap->request = 0;
	pthread_cond_signal(&gap->cond);
}

//--------------------------------------------
void msggap_destroy(msggap_t *gap)
{
	pthread_cond_destroy(&gap->cond);
	pthread_mutex_destroy(&gap->mutex);
}
