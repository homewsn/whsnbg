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

#ifndef __RULES_H__
#define __RULES_H__

#include "list.h"


#ifdef _MSC_VER
#include "stdint_msc.h"	/* uint8_t ... uint64_t */
#else
#include <stdint.h>		/* uint8_t ... uint64_t */
#include <stddef.h>		/* size_t */
#endif

#define RULES_VERSION					"1.00"
#define RULES_VERSION_STRING_TOKENS		2

#define RT_TRIGGER_CRON					"trigger-cron"
#define RT_TRIGGER_TOPIC				"trigger-topic"
#define RT_CONDITION_TOPIC_VALUE		"condition-topic-value"
#define RT_CONDITION_TOPIC_TOPIC		"condition-topic-topic"
#define RT_CONDITION_TOPIC_VARIABLE		"condition-topic-variable"
#define RT_CONDITION_VARIABLE_VALUE		"condition-variable-value"
#define RT_CONDITION_VARIABLE_VARIABLE	"condition-variable-variable"
#define RT_ACTION_VALUE					"action-value"
#define RT_ACTION_TOPIC					"action-topic"
#define RT_ACTION_VARIABLE				"action-variable"
#define RT_VARIABLE_INIT				"variable-init"
#define RT_VARIABLE_SET					"variable-set"
#define RT_VARIABLE_INCREMENT			"variable-increment"
#define RT_VARIABLE_DECREMENT			"variable-decrement"

typedef enum rf_type
{
	RF_NONE	= 0,
	RF_CONDITION_TOPIC_VALUE,
	RF_CONDITION_TOPIC_TOPIC,
	RF_CONDITION_TOPIC_VARIABLE,
	RF_CONDITION_VARIABLE_VALUE,
	RF_CONDITION_VARIABLE_VARIABLE,
	RF_ACTION_VALUE,
	RF_ACTION_TOPIC,
	RF_ACTION_VARIABLE,
	RF_VARIABLE_SET,
	RF_VARIABLE_INCREMENT,
	RF_VARIABLE_DECREMENT
} rf_type_t;

typedef struct rf_condition_param
{
	size_t nextid_true;
	size_t nextid_false;
	uint8_t *name;
	uint16_t name_len;
	char *condition;
	uint8_t *str;
	uint16_t str_len;
} rf_condition_param_t;

typedef struct rf_action_param
{
	size_t nextid;
	uint8_t *name;
	uint16_t name_len;
	uint8_t *str;
	uint16_t str_len;
	uint8_t retain;
} rf_action_param_t;

#define rf_variable_param_t rf_action_param_t

typedef struct rules_function
{
	list_t next;
	rf_type_t type;
	size_t (*func)(void *param);
} rules_function_t;

typedef struct rules_node
{
	list_t next;
	size_t id;
	rf_type_t type;					// function type
	void *param;					// function parameter
} rules_node_t;

rules_function_t *rules_function_add(rules_function_t **list, rf_type_t type, size_t (*func)(void *param));
#define rules_function_head(a) (rules_function_t *)list_head((list_t **)a)
#define rules_function_next(a) (rules_function_t *)list_next((list_t *)a)
void rules_function_remove_all(rules_function_t **list);

rules_node_t *rules_node_add(rules_node_t **list, size_t id, rf_type_t type, void *param);
#define rules_node_head(a) (rules_node_t *)list_head((list_t **)a)
#define rules_node_next(a) (rules_node_t *)list_next((list_t *)a)
void rules_node_remove_all(rules_node_t **list);

void rules_functions_init(rules_function_t **rfs);
size_t func_execute(rules_function_t **rfs, rf_type_t type, void *param);
void node_execute(rules_function_t **rfs, rules_node_t **rns, size_t id);

#endif /* __RULES_H__ */
