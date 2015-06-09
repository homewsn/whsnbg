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

#include "config.h"
#ifdef LINUX_DAEMON_VERSION
#include <unistd.h>
#include <syslog.h>
#endif

#include <stdio.h>			/* sprintf */
#include "os_port.h"
#include "msg_mqtt_mysql.h"
#include "thread_state.h"
#include "thread_mysql.h"
#include <mysql.h>			/* MySQL functions */

#ifdef WIN32
#pragma comment(lib,"libmysql.lib")
#endif // WIN32

#ifdef _MSC_VER
#pragma warning (disable:4996) // This function may be unsafe.
#endif


#ifndef NDPRINTF
#include <stdio.h>
#ifdef LINUX_DAEMON_VERSION
#define dprintf(...) syslog(LOG_DEBUG, __VA_ARGS__)
#define print_error_mysql(line) syslog(LOG_DEBUG, "MySQL Error on line %d: %s\n", line, mysql_error(mysql_conn))
#else
#define dprintf(...) printf(__VA_ARGS__)
#ifdef WIN32
#define print_error_mysql(line) \
	do { \
	char output[1024]; \
	fprintf(stderr, "MySQL Error on line %d: %s\n", line, mysql_error(mysql_conn)); \
	sprintf((char *)&output, "MySQL Error on line %d: %s\n", line, mysql_error(mysql_conn)); \
	OutputDebugStringA(output); \
	} while (0)
#else
#define print_error_mysql(line) fprintf(stderr, "MySQL Error on line %d: %s\n", line, mysql_error(mysql_conn))
#endif
#endif
#else
#define dprintf(...)
#define print_error_mysql(...)
#endif

//--------------------------------------------
#define MYSQL_QUERY_ADD_LONG_DATA "INSERT INTO `data_long` \
( `sensor_id` , `sensor_param` , `time` , `value` ) \
VALUES ( '%lu' , '%lu' , FROM_UNIXTIME( '%llu' ) , '%ld' );"

#define MYSQL_QUERY_ADD_FLOAT_DATA "INSERT INTO `data_float` \
( `sensor_id` , `sensor_param` , `time` , `value` ) \
VALUES ( '%lu' , '%lu' , FROM_UNIXTIME( '%llu' ) , '%f' );"

#define MYSQL_QUERY_ADD_UTF8STR_DATA "INSERT INTO `data_utf8str` \
( `sensor_id` , `sensor_param` , `time` , `value` ) \
VALUES ( '%lu' , '%lu' , FROM_UNIXTIME( '%llu' ) , '%s' );"

#define MYSQL_QUERY_UPDATE_SENSOR_PARAM "INSERT INTO `parameters` \
( `sensor_id`, `sensor_param`, `unit` ) \
VALUES ( '%lu', '%lu', '%s' ) \
ON DUPLICATE KEY UPDATE unit='%s';"

#define MYSQL_QUERY_UPDATE_SENSOR_IP "INSERT INTO `sensors` \
( `sensor_id`, `st_duration`, `location`, `sensor_ip` ) \
VALUES ( '%lu', '0', '', '%s' ) \
ON DUPLICATE KEY UPDATE sensor_ip='%s';"

#define MYSQL_QUERY_UPDATE_SENSOR_LOCATION "INSERT INTO `sensors` \
( `sensor_id`, `st_duration`, `location`, `sensor_ip` ) \
VALUES ( '%lu', '0', '%s', '' ) \
ON DUPLICATE KEY UPDATE location='%s';"

#define MYSQL_QUERY_UPDATE_SENSOR_SLEEPTIMEDURATION "INSERT INTO `sensors` \
( `sensor_id`, `st_duration`, `location`, `sensor_ip` ) \
VALUES ( '%lu', '%lu', '', '' ) \
ON DUPLICATE KEY UPDATE st_duration='%lu';"



//--------------------------------------------
static thread_mysql_options_t *thread_options;
static MYSQL *mysql_conn = NULL;
static char querybuf[512];
static volatile thread_state_t thread_state;


//--------------------------------------------
static void mysql_packet_handle(msg_mqtt_mysql_t *ms)
{
	if (ms->type == MYSQL_ADD_LONG_DATA)
	{
		msg_mysql_add_long_data_t *msg = (msg_mysql_add_long_data_t *)ms->msg_mysql;
		sprintf(querybuf, MYSQL_QUERY_ADD_LONG_DATA, (unsigned long)msg->sensor_id, (unsigned long)msg->sensor_param, (unsigned long long)msg->timestamp, msg->long_data);
		if (mysql_query(mysql_conn, querybuf) != 0)
			print_error_mysql(__LINE__);
		return;
	}

	if (ms->type == MYSQL_ADD_FLOAT_DATA)
	{
		msg_mysql_add_float_data_t *msg = (msg_mysql_add_float_data_t *)ms->msg_mysql;
		sprintf(querybuf, MYSQL_QUERY_ADD_FLOAT_DATA, (unsigned long)msg->sensor_id, (unsigned long)msg->sensor_param, (unsigned long long)msg->timestamp, msg->float_data);
		if (mysql_query(mysql_conn, querybuf) != 0)
			print_error_mysql(__LINE__);
		return;
	}

	if (ms->type == MYSQL_ADD_UTF8STR_DATA)
	{
		msg_mysql_add_utf8str_data_t *msg = (msg_mysql_add_utf8str_data_t *)ms->msg_mysql;
		sprintf(querybuf, MYSQL_QUERY_ADD_UTF8STR_DATA, (unsigned long)msg->sensor_id, (unsigned long)msg->sensor_param, (unsigned long long)msg->timestamp, msg->utf8str_data);
		if (mysql_query(mysql_conn, querybuf) != 0)
			print_error_mysql(__LINE__);
		free(msg->utf8str_data);
		return;
	}

	if (ms->type == MYSQL_UPDATE_SENSOR_PARAM)
	{
		msg_mysql_add_sensor_param_t *msg = (msg_mysql_add_sensor_param_t *)ms->msg_mysql;
		sprintf(querybuf, MYSQL_QUERY_UPDATE_SENSOR_PARAM, (unsigned long)msg->sensor_id, (unsigned long)msg->sensor_param, msg->utf8str_data, msg->utf8str_data);
		if (mysql_query(mysql_conn, querybuf) != 0)
			print_error_mysql(__LINE__);
		free(msg->utf8str_data);
		return;
	}

	if (ms->type == MYSQL_UPDATE_SENSOR_IP)
	{
		msg_mysql_add_sensor_utf8str_t *msg = (msg_mysql_add_sensor_utf8str_t *)ms->msg_mysql;
		sprintf(querybuf, MYSQL_QUERY_UPDATE_SENSOR_IP, (unsigned long)msg->sensor_id, msg->utf8str_data, msg->utf8str_data);
		if (mysql_query(mysql_conn, querybuf) != 0)
			print_error_mysql(__LINE__);
		return;
	}

	if (ms->type == MYSQL_UPDATE_SENSOR_LOCATION)
	{
		msg_mysql_add_sensor_utf8str_t *msg = (msg_mysql_add_sensor_utf8str_t *)ms->msg_mysql;
		sprintf(querybuf, MYSQL_QUERY_UPDATE_SENSOR_LOCATION, (unsigned long)msg->sensor_id, msg->utf8str_data, msg->utf8str_data);
		if (mysql_query(mysql_conn, querybuf) != 0)
			print_error_mysql(__LINE__);
		free(msg->utf8str_data);
		return;
	}

	if (ms->type == MYSQL_UPDATE_SENSOR_SLEEPTIMEDURATION)
	{
		msg_mysql_add_sensor_long_t *msg = (msg_mysql_add_sensor_long_t *)ms->msg_mysql;
		sprintf(querybuf, MYSQL_QUERY_UPDATE_SENSOR_SLEEPTIMEDURATION, (unsigned long)msg->sensor_id, (unsigned long)msg->long_data, (unsigned long)msg->long_data);
		if (mysql_query(mysql_conn, querybuf) != 0)
			print_error_mysql(__LINE__);
		return;
	}
}


//--------------------------------------------
//** main thread

//--------------------------------------------
static void thread_run(void *param)
{
	for (;;)
	{
		msg_mqtt_mysql_t *ms;

		if ((ms = msg_mqtt_mysql_get_first()) != NULL)
		{
			mysql_packet_handle(ms);
			msg_mqtt_mysql_remove(ms);
		}
		if (thread_state == THREAD_STAYING)
			break;
		sleep(10);
	}

	thread_state = THREAD_STOPPED;
}

//--------------------------------------------
#ifdef WIN32
static unsigned int __stdcall thread_launcher(void *param)
{
	thread_run(param);
	return 0;
}
#else
static void *thread_launcher(void *param)
{
	thread_run(param);
	return NULL;
}
#endif

//--------------------------------------------
int thread_mysql_start(void)
{
	pthread_t thread;
	void *param = NULL;

	mysql_conn = mysql_init(NULL);

#if MYSQL_VERSION_ID >= 50106
	{
		my_bool reconnect = 1;
		if (mysql_options(mysql_conn, MYSQL_OPT_RECONNECT, &reconnect) < 0)
			print_error_mysql(__LINE__);
	}
#else
#ifdef _MSC_VER
#pragma message ("Your MySQL client lib version does not support reconnecting after a timeout.\nPlease upgrade your MySQL client libs to at least 5.01.06 version to resolve this problem.")
#else
#warning "Your MySQL client lib version does not support reconnecting after a timeout.\nPlease upgrade your MySQL client libs to at least 5.01.06 version to resolve this problem."
#endif
#endif

	mysql_options(mysql_conn, MYSQL_SET_CHARSET_NAME, "utf8");

	if (!mysql_real_connect(mysql_conn,
		thread_options->mysql_server,
		thread_options->mysql_user,
		thread_options->mysql_password,
		thread_options->mysql_database,
		thread_options->mysql_port,
		NULL, 0))
	{
		print_error_mysql(__LINE__);
		free(thread_options);
		return -1;
	}

	thread_state = THREAD_RUNNING;
	thread_begin(thread_launcher, param, &thread);
	return 0;
}

//--------------------------------------------
void thread_mysql_stop(void)
{
	if (thread_state == THREAD_RUNNING)
		thread_state = THREAD_STAYING;
	while (thread_state != THREAD_STOPPED)
		sleep(10);
	if (mysql_conn != NULL)
		mysql_close(mysql_conn);
	free(thread_options);
}

//--------------------------------------------
void thread_mysql_setup(thread_mysql_options_t *options)
{
	thread_options = options;
}
