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

#include "config.h"
#include <stdlib.h>			/* exit */
#include <stdio.h>			/* getchar, printf */
#include "os_port.h"
#include "msg_udp_mqtt.h"
#include "msg_mqtt_udp.h"
#include "msg_tcp_mqtt.h"
#include "msg_mqtt_tcp.h"
#include "thread_udp.h"
#include "thread_tcp.h"
#include "thread_test_mqtt.h"
#include "thread_test_mqttsn.h"
#include "thread_tick.h"
#include "tick_counter.h"
#include "test_mqttsn.h"
#include "test_mqtt.h"

#ifdef _MSC_VER
#pragma warning (disable:4996) // This function may be unsafe.
#endif

#ifdef WIN32
#include "getopt.h"
#endif

//--------------------------------------------
size_t test_suite_get_size()
{
	return (test_mqttsn_get_size() < test_mqtt_get_size() ? test_mqttsn_get_size() : test_mqtt_get_size());
}

//--------------------------------------------
void print_usage()
{
	printf("Usage: sensim -a whsnbg_ip_addr[127.0.0.1] -p whsnbg_mqttsn_udp_port[1883] -s test_suite_number[1...3]\n");
	printf("test_suite_number:\n");
	printf("\t1 - general connect test\n");
	printf("\t2 - sleep sensor test\n");
	printf("\t3 - actuator test\n");
}

//--------------------------------------------
int main(int argc, char *argv[])
{
	int option;
	unsigned short port;
	unsigned long addr;
	size_t suite;
#ifdef WIN32
	WSADATA data;
#endif

	if (argc != 7)
	{
		print_usage();
		exit(EXIT_FAILURE);
	}

	option = 0;
	while ((option = getopt(argc, argv, "a:p:s:")) != -1)
	{
		switch (option)
		{
		case 'a':
			addr = inet_addr(optarg);
			break;
		case 'p':
			port = (unsigned short)atoi(optarg);
			break;
		case 's':
			suite = (size_t)atoi(optarg);
			if (suite >= 1 && suite <= test_suite_get_size())
			{
				suite -= 1;
				break;
			}
		default:
			print_usage();
			exit(EXIT_FAILURE);
		}
	}

	test_mqtt_set_list(suite);
	test_mqttsn_set_list(suite);
	test_mqtt_setup(0, 0);
	test_mqttsn_setup(0, 0);

#ifdef WIN32
	WSAStartup(MAKEWORD(2, 2), &data);
#endif

	tick_counter_init();
	msg_udp_mqtt_init();
	msg_mqtt_udp_init();
	msg_tcp_mqtt_init();
	msg_mqtt_tcp_init();

	thread_udp_addr_setup(port, addr);
	if (thread_udp_start() < 0)
	{
		exit(EXIT_FAILURE);
	}
	thread_tcp_addr_setup(port, addr);
	if (thread_tcp_start() < 0)
	{
		exit(EXIT_FAILURE);
	}
	thread_tick_start();
	thread_test_mqttsn_start();
	thread_test_mqtt_start();

	getchar();

	msg_udp_mqtt_close();
	msg_mqtt_udp_close();
	msg_tcp_mqtt_close();
	msg_mqtt_tcp_close();

	thread_test_mqttsn_stop();
	thread_test_mqtt_stop();
	thread_tick_stop();
	thread_udp_stop();
	thread_tcp_stop();

	test_mqttsn_destroy();
	test_mqtt_destroy();

	tick_counter_destroy();
	msg_udp_mqtt_destroy();
	msg_mqtt_udp_destroy();
	msg_tcp_mqtt_destroy();
	msg_mqtt_tcp_destroy();

#ifdef WIN32
	WSACleanup();
#endif

	exit(EXIT_SUCCESS);
}
