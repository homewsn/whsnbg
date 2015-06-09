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

#ifdef _MSC_VER
#include "stdint_msc.h"	/* uint8_t ... uint64_t */
#else
#include <stdint.h>		/* uint8_t ... uint64_t */
#endif

#include <assert.h>		/* assert */
#include "os_port.h"
#include "msg_udp_mqtt.h"
#include "msg_mqtt_udp.h"
#include "thread_state.h"
#include "thread_udp.h"

#ifdef _MSC_VER
#pragma warning (disable:4996) // This function may be unsafe.
#endif

#ifndef NDEBUG
#include <stdio.h>
#ifdef LINUX_DAEMON_VERSION
#include <errno.h>		/* errno */
#include <string.h>		/* strerror */
#define dprintf(...) syslog(LOG_DEBUG, __VA_ARGS__)
#define print_error_socket(line) syslog(LOG_DEBUG, "Socket Error on line %d: %s\n", line, strerror(errno))
#else
#define dprintf(...) printf(__VA_ARGS__)
#ifdef WIN32
#define print_error_socket(line) \
	do { \
	char output[1024]; \
	LPTSTR s = NULL; \
	FormatMessageA(	FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, \
	NULL, \
	WSAGetLastError(), \
	0, \
	(LPTSTR)&s, \
	0, \
	NULL); \
	fprintf(stderr, "Socket Error on line %d: %s\n", line, s); \
	sprintf((char *)&output, "Socket Error on line %d: %s\n", line, s); \
	LocalFree(s); \
	OutputDebugStringA(output); \
	} while (0)
#else
#include <errno.h>		/* errno */
#include <string.h>		/* strerror */
#define print_error_socket(line) fprintf(stderr, "Socket Error on line %d: %s\n", line, strerror(errno))
#endif
#endif
#else
#define dprintf(...)
#define print_error_socket(...)
#endif




//--------------------------------------------
typedef struct serv_udp
{
	unsigned short port;
	SOCKET sock;
} serv_udp_t;

static serv_udp_t serv;
static thread_udp_options_t *thread_options;
static char sock_buf[MQTTSN_IF_MTU];
static volatile thread_state_t thread_state;



//--------------------------------------------
//** servers

//--------------------------------------------
static int serv_init(struct in_addr *in_addr)
{
	struct sockaddr_in addr;

	if ((serv.sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		print_error_socket(__LINE__);
		return -1;
	}

	addr.sin_family = AF_INET;
	addr.sin_port = htons(serv.port);
	addr.sin_addr = *in_addr;

	if (bind(serv.sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
	{
		print_error_socket(__LINE__);
		return -1;
	}

	return 0;
}

//--------------------------------------------
static void serv_close(void)
{
	if (serv.sock != INVALID_SOCKET)
		closesocket(serv.sock);
}



//--------------------------------------------
//** main thread

//--------------------------------------------
static void thread_run(void *param)
{
	for (;;)
	{
		fd_set rd;
		int res;
		struct timeval tv = { 0, 10000 }; /* 0.01 sec */

		FD_ZERO(&rd);
		FD_SET(serv.sock, &rd);

		res = select((int)serv.sock + 1, &rd, NULL, NULL, &tv);

		if (res < 0)
		{
			print_error_socket(__LINE__);
			break;
		}

		if (res == 0)
		{
			// timeout
			msg_mqtt_udp_t *ms;
			if ((ms = msg_mqtt_udp_get_first()) != NULL)
			{
				if (sendto(serv.sock, ms->msg_buf, (int)ms->msg_cnt, 0, (const struct sockaddr *)&ms->addr, sizeof(ms->addr)) < 0)
					print_error_socket(__LINE__);
				msg_mqtt_udp_remove(ms);
			}
			if (thread_state == THREAD_STAYING)
				break;
			continue;
		}

		if (FD_ISSET(serv.sock, &rd))
		{
			struct sockaddr_in addr;
			int recv_cnt;
			int len = sizeof(addr);

			recv_cnt = recvfrom(serv.sock, sock_buf, sizeof(sock_buf), 0, (struct sockaddr *)&addr, &len);
			if (recv_cnt > 0)
				msg_udp_mqtt_add_packet(&addr, sock_buf, recv_cnt);
			if (recv_cnt < 0)
				print_error_socket(__LINE__);
		}
	}

	serv_close();
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
int thread_udp_start(void)
{
	pthread_t thread;
	void *param = NULL;
	struct in_addr addr;
	int res;

	res = get_device_ipaddress(thread_options->mqttsn_iface, &addr);
	dprintf("%s: %s\n", thread_options->mqttsn_iface, inet_ntoa(addr));
	if (res < 0)
	{
		print_error_socket(__LINE__);
		free(thread_options);
		return -1;
	}

	if (serv_init(&addr) < 0)
	{
		free(thread_options);
		return -1;
	}

	thread_state = THREAD_RUNNING;
	thread_begin(thread_launcher, param, &thread);
	return 0;
}

//--------------------------------------------
void thread_udp_stop(void)
{
	if (thread_state == THREAD_RUNNING)
		thread_state = THREAD_STAYING;
	while (thread_state != THREAD_STOPPED)
		sleep(10);
	free(thread_options);
}

//--------------------------------------------
void thread_udp_serv_setup(thread_udp_options_t *options)
{
	thread_options = options;

	serv.port = (unsigned short)thread_options->mqttsn_port;
	serv.sock = INVALID_SOCKET;
}
