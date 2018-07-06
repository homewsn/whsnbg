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
static mqttsn_connect_header_t header_connect =
{
	/*.flags =*/ {
		/*.dup =*/ 0,
		/*.qos =*/ 0,
		/*.retain =*/ 0,
		/*.will =*/ 1,
		/*.clean_session =*/ 1,
		/*.topic_id_type =*/ 0,
		},
	/*.protocol_id =*/ 1,
	/*.duration =*/ 10,
	/*.client_id_length =*/ 4,
	/*.client_id =*/ "5738",
};


//--------------------------------------------
// WILLTOPIC
// Flags not used: dup, will, clean_session, topic_id_type
//--------------------------------------------
static mqttsn_willtopic_header_t header_willtopic =
{
	/*.flags =*/ {
		/*.dup =*/ 0,
		/*.qos =*/ 1,
		/*.retain =*/ 1,
		/*.will =*/ 0,
		/*.clean_session =*/ 0,
		/*.topic_id_type =*/ 0,
	},
	/*.will_topic_length =*/ 12,
	/*.will_topic =*/ "devices/5738",
};


//--------------------------------------------
// WILLMSG
//--------------------------------------------
static mqttsn_willmsg_header_t header_willmsg =
{
	/*.will_msg_length =*/ 7,
	/*.will_msg =*/ "offline",
};


//--------------------------------------------
// CONNACK
//--------------------------------------------
static mqttsn_return_code_t return_code_connack = MQTTSN_ACCEPTED;


//--------------------------------------------
// REGISTER
//--------------------------------------------
static mqttsn_register_header_t header_register_t0_m1 =
{
	/*.topic_id =*/ 0,
	/*.msg_id =*/ 1,
	/*.name_length =*/ 12,
	/*.name =*/ "devices/5738",
};

static mqttsn_register_header_t header_register_t0_m2 =
{
	/*.topic_id =*/ 0,
	/*.msg_id =*/ 2,
	/*.name_length =*/ 15,
	/*.name =*/ "devices/5738/ip",
};

static mqttsn_register_header_t header_register_t0_m3 =
{
	/*.topic_id =*/ 0,
	/*.msg_id =*/ 3,
	/*.name_length =*/ 20,
	/*.name =*/ "devices/5738/timeout",
};

static mqttsn_register_header_t header_register_t0_m4 =
{
	/*.topic_id =*/ 0,
	/*.msg_id =*/ 4,
	/*.name_length =*/ 12,
	/*.name =*/ "sensors/5738",
};

static mqttsn_register_header_t header_register_t0_m5 =
{
	/*.topic_id =*/ 0,
	/*.msg_id =*/ 5,
	/*.name_length =*/ 14,
	/*.name =*/ "sensors/5738/0",
};

static mqttsn_register_header_t header_register_t0_m6 =
{
	/*.topic_id =*/ 0,
	/*.msg_id =*/ 6,
	/*.name_length =*/ 14,
	/*.name =*/ "sensors/5738/1",
};

static mqttsn_register_header_t header_register_t0_m7 =
{
	/*.topic_id =*/ 0,
	/*.msg_id =*/ 7,
	/*.name_length =*/ 19,
	/*.name =*/ "sensors/5738/0/type",
};

static mqttsn_register_header_t header_register_t0_m8 =
{
	/*.topic_id =*/ 0,
	/*.msg_id =*/ 8,
	/*.name_length =*/ 19,
	/*.name =*/ "sensors/5738/1/type",
};

static mqttsn_register_header_t header_register_t0_m9 =
{
	/*.topic_id =*/ 0,
	/*.msg_id =*/ 9,
	/*.name_length =*/ 19,
	/*.name =*/ "sensors/5738/0/unit",
};

static mqttsn_register_header_t header_register_t0_m10 =
{
	/*.topic_id =*/ 0,
	/*.msg_id =*/ 10,
	/*.name_length =*/ 19,
	/*.name =*/ "sensors/5738/1/unit",
};

static mqttsn_register_header_t header_register_t0_m11 =
{
	/*.topic_id =*/ 0,
	/*.msg_id =*/ 11,
	/*.name_length =*/ 14,
	/*.name =*/ "actuators/5738",
};

static mqttsn_register_header_t header_register_t0_m12 =
{
	/*.topic_id =*/ 0,
	/*.msg_id =*/ 12,
	/*.name_length =*/ 16,
	/*.name =*/ "actuators/5738/2",
};

static mqttsn_register_header_t header_register_t0_m13 =
{
	/*.topic_id =*/ 0,
	/*.msg_id =*/ 13,
	/*.name_length =*/ 21,
	/*.name =*/ "actuators/5738/2/type",
};

static mqttsn_register_header_t header_register_t0_m14 =
{
	/*.topic_id =*/ 0,
	/*.msg_id =*/ 14,
	/*.name_length =*/ 21,
	/*.name =*/ "actuators/5738/2/unit",
};

static mqttsn_register_header_t header_register_t0_m15 =
{
	/*.topic_id =*/ 0,
	/*.msg_id =*/ 15,
	/*.name_length =*/ 24,
	/*.name =*/ "actuators/5738/2/command",
};


//--------------------------------------------
// REGACK
//--------------------------------------------
static mqttsn_regack_header_t header_regack_t1_m1 =
{
	/*.topic_id =*/ 1,
	/*.msg_id =*/ 1,
	/*.return_code = */ MQTTSN_ACCEPTED,
};

static mqttsn_regack_header_t header_regack_t2_m2 =
{
	/*.topic_id =*/ 2,
	/*.msg_id =*/ 2,
	/*.return_code = */ MQTTSN_ACCEPTED,
};

static mqttsn_regack_header_t header_regack_t3_m3 =
{
	/*.topic_id =*/ 3,
	/*.msg_id =*/ 3,
	/*.return_code = */ MQTTSN_ACCEPTED,
};

static mqttsn_regack_header_t header_regack_t4_m4 =
{
	/*.topic_id =*/ 4,
	/*.msg_id =*/ 4,
	/*.return_code = */ MQTTSN_ACCEPTED,
};

static mqttsn_regack_header_t header_regack_t5_m5 =
{
	/*.topic_id =*/ 5,
	/*.msg_id =*/ 5,
	/*.return_code = */ MQTTSN_ACCEPTED,
};

static mqttsn_regack_header_t header_regack_t6_m6 =
{
	/*.topic_id =*/ 6,
	/*.msg_id =*/ 6,
	/*.return_code = */ MQTTSN_ACCEPTED,
};

static mqttsn_regack_header_t header_regack_t7_m7 =
{
	/*.topic_id =*/ 7,
	/*.msg_id =*/ 7,
	/*.return_code = */ MQTTSN_ACCEPTED,
};

static mqttsn_regack_header_t header_regack_t8_m8 =
{
	/*.topic_id =*/ 8,
	/*.msg_id =*/ 8,
	/*.return_code = */ MQTTSN_ACCEPTED,
};

static mqttsn_regack_header_t header_regack_t9_m9 =
{
	/*.topic_id =*/ 9,
	/*.msg_id =*/ 9,
	/*.return_code = */ MQTTSN_ACCEPTED,
};

static mqttsn_regack_header_t header_regack_t10_m10 =
{
	/*.topic_id =*/ 10,
	/*.msg_id =*/ 10,
	/*.return_code = */ MQTTSN_ACCEPTED,
};

static mqttsn_regack_header_t header_regack_t11_m11 =
{
	/*.topic_id =*/ 11,
	/*.msg_id =*/ 11,
	/*.return_code = */ MQTTSN_ACCEPTED,
};

static mqttsn_regack_header_t header_regack_t12_m12 =
{
	/*.topic_id =*/ 12,
	/*.msg_id =*/ 12,
	/*.return_code = */ MQTTSN_ACCEPTED,
};

static mqttsn_regack_header_t header_regack_t13_m13 =
{
	/*.topic_id =*/ 13,
	/*.msg_id =*/ 13,
	/*.return_code = */ MQTTSN_ACCEPTED,
};

static mqttsn_regack_header_t header_regack_t14_m14 =
{
	/*.topic_id =*/ 14,
	/*.msg_id =*/ 14,
	/*.return_code = */ MQTTSN_ACCEPTED,
};

static mqttsn_regack_header_t header_regack_t15_m15 =
{
	/*.topic_id =*/ 15,
	/*.msg_id =*/ 15,
	/*.return_code = */ MQTTSN_ACCEPTED,
};


//--------------------------------------------
// PUBLISH
// Flags not used: will, clean_session
//--------------------------------------------
static mqttsn_publish_header_t header_publish_t1_m16 =
{
	/*.flags =*/ {
		/*.dup =*/ 0,
		/*.qos =*/ 1,
		/*.retain =*/ 1,
		/*.will =*/ 0,
		/*.clean_session =*/ 0,
		/*.topic_id_type =*/ 0,
	},
	/*.topic_id =*/ 1,
	/*.msg_id =*/ 16,
	/*.data_length =*/ 6,
	/*.data =*/ "online",
};

static mqttsn_publish_header_t header_publish_t2_m17 =
{
	/*.flags =*/ {
		/*.dup =*/ 0,
		/*.qos =*/ 1,
		/*.retain =*/ 1,
		/*.will =*/ 0,
		/*.clean_session =*/ 0,
		/*.topic_id_type =*/ 0,
	},
	/*.topic_id =*/ 2,
	/*.msg_id =*/ 17,
	/*.data_length =*/ 13,
	/*.data =*/ "172.16.106.22",
};

static mqttsn_publish_header_t header_publish_t7_m18 =
{
	/*.flags =*/ {
		/*.dup =*/ 0,
		/*.qos =*/ 1,
		/*.retain =*/ 1,
		/*.will =*/ 0,
		/*.clean_session =*/ 0,
		/*.topic_id_type =*/ 0,
	},
	/*.topic_id =*/ 7,
	/*.msg_id =*/ 18,
	/*.data_length =*/ 5,
	/*.data =*/ "float",
};

static mqttsn_publish_header_t header_publish_t8_m19 =
{
	/*.flags =*/ {
		/*.dup =*/ 0,
		/*.qos =*/ 1,
		/*.retain =*/ 1,
		/*.will =*/ 0,
		/*.clean_session =*/ 0,
		/*.topic_id_type =*/ 0,
	},
	/*.topic_id =*/ 8,
	/*.msg_id =*/ 19,
	/*.data_length =*/ 4,
	/*.data =*/ "long",
};

static mqttsn_publish_header_t header_publish_t9_m20 =
{
	/*.flags =*/ {
		/*.dup =*/ 0,
		/*.qos =*/ 1,
		/*.retain =*/ 1,
		/*.will =*/ 0,
		/*.clean_session =*/ 0,
		/*.topic_id_type =*/ 0,
	},
	/*.topic_id =*/ 9,
	/*.msg_id =*/ 20,
	/*.data_length =*/ 1,
	/*.data =*/ "V",
};

static mqttsn_publish_header_t header_publish_t10_m21 =
{
	/*.flags =*/ {
		/*.dup =*/ 0,
		/*.qos =*/ 1,
		/*.retain =*/ 1,
		/*.will =*/ 0,
		/*.clean_session =*/ 0,
		/*.topic_id_type =*/ 0,
	},
	/*.topic_id =*/ 10,
	/*.msg_id =*/ 21,
	/*.data_length =*/ 12,
	/*.data =*/ "on(1)-off(0)",
};

static mqttsn_publish_header_t header_publish_t13_m22 =
{
	/*.flags =*/ {
		/*.dup =*/ 0,
		/*.qos =*/ 1,
		/*.retain =*/ 1,
		/*.will =*/ 0,
		/*.clean_session =*/ 0,
		/*.topic_id_type =*/ 0,
	},
	/*.topic_id =*/ 13,
	/*.msg_id =*/ 22,
	/*.data_length =*/ 4,
	/*.data =*/ "long",
};

static mqttsn_publish_header_t header_publish_t14_m23 =
{
	/*.flags =*/ {
		/*.dup =*/ 0,
		/*.qos =*/ 1,
		/*.retain =*/ 1,
		/*.will =*/ 0,
		/*.clean_session =*/ 0,
		/*.topic_id_type =*/ 0,
	},
	/*.topic_id =*/ 14,
	/*.msg_id =*/ 23,
	/*.data_length =*/ 12,
	/*.data =*/ "on(1)-off(0)",
};

static mqttsn_publish_header_t header_publish_t5_m26 =
{
	/*.flags =*/ {
		/*.dup =*/ 0,
		/*.qos =*/ 1,
		/*.retain =*/ 1,
		/*.will =*/ 0,
		/*.clean_session =*/ 0,
		/*.topic_id_type =*/ 0,
	},
	/*.topic_id =*/ 5,
	/*.msg_id =*/ 26,
	/*.data_length =*/ 4,
	/*.data =*/ "3.32",
};

static mqttsn_publish_header_t header_publish_t6_m27 =
{
	/*.flags =*/ {
		/*.dup =*/ 0,
		/*.qos =*/ 1,
		/*.retain =*/ 1,
		/*.will =*/ 0,
		/*.clean_session =*/ 0,
		/*.topic_id_type =*/ 0,
	},
	/*.topic_id =*/ 6,
	/*.msg_id =*/ 27,
	/*.data_length =*/ 1,
	/*.data =*/ "0",
};

static mqttsn_publish_header_t header_publish_t12_m28 =
{
	/*.flags =*/ {
		/*.dup =*/ 0,
		/*.qos =*/ 1,
		/*.retain =*/ 1,
		/*.will =*/ 0,
		/*.clean_session =*/ 0,
		/*.topic_id_type =*/ 0,
	},
	/*.topic_id =*/ 12,
	/*.msg_id =*/ 28,
	/*.data_length =*/ 1,
	/*.data =*/ "0",
};

static mqttsn_publish_header_t header_publish_t5_m29 =
{
	/*.flags =*/ {
		/*.dup =*/ 0,
		/*.qos =*/ 1,
		/*.retain =*/ 1,
		/*.will =*/ 0,
		/*.clean_session =*/ 0,
		/*.topic_id_type =*/ 0,
	},
	/*.topic_id =*/ 5,
	/*.msg_id =*/ 29,
	/*.data_length =*/ 5,
	/*.data =*/ "3.367",
};

static mqttsn_publish_header_t header_publish_t6_m30 =
{
	/*.flags =*/ {
		/*.dup =*/ 0,
		/*.qos =*/ 1,
		/*.retain =*/ 1,
		/*.will =*/ 0,
		/*.clean_session =*/ 0,
		/*.topic_id_type =*/ 0,
	},
	/*.topic_id =*/ 6,
	/*.msg_id =*/ 30,
	/*.data_length =*/ 1,
	/*.data =*/ "1",
};

static mqttsn_publish_header_t header_publish_t12_m31 =
{
	/*.flags =*/ {
		/*.dup =*/ 0,
		/*.qos =*/ 1,
		/*.retain =*/ 1,
		/*.will =*/ 0,
		/*.clean_session =*/ 0,
		/*.topic_id_type =*/ 0,
	},
	/*.topic_id =*/ 12,
	/*.msg_id =*/ 31,
	/*.data_length =*/ 1,
	/*.data =*/ "1",
};

static mqttsn_publish_header_t header_publish_t3_m1 =
{
	/*.flags =*/ {
		/*.dup =*/ 0,
		/*.qos =*/ 1,
		/*.retain =*/ 1,
		/*.will =*/ 0,
		/*.clean_session =*/ 0,
		/*.topic_id_type =*/ 0,
	},
	/*.topic_id =*/ 3,
	/*.msg_id =*/ 1,
	/*.data_length =*/ 2,
	/*.data =*/ "20",
};

static mqttsn_publish_header_t header_publish_t15_m2 =
{
	/*.flags =*/ {
		/*.dup =*/ 0,
		/*.qos =*/ 1,
		/*.retain =*/ 1,
		/*.will =*/ 0,
		/*.clean_session =*/ 0,
		/*.topic_id_type =*/ 0,
	},
	/*.topic_id =*/ 15,
	/*.msg_id =*/ 2,
	/*.data_length =*/ 1,
	/*.data =*/ "1",
};


//--------------------------------------------
// PUBACK
//--------------------------------------------
static mqttsn_puback_header_t header_puback_t1_m16 =
{
	/*.topic_id =*/ 1,
	/*.msg_id =*/ 16,
	/*.return_code = */ MQTTSN_ACCEPTED,
};

static mqttsn_puback_header_t header_puback_t2_m17 =
{
	/*.topic_id =*/ 2,
	/*.msg_id =*/ 17,
	/*.return_code = */ MQTTSN_ACCEPTED,
};

static mqttsn_puback_header_t header_puback_t7_m18 =
{
	/*.topic_id =*/ 7,
	/*.msg_id =*/ 18,
	/*.return_code = */ MQTTSN_ACCEPTED,
};

static mqttsn_puback_header_t header_puback_t8_m19 =
{
	/*.topic_id =*/ 8,
	/*.msg_id =*/ 19,
	/*.return_code = */ MQTTSN_ACCEPTED,
};

static mqttsn_puback_header_t header_puback_t9_m20 =
{
	/*.topic_id =*/ 9,
	/*.msg_id =*/ 20,
	/*.return_code = */ MQTTSN_ACCEPTED,
};

static mqttsn_puback_header_t header_puback_t10_m21 =
{
	/*.topic_id =*/ 10,
	/*.msg_id =*/ 21,
	/*.return_code = */ MQTTSN_ACCEPTED,
};

static mqttsn_puback_header_t header_puback_t13_m22 =
{
	/*.topic_id =*/ 13,
	/*.msg_id =*/ 22,
	/*.return_code = */ MQTTSN_ACCEPTED,
};

static mqttsn_puback_header_t header_puback_t14_m23 =
{
	/*.topic_id =*/ 14,
	/*.msg_id =*/ 23,
	/*.return_code = */ MQTTSN_ACCEPTED,
};

static mqttsn_puback_header_t header_puback_t5_m26 =
{
	/*.topic_id =*/ 5,
	/*.msg_id =*/ 26,
	/*.return_code = */ MQTTSN_ACCEPTED,
};

static mqttsn_puback_header_t header_puback_t6_m27 =
{
	/*.topic_id =*/ 6,
	/*.msg_id =*/ 27,
	/*.return_code = */ MQTTSN_ACCEPTED,
};

static mqttsn_puback_header_t header_puback_t12_m28 =
{
	/*.topic_id =*/ 12,
	/*.msg_id =*/ 28,
	/*.return_code = */ MQTTSN_ACCEPTED,
};

static mqttsn_puback_header_t header_puback_t3_m1 =
{
	/*.topic_id =*/ 3,
	/*.msg_id =*/ 1,
	/*.return_code = */ MQTTSN_ACCEPTED,
};

static mqttsn_puback_header_t header_puback_t15_m2 =
{
	/*.topic_id =*/ 15,
	/*.msg_id =*/ 2,
	/*.return_code = */ MQTTSN_ACCEPTED,
};


//--------------------------------------------
// SUBSCRIBE
// Flags not used: retain, will, clean_session
//--------------------------------------------
static mqttsn_subscribe_header_t header_subscribe_t3_m24 =
{
	/*.flags =*/ {
		/*.dup =*/ 0,
		/*.qos =*/ 1,
		/*.retain =*/ 0,
		/*.will =*/ 0,
		/*.clean_session =*/ 0,
		/*.topic_id_type =*/ 1,
	},
	/*.msg_id =*/ 24,
	/*.topic_id =*/ 3,
};

static mqttsn_subscribe_header_t header_subscribe_t15_m25 =
{
	/*.flags =*/ {
		/*.dup =*/ 0,
		/*.qos =*/ 1,
		/*.retain =*/ 0,
		/*.will =*/ 0,
		/*.clean_session =*/ 0,
		/*.topic_id_type =*/ 1,
	},
	/*.msg_id =*/ 25,
	/*.topic_id =*/ 15,
};


//--------------------------------------------
// SUBACK
// Flags not used: dup, retain, will, clean_session, topic_id_type
// qos - contains the granted QoS level
//--------------------------------------------
static mqttsn_suback_header_t header_suback_t3_m24 =
{
	/*.flags =*/ {
		/*.dup =*/ 0,
		/*.qos =*/ 1,
		/*.retain =*/ 0,
		/*.will =*/ 0,
		/*.clean_session =*/ 0,
		/*.topic_id_type =*/ 0,
	},
	/*.topic_id =*/ 3,
	/*.msg_id =*/ 24,
	/*.return_code = */ MQTTSN_ACCEPTED,
};

static mqttsn_suback_header_t header_suback_t15_m25 =
{
	/*.flags =*/ {
		/*.dup =*/ 0,
		/*.qos =*/ 1,
		/*.retain =*/ 0,
		/*.will =*/ 0,
		/*.clean_session =*/ 0,
		/*.topic_id_type =*/ 0,
	},
	/*.topic_id =*/ 15,
	/*.msg_id =*/ 25,
	/*.return_code = */ MQTTSN_ACCEPTED,
};





//--------------------------------------------
// *** Test schedule ***
//--------------------------------------------
static test_t test_list[] =
{
	{
		0,                                      // immediately
		&mqttsn_command_connect,                // send CONNECT with
		(void *)&header_connect,                // will=1, clean_session=1, duration=10, client_id="5738"
		&mqttsn_response_willtopicreq,          // wait for WILLTOPICREQ
		(void *)0,
		(void *)&mqttsn_noresponse_command_connect_willtopicreq,
		1,                                      // for 1 sec
	},
	{
		0,                                      // then
		&mqttsn_command_willtopic,              // send WILLTOPIC with
		(void *)&header_willtopic,              // qos=1, retain=1, will_topic="devices/5318"
		&mqttsn_response_willmsgreq,            // wait for WILLMSGREQ
		(void *)0,
		(void *)&mqttsn_noresponse_command_connect_willmsgreq,
		1,                                      // for 1 sec
	},
	{
		0,                                      // then
		&mqttsn_command_willmsg,                // send WILLMSG with
		(void *)&header_willmsg,                // will_msg="offline"
		&mqttsn_response_connack,               // wait for CONNACK with
		(void *)&return_code_connack,           // MQTTSN_ACCEPTED
		(void *)&mqttsn_noresponse_command_connect_connack,
		1,                                      // for 1 sec
	},
	{
		0,                                      // then
		&mqttsn_command_register,               // send REGISTER with
		(void *)&header_register_t0_m1,         // topic_id=0, msg_id=1(auto), name_length=12(auto), name="devices/5738"
		&mqttsn_response_regack,                // wait for REGACK with
		(void *)&header_regack_t1_m1,           // topic_id=1, msg_id=1(auto), return_code=MQTTSN_ACCEPTED
		(void *)&mqttsn_noresponse_command_register_regack,
		1,                                      // for 1 sec
	},
	{
		0,                                      // then
		&mqttsn_command_register,               // send REGISTER with
		(void *)&header_register_t0_m2,         // topic_id=0, msg_id=2(auto), name_length=15(auto), name="devices/5738/ip"
		&mqttsn_response_regack,                // wait for REGACK with
		(void *)&header_regack_t2_m2,           // topic_id=2, msg_id=2(auto), return_code=MQTTSN_ACCEPTED
		(void *)&mqttsn_noresponse_command_register_regack,
		1,                                      // for 1 sec
	},
	{
		0,                                      // then
		&mqttsn_command_register,               // send REGISTER with
		(void *)&header_register_t0_m3,         // topic_id=0, msg_id=3(auto), name_length=20(auto), name="devices/5738/timeout"
		&mqttsn_response_regack,                // wait for REGACK with
		(void *)&header_regack_t3_m3,           // topic_id=3, msg_id=3(auto), return_code=MQTTSN_ACCEPTED
		(void *)&mqttsn_noresponse_command_register_regack,
		1,                                      // for 1 sec
	},
	{
		0,                                      // then
		&mqttsn_command_register,               // send REGISTER with
		(void *)&header_register_t0_m4,         // topic_id=0, msg_id=4(auto), name_length=12(auto), name="sensors/5738"
		&mqttsn_response_regack,                // wait for REGACK with
		(void *)&header_regack_t4_m4,           // topic_id=4, msg_id=4(auto), return_code=MQTTSN_ACCEPTED
		(void *)&mqttsn_noresponse_command_register_regack,
		1,                                      // for 1 sec
	},
	{
		0,                                      // then
		&mqttsn_command_register,               // send REGISTER with
		(void *)&header_register_t0_m5,         // topic_id=0, msg_id=5(auto), name_length=14(auto), name="sensors/5738/0"
		&mqttsn_response_regack,                // wait for REGACK with
		(void *)&header_regack_t5_m5,           // topic_id=5, msg_id=5(auto), return_code=MQTTSN_ACCEPTED
		(void *)&mqttsn_noresponse_command_register_regack,
		1,                                      // for 1 sec
	},
	{
		0,                                      // then
		&mqttsn_command_register,               // send REGISTER with
		(void *)&header_register_t0_m6,         // topic_id=0, msg_id=6(auto), name_length=14(auto), name="sensors/5738/1"
		&mqttsn_response_regack,                // wait for REGACK with
		(void *)&header_regack_t6_m6,           // topic_id=6, msg_id=6(auto), return_code=MQTTSN_ACCEPTED
		(void *)&mqttsn_noresponse_command_register_regack,
		1,                                      // for 1 sec
	},
	{
		0,                                      // then
		&mqttsn_command_register,               // send REGISTER with
		(void *)&header_register_t0_m7,         // topic_id=0, msg_id=7(auto), name_length=19(auto), name="sensors/5738/0/type"
		&mqttsn_response_regack,                // wait for REGACK with
		(void *)&header_regack_t7_m7,           // topic_id=7, msg_id=7(auto), return_code=MQTTSN_ACCEPTED
		(void *)&mqttsn_noresponse_command_register_regack,
		1,                                      // for 1 sec
	},
	{
		0,                                      // then
		&mqttsn_command_register,               // send REGISTER with
		(void *)&header_register_t0_m8,         // topic_id=0, msg_id=8(auto), name_length=19(auto), name="sensors/5738/1/type"
		&mqttsn_response_regack,                // wait for REGACK with
		(void *)&header_regack_t8_m8,           // topic_id=8, msg_id=8(auto), return_code=MQTTSN_ACCEPTED
		(void *)&mqttsn_noresponse_command_register_regack,
		1,                                      // for 1 sec
	},
	{
		0,                                      // then
		&mqttsn_command_register,               // send REGISTER with
		(void *)&header_register_t0_m9,         // topic_id=0, msg_id=9(auto), name_length=19(auto), name="sensors/5738/0/unit"
		&mqttsn_response_regack,                // wait for REGACK with
		(void *)&header_regack_t9_m9,           // topic_id=9, msg_id=9(auto), return_code=MQTTSN_ACCEPTED
		(void *)&mqttsn_noresponse_command_register_regack,
		1,                                      // for 1 sec
	},
	{
		0,                                      // then
		&mqttsn_command_register,               // send REGISTER with
		(void *)&header_register_t0_m10,        // topic_id=0, msg_id=10(auto), name_length=19(auto), name="sensors/5738/1/unit"
		&mqttsn_response_regack,                // wait for REGACK with
		(void *)&header_regack_t10_m10,         // topic_id=10, msg_id=10(auto), return_code=MQTTSN_ACCEPTED
		(void *)&mqttsn_noresponse_command_register_regack,
		1,                                      // for 1 sec
	},
	{
		0,                                      // then
		&mqttsn_command_register,               // send REGISTER with
		(void *)&header_register_t0_m11,        // topic_id=0, msg_id=11(auto), name_length=14(auto), name="actuators/5738"
		&mqttsn_response_regack,                // wait for REGACK with
		(void *)&header_regack_t11_m11,         // topic_id=11, msg_id=11(auto), return_code=MQTTSN_ACCEPTED
		(void *)&mqttsn_noresponse_command_register_regack,
		1,                                      // for 1 sec
	},
	{
		0,                                      // then
		&mqttsn_command_register,               // send REGISTER with
		(void *)&header_register_t0_m12,        // topic_id=0, msg_id=12(auto), name_length=16(auto), name="actuators/5738/2"
		&mqttsn_response_regack,                // wait for REGACK with
		(void *)&header_regack_t12_m12,         // topic_id=12, msg_id=12(auto), return_code=MQTTSN_ACCEPTED
		(void *)&mqttsn_noresponse_command_register_regack,
		1,                                      // for 1 sec
	},
	{
		0,                                      // then
		&mqttsn_command_register,               // send REGISTER with
		(void *)&header_register_t0_m13,        // topic_id=0, msg_id=13(auto), name_length=21(auto), name="actuators/5738/2/type"
		&mqttsn_response_regack,                // wait for REGACK with
		(void *)&header_regack_t13_m13,         // topic_id=13, msg_id=13(auto), return_code=MQTTSN_ACCEPTED
		(void *)&mqttsn_noresponse_command_register_regack,
		1,                                      // for 1 sec
	},
	{
		0,                                      // then
		&mqttsn_command_register,               // send REGISTER with
		(void *)&header_register_t0_m14,        // topic_id=0, msg_id=14(auto), name_length=21(auto), name="actuators/5738/2/unit"
		&mqttsn_response_regack,                // wait for REGACK with
		(void *)&header_regack_t14_m14,         // topic_id=14, msg_id=14(auto), return_code=MQTTSN_ACCEPTED
		(void *)&mqttsn_noresponse_command_register_regack,
		1,                                      // for 1 sec
	},
	{
		0,                                      // then
		&mqttsn_command_register,               // send REGISTER with
		(void *)&header_register_t0_m15,        // topic_id=0, msg_id=15(auto), name_length=24(auto), name="actuators/5738/2/command"
		&mqttsn_response_regack,                // wait for REGACK with
		(void *)&header_regack_t15_m15,         // topic_id=15, msg_id=15(auto), return_code=MQTTSN_ACCEPTED
		(void *)&mqttsn_noresponse_command_register_regack,
		1,                                      // for 1 sec
	},
	{
		0,                                      // then
		&mqttsn_command_publish,                // send PUBLISH with
		(void *)&header_publish_t1_m16,         // qos=1, retain=1, topic_id=1, msg_id=16(auto), data_length=6(auto), data="online"
		&mqttsn_response_puback,                // wait for PUBACK with
		(void *)&header_puback_t1_m16,          // topic_id=1, msg_id=16(auto), return_code=MQTTSN_ACCEPTED
		(void *)&mqttsn_noresponse_command_publish_puback,
		1,                                      // for 1 sec
	},
	{
		0,                                      // then
		&mqttsn_command_publish,                // send PUBLISH with
		(void *)&header_publish_t2_m17,         // qos=1, retain=1, topic_id=2, msg_id=17(auto), data_length=13(auto), data="172.16.106.22"
		&mqttsn_response_puback,                // wait for PUBACK with
		(void *)&header_puback_t2_m17,          // topic_id=2, msg_id=17(auto), return_code=MQTTSN_ACCEPTED
		(void *)&mqttsn_noresponse_command_publish_puback,
		1,                                      // for 1 sec
	},
	{
		0,                                      // then
		&mqttsn_command_publish,                // send PUBLISH with
		(void *)&header_publish_t7_m18,         // qos=1, retain=1, topic_id=7, msg_id=18(auto), data_length=5(auto), data="float"
		&mqttsn_response_puback,                // wait for PUBACK with
		(void *)&header_puback_t7_m18,          // topic_id=7, msg_id=18(auto), return_code=MQTTSN_ACCEPTED
		(void *)&mqttsn_noresponse_command_publish_puback,
		1,                                      // for 1 sec
	},
	{
		0,                                      // then
		&mqttsn_command_publish,                // send PUBLISH with
		(void *)&header_publish_t8_m19,         // qos=1, retain=1, topic_id=8, msg_id=19(auto), data_length=4(auto), data="long"
		&mqttsn_response_puback,                // wait for PUBACK with
		(void *)&header_puback_t8_m19,          // topic_id=8, msg_id=19(auto), return_code=MQTTSN_ACCEPTED
		(void *)&mqttsn_noresponse_command_publish_puback,
		1,                                      // for 1 sec
	},
	{
		0,                                      // then
		&mqttsn_command_publish,                // send PUBLISH with
		(void *)&header_publish_t9_m20,         // qos=1, retain=1, topic_id=9, msg_id=20(auto), data_length=1(auto), data="V"
		&mqttsn_response_puback,                // wait for PUBACK with
		(void *)&header_puback_t9_m20,          // topic_id=9, msg_id=20(auto), return_code=MQTTSN_ACCEPTED
		(void *)&mqttsn_noresponse_command_publish_puback,
		1,                                      // for 1 sec
	},
	{
		0,                                      // then
		&mqttsn_command_publish,                // send PUBLISH with
		(void *)&header_publish_t10_m21,        // qos=1, retain=1, topic_id=10, msg_id=21(auto), data_length=12(auto), data="on(1)-off(0)"
		&mqttsn_response_puback,                // wait for PUBACK with
		(void *)&header_puback_t10_m21,         // topic_id=10, msg_id=21(auto), return_code=MQTTSN_ACCEPTED
		(void *)&mqttsn_noresponse_command_publish_puback,
		1,                                      // for 1 sec
	},
	{
		0,                                      // then
		&mqttsn_command_publish,                // send PUBLISH with
		(void *)&header_publish_t13_m22,        // qos=1, retain=1, topic_id=13, msg_id=22(auto), data_length=4(auto), data="long"
		&mqttsn_response_puback,                // wait for PUBACK with
		(void *)&header_puback_t13_m22,         // topic_id=13, msg_id=22(auto), return_code=MQTTSN_ACCEPTED
		(void *)&mqttsn_noresponse_command_publish_puback,
		1,                                      // for 1 sec
	},
	{
		0,                                      // then
		&mqttsn_command_publish,                // send PUBLISH with
		(void *)&header_publish_t14_m23,        // qos=1, retain=1, topic_id=14, msg_id=23(auto), data_length=12(auto), data="on(1)-off(0)"
		&mqttsn_response_puback,                // wait for PUBACK with
		(void *)&header_puback_t14_m23,         // topic_id=14, msg_id=23(auto), return_code=MQTTSN_ACCEPTED
		(void *)&mqttsn_noresponse_command_publish_puback,
		1,                                      // for 1 sec
	},
	{
		0,                                      // then
		&mqttsn_command_subscribe,              // send SUBSCRIBE with
		(void *)&header_subscribe_t3_m24,       // qos=1, topic_id_type=1, topic_id=3, msg_id=24(auto)
		&mqttsn_response_suback,                // wait for SUBACK with
		(void *)&header_suback_t3_m24,          // qos=1, topic_id=3, msg_id=24(auto), return_code=MQTTSN_ACCEPTED
		(void *)&mqttsn_noresponse_command_subscribe_suback,
		1,                                      // for 1 sec
	},
	{
		0,                                      // then
		&mqttsn_command_subscribe,              // send SUBSCRIBE with
		(void *)&header_subscribe_t15_m25,      // qos=1, topic_id_type=1, topic_id=15, msg_id=25(auto)
		&mqttsn_response_suback,                // wait for SUBACK with
		(void *)&header_suback_t15_m25,         // qos=1, topic_id=15, msg_id=25(auto), return_code=MQTTSN_ACCEPTED
		(void *)&mqttsn_noresponse_command_subscribe_suback,
		1,                                      // for 1 sec
	},
	{
		0,                                      // then
		&mqttsn_command_publish,                // send PUBLISH with
		(void *)&header_publish_t5_m26,         // qos=1, retain=1, topic_id=5, msg_id=26(auto), data_length=4(auto), data="3.32"
		&mqttsn_response_puback,                // wait for PUBACK with
		(void *)&header_puback_t5_m26,          // topic_id=5, msg_id=26(auto), return_code=MQTTSN_ACCEPTED
		(void *)&mqttsn_noresponse_command_publish_puback,
		1,                                      // for 1 sec
	},
	{
		0,                                      // then
		&mqttsn_command_publish,                // send PUBLISH with
		(void *)&header_publish_t6_m27,         // qos=1, retain=1, topic_id=6, msg_id=27(auto), data_length=1(auto), data="0"
		&mqttsn_response_puback,                // wait for PUBACK with
		(void *)&header_puback_t6_m27,          // topic_id=6, msg_id=27(auto), return_code=MQTTSN_ACCEPTED
		(void *)&mqttsn_noresponse_command_publish_puback,
		1,                                      // for 1 sec
	},
	{
		0,                                      // then
		&mqttsn_command_publish,                // send PUBLISH with
		(void *)&header_publish_t12_m28,        // qos=1, retain=1, topic_id=12, msg_id=28(auto), data_length=1(auto), data="0"
		&mqttsn_response_puback,                // wait for PUBACK with
		(void *)&header_puback_t12_m28,         // topic_id=12, msg_id=28(auto), return_code=MQTTSN_ACCEPTED
		(void *)&mqttsn_noresponse_command_publish_puback,
		1,                                      // for 1 sec
	},
	{
		0,                                      // then
		(void *)0,                              // send nothing
		(void *)0,
		&mqttsn_response_publish,               // wait for PUBLISH with
		(void *)&header_publish_t3_m1,          // qos=1, retain=1, topic_id_type=0, topic_id=3, msg_id=1, data_length=2, data="20"
		(void *)&mqttsn_noresponse_nothing_publish,
		10,                                     // for 10 sec
	},
	{
		0,                                      // then
		&mqttsn_command_puback,                 // send PUBACK with
		(void *)&header_puback_t3_m1,           // topic_id=3, msg_id=1, return_code=MQTTSN_ACCEPTED
		&mqttsn_response_nothing,               // wait for nothing
		(void *)0,
		(void *)&mqttsn_noresponse_nothing,
		1,                                      // for 1 sec
	},
	{
		0,                                      // then
		(void *)0,                              // send nothing
		(void *)0,
		&mqttsn_response_publish,               // wait for PUBLISH with
		(void *)&header_publish_t15_m2,         // qos=1, retain=1, topic_id_type=0, topic_id=15, msg_id=2, data_length=1, data="1"
		(void *)&mqttsn_noresponse_nothing_publish,
		10,                                     // for 10 sec
	},
	{
		0,                                      // then
		&mqttsn_command_puback,                 // send PUBACK with
		(void *)&header_puback_t15_m2,          // topic_id=15, msg_id=2, return_code=MQTTSN_ACCEPTED
		&mqttsn_response_nothing,               // wait for nothing
		(void *)0,
		(void *)&mqttsn_noresponse_nothing,
		1,                                      // for 1 sec
	},
};

//--------------------------------------------
size_t test_mqttsn_list3_get_size(void)
{
	return (sizeof(test_list) / sizeof(test_t));
}

//--------------------------------------------
test_t *test_mqttsn_list3_get_test(size_t number)
{
	return &test_list[number];
}