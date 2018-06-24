/*
* Copyright (c) 2013-2016 Vladimir Alemasov
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

#include "config.h"

#include <stdlib.h>			/* strtol */
#include <stdio.h>			/* fopen */
#include <ctype.h>			/* isspace */
#include <string.h>			/* strlen, strcmp */
#include <errno.h>			/* errno */
#include "cJSON.h"			/* cJSON lib */
#include "rules.h"
#include "os_port.h"
#include "thread_rules.h"
#include "thread_cron.h"
#include "thread_mqtt.h"
#include "mqtt.h"
#include "msg_rules_mqtt.h"

#ifdef _MSC_VER
#pragma warning (disable:4996) // This function may be unsafe.
#endif

#ifndef NDPRINTF
#ifdef LINUX_DAEMON_VERSION
#include <syslog.h>
#define dprintf(...) syslog(LOG_DEBUG, __VA_ARGS__)
#else
#define dprintf(...) printf(__VA_ARGS__)
#endif
#else
#define dprintf(...)
#endif


//--------------------------------------------
int check_version(const char *version)
{
	int cnt;
	int checked_ver[RULES_VERSION_STRING_TOKENS], needed_ver[RULES_VERSION_STRING_TOKENS];

	sscanf(RULES_VERSION, "%d.%d", &needed_ver[0], &needed_ver[1]);
	if (sscanf(version, "%d.%d", &checked_ver[0], &checked_ver[1]) == RULES_VERSION_STRING_TOKENS)
	{
		for (cnt = 0; cnt < RULES_VERSION_STRING_TOKENS; cnt++)
		{
			if (needed_ver[cnt] > checked_ver[cnt])
				return -1;
			else if (needed_ver[cnt] < checked_ver[cnt])
				return 1;
		}
		return 0;
	}
	return -1;
}

//--------------------------------------------
void get_variable_topic(char *var, size_t var_len, uint8_t **name, uint16_t *name_len)
{
	*name_len = (uint16_t)(sizeof(MQTT_SYSTOPIC_RULESENGINE_VARIABLES) + var_len);
	*name = (uint8_t *)malloc((size_t)(*name_len));
	memcpy(*name, MQTT_SYSTOPIC_RULESENGINE_VARIABLES, sizeof(MQTT_SYSTOPIC_RULESENGINE_VARIABLES) - 1);
	memcpy(*name + sizeof(MQTT_SYSTOPIC_RULESENGINE_VARIABLES) - 1, "/", 1);
	memcpy(*name + sizeof(MQTT_SYSTOPIC_RULESENGINE_VARIABLES), var, var_len);
}

//--------------------------------------------
static void variable_init(char *var, size_t var_len, char *data, size_t data_len, int retain)
{
	list_data_t *list;
	uint8_t *name;
	uint16_t name_len;

	get_variable_topic(var, var_len, &name, &name_len);
	list = list_data_new(sizeof(list_data_t), (uint8_t *)name, (uint16_t)name_len, (uint8_t *)data, (uint16_t)data_len);
	free(name);
	msg_rules_mqtt_add_packet(list, (uint8_t)retain);
}

//--------------------------------------------
int parse_json_file(void)
{
	cJSON *root;
	cJSON *item;
	cJSON *node;
	cJSON *id;
	cJSON *type;
	cJSON *topic;
	cJSON *cond;
	cJSON *value;
	cJSON *nextid;
	cJSON *nextid_true;
	cJSON *nextid_false;
	cJSON *retain;
	cJSON *variable;
	int cnt;
	FILE *fp = NULL;
	long fsize;
	char *str;
	rf_type_t rf_type;

	fp = fopen(RULES_FILE, "rb");
	if (fp == NULL)
	{
		dprintf("can't open %s file: %s\n", RULES_FILE, strerror(errno));
		return -1;
	}

	fseek(fp, 0, SEEK_END);
	fsize = ftell(fp);

	str = malloc(fsize + 1);
	memset(str, 0, fsize);

	fseek(fp, 0, SEEK_SET);
	if (fread(str, fsize, 1, fp) != fsize)
	{
		fclose(fp);
		dprintf("Failed to read %s file\n", RULES_FILE);
		goto error;

	}
	str[fsize] = '\0';
	fclose(fp);

	root = cJSON_Parse(str);
	if (root == NULL)
	{
		dprintf("Failed to parse %s file\n", RULES_FILE);
		goto error;
	}

	item = cJSON_GetObjectItem(root, "version");
	if (item == NULL || item->type != cJSON_String)
		goto error;
	if (check_version(item->valuestring) < 0)
	{
		dprintf("Rules file version is %s, must be less or equal %s\n", item->valuestring, RULES_VERSION);
		goto error;
	}

	item = cJSON_GetObjectItem(root, "nodes");
	if (item == NULL)
		goto error;
	rf_type = RF_NONE;
	for (cnt = 0 ; cnt < cJSON_GetArraySize(item); cnt++)
	{
		node = cJSON_GetArrayItem(item, cnt);
		if (node == NULL)
			goto error;
		type = cJSON_GetObjectItem(node, "type");
		if (type == NULL)
			goto error;
		if (strncmp(type->valuestring, RT_TRIGGER_CRON, sizeof(RT_TRIGGER_CRON) - 1) == 0)
		{
			value = cJSON_GetObjectItem(node, "value");
			if (value == NULL || value->type != cJSON_String)
				goto error;
			nextid = cJSON_GetObjectItem(node, "nextid");
			if (nextid == NULL || nextid->type != cJSON_Number)
				goto error;
			thread_cron_trigger_add(value->valuestring, (uint32_t)nextid->valuedouble);
			continue;
		}
		if (strncmp(type->valuestring, RT_TRIGGER_TOPIC, sizeof(RT_TRIGGER_TOPIC) - 1) == 0)
		{
			topic = cJSON_GetObjectItem(node, "topic");
			if (topic == NULL || topic->type != cJSON_String)
				goto error;
			nextid = cJSON_GetObjectItem(node, "nextid");
			if (nextid == NULL || nextid->type != cJSON_Number)
				goto error;
			thread_mqtt_trigger_add(topic->valuestring, (uint32_t)nextid->valuedouble);
			continue;
		}
		if (strncmp(type->valuestring, RT_VARIABLE_INIT, sizeof(RT_VARIABLE_INIT) - 1) == 0)
		{
			variable = cJSON_GetObjectItem(node, "variable");
			if (variable == NULL || variable->type != cJSON_String)
				goto error;
			value = cJSON_GetObjectItem(node, "value");
			if (value == NULL || value->type != cJSON_String)
				goto error;
			retain = cJSON_GetObjectItem(node, "retain");
			if (retain == NULL || retain->type != cJSON_Number)
				goto error;
			variable_init(variable->valuestring,
				strlen(variable->valuestring),
				value->valuestring,
				strlen(value->valuestring),
				retain->valueint);
			continue;
		}

		if (strncmp(type->valuestring, RT_CONDITION_TOPIC_VALUE, sizeof(RT_CONDITION_TOPIC_VALUE) - 1) == 0)
			rf_type = RF_CONDITION_TOPIC_VALUE;
		else if (strncmp(type->valuestring, RT_CONDITION_TOPIC_TOPIC, sizeof(RT_CONDITION_TOPIC_TOPIC) - 1) == 0)
			rf_type = RF_CONDITION_TOPIC_TOPIC;
		else if (strncmp(type->valuestring, RT_CONDITION_TOPIC_VARIABLE, sizeof(RT_CONDITION_TOPIC_VARIABLE) - 1) == 0)
			rf_type = RF_CONDITION_TOPIC_VARIABLE;
		else if (strncmp(type->valuestring, RT_CONDITION_VARIABLE_VALUE, sizeof(RT_CONDITION_VARIABLE_VALUE) - 1) == 0)
			rf_type = RF_CONDITION_VARIABLE_VALUE;
		else if (strncmp(type->valuestring, RT_CONDITION_VARIABLE_VARIABLE, sizeof(RT_CONDITION_VARIABLE_VARIABLE) - 1) == 0)
			rf_type = RF_CONDITION_VARIABLE_VARIABLE;
		else if (strncmp(type->valuestring, RT_ACTION_VALUE, sizeof(RT_ACTION_VALUE) - 1) == 0)
			rf_type = RF_ACTION_VALUE;
		else if (strncmp(type->valuestring, RT_ACTION_TOPIC, sizeof(RT_ACTION_TOPIC) - 1) == 0)
			rf_type = RF_ACTION_TOPIC;
		else if (strncmp(type->valuestring, RT_ACTION_VARIABLE, sizeof(RT_ACTION_VARIABLE) - 1) == 0)
			rf_type = RF_ACTION_VARIABLE;
		else if (strncmp(type->valuestring, RT_VARIABLE_SET, sizeof(RT_VARIABLE_SET) - 1) == 0)
			rf_type = RF_VARIABLE_SET;
		else if (strncmp(type->valuestring, RT_VARIABLE_INCREMENT, sizeof(RT_VARIABLE_INCREMENT) - 1) == 0)
			rf_type = RF_VARIABLE_INCREMENT;
		else if (strncmp(type->valuestring, RT_VARIABLE_DECREMENT, sizeof(RT_VARIABLE_DECREMENT) - 1) == 0)
			rf_type = RF_VARIABLE_DECREMENT;

		if (rf_type == RF_CONDITION_TOPIC_VALUE ||
			rf_type == RF_CONDITION_TOPIC_TOPIC ||
			rf_type == RF_CONDITION_TOPIC_VARIABLE ||
			rf_type == RF_CONDITION_VARIABLE_VALUE ||
			rf_type == RF_CONDITION_VARIABLE_VARIABLE)
		{
			rf_condition_param_t *rfp;

			id = cJSON_GetObjectItem(node, "id");
			if (id == NULL || id->type != cJSON_Number)
				goto error;

			if (rf_type == RF_CONDITION_TOPIC_VALUE ||
				rf_type == RF_CONDITION_TOPIC_TOPIC ||
				rf_type == RF_CONDITION_TOPIC_VARIABLE)
				topic = cJSON_GetObjectItem(node, "topic");
			else if (rf_type == RF_CONDITION_VARIABLE_VALUE ||
				rf_type == RF_CONDITION_VARIABLE_VARIABLE)
				topic = cJSON_GetObjectItem(node, "variable");
			if (topic == NULL || topic->type != cJSON_String)
				goto error;

			cond = cJSON_GetObjectItem(node, "condition");
			if (cond == NULL || cond->type != cJSON_String)
				goto error;

			if (rf_type == RF_CONDITION_TOPIC_VALUE ||
				rf_type == RF_CONDITION_VARIABLE_VALUE)
				value = cJSON_GetObjectItem(node, "value");
			else if (rf_type == RF_CONDITION_TOPIC_TOPIC)
				value = cJSON_GetObjectItem(node, "value-topic");
			else if (rf_type == RF_CONDITION_TOPIC_VARIABLE ||
				rf_type == RF_CONDITION_VARIABLE_VARIABLE)
				value = cJSON_GetObjectItem(node, "value-variable");
			if (value == NULL || topic->type != cJSON_String)
				goto error;

			nextid_true = cJSON_GetObjectItem(node, "nextid-true");
			if (nextid_true == NULL || nextid->type != cJSON_Number)
				goto error;

			nextid_false = cJSON_GetObjectItem(node, "nextid-false");
			if (nextid_false == NULL || nextid->type != cJSON_Number)
				goto error;

			rfp = (rf_condition_param_t *)malloc(sizeof(rf_condition_param_t));
			memset(rfp, 0, sizeof(rf_condition_param_t));
			rfp->nextid_true = (uint32_t)nextid_true->valuedouble;
			rfp->nextid_false = (uint32_t)nextid_false->valuedouble;
			rfp->condition = (char *)malloc(strlen(cond->valuestring) + 1);
			strcpy(rfp->condition, cond->valuestring);

			if (rf_type == RF_CONDITION_TOPIC_VARIABLE ||
				rf_type == RF_CONDITION_VARIABLE_VARIABLE)
				get_variable_topic(value->valuestring, strlen(value->valuestring), &rfp->str, &rfp->str_len);
			else
			{
				rfp->str_len = (uint16_t)strlen(value->valuestring);
				rfp->str = (uint8_t *)malloc(rfp->str_len);
				memcpy(rfp->str, value->valuestring, rfp->str_len);
			}

			if (rf_type == RF_CONDITION_VARIABLE_VALUE ||
				rf_type == RF_CONDITION_VARIABLE_VARIABLE)
				get_variable_topic(topic->valuestring, strlen(topic->valuestring), &rfp->name, &rfp->name_len);
			else
			{
				rfp->name_len = (uint16_t)strlen(topic->valuestring);
				rfp->name = (uint8_t *)malloc(rfp->name_len);
				memcpy(rfp->name, topic->valuestring, rfp->name_len);
			}

			thread_rules_add_node((uint32_t)id->valuedouble, rf_type, (void *)rfp);
			continue;
		}

		if (rf_type == RF_ACTION_VALUE ||
			rf_type == RF_ACTION_TOPIC ||
			rf_type == RF_ACTION_VARIABLE)
		{
			rf_action_param_t *rfp;

			id = cJSON_GetObjectItem(node, "id");
			if (id == NULL || id->type != cJSON_Number)
				goto error;
			topic = cJSON_GetObjectItem(node, "topic");
			if (topic == NULL || topic->type != cJSON_String)
				goto error;
			if (rf_type == RF_ACTION_VALUE)
				value = cJSON_GetObjectItem(node, "value");
			else if (rf_type == RF_ACTION_TOPIC)
				value = cJSON_GetObjectItem(node, "value-topic");
			else if (rf_type == RF_ACTION_VARIABLE)
				value = cJSON_GetObjectItem(node, "value-variable");
			if (value == NULL || value->type != cJSON_String)
				goto error;
			nextid = cJSON_GetObjectItem(node, "nextid");
			if (nextid == NULL || nextid->type != cJSON_Number)
				goto error;
			retain = cJSON_GetObjectItem(node, "retain");
			if (retain == NULL || retain->type != cJSON_Number)
				goto error;

			rfp = (rf_action_param_t *)malloc(sizeof(rf_action_param_t));
			memset(rfp, 0, sizeof(rf_action_param_t));
			rfp->nextid = (uint32_t)nextid->valuedouble;
			rfp->retain = retain->valueint;
			if (rf_type == RF_ACTION_VARIABLE)
				get_variable_topic(value->valuestring, strlen(value->valuestring), &rfp->str, &rfp->str_len);
			else
			{
				rfp->str_len = (uint16_t)strlen(value->valuestring);
				rfp->str = (uint8_t *)malloc(rfp->str_len);
				memcpy(rfp->str, value->valuestring, rfp->str_len);
			}
			rfp->name_len = (uint16_t)strlen(topic->valuestring);
			rfp->name = (uint8_t *)malloc(rfp->name_len);
			memcpy(rfp->name, topic->valuestring, rfp->name_len);

			thread_rules_add_node((uint32_t)id->valuedouble, rf_type, (void *)rfp);
			continue;
		}

		if (rf_type == RF_VARIABLE_SET ||
			rf_type == RF_VARIABLE_INCREMENT ||
			rf_type == RF_VARIABLE_DECREMENT)
		{
			rf_variable_param_t *rfp;

			id = cJSON_GetObjectItem(node, "id");
			if (id == NULL || id->type != cJSON_Number)
				goto error;
			variable = cJSON_GetObjectItem(node, "variable");
			if (variable == NULL || variable->type != cJSON_String)
				goto error;
			if (rf_type == RF_VARIABLE_SET)
			{
				value = cJSON_GetObjectItem(node, "value");
				if (value == NULL || value->type != cJSON_String)
					goto error;
			}
			nextid = cJSON_GetObjectItem(node, "nextid");
			if (nextid == NULL || nextid->type != cJSON_Number)
				goto error;
			retain = cJSON_GetObjectItem(node, "retain");
			if (retain == NULL || retain->type != cJSON_Number)
				goto error;

			rfp = (rf_variable_param_t *)malloc(sizeof(rf_variable_param_t));
			memset(rfp, 0, sizeof(rf_variable_param_t));
			get_variable_topic(variable->valuestring, strlen(variable->valuestring), &rfp->name, &rfp->name_len);
			rfp->nextid = (uint32_t)nextid->valuedouble;
			rfp->retain = retain->valueint;
			if (rf_type == RF_VARIABLE_SET)
			{
				rfp->str_len = (uint16_t)strlen(value->valuestring);
				rfp->str = (uint8_t *)malloc(rfp->str_len);
				memcpy(rfp->str, value->valuestring, rfp->str_len);
			}

			thread_rules_add_node((uint32_t)id->valuedouble, rf_type, (void *)rfp);
			continue;
		}

	}

	cJSON_Delete(root);
	thread_mqtt_set_rules_topic_data(str, fsize);
	free(str);
	return 0;

error:
	cJSON_Delete(root);
	thread_mqtt_set_rules_topic_data(NULL, 0);
	free(str);
	thread_cron_remove_all();
	thread_mqtt_trigger_remove_all();
	thread_rules_remove_all();
	dprintf("Corrupted or wrong %s rules file\n", RULES_FILE);
	return -1;
}

//--------------------------------------------
void publish_rules_engine_version(void)
{
	list_data_t *list;

	list = list_data_new(sizeof(list_data_t),
		MQTT_SYSTOPIC_RULESENGINE_VERSION,
		sizeof(MQTT_SYSTOPIC_RULESENGINE_VERSION) - 1,
		RULES_VERSION,
		sizeof(RULES_VERSION) - 1);
	msg_rules_mqtt_add_packet(list, 1);
}
