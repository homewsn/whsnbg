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

#include "msg_mqtt_mysql.h"
#include <string.h>		/* memset */

static msgqueue_t queue;

//--------------------------------------------
void msg_mqtt_mysql_init(void)
{
	msg_init(&queue);
}

//--------------------------------------------
msg_mqtt_mysql_t *msg_mqtt_mysql_new(void)
{
	msg_mqtt_mysql_t *ms;
	ms = (msg_mqtt_mysql_t *)malloc(sizeof(msg_mqtt_mysql_t));
	memset(ms, 0, sizeof(msg_mqtt_mysql_t));
	return ms;
}

//--------------------------------------------
void msg_mqtt_mysql_add(msg_mqtt_mysql_t *ms)
{
	msg_add(&queue, (msg_t *)ms);
}

//--------------------------------------------
void msg_mqtt_mysql_remove(msg_mqtt_mysql_t *ms)
{
	msg_remove(&queue, (msg_t *)ms);
	if (ms->msg_mysql != NULL)
		free(ms->msg_mysql);
	free(ms);
}

//--------------------------------------------
msg_mqtt_mysql_t* msg_mqtt_mysql_get_first(void)
{
	return (msg_mqtt_mysql_t *)msg_get_first(&queue);
}

//--------------------------------------------
void msg_mqtt_mysql_destroy(void)
{
	msg_destroy(&queue);
}

//--------------------------------------------
void msg_mqtt_mysql_add_long_data(uint32_t sensor_id, uint32_t sensor_param, long long_data)
{
	msg_mqtt_mysql_t *ms = msg_mqtt_mysql_new();
	ms->type = MYSQL_ADD_LONG_DATA;
	ms->msg_mysql = (void *)malloc(sizeof(msg_mysql_add_long_data_t));
	((msg_mysql_add_long_data_t *)(ms->msg_mysql))->timestamp = time(NULL); // unix timestamp (UTC)
	((msg_mysql_add_long_data_t *)(ms->msg_mysql))->sensor_id = sensor_id;
	((msg_mysql_add_long_data_t *)(ms->msg_mysql))->sensor_param = sensor_param;
	((msg_mysql_add_long_data_t *)(ms->msg_mysql))->long_data = long_data;
	msg_mqtt_mysql_add(ms);
}

//--------------------------------------------
void msg_mqtt_mysql_add_float_data(uint32_t sensor_id, uint32_t sensor_param, float float_data)
{
	msg_mqtt_mysql_t *ms = msg_mqtt_mysql_new();
	ms->type = MYSQL_ADD_FLOAT_DATA;
	ms->msg_mysql = (void *)malloc(sizeof(msg_mysql_add_float_data_t));
	((msg_mysql_add_float_data_t *)(ms->msg_mysql))->timestamp = time(NULL); // unix timestamp (UTC)
	((msg_mysql_add_float_data_t *)(ms->msg_mysql))->sensor_id = sensor_id;
	((msg_mysql_add_float_data_t *)(ms->msg_mysql))->sensor_param = sensor_param;
	((msg_mysql_add_float_data_t *)(ms->msg_mysql))->float_data = float_data;
	msg_mqtt_mysql_add(ms);
}

//--------------------------------------------
void msg_mqtt_mysql_add_utf8str_data(uint32_t sensor_id, uint32_t sensor_param, char *utf8str_data)
{
	msg_mqtt_mysql_t *ms = msg_mqtt_mysql_new();
	ms->type = MYSQL_ADD_UTF8STR_DATA;
	ms->msg_mysql = (void *)malloc(sizeof(msg_mysql_add_utf8str_data_t));
	((msg_mysql_add_utf8str_data_t *)(ms->msg_mysql))->timestamp = time(NULL); // unix timestamp (UTC)
	((msg_mysql_add_utf8str_data_t *)(ms->msg_mysql))->sensor_id = sensor_id;
	((msg_mysql_add_utf8str_data_t *)(ms->msg_mysql))->sensor_param = sensor_param;
	((msg_mysql_add_utf8str_data_t *)(ms->msg_mysql))->utf8str_data = utf8str_data;
	msg_mqtt_mysql_add(ms);
}

//--------------------------------------------
void msg_mqtt_mysql_update_sensor_param(uint32_t sensor_id, uint32_t sensor_param, char *utf8str_data)
{
	msg_mqtt_mysql_t *ms = msg_mqtt_mysql_new();
	ms->type = MYSQL_UPDATE_SENSOR_PARAM;
	ms->msg_mysql = (void *)malloc(sizeof(msg_mysql_add_sensor_param_t));
	((msg_mysql_add_sensor_param_t *)(ms->msg_mysql))->sensor_id = sensor_id;
	((msg_mysql_add_sensor_param_t *)(ms->msg_mysql))->sensor_param = sensor_param;
	((msg_mysql_add_sensor_param_t *)(ms->msg_mysql))->utf8str_data = utf8str_data;
	msg_mqtt_mysql_add(ms);
}

//--------------------------------------------
void msg_mqtt_mysql_update_sensor_ip(uint32_t sensor_id, char *utf8str_data)
{
	msg_mqtt_mysql_t *ms = msg_mqtt_mysql_new();
	ms->type = MYSQL_UPDATE_SENSOR_IP;
	ms->msg_mysql = (void *)malloc(sizeof(msg_mysql_add_sensor_utf8str_t));
	((msg_mysql_add_sensor_utf8str_t *)(ms->msg_mysql))->sensor_id = sensor_id;
	((msg_mysql_add_sensor_utf8str_t *)(ms->msg_mysql))->utf8str_data = utf8str_data;
	msg_mqtt_mysql_add(ms);
}

//--------------------------------------------
void msg_mqtt_mysql_update_sensor_location(uint32_t sensor_id, char *utf8str_data)
{
	msg_mqtt_mysql_t *ms = msg_mqtt_mysql_new();
	ms->type = MYSQL_UPDATE_SENSOR_LOCATION;
	ms->msg_mysql = (void *)malloc(sizeof(msg_mysql_add_sensor_utf8str_t));
	((msg_mysql_add_sensor_utf8str_t *)(ms->msg_mysql))->sensor_id = sensor_id;
	((msg_mysql_add_sensor_utf8str_t *)(ms->msg_mysql))->utf8str_data = utf8str_data;
	msg_mqtt_mysql_add(ms);
}

//--------------------------------------------
void msg_mqtt_mysql_update_sensor_sleeptimeduration(uint32_t sensor_id, long long_data)
{
	msg_mqtt_mysql_t *ms = msg_mqtt_mysql_new();
	ms->type = MYSQL_UPDATE_SENSOR_SLEEPTIMEDURATION;
	ms->msg_mysql = (void *)malloc(sizeof(msg_mysql_add_sensor_long_t));
	((msg_mysql_add_sensor_long_t *)(ms->msg_mysql))->sensor_id = sensor_id;
	((msg_mysql_add_sensor_long_t *)(ms->msg_mysql))->long_data = long_data;
	msg_mqtt_mysql_add(ms);
}

