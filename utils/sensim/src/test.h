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

#ifndef TEST_H_
#define TEST_H_

//--------------------------------------------
typedef struct test
{
	uint32_t delay;
	void (*command)(void *);
	void *to_send;
	int (*response)(void *, void *);
	void *expected;
	int (*noresponse)(void);
	uint32_t timeout;
} test_t;

#endif /* TEST_H_ */
