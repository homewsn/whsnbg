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
#include "mqtt.h"
#include "msg_mqtt_tcp.h"
#include "test_mqtt.h"

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
//--------------------------------------------
static mqtt_connect_header_t header_connect_1 =
{
	/*.flags =*/ {
		/*.user_name =*/ 0,
		/*.password =*/ 0,
		/*.will_retain =*/ 0,
		/*.will_qos =*/ 0,
		/* will =*/ 0,
		/*.clean_session =*/ 1,
	},
	/*.keep_alive_timer =*/ 0,
	/*.client_id_length =*/ 9,
	/*.client_id =*/ "mqtt_test",
	/*.will_topic_length =*/ 0,
	/*.will_topic =*/ "",
	/*.will_message_length =*/ 0,
	/*.will_message =*/ "",
	/*.user_name_length =*/ 0,
	/*.user_name =*/ "",
	/*.password_length =*/ 0,
	/*.password =*/ "",
};

static mqtt_connect_header_t header_connect_2 =
{
	/*.flags =*/ {
		/*.user_name =*/ 1,
		/*.password =*/ 1,
		/*.will_retain =*/ 0,
		/*.will_qos =*/ 0,
		/* will =*/ 0,
		/*.clean_session =*/ 0,
	},
	/*.keep_alive_timer =*/ 5,
	/*.client_id_length =*/ 9,
	/*.client_id =*/ "mqtt_test",
	/*.will_topic_length =*/ 0,
	/*.will_topic =*/ "",
	/*.will_message_length =*/ 0,
	/*.will_message =*/ "",
	/*.user_name_length =*/ 5,
	/*.user_name =*/ "name1",
	/*.password_length =*/ 9,
	/*.password =*/ "password1",
};

static mqtt_connect_header_t header_connect_3 =
{
	/*.flags =*/ {
		/*.user_name =*/ 0,
		/*.password =*/ 0,
		/*.will_retain =*/ 1,
		/*.will_qos =*/ 1,
		/* will =*/ 1,
		/*.clean_session =*/ 1,
	},
	/*.keep_alive_timer =*/ 5,
	/*.client_id_length =*/ 9,
	/*.client_id =*/ "mqtt_test",
	/*.will_topic_length =*/ 9,
	/*.will_topic =*/ "mqtt_test",
	/*.will_message_length =*/ 7,
	/*.will_message =*/ "offline",
	/*.user_name_length =*/ 0,
	/*.user_name =*/ "",
	/*.password_length =*/ 0,
	/*.password =*/ "",
};

static mqtt_connect_header_t header_connect_4 =
{
	/*.flags =*/ {
		/*.user_name =*/ 0,
		/*.password =*/ 0,
		/*.will_retain =*/ 0,
		/*.will_qos =*/ 0,
		/* will =*/ 0,
		/*.clean_session =*/ 0,
	},
	/*.keep_alive_timer =*/ 5,
	/*.client_id_length =*/ 9,
	/*.client_id =*/ "mqtt_test",
	/*.will_topic_length =*/ 0,
	/*.will_topic =*/ "",
	/*.will_message_length =*/ 0,
	/*.will_message =*/ "",
	/*.user_name_length =*/ 0,
	/*.user_name =*/ "",
	/*.password_length =*/ 0,
	/*.password =*/ "",
};


//--------------------------------------------
// CONNACK
//--------------------------------------------
static mqtt_connack_return_code_t return_code_connack_1 = MQTT_ACCEPTED;

static mqtt_connack_return_code_t return_code_connack_2 = MQTT_REFUSED_PROTOCOL_VERSION;


//--------------------------------------------
// SUBSCRIBE
//--------------------------------------------
static list_sub_t list_subscribe_mqtt_test =
{
	/*.next =*/ 0,
	/*.name =*/ "mqtt_test",
	/*.name_len =*/ 9,
	/*.qos =*/ 1,
};
static mqtt_subscribe_header_t subscribe_header_m1 =
{
	/*.msg_id =*/ 1,
	/*.sub_list =*/ 0,
	/*.sub_item =*/ &list_subscribe_mqtt_test,
	/*.qos_size =*/ 1,
};


//--------------------------------------------
// SUBACK
//--------------------------------------------
static uint8_t suback_header_qos_buf[] =
{
	1,
};
static mqtt_suback_header_t suback_header_m1 =
{
	/*.msg_id =*/ 1,
	/*.qos_buf =*/ suback_header_qos_buf,
	/*.qos_size =*/ 1,
};


//--------------------------------------------
// PUBLISH
//--------------------------------------------
static list_pub_t list_publish_mqtt_test_offline =
{
	/*.next =*/ 0,
	/*.name =*/ "mqtt_test",
	/*.name_len =*/ 9,
	/*.data =*/ "offline",
	/*.data_len =*/ 7,
	/*.retain =*/ 1,
};
static mqtt_publish_header_t publish_header_offline_m1 =
{
	/*.msg_id =*/ 1,
	/*.qos =*/ 1,
	/*.pub_list =*/ 0,
	/*.pub_item =*/ &list_publish_mqtt_test_offline,
};


//--------------------------------------------
// PUBACK
//--------------------------------------------
static mqtt_puback_header_t puback_header_m1 =
{
	/*.msg_id =*/ 1,
};



//--------------------------------------------
// *** Test schedule ***
//--------------------------------------------
static test_t test_list[] =
{
	{
		0,                                      // immediately
		&mqtt_command_connect,                  // send CONNECT with
		(void *)&header_connect_1,              // clean_session=1, keep_alive_timer=0, client_id="mqtt_test"
		&mqtt_response_connack,                 // wait for CONNACK with
		(void *)&return_code_connack_1,         // MQTTSN_ACCEPTED
		(void *)&mqtt_noresponse_command_connect_connack,
		1,                                      // for 1 sec
	},
	{
		0,                                      // then
		&mqtt_command_disconnect,               // send DISCONNECT
		(void *)0,
		&mqtt_response_disconnect,              // wait for nothing
		(void *)0,
		(void *)&mqtt_noresponse_command_disconnect_passed,
		1,                                      // for 1 sec
	},
	{
		3,                                      // then after a delay of 3 sec
		&mqtt_command_connect,                  // send CONNECT with
		(void *)&header_connect_1,              // clean_session=1, keep_alive_timer=0, client_id="mqtt_test"
		&mqtt_response_connack,                 // wait for CONNACK with
		(void *)&return_code_connack_1,         // MQTTSN_ACCEPTED
		(void *)&mqtt_noresponse_command_connect_connack,
		1,                                      // for 1 sec
	},
	{
		3,                                      // then after a delay of 3 sec
		&mqtt_command_connect,                  // send CONNECT with
		(void *)&header_connect_1,              // clean_session=1, keep_alive_timer=0, client_id="mqtt_test"
		&mqtt_response_connack,                 // wait for CONNACK with
		(void *)&return_code_connack_2,         // MQTT_REFUSED_PROTOCOL_VERSION
		(void *)&mqtt_noresponse_command_connect_connack,
		1,                                      // for 1 sec
	},
	{
		3,                                      // then after a delay of 3 sec
		&mqtt_command_connect,                  // send CONNECT with
		(void *)&header_connect_2,              // clean_session=1, keep_alive_timer=5, client_id="mqtt_test", user_name="name1", password="password1"
		&mqtt_response_connack,                 // wait for CONNACK with
		(void *)&return_code_connack_1,         // MQTTSN_ACCEPTED
		(void *)&mqtt_noresponse_command_connect_connack,
		1,                                      // for 1 sec
	},
	{
		0,                                      // then
		&mqtt_command_disconnect,               // send DISCONNECT
		(void *)0,
		&mqtt_response_disconnect,              // wait for nothing
		(void *)0,
		(void *)&mqtt_noresponse_command_disconnect_passed,
		1,                                      // for 1 sec
	},
	{
		3,                                      // then after a delay of 3 sec
		&mqtt_command_connect,                  // send CONNECT with
		(void *)&header_connect_3,              // clean_session=1, keep_alive_timer=5, client_id="mqtt_test", will=1, will_retain=1, will_qos=1, will_topic="mqtt_test", will_message="offline"
		&mqtt_response_connack,                 // wait for CONNACK with
		(void *)&return_code_connack_1,         // MQTTSN_ACCEPTED
		(void *)&mqtt_noresponse_command_connect_connack,
		1,                                      // for 1 sec
	},
	{
		10,                                     // then after a delay of 10 sec
		&mqtt_command_connect,                  // send CONNECT with
		(void *)&header_connect_1,              // clean_session=1, keep_alive_timer=0, client_id="mqtt_test"
		&mqtt_response_connack,                 // wait for CONNACK with
		(void *)&return_code_connack_1,         // MQTTSN_ACCEPTED
		(void *)&mqtt_noresponse_command_connect_connack,
		1,                                      // for 1 sec
	},
	{
		0,                                      // then
		&mqtt_command_subscribe,                // send SUBSCRIBE with
		(void *)&subscribe_header_m1,           // msg_id=1, topic_name="mqtt_test"
		&mqtt_response_suback,                  // wait for SUBACK with
		(void *)&suback_header_m1,              // msg_id=1
		(void *)&mqtt_noresponse_command_subscribe_suback,
		1,                                      // for 1 sec
	},
	{
		0,                                      // then
		(void *)0,                              // send nothing
		(void *)0,
		&mqtt_response_publish,                 // wait for PUBLISH with
		(void *)&publish_header_offline_m1,     // msg_id=1, qos=1, retain=1, topic_name="mqtt_test", topic_data="offline"
		(void *)&mqtt_noresponse_nothing_publish,
		1,                                      // for 1 sec
	},
	{
		0,                                      // then
		&mqtt_command_puback,                   // send PUBACK
		(void *)&puback_header_m1,              // with msg_id=1
		&mqtt_response_nothing,                 // wait for nothing
		(void *)0,
		(void *)&mqtt_noresponse_nothing,
		1,                                      // for 1 sec
	},
	{
		0,                                      // then
		&mqtt_command_disconnect,               // send DISCONNECT
		(void *)0,
		&mqtt_response_disconnect,              // wait for nothing
		(void *)0,
		(void *)&mqtt_noresponse_command_disconnect_passed,
		1,                                      // for 1 sec
	},
	{
		3,                                      // then after a delay of 3 sec
		&mqtt_command_connect,                  // send CONNECT with
		(void *)&header_connect_4,              // clean_session=0, keep_alive_timer=5, client_id="mqtt_test"
		&mqtt_response_connack,                 // wait for CONNACK with
		(void *)&return_code_connack_1,         // MQTTSN_ACCEPTED
		(void *)&mqtt_noresponse_command_connect_connack,
		1,                                      // for 1 sec
	},
	{
		3,                                      // then after a delay of 3 sec
		&mqtt_command_pingreq,                  // send PINGREQ
		(void *)0,
		&mqtt_response_pingresp,                // wait for PINGRESP
		(void *)0,
		(void *)&mqtt_noresponse_command_pingreq_pingresp,
		1,                                      // for 1 sec
	},
	{
		3,                                      // then after a delay of 3 sec
		&mqtt_command_pingreq,                  // send PINGREQ
		(void *)0,
		&mqtt_response_pingresp,                // wait for PINGRESP
		(void *)0,
		(void *)&mqtt_noresponse_command_pingreq_pingresp,
		1,                                      // for 1 sec
	},
	{
		10,                                     // then after a delay of 10 sec
		&mqtt_command_disconnect,               // send DISCONNECT
		(void *)0,
		&mqtt_response_disconnect,              // wait for nothing
		(void *)0,
		(void *)&mqtt_noresponse_command_disconnect_passed,
		1,                                      // for 1 sec
	},
};

//--------------------------------------------
size_t test_mqtt_list1_get_size(void)
{
	return (sizeof(test_list) / sizeof(test_t));
}

//--------------------------------------------
test_t *test_mqtt_list1_get_test(size_t number)
{
	return &test_list[number];
}
