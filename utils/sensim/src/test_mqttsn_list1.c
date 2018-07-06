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

#include <stdio.h>		/* printf */
#include <assert.h>		/* assert */
#include "os_port.h"
#include "mqttsn.h"
#include "msg_mqtt_udp.h"
#include "test_mqttsn.h"

#ifdef _MSC_VER
#pragma warning (disable:4996) // This function may be unsafe.
#endif

#ifndef NDPRINTF
#define dprintf(...) printf(__VA_ARGS__)
#else
#define dprintf(...)
#endif


//--------------------------------------------
// *** Test structures ***
//--------------------------------------------

//--------------------------------------------
// CONNECT
// Flags not used: dup, qos, retain, topic_id_type
// protocol_id always is equal to 1
//--------------------------------------------
static mqttsn_connect_header_t header_connect_init_1 =
{
	/*.flags =*/ {
		/*.dup =*/ 0,
		/*.qos =*/ 0,
		/*.retain =*/ 0,
		/*.will =*/ 0,
		/*.clean_session =*/ 0,
		/*.topic_id_type =*/ 0,
		},
	/*.protocol_id =*/ 1,
	/*.duration =*/ 5,
	/*.client_id_length =*/ 11,
	/*.client_id =*/ "mqttsn_test",
};

static mqttsn_connect_header_t header_connect_init_2 =
{
	/*.flags =*/ {
		/*.dup =*/ 0,
		/*.qos =*/ 0,
		/*.retain =*/ 0,
		/*.will =*/ 0,
		/*.clean_session =*/ 1,
		/*.topic_id_type =*/ 0,
		},
	/*.protocol_id =*/ 1,
	/*.duration =*/ 0,
	/*.client_id_length =*/ 11,
	/*.client_id =*/ "mqttsn_test",
};

static mqttsn_connect_header_t header_connect_init_3 =
{
	/*.flags =*/ {
		/*.dup =*/ 0,
		/*.qos =*/ 0,
		/*.retain =*/ 0,
		/*.will =*/ 0,
		/*.clean_session =*/ 0,
		/*.topic_id_type =*/ 0,
	},
	/*.protocol_id =*/ 1,
	/*.duration =*/ 0,
	/*.client_id_length =*/ 11,
	/*.client_id =*/ "mqttsn_test",
};

static mqttsn_connect_header_t header_connect_init_4 =
{
	/*.flags =*/ {
		/*.dup =*/ 0,
		/*.qos =*/ 0,
		/*.retain =*/ 0,
		/*.will =*/ 0,
		/*.clean_session =*/ 1,
		/*.topic_id_type =*/ 0,
	},
	/*.protocol_id =*/ 1,
	/*.duration =*/ 5,
	/*.client_id_length =*/ 11,
	/*.client_id =*/ "mqttsn_test",
};


//--------------------------------------------
// CONNACK
//--------------------------------------------
static mqttsn_return_code_t return_code_connack = MQTTSN_ACCEPTED;

static mqttsn_return_code_t return_code_connack_3 = MQTTSN_REFUSED_NOT_SUPPORTED;


//--------------------------------------------
// DISCONNECT
//--------------------------------------------
static uint16_t duration_disconnect_5 = 5;


//--------------------------------------------
// PINGREQ
//--------------------------------------------
static mqttsn_pingreq_header_t header_pingreq_connected =
{
	/*.client_id_length =*/ 11,
	/*.client_id = */ "mqttsn_test",
};

static mqttsn_pingreq_header_t header_pingreq_unconnected =
{
	/*.client_id_length =*/ 11,
	/*.client_id = */ "unconnected",
};



//--------------------------------------------
// *** Test schedule ***
//--------------------------------------------
static test_t test_list[] =
{
	{
		0,                                      // immediately
		&mqttsn_command_connect,                // send CONNECT with
		(void *)&header_connect_init_1,         // clean_session=0, duration=5, client_id="mqttsn_test"
		&mqttsn_response_connack,               // wait for CONNACK with
		(void *)&return_code_connack_3,         // MQTTSN_REFUSED_NOT_SUPPORTED
		(void *)&mqttsn_noresponse_command_connect_connack,
		1,                                      // for 1 sec
	},
	{
		0,                                      // immediately
		&mqttsn_command_connect,                // send CONNECT with
		(void *)&header_connect_init_2,         // clean_session=1, duration=0, client_id="mqttsn_test"
		&mqttsn_response_connack,               // wait for CONNACK with
		(void *)&return_code_connack_3,         // MQTTSN_REFUSED_NOT_SUPPORTED
		(void *)&mqttsn_noresponse_command_connect_connack,
		1,                                      // for 1 sec
	},
	{
		0,                                      // immediately
		&mqttsn_command_connect,                // send CONNECT with
		(void *)&header_connect_init_3,         // clean_session=0, duration=0, client_id="mqttsn_test"
		&mqttsn_response_connack,               // wait for CONNACK with
		(void *)&return_code_connack_3,         // MQTTSN_REFUSED_NOT_SUPPORTED
		(void *)&mqttsn_noresponse_command_connect_connack,
		1,                                      // for 1 sec
	},
	{
		0,                                      // immediately
		&mqttsn_command_connect,                // send CONNECT with
		(void *)&header_connect_init_4,         // clean_session=1, duration=5, client_id="mqttsn_test"
		&mqttsn_response_connack,               // wait for CONNACK with
		(void *)&return_code_connack,           // MQTTSN_ACCEPTED
		(void *)&mqttsn_noresponse_command_connect_connack,
		1,                                      // for 1 sec
	},
	{
		0,                                      // immediately
		&mqttsn_command_connect,                // send CONNECT with
		(void *)&header_connect_init_1,         // clean_session=0, duration=5, client_id="mqttsn_test"
		&mqttsn_response_connack,               // wait for CONNACK with
		(void *)&return_code_connack,           // MQTTSN_ACCEPTED
		(void *)&mqttsn_noresponse_command_connect_connack,
		1,                                      // for 1 sec
	},
	{
		0,                                      // then
		&mqttsn_command_disconnect,             // send DISCONNECT
		(void *)&duration_disconnect_5,         // duration=5
		&mqttsn_response_disconnect,            // wait for DISCONNECT
		(void *)0,
		(void *)&mqttsn_noresponse_command_disconnect_disconnect,
		1,                                      // for 1 sec
	},
	{
		0,                                      // then
		(void *)0,                              // send nothing
		(void *)0,
		&mqttsn_response_nothing,               // wait for nothing
		(void *)0,
		(void *)&mqttsn_noresponse_nothing,
		5,                                      // for 5 sec
	},
	{
		0,                                      // then
		&mqttsn_command_pingreq,                // send PINGREQ with
		(void *)&header_pingreq_connected,      // client_id="mqttsn_test"
		&mqttsn_response_pingresp,              // wait for PINGRESP
		(void *)0,
		(void *)&mqttsn_noresponse_command_pingreq_pingresp,
		1,                                      // for 1 sec
	},
	{
		0,                                      // then
		(void *)0,                              // send nothing
		(void *)0,
		&mqttsn_response_nothing,               // wait for nothing
		(void *)0,
		(void *)&mqttsn_noresponse_nothing,
		5,                                      // for 5 sec
	},
	{
		0,                                      // then
		&mqttsn_command_pingreq,                // send PINGREQ with
		(void *)&header_pingreq_unconnected,    // client_id="unconnected"
		&mqttsn_response_pingresp,              // wait for PINGRESP
		(void *)0,
		(void *)&mqttsn_noresponse_command_pingreq_pingresp,
		1,                                      // for 1 sec
	},
	{
		0,                                      // then
		(void *)0,                              // send nothing
		(void *)0,
		&mqttsn_response_nothing,               // wait for nothing
		(void *)0,
		(void *)&mqttsn_noresponse_nothing,
		5,                                      // for 5 sec
	},
	{
		0,                                      // immediately
		&mqttsn_command_connect,                // send CONNECT with
		(void *)&header_connect_init_1,         // clean_session=0, duration=5, client_id="mqttsn_test"
		&mqttsn_response_connack,               // wait for CONNACK with
		(void *)&return_code_connack,           // MQTTSN_ACCEPTED
		(void *)&mqttsn_noresponse_command_connect_connack,
		1,                                      // for 1 sec
	},
	{
		0,                                      // then
		&mqttsn_command_disconnect,             // send DISCONNECT
		(void *)&duration_disconnect_5,         // duration=5
		&mqttsn_response_disconnect,            // wait for DISCONNECT
		(void *)0,
		(void *)&mqttsn_noresponse_command_disconnect_disconnect,
		1,                                      // for 1 sec
	},
};

//--------------------------------------------
size_t test_mqttsn_list1_get_size(void)
{
	return (sizeof(test_list) / sizeof(test_t));
}

//--------------------------------------------
test_t *test_mqttsn_list1_get_test(size_t number)
{
	return &test_list[number];
}