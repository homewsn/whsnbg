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

#ifndef __PARSE_CONF_H__
#define __PARSE_CONF_H__

#include "config.h"

// .conf file
#define MAX_OPTIONS				20
#define MAX_CONF_NAME_SIZE		32
#define MAX_CONF_VALUE_SIZE		64
#define MAX_CONF_FILE_LINE_SIZE	MAX_CONF_NAME_SIZE + MAX_CONF_VALUE_SIZE + 10

#ifdef THREAD_MYSQL
//--------------------------------------------
extern long mysql_enable;
#endif

//--------------------------------------------
int parse_conf_file(void);

#endif /* __PARSE_CONF_H__ */
