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

#include <stdlib.h>		/* malloc, strtod */
#include <string.h>		/* memset, memcmp, memcpy */
#include <assert.h>		/* assert */
#include <errno.h>		/* errno */
#include <stdio.h>		/* sscanf, sprintf */
#include "rules.h"
#include "lists.h"
#include "thread_mqtt.h"
#include "msg_rules_mqtt.h"

#ifdef _MSC_VER
#pragma warning (disable:4996) // This function may be unsafe.
#endif

#define MAX_LONG_STRING		11

//--------------------------------------------
static void string_to_double(double **doublevalue, const uint8_t *data, uint16_t data_len)
{
	uint8_t *end;

	end = NULL;
	errno = 0;
	**doublevalue = strtod((char *)data, (char **)&end);
	if (errno == 0 && end == (uint8_t *)(data + data_len))
		return;
	*doublevalue = NULL;
}

//--------------------------------------------
static void string_to_long(long **longvalue, const uint8_t *data, uint16_t data_len)
{
	uint8_t *end;

	end = NULL;
	errno = 0;
	**longvalue = strtol((char *)data, (char **)&end, 10);
	if (errno == 0 && end == (uint8_t *)(data + data_len))
		return;
	*longvalue = NULL;
}

//--------------------------------------------
static void long_to_string(long longvalue, uint8_t **data, uint16_t *data_len)
{
	if (*data != NULL)
		free(*data);
	*data_len = MAX_LONG_STRING + 1;
	*data = (uint8_t *)malloc(*data_len);
	sprintf(*data, "%ld", longvalue);
	*data_len = (uint16_t)strlen(*data);
}

//--------------------------------------------
static size_t compare_string_values(const char *str1, size_t str1_len, const char *str2, size_t str2_len, const char *condition, size_t nextid_true, size_t nextid_false)
{
	if (strcmp(condition, "==") == 0)
	{
		if (str1_len == str2_len && (strncmp(str1, str2, str1_len) == 0))
			return nextid_true;
		else
			return nextid_false;
	}
	else if (strcmp(condition, "!=") == 0)
	{
		if (str1_len != str2_len || (strncmp(str1, str2, str1_len > str2_len ? str2_len : str1_len) != 0))
			return nextid_true;
		else
			return nextid_false;
	}

	return 0;
}

//--------------------------------------------
static size_t compare_double_values(double value1, double value2, const char *condition, size_t nextid_true, size_t nextid_false)
{
	if (strcmp(condition, "==") == 0)
	{
		if (value1 == value2)
			return nextid_true;
		else
			return nextid_false;
	}
	else if (strcmp(condition, "!=") == 0)
	{
		if (value1 != value2)
			return nextid_true;
		else
			return nextid_false;
	}
	else if (strcmp(condition, ">=") == 0)
	{
		if (value1 >= value2)
			return nextid_true;
		else
			return nextid_false;
	}
	else if (strcmp(condition, "<=") == 0)
	{
		if (value1 <= value2)
			return nextid_true;
		else
			return nextid_false;
	}
	else if (strcmp(condition, ">") == 0)
	{
		if (value1 > value2)
			return nextid_true;
		else
			return nextid_false;
	}
	else if (strcmp(condition, "<") == 0)
	{
		if (value1 < value2)
			return nextid_true;
		else
			return nextid_false;
	}

	return 0;
}

//--------------------------------------------
static size_t rf_condition_value(void *param)
{
	rf_condition_param_t *rfp;
	list_data_t *list;
	double value1;
	double value2;
	double *ptr;
	size_t nextid;

	assert(param != NULL);

	rfp = param;

	list = list_data_new(sizeof(list_data_t), rfp->name, rfp->name_len, NULL, 0);
	msggap_rules_mqtt_request(list);

	if (list->data == NULL)
	{
		nextid = 0;
		goto exit;
	}

	ptr = &value1;
	string_to_double(&ptr, list->data, list->data_len);
	if (ptr == NULL)
		goto comparestrings;

	ptr = &value2;
	string_to_double(&ptr, rfp->str, rfp->str_len);
	if (ptr == NULL)
		goto comparestrings;

	nextid = compare_double_values(value1, value2, rfp->condition, rfp->nextid_true, rfp->nextid_false);
	goto exit;

comparestrings:
	nextid = compare_string_values(list->data, list->data_len, rfp->str, rfp->str_len, rfp->condition, rfp->nextid_true, rfp->nextid_false);

exit:
	list_data_remove(list);
	return nextid;
}

//--------------------------------------------
static size_t rf_condition_topic(void *param)
{
	rf_condition_param_t *rfp;
	list_data_t *list;
	list_data_t *item;
	double value1;
	double value2;
	double *ptr;
	size_t nextid;

	assert(param != NULL);

	rfp = param;

	list = list_data_new(sizeof(list_data_t), rfp->name, rfp->name_len, NULL, 0);
	list_data_add(&list, sizeof(list_data_t), rfp->str, rfp->str_len, NULL, 0);
	msggap_rules_mqtt_request(list);

	item = list_data_next(list);

	if (list->data == NULL || item->data == NULL)
	{
		nextid = 0;
		goto exit;
	}

	ptr = &value1;
	string_to_double(&ptr, list->data, list->data_len);
	if (ptr == NULL)
		goto comparestrings;

	ptr = &value2;
	string_to_double(&ptr, item->data, item->data_len);
	if (ptr == NULL)
		goto comparestrings;

	nextid = compare_double_values(value1, value2, rfp->condition, rfp->nextid_true, rfp->nextid_false);
	goto exit;

comparestrings:
	nextid = compare_string_values(list->data, list->data_len, item->data, item->data_len, rfp->condition, rfp->nextid_true, rfp->nextid_false);

exit:
	list_data_remove_all(&list);
	return nextid;
}

//--------------------------------------------
static size_t rf_action_value(void *param)
{
	rf_action_param_t *rfp;
	list_data_t *list;

	assert(param != NULL);

	rfp = param;
	list = list_data_new(sizeof(list_data_t), rfp->name, rfp->name_len, rfp->str, rfp->str_len);
	msg_rules_mqtt_add_packet(list, rfp->retain);

	return rfp->nextid;
}

//--------------------------------------------
static size_t rf_action_topic(void *param)
{
	rf_action_param_t *rfp;
	list_data_t *list;

	assert(param != NULL);

	rfp = param;
	list = list_data_new(sizeof(list_data_t), rfp->name, rfp->name_len, NULL, 0);
	list_data_add(&list, sizeof(list_data_t), rfp->str, rfp->str_len, NULL, 0);
	msg_rules_mqtt_add_packet(list, rfp->retain);

	return rfp->nextid;
}

//--------------------------------------------
static size_t rf_variable_set(void *param)
{
	rf_variable_param_t *rfp;
	list_data_t *list;

	assert(param != NULL);

	rfp = param;
	list = list_data_new(sizeof(list_data_t), rfp->name, rfp->name_len, rfp->str, rfp->str_len);
	msg_rules_mqtt_add_packet(list, rfp->retain);

	return rfp->nextid;
}

//--------------------------------------------
static size_t rf_variable_increment_decrement(void *param, uint8_t increment)
{
	rf_variable_param_t *rfp;
	list_data_t *list;
	long value;
	long *ptr;

	assert(param != NULL);

	rfp = param;
	list = list_data_new(sizeof(list_data_t), rfp->name, rfp->name_len, NULL, 0);
	msggap_rules_mqtt_request(list);

	if (list->data == NULL)
		goto exit;

	ptr = &value;
	string_to_long(&ptr, list->data, list->data_len);
	if (ptr == NULL)
		goto exit;

	if (increment != 0)
		++value;
	else
		--value;
	long_to_string(value, &list->data, &list->data_len);
	msg_rules_mqtt_add_packet(list, rfp->retain);
	return rfp->nextid;

exit:
	list_data_remove_all(&list);
	return 0;
}

//--------------------------------------------
static size_t rf_variable_increment(void *param)
{
	return rf_variable_increment_decrement(param, 1);
}

//--------------------------------------------
static size_t rf_variable_decrement(void *param)
{
	return rf_variable_increment_decrement(param, 0);
}

//--------------------------------------------
rules_function_t *rules_function_add(rules_function_t **rfs, rf_type_t type, size_t (*func)(void *param))
{
	rules_function_t *rf;

	rf = (rules_function_t *)malloc(sizeof(rules_function_t));
	memset(rf, 0, sizeof(rules_function_t));

	rf->type = type;
	rf->func = func;

	list_add((list_t **)rfs, (list_t *)rf);
	return rf;
}

//--------------------------------------------
rules_node_t *rules_node_add(rules_node_t **list, size_t id, rf_type_t type, void *param)
{
	rules_node_t *item;

	item = (rules_node_t *)malloc(sizeof(rules_node_t));
	memset(item, 0, sizeof(rules_node_t));

	item->id = id;
	item->type = type;
	item->param = param;

	list_add((list_t **)list, (list_t *)item);
	return item;
}

//--------------------------------------------
void rules_function_remove_all(rules_function_t **list)
{
	rules_function_t *next;
	rules_function_t *item;

	assert(list != NULL);

	item = *list;
	while (item != NULL)
	{
		next = rules_function_next(item);
		free(item);
		item = next;
	}
	*list = NULL;
}

//--------------------------------------------
void rules_node_remove_all(rules_node_t **list)
{
	rules_node_t *next;
	rules_node_t *item;

	assert(list != NULL);

	item = *list;
	while (item != NULL)
	{
		next = rules_node_next(item);
		if (item->type == RF_CONDITION_TOPIC_VALUE ||
			item->type == RF_CONDITION_TOPIC_TOPIC ||
			item->type == RF_CONDITION_TOPIC_VARIABLE ||
			item->type == RF_CONDITION_VARIABLE_VALUE ||
			item->type == RF_CONDITION_VARIABLE_VARIABLE)
		{
			rf_condition_param_t *rfp;
			rfp = (rf_condition_param_t *)item->param;
			if (rfp->condition != NULL)
				free(rfp->condition);
			if (rfp->name != NULL)
				free(rfp->name);
			if (rfp->str != NULL)
				free(rfp->str);
		}
		else if (item->type == RF_ACTION_VALUE ||
			item->type == RF_ACTION_TOPIC ||
			item->type == RF_ACTION_VARIABLE)
		{
			rf_action_param_t *rfp;
			rfp = (rf_action_param_t *)item->param;
			if (rfp->name != NULL)
				free(rfp->name);
			if (rfp->str != NULL)
				free(rfp->str);
		}
		else if (item->type == RF_VARIABLE_SET ||
			item->type == RF_VARIABLE_INCREMENT ||
			item->type == RF_VARIABLE_DECREMENT)
		{
			rf_variable_param_t *rfp;
			rfp = (rf_variable_param_t *)item->param;
			if (rfp->name != NULL)
				free(rfp->name);
			if (rfp->str != NULL)
				free(rfp->str);
		}
		free(item);
		item = next;
	}
	*list = NULL;
}


//--------------------------------------------
void rules_functions_init(rules_function_t **rfs)
{
	rules_function_add(rfs, RF_CONDITION_TOPIC_VALUE, rf_condition_value);
	rules_function_add(rfs, RF_CONDITION_TOPIC_TOPIC, rf_condition_topic);
	rules_function_add(rfs, RF_CONDITION_TOPIC_VARIABLE, rf_condition_topic);
	rules_function_add(rfs, RF_CONDITION_VARIABLE_VALUE, rf_condition_value);
	rules_function_add(rfs, RF_CONDITION_VARIABLE_VARIABLE, rf_condition_topic);
	rules_function_add(rfs, RF_ACTION_VALUE, rf_action_value);
	rules_function_add(rfs, RF_ACTION_TOPIC, rf_action_topic);
	rules_function_add(rfs, RF_ACTION_VARIABLE, rf_action_topic);
	rules_function_add(rfs, RF_VARIABLE_SET, rf_variable_set);
	rules_function_add(rfs, RF_VARIABLE_INCREMENT, rf_variable_increment);
	rules_function_add(rfs, RF_VARIABLE_DECREMENT, rf_variable_decrement);
}

//--------------------------------------------
size_t func_execute(rules_function_t **rfs, rf_type_t type, void *param)
{
	rules_function_t *rf;

	rf = rules_function_head(rfs);
	while (rf != NULL)
	{
		if (rf->type == type)
			return rf->func(param);
		rf = rules_function_next(rf);
	}
	return 0;
}

//--------------------------------------------
void node_execute(rules_function_t **rfs, rules_node_t **rns, size_t id)
{
	rules_node_t *rn;
	size_t nextid;

	rn = rules_node_head(rns);
	while (rn != NULL)
	{
		if (rn->id == id)
		{
			if ((nextid = func_execute(rfs, rn->type, rn->param)) != 0)
				node_execute(rfs, rns, nextid);
			return;
		}
		rn = rules_node_next(rn);
	}
}
