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

#ifndef __THREAD_MYSQL_H__
#define __THREAD_MYSQL_H__

#include "parse_conf.h"

typedef struct thread_mysql_options
{
	char mysql_server[MAX_CONF_VALUE_SIZE + 1];
	char mysql_user[MAX_CONF_VALUE_SIZE + 1];
	char mysql_password[MAX_CONF_VALUE_SIZE + 1];
	char mysql_database[MAX_CONF_VALUE_SIZE + 1];
	long mysql_port;
} thread_mysql_options_t;

int thread_mysql_start(void);
void thread_mysql_stop(void);
void thread_mysql_setup(thread_mysql_options_t *options);

#endif /* __THREAD_MYSQL_H__ */
