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

#include <stdlib.h>			/* strtol */
#include <stdio.h>			/* fopen */
#include <ctype.h>			/* isspace */
#include <string.h>			/* strlen, strcmp */
#include <errno.h>			/* errno */
#include "os_port.h"
#include "thread_tcp.h"
#include "thread_udp.h"
#include "thread_mysql.h"
#include "thread_mqtt.h"

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

#ifdef THREAD_MYSQL
long mysql_enable = 0;
#endif

//--------------------------------------------
typedef struct
{
	thread_tcp_options_t *tcp_options;
	thread_udp_options_t *udp_options;
	thread_mysql_options_t *mysql_options;
} conf_options_t;

//--------------------------------------------
static void parse_conf_long(long *long_value, const char *string_value)
{
	long value;
	char *end = NULL;

	errno = 0;
	value = strtol(string_value, &end, 10);
	if (errno == 0 && end == string_value + strlen(string_value))
		*long_value = value;
}

//--------------------------------------------
int parse_conf_file(void)
{
	conf_options_t options = { 0 };
	FILE *fp = NULL;
	char name[MAX_CONF_NAME_SIZE + 1];
	char value[MAX_CONF_VALUE_SIZE + 1];
	char buf[MAX_CONF_FILE_LINE_SIZE + 1];
	char username[MAX_CONF_VALUE_SIZE + 1];
	char userpass[MAX_CONF_VALUE_SIZE + 1];
	long userpubenable = 0;
	long auth = 0;
	size_t cnt;

	fp = fopen(CONFIG_FILE, "r");
	if (fp == NULL)
	{
		dprintf("can't open %s file: %s\n", CONFIG_FILE, strerror(errno));
		return -1;
	}

	options.tcp_options = (thread_tcp_options_t *)malloc(sizeof(thread_tcp_options_t));
	options.udp_options = (thread_udp_options_t *)malloc(sizeof(thread_udp_options_t));
#ifdef THREAD_MYSQL
	options.mysql_options = (thread_mysql_options_t *)malloc(sizeof(thread_mysql_options_t));
#endif

	while (fgets(buf, MAX_CONF_FILE_LINE_SIZE, fp) != NULL)
	{
		for (cnt = 0; isspace(*(unsigned char *)&buf[cnt]);)
			cnt++;
		if (buf[cnt] == '#' || buf[cnt] == '\0' || buf[cnt] == '=')
			continue;

		if (sscanf(buf, " %"STRINGIFY(MAX_CONF_NAME_SIZE)"[^ =] = %"STRINGIFY(MAX_CONF_VALUE_SIZE)"[^ \n\r]", (char *)&name, (char *)&value) == 2)
		{
			if (strcmp(name, "mqtt_iface") == 0)
				strcpy(options.tcp_options->mqtt_iface, value);
			else if (strcmp(name, "mqtt_port") == 0)
				parse_conf_long(&options.tcp_options->mqtt_port, value);
			else if (strcmp(name, "mqtt_tls_port") == 0)
				parse_conf_long(&options.tcp_options->mqtt_tls_port, value);
			else if (strcmp(name, "mqtt_ws_port") == 0)
				parse_conf_long(&options.tcp_options->mqtt_ws_port, value);
			else if (strcmp(name, "mqtt_ws_tls_port") == 0)
				parse_conf_long(&options.tcp_options->mqtt_ws_tls_port, value);
			else if (strcmp(name, "mqttsn_iface") == 0)
				strcpy(options.udp_options->mqttsn_iface, value);
			else if (strcmp(name, "mqttsn_port") == 0)
				parse_conf_long(&options.udp_options->mqttsn_port, value);
#ifdef THREAD_MYSQL
			else if (strcmp(name, "mysql_enable") == 0)
				parse_conf_long(&mysql_enable, value);
			else if (strcmp(name, "mysql_server") == 0)
				strcpy(options.mysql_options->mysql_server, value);
			else if (strcmp(name, "mysql_user") == 0)
				strcpy(options.mysql_options->mysql_user, value);
			else if (strcmp(name, "mysql_password") == 0)
				strcpy(options.mysql_options->mysql_password, value);
			else if (strcmp(name, "mysql_database") == 0)
				strcpy(options.mysql_options->mysql_database, value);
			else if (strcmp(name, "mysql_port") == 0)
				parse_conf_long(&options.mysql_options->mysql_port, value);
#endif
			else if (strcmp(name, "mqtt_auth_enable") == 0)
				parse_conf_long(&auth, value);
			else if (auth == 1)
			{
				if (strcmp(name, "user_name") == 0)
					strcpy(username, value);
				else if (strcmp(name, "user_password") == 0)
					strcpy(userpass, value);
				else if (strcmp(name, "user_publish_enable") == 0)
				{
					parse_conf_long(&userpubenable, value);
					thread_mqtt_user_add(username, userpass, (uint8_t)userpubenable);
				}
			}
		}
	}

	fclose(fp);

	thread_tcp_serv_setup(options.tcp_options);
	thread_udp_serv_setup(options.udp_options);

#ifdef THREAD_MYSQL
	if (mysql_enable == 1)
		thread_mysql_setup(options.mysql_options);
	else
		free(options.mysql_options);
#endif

	return 0;
}
