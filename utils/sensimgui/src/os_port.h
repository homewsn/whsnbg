/*
* Copyright (c) 2013-2015, 2018 Vladimir Alemasov
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

#ifndef OS_PORT_H_
#define OS_PORT_H_

// stringification macro
#define STR(x)			#x
#define STRINGIFY(x)	STR(x)

#ifdef WIN32 /* Windows */
#include <winsock2.h>
#include <process.h>    /* _beginthreadex */
#include <string.h>		/* memcpy */

#pragma comment(lib,"ws2_32.lib")

typedef HANDLE pthread_t;
typedef HANDLE pthread_mutex_t;
typedef struct pthread_cond
{
	HANDLE signal;
	HANDLE broadcast;
} pthread_cond_t;

#undef sleep

#define STDCALL __stdcall
#define sleep(a) Sleep(a)
#define strncasecmp(a, b, c) strncmp(a, b, c)

#define SHUT_RD		SD_RECEIVE
#define SHUT_WR		SD_SEND
#define SHUT_RDWR	SD_BOTH

#else /* Linux, OpenWRT */
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <stdlib.h>
#include <pthread.h>	/* pthread_create */
#include <sched.h>		/* sched_yield */
#include <string.h>		/* strcmp */
#include <unistd.h>		/* usleep, close */

typedef int SOCKET;

#define closesocket(a) close(a)
#define STDCALL
#define INVALID_SOCKET (-1)
#define sleep(a) usleep((a) * 1000)

#endif

#ifdef WIN32 /* Windows */
static int thread_begin(unsigned (__stdcall *func)(void *), void *param, pthread_t *threadidptr)
{
	uintptr_t uip;
	HANDLE threadhandle;

	uip = _beginthreadex(NULL, 0, (unsigned (__stdcall *)(void *)) func, param, 0, NULL);
	threadhandle = (HANDLE) uip;
	if (threadidptr != NULL)
		*threadidptr = threadhandle;
	return (threadhandle == NULL) ? -1 : 0;
}

static int sched_yield(void)
{
	Sleep(10);
	return 0;
}

static int pthread_mutex_init(pthread_mutex_t *mutex, void *unused)
{
	*mutex = CreateMutex(NULL, FALSE, NULL);
	return *mutex == NULL ? -1 : 0;
}

static int pthread_mutex_destroy(pthread_mutex_t *mutex)
{
	return CloseHandle(*mutex) == 0 ? -1 : 0;
}

static int pthread_mutex_lock(pthread_mutex_t *mutex)
{
	return WaitForSingleObject(*mutex, INFINITE) == WAIT_OBJECT_0? 0 : -1;
}

static int pthread_mutex_unlock(pthread_mutex_t *mutex)
{
	return ReleaseMutex(*mutex) == 0 ? -1 : 0;
}

static int pthread_cond_init(pthread_cond_t *cv, const void *unused)
{
	cv->signal = CreateEvent(NULL, FALSE, FALSE, NULL);
	cv->broadcast = CreateEvent(NULL, TRUE, FALSE, NULL);
	return cv->signal != NULL && cv->broadcast != NULL ? 0 : -1;
}

static int pthread_cond_wait(pthread_cond_t *cv, pthread_mutex_t *mutex)
{
	HANDLE handles[] = {cv->signal, cv->broadcast};
	ReleaseMutex(*mutex);
	WaitForMultipleObjects(2, handles, FALSE, INFINITE);
	return WaitForSingleObject(*mutex, INFINITE) == WAIT_OBJECT_0? 0 : -1;
}

static int pthread_cond_signal(pthread_cond_t *cv)
{
	return SetEvent(cv->signal) == 0 ? -1 : 0;
}

static int pthread_cond_broadcast(pthread_cond_t *cv)
{
	return PulseEvent(cv->broadcast) == 0 ? -1 : 0;
}

static int pthread_cond_destroy(pthread_cond_t *cv)
{
	return CloseHandle(cv->signal) && CloseHandle(cv->broadcast) ? 0 : -1;
}

static int get_device_ipaddress(const char *device, struct in_addr *in_addr)
{
	// TODO
	in_addr->s_addr = htonl(INADDR_ANY);
	return 0;
}


#else /* Linux, OpenWRT */
static int thread_begin(void *func(void *), void *param, pthread_t *threadidptr)
{
	pthread_t thread_id;
	pthread_attr_t attr;
	int result;

	pthread_attr_init(&attr);

#if defined(USE_STACK_SIZE) && USE_STACK_SIZE > 1
	// Compile-time option to control stack size, e.g. -DUSE_STACK_SIZE=16384
	pthread_attr_setstacksize(&attr, USE_STACK_SIZE);
#endif /* defined(USE_STACK_SIZE) && USE_STACK_SIZE > 1 */

	result = pthread_create(&thread_id, &attr, func, param);
	pthread_attr_destroy(&attr);
	pthread_detach(thread_id);
	if (threadidptr != NULL)
	{
		*threadidptr = thread_id;
	}
	return result;
}

static int get_device_ipaddress(const char *device, struct in_addr *in_addr)
{
	struct ifaddrs *ifaddr, *ifa;

	in_addr->s_addr = 0;
	if (getifaddrs(&ifaddr) == -1)
		return -1;

	for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
	{
		if (ifa->ifa_addr == NULL)
			continue;
		if (ifa->ifa_addr->sa_family == AF_INET)
		{
			if (strcmp(ifa->ifa_name, device) == 0)
				*in_addr = ((struct sockaddr_in *)(ifa->ifa_addr))->sin_addr;
		}
	}
	freeifaddrs(ifaddr);
	if (in_addr->s_addr == 0)
		return -1;
	return 0;
}
#endif

#endif /* OS_PORT_H_ */
