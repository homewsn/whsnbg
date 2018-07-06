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
static mqtt_connect_header_t header_connect =
{
	/*.flags =*/ {
		/*.user_name =*/ 0,
		/*.password =*/ 0,
		/*.will_retain =*/ 0,
		/*.will_qos =*/ 0,
		/* will =*/ 0,
		/*.clean_session =*/ 0,
	},
	/*.keep_alive_timer =*/ 60,
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
static mqtt_connack_return_code_t return_code_connack = MQTT_ACCEPTED;


//--------------------------------------------
// SUBSCRIBE
//--------------------------------------------
static list_sub_t list_subscribe_devices5738 =
{
	/*.next =*/ 0,
	/*.name =*/ "devices/5738",
	/*.name_len =*/ 12,
	/*.qos =*/ 1,
};
static mqtt_subscribe_header_t subscribe_header_m2 =
{
	/*.msg_id =*/ 2,
	/*.sub_list =*/ 0,
	/*.sub_item =*/ &list_subscribe_devices5738,
	/*.qos_size =*/ 1,
};


//--------------------------------------------
// SUBACK
//--------------------------------------------
static uint8_t suback_header_qos_buf[] =
{
	1,
};
static mqtt_suback_header_t suback_header_m2 =
{
	/*.msg_id =*/ 2,
	/*.qos_buf =*/ suback_header_qos_buf,
	/*.qos_size =*/ 1,
};


//--------------------------------------------
// PUBLISH
//--------------------------------------------
static list_pub_t list_publish_devices5738sleep_0 =
{
	/*.next =*/ 0,
	/*.name =*/ "devices/5738/sleep",
	/*.name_len =*/ 18,
	/*.data =*/ "0",
	/*.data_len =*/ 1,
	/*.retain =*/ 1,
};
static mqtt_publish_header_t publish_header_m1 =
{
	/*.msg_id =*/ 1,
	/*.qos =*/ 2,
	/*.pub_list =*/ 0,
	/*.pub_item =*/ &list_publish_devices5738sleep_0,
};

static list_pub_t list_publish_devices5738sleep_1 =
{
	/*.next =*/ 0,
	/*.name =*/ "devices/5738/sleep",
	/*.name_len =*/ 18,
	/*.data =*/ "1",
	/*.data_len =*/ 1,
	/*.retain =*/ 1,
};
static mqtt_publish_header_t publish_header_m3 =
{
	/*.msg_id =*/ 3,
	/*.qos =*/ 1,
	/*.pub_list =*/ 0,
	/*.pub_item =*/ &list_publish_devices5738sleep_1,
};

static list_pub_t list_publish_devices5738_online =
{
	/*.next =*/ 0,
	/*.name =*/ "devices/5738",
	/*.name_len =*/ 12,
	/*.data =*/ "online",
	/*.data_len =*/ 6,
	/*.retain =*/ 1,
};
static mqtt_publish_header_t publish_header_online_m1 =
{
	/*.msg_id =*/ 1,
	/*.qos =*/ 1,
	/*.pub_list =*/ 0,
	/*.pub_item =*/ &list_publish_devices5738_online,
};

static list_pub_t list_publish_devices5738_offline =
{
	/*.next =*/ 0,
	/*.name =*/ "devices/5738",
	/*.name_len =*/ 12,
	/*.data =*/ "offline",
	/*.data_len =*/ 7,
	/*.retain =*/ 1,
};
static mqtt_publish_header_t publish_header_offline_m2 =
{
	/*.msg_id =*/ 2,
	/*.qos =*/ 1,
	/*.pub_list =*/ 0,
	/*.pub_item =*/ &list_publish_devices5738_offline,
};


//--------------------------------------------
// PUBACK
//--------------------------------------------
static mqtt_puback_header_t puback_header_m1 =
{
	/*.msg_id =*/ 1,
};

static mqtt_puback_header_t puback_header_m2 =
{
	/*.msg_id =*/ 2,
};

static mqtt_puback_header_t puback_header_m3 =
{
	/*.msg_id =*/ 3,
};


//--------------------------------------------
// PUBREC
//--------------------------------------------
static mqtt_pubrec_header_t pubrec_header_m1 =
{
	/*.msg_id =*/ 1,
};


//--------------------------------------------
// PUBREL
//--------------------------------------------
static mqtt_pubrel_header_t pubrel_header_m1 =
{
	/*.msg_id =*/ 1,
};


//--------------------------------------------
// PUBCOMP
//--------------------------------------------
static mqtt_pubcomp_header_t pubcomp_header_m1 =
{
	/*.msg_id =*/ 1,
};



//--------------------------------------------
// *** Test schedule ***
//--------------------------------------------
static test_t test_list[] =
{
	{
		5,                                      // after a delay of 5 sec
		&mqtt_command_connect,                  // send CONNECT with
		(void *)&header_connect,                // keep_alive_timer=60, client_id="mqtt_test"
		&mqtt_response_connack,                 // wait for CONNACK with
		(void *)&return_code_connack,           // MQTT_ACCEPTED
		(void *)&mqtt_noresponse_command_connect_connack,
		1,                                      // for 1 sec
	},
	{
		0,                                      // then
		&mqtt_command_publish,                  // send PUBLISH with
		(void *)&publish_header_m1,             // msg_id=1, qos=2, retain=1, topic_name="devices/5738/sleep", topic_data="0"
		&mqtt_response_pubrec,                  // wait for PUBREC with
		(void *)&pubrec_header_m1,              // msg_id=1
		(void *)&mqtt_noresponse_command_publish_pubrec,
		1,                                      // for 1 sec
	},
	{
		0,                                      // then
		&mqtt_command_pubrel,                   // send PUBREL with
		(void *)&pubrel_header_m1,              // msg_id=1
		&mqtt_response_pubcomp,                 // wait for PUBCOMP with
		(void *)&pubcomp_header_m1,             // msg_id=1
		(void *)&mqtt_noresponse_command_pubrel_pubcomp,
		1,                                      // for 1 sec
	},
	{
		0,                                      // then
		&mqtt_command_subscribe,                // send SUBSCRIBE with
		(void *)&subscribe_header_m2,           // msg_id=2, topic_name="devices/5738"
		&mqtt_response_suback,                  // wait for SUBACK with
		(void *)&suback_header_m2,              // msg_id=2
		(void *)&mqtt_noresponse_command_subscribe_suback,
		1,                                      // for 1 sec
	},
	{
		0,                                      // then
		(void *)0,                              // send nothing
		(void *)0,
		&mqtt_response_publish,                 // wait for PUBLISH with
		(void *)&publish_header_online_m1,      // msg_id=1, qos=1, retain=1, topic_name="devices/5738", topic_data="online"
		(void *)&mqtt_noresponse_nothing_publish,
		10,                                     // for 10 sec
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
		9,                                      // then after a delay of 9 sec
		&mqtt_command_publish,                  // send PUBLISH with
		(void *)&publish_header_m3,             // msg_id=3, qos=1, retain=1, topic_name="devices/5738/sleep", topic_data="1"
		&mqtt_response_puback,                  // wait for PUBACK with
		(void *)&puback_header_m3,              // msg_id=3
		(void *)&mqtt_noresponse_command_publish_puback,
		1,                                      // for 1 sec
	},
	{
		0,                                      // then
		(void *)0,                              // send nothing
		(void *)0,
		&mqtt_response_publish,                 // wait for PUBLISH with
		(void *)&publish_header_offline_m2,     // msg_id=2, qos=1, retain=1, topic_name="devices/5738", topic_data="offline"
		(void *)&mqtt_noresponse_nothing_publish,
		30,                                     // for 30 sec
	},
	{
		0,                                      // then
		&mqtt_command_puback,                   // send PUBACK
		(void *)&puback_header_m2,              // with msg_id=2
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
};

//--------------------------------------------
size_t test_mqtt_list2_get_size(void)
{
	return (sizeof(test_list) / sizeof(test_t));
}

//--------------------------------------------
test_t *test_mqtt_list2_get_test(size_t number)
{
	return &test_list[number];
}
