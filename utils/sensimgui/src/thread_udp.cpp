/*
* Copyright (c) 2013-2018 Vladimir Alemasov
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

#include <assert.h>		/* assert */

#include "config.h"
#include "os_port.h"
#include "msg_udp_mqtt.h"
#include "msg_mqtt_udp.h"

#ifdef _MSC_VER
#pragma warning (disable:4996)
#endif

#include <stdio.h>
#define print_error_socket(line) \
	do { \
	char output[1024]; \
	LPTSTR s = NULL; \
	FormatMessage(	FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, \
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


//--------------------------------------------

static SOCKET sock;

#define MTU 100
static char sock_buf[MTU];
static volatile int stop_flag = 0;


//--------------------------------------------
//** main thread

//--------------------------------------------
static void thread_run(void *param)
{
	HWND hWnd = (HWND)param;

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
			assert(0);
			break;
		}

		if (res == 0)
		{
			// timeout
			msg_mqtt_udp_t *ms;
			if ((ms = msg_mqtt_udp_get_first()) != NULL)
			{
				sendto(sock, (const char *)ms->msg_buf, (int)ms->msg_cnt, 0, (const struct sockaddr *)&ms->addr, sizeof(ms->addr));
				msg_mqtt_udp_remove(ms);
			}
			if (stop_flag == 1)
				break;
			continue;
		}

		if (FD_ISSET(sock, &rd))
		{
			struct sockaddr_in addr;
			int recv_cnt;
			int len = sizeof(addr);

			recv_cnt = recvfrom(sock, sock_buf, sizeof(sock_buf), 0, (struct sockaddr *)&addr, &len);
			if (recv_cnt > 0)
			{
				msg_udp_mqtt_add_packet(&addr, (unsigned char *)sock_buf, recv_cnt);
				PostMessage(hWnd, WM_MQTTSN_MSG, (WPARAM)0, (LPARAM)0);
			}
		}
	}

	if (sock != INVALID_SOCKET)
		closesocket(sock);

	stop_flag = 2;
}

//--------------------------------------------
static unsigned __stdcall thread_launcher(void *param)
{
	thread_run(param);
	return 0;
}

//--------------------------------------------
int thread_udp_start(HWND hWnd)
{
	pthread_t thread;
	void *param = NULL;

	if ((int)(sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		print_error_socket(__LINE__);
		return -1;
	}

	thread_begin(thread_launcher, (void *)hWnd, &thread);
	return 0;
}

//--------------------------------------------
void thread_udp_stop(void)
{
	if (stop_flag == 0)
		stop_flag = 1;
	while (stop_flag != 2)
		sleep(10);
}
