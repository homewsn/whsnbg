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

#ifndef THREAD_UDP_H_
#define THREAD_UDP_H_

int thread_udp_start(void);
void thread_udp_stop(void);
void thread_udp_addr_setup(unsigned short port, unsigned long address);

#endif /* THREAD_UDP_H_ */
