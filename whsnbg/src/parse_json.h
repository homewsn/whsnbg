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

#ifndef __PARSE_JSON_H__
#define __PARSE_JSON_H__

#include "config.h"

//--------------------------------------------
void get_variable_topic(char *var, size_t var_len, char **name, size_t *name_len);
int parse_json_file(void);
void publish_rules_engine_version(void);

#endif /* __PARSE_JSON_H__ */
