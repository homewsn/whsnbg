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
#include <signal.h>
#include <errno.h>			/* errno */
#endif

#include <stdlib.h>			/* exit */
#include <stdio.h>			/* getchar */
#include "os_port.h"
#include "msg_tcp_mqtt.h"
#include "msg_mqtt_tcp.h"
#include "msg_udp_mqtt.h"
#include "msg_mqtt_udp.h"
#include "msg_mqtt_mysql.h"
#include "msg_trigger_rules.h"
#include "msg_rules_mqtt.h"
#include "thread_mqtt.h"
#include "thread_tcp.h"
#include "thread_udp.h"
#include "thread_mysql.h"
#include "thread_cron.h"
#include "thread_rules.h"
#include "parse_conf.h"
#include "parse_json.h"

#ifdef _MSC_VER
#pragma warning (disable:4996) // This function may be unsafe.
#endif

#ifdef LINUX_DAEMON_VERSION
unsigned int signal_exit = 0;

//--------------------------------------------
void signal_handler(int sig)
{
	switch(sig)
	{
	case SIGTERM:
		syslog(LOG_WARNING, "Received SIGTERM signal.");
		signal_exit = 1;
		break;
	}
}
#endif


//--------------------------------------------
int main(void)
{
#ifdef WIN32
	WSADATA data;
	WSAStartup(MAKEWORD(2, 2), &data);
#endif

#ifdef LINUX_DAEMON_VERSION
	pid_t pid;

	// setup signal handling
	signal(SIGTERM, signal_handler);

	// fork off the parent process
	pid = fork();
	if (pid < 0)
	{
		fprintf(stderr, "fork failed: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}
	if (pid > 0)
	{
		// kill the parent process
		fprintf(stdout, "process id of child process %d \n", pid);
		exit(EXIT_SUCCESS); 
	}
	// change the file mode mask
	umask(0);
	// set new session
	if (setsid() < 0)
	{
		fprintf(stderr, "setsid failed: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}
	// change the current working directory to root.
	if ((chdir("/")) < 0)
	{
		fprintf(stderr, "chdir failed: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}
	// close stdin, stdout and stderr
	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);

	// open a log file
	openlog("whsnbg", LOG_PID, LOG_DAEMON);
#endif

	if (parse_conf_file() < 0)
		exit(EXIT_FAILURE);

	msg_tcp_mqtt_init();
	msg_mqtt_tcp_init();
	msg_udp_mqtt_init();
	msg_mqtt_udp_init();
#ifdef THREAD_MYSQL
	if (mysql_enable == 1)
		msg_mqtt_mysql_init();
#endif
#ifdef RULES_ENGINE
	msg_trigger_rules_init();
	msggap_rules_mqtt_init();
	msg_rules_mqtt_init();
#endif

	if (thread_tcp_start() < 0)
		exit(EXIT_FAILURE);
	if (thread_udp_start() < 0)
		exit(EXIT_FAILURE);
	thread_mqtt_start();
#ifdef THREAD_MYSQL
	if (mysql_enable == 1)
	{
		if (thread_mysql_start() < 0)
			exit(EXIT_FAILURE);
	}
#endif
#ifdef RULES_ENGINE
	parse_json_file();
	publish_rules_engine_version();
	thread_rules_start();
	thread_cron_start();
#endif

#ifdef LINUX_DAEMON_VERSION
	while (!signal_exit)
		sleep(500);
#else
	getchar();
#endif


	msg_tcp_mqtt_close();
	msg_mqtt_tcp_close();
	msg_udp_mqtt_close();
	msg_mqtt_udp_close();
#ifdef THREAD_MYSQL
	if (mysql_enable == 1)
		msg_mqtt_mysql_close();
#endif
#ifdef RULES_ENGINE
	msg_trigger_rules_close();
	msggap_rules_mqtt_close();
	msg_rules_mqtt_close();
#endif


	thread_udp_stop();
	thread_tcp_stop();
	thread_mqtt_stop();
#ifdef THREAD_MYSQL
	if (mysql_enable == 1)
		thread_mysql_stop();
#endif
#ifdef RULES_ENGINE
	thread_cron_stop();
	thread_rules_stop();
#endif


	msg_tcp_mqtt_destroy();
	msg_udp_mqtt_destroy();
	msg_mqtt_tcp_destroy();
	msg_mqtt_udp_destroy();
#ifdef THREAD_MYSQL
	if (mysql_enable == 1)
		msg_mqtt_mysql_destroy();
#endif
#ifdef RULES_ENGINE
	msg_trigger_rules_destroy();
	msggap_rules_mqtt_destroy();
	msg_rules_mqtt_destroy();
#endif


#ifdef LINUX_DAEMON_VERSION
	syslog(LOG_NOTICE, "exit");
	closelog();
#endif

#ifdef WIN32
	WSACleanup();
#endif

	exit(EXIT_SUCCESS);
}
