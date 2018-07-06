/*
* Copyright (c) 2018 Vladimir Alemasov
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

#ifdef _MSC_VER
#include "stdint_msc.h"	/* uint8_t ... uint64_t */
#else
#include <stdint.h>		/* uint8_t ... uint64_t */
#endif

#include <assert.h>		/* assert */
#include <stdio.h>
#include "os_port.h"
#include "msg_udp_mqtt.h"
#include "msg_mqtt_udp.h"
#include "thread_state.h"

#ifdef _MSC_VER
#pragma warning (disable:4996) // This function may be unsafe.
#endif

#ifndef NDPRINTF
#include <stdio.h>
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
	(LPSTR)&s, \
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
#else
#define dprintf(...)
#define print_error_socket(...)
#endif

#define MTU 110
//--------------------------------------------
static SOCKET sock;
static char sock_buf[MTU];
static volatile thread_state_t thread_state;

static struct sockaddr_in addr;
//--------------------------------------------
void thread_udp_addr_setup(unsigned short port, unsigned long address)
{
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = address;
}

//--------------------------------------------
static void thread_run(void *param)
{
	for (;;)
	{
		fd_set rd;
		int res;
		struct timeval tv = { 0, 10000 }; /* 0.01 sec */

		FD_ZERO(&rd);
		FD_SET(sock, &rd);

		res = select((int)sock + 1, &rd, NULL, NULL, &tv);

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
				if (sendto(sock, ms->msg_buf, (int)ms->msg_cnt, 0, (const struct sockaddr *)&addr, sizeof(addr)) < 0)
					print_error_socket(__LINE__);
				msg_mqtt_udp_remove(ms);
			}
			if (thread_state == THREAD_STAYING)
				break;
			continue;
		}

		if (FD_ISSET(sock, &rd))
		{
			struct sockaddr_in addr_in;
			int recv_cnt;
			int len = sizeof(addr);

			recv_cnt = recvfrom(sock, sock_buf, sizeof(sock_buf), 0, (struct sockaddr *)&addr_in, &len);
			if (recv_cnt > 0)
				msg_udp_mqtt_add_packet(&addr_in, sock_buf, recv_cnt);
			if (recv_cnt < 0)
				print_error_socket(__LINE__);
		}
	}

	if (sock != INVALID_SOCKET)
		closesocket(sock);

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

	if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		print_error_socket(__LINE__);
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
}
