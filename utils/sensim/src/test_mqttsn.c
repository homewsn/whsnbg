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
#include "test_mqttsn_list1.h"
#include "test_mqttsn_list2.h"
#include "test_mqttsn_list3.h"

#ifdef _MSC_VER
#pragma warning (disable:4996) // This function may be unsafe.
#endif

#ifndef NDPRINTF
#define dprintf(...) printf(__VA_ARGS__)
#else
#define dprintf(...)
#endif

//--------------------------------------------
// *** Network addressing ***
//--------------------------------------------
static struct sockaddr_in mqttsn_dummy_addr;

//--------------------------------------------
// *** Message Ids ***
//--------------------------------------------
static size_t current_test_list = 0;
static int auto_msg_id = 0;
static int auto_str_len = 0;
static uint16_t msg_id = 0;
//list_clreg_t *reg_topics;

void test_mqttsn_setup(int auto_msgid, int auto_strlen)
{
	auto_msg_id = auto_msgid;
	auto_str_len = auto_strlen;
/*	if (auto_msg_id)
	{
		list_clreg_init(&reg_topics);
	}*/
}
void test_mqttsn_destroy(void)
{
/*	if (auto_msg_id)
	{
		list_clreg_remove_all(&reg_topics);
	}*/
}


//--------------------------------------------
// *** Test functions ***
//--------------------------------------------
static int mqttsn_response_unexpected(mqttsn_msg_type_t msg_type)
{
	printf(">> The unexpected MQTT-SN command=%d was received.\n", msg_type);
	return 1;
}

//--------------------------------------------
// CONNECT
//--------------------------------------------
void mqttsn_command_connect(void *to_send)
{
	unsigned char *buf;
	size_t size;
	mqttsn_connect_header_t *header;

	assert(to_send != NULL);

	header = (mqttsn_connect_header_t *)to_send;
	if (auto_str_len)
	{
		header->client_id_length = (uint16_t)strlen(header->client_id);
	}
	mqttsn_connect_encode(&buf, &size, header);
	printf("<< MQTTSN_CONNECT: Will=%d, CleanSession=%d, Duration=%d, ClientId=%.*s\n",
		header->flags.will,
		header->flags.clean_session,
		header->duration,
		header->client_id_length,
		header->client_id);
	msg_mqtt_udp_add_packet(&mqttsn_dummy_addr, buf, size);
}

int mqttsn_noresponse_command_connect_connack(void)
{
	printf("MQTTSN_CONNACK is not received\n");
	printf("MQTTSN_CONNECT: test failed\n");
	return 1;
}

int mqttsn_noresponse_command_connect_publish(void)
{
	printf("MQTTSN_PUBLISH is not received\n");
	printf("MQTTSN_CONNECT: test failed\n");
	return 1;
}

int mqttsn_noresponse_command_connect_willtopicreq(void)
{
	printf("MQTTSN_WILLTOPICREQ is not received\n");
	printf("MQTTSN_CONNECT: test failed\n");
	return 1;
}

int mqttsn_noresponse_command_connect_willmsgreq(void)
{
	printf("MQTTSN_WILLMSGREQ is not received\n");
	printf("MQTTSN_CONNECT-WILLTOPIC: test failed\n");
	return 1;
}

int mqttsn_noresponse_command_connect_passed(void)
{
	printf("MQTTSN_CONNACK is not received\n");
	printf("MQTTSN_CONNECT: test passed\n\n");
	return 0;
}


//--------------------------------------------
// CONNACK
//--------------------------------------------
int mqttsn_response_connack(void *received, void *expected)
{
	mqttsn_return_code_t received_code;
	mqttsn_return_code_t expected_code;
	mqttsn_msg_type_t msg_type;

	assert(received != NULL);
	assert(expected != NULL);

	if ((msg_type = *(mqttsn_msg_type_t *)received) != MQTTSN_CONNACK)
	{
		return mqttsn_response_unexpected(msg_type);
	}

	mqttsn_connack_decode(received, &received_code);
	printf(">> MQTTSN_CONNACK: ReturnCode=%d\n", received_code);
	expected_code = *((mqttsn_return_code_t *)(expected));
	if (received_code != expected_code)
	{
		printf("MQTTSN_CONNACK: the expected ReturnCode=%d is not equal to the received ReturnCode=%d\n",
			expected_code,
			received_code);
		printf("MQTTSN_CONNECT: test failed\n");
		return 1;
	}
	printf("MQTTSN_CONNECT: test passed\n\n");
	return 0;
}

//--------------------------------------------
// DISCONNECT
//--------------------------------------------
void mqttsn_command_disconnect(void *to_send)
{
	unsigned char *buf;
	size_t size;
	uint16_t duration;

	assert(to_send != NULL);

	duration = *((uint16_t *)to_send);
	mqttsn_disconnect_encode(&buf, &size, duration);
	printf("<< MQTTSN_DISCONNECT: Duration=%d\n", duration);
	msg_mqtt_udp_add_packet(&mqttsn_dummy_addr, buf, size);
}

int mqttsn_noresponse_command_disconnect_disconnect(void)
{
	printf("MQTTSN_DISCONNECT is not received\n");
	printf("MQTTSN_DISCONNECT: test failed\n");
	return 1;
}

int mqttsn_response_disconnect(void *received, void *expected)
{
	mqttsn_fixed_header_t fixhdr = { 0 };
	uint16_t duration;
	mqttsn_msg_type_t msg_type;

	assert(received != NULL);

	if ((msg_type = *(mqttsn_msg_type_t *)received) != MQTTSN_DISCONNECT)
	{
		return mqttsn_response_unexpected(msg_type);
	}

	fixhdr = *((mqttsn_fixed_header_t *)(received));
	if (fixhdr.rem_len == 1)
	{
		duration = ntohs(*((uint16_t *)&fixhdr.rem_buf[0]));
		printf(">> MQTTSN_DISCONNECT: The duration field is equal to %d when it should not exist at all.\n", duration);
		printf("MQTTSN_DISCONNECT: test failed\n");
		return 1;
	}
	printf(">> MQTTSN_DISCONNECT\n");
	printf("MQTTSN_DISCONNECT: test passed\n\n");
	return 0;
}

//--------------------------------------------
// PINGREQ
//--------------------------------------------
void mqttsn_command_pingreq(void *to_send)
{
	unsigned char *buf;
	size_t size;
	mqttsn_pingreq_header_t *header;

	assert(to_send != NULL);

	header = (mqttsn_pingreq_header_t *)to_send;
	mqttsn_pingreq_encode(&buf, &size, header);
	if (header->client_id_length != 0)
	{
		printf("<< MQTTSN_PINGREQ: ClientId=%.*s\n", header->client_id_length, header->client_id);
	}
	else
	{
		printf("<< MQTTSN_PINGREQ\n");
	}
	msg_mqtt_udp_add_packet(&mqttsn_dummy_addr, buf, size);
}

int mqttsn_noresponse_command_pingreq_pingresp(void)
{
	printf("MQTTSN_PINGRESP is not received\n");
	printf("MQTTSN_PINGREQ: test failed\n");
	return 1;
}

int mqttsn_noresponse_command_pingreq_publish(void)
{
	printf("MQTTSN_PUBLISH is not received\n");
	printf("MQTTSN_PINGREQ: test failed\n");
	return 1;
}


//--------------------------------------------
// PINGRESP
//--------------------------------------------
int mqttsn_response_pingresp(void *received, void *expected)
{
	mqttsn_msg_type_t msg_type;

	assert(received != NULL);

	if ((msg_type = *(mqttsn_msg_type_t *)received) != MQTTSN_PINGRESP)
	{
		return mqttsn_response_unexpected(msg_type);
	}

	printf(">> MQTTSN_PINGRESP\n");
	printf("MQTTSN_PINGREQ: test passed\n\n");
	return 0;
}

//--------------------------------------------
// WILLTOPICREQ
//--------------------------------------------
int mqttsn_response_willtopicreq(void *received, void *expected)
{
	mqttsn_msg_type_t msg_type;

	assert(received != NULL);

	if ((msg_type = *(mqttsn_msg_type_t *)received) != MQTTSN_WILLTOPICREQ)
	{
		return mqttsn_response_unexpected(msg_type);
	}

	printf(">> MQTTSN_WILLTOPICREQ\n");
	printf("MQTTSN_CONNECT-WILLTOPICREQ: test passed\n\n");
	return 0;
}

//--------------------------------------------
// WILLTOPIC
//--------------------------------------------
void mqttsn_command_willtopic(void *to_send)
{
	unsigned char *buf;
	size_t size;
	mqttsn_willtopic_header_t *header;

	assert(to_send != NULL);

	header = (mqttsn_willtopic_header_t *)to_send;
	if (auto_str_len)
	{
		header->will_topic_length = (uint16_t)strlen(header->will_topic);
	}
	mqttsn_willtopic_encode(&buf, &size, header);
	printf("<< MQTTSN_WILLTOPIC: QoS=%d, Retain=%d, WillTopic=%.*s\n",
		header->flags.qos,
		header->flags.retain,
		header->will_topic_length,
		header->will_topic);
	msg_mqtt_udp_add_packet(&mqttsn_dummy_addr, buf, size);
}

//--------------------------------------------
// WILLMSGREQ
//--------------------------------------------
int mqttsn_response_willmsgreq(void *received, void *expected)
{
	mqttsn_msg_type_t msg_type;

	assert(received != NULL);

	if ((msg_type = *(mqttsn_msg_type_t *)received) != MQTTSN_WILLMSGREQ)
	{
		return mqttsn_response_unexpected(msg_type);
	}

	printf(">> MQTTSN_WILLMSGREQ\n");
	printf("MQTTSN_CONNECT-WILLMSGREQ: test passed\n\n");
	return 0;
}

//--------------------------------------------
// WILLMSG
//--------------------------------------------
void mqttsn_command_willmsg(void *to_send)
{
	unsigned char *buf;
	size_t size;
	mqttsn_willmsg_header_t *header;

	assert(to_send != NULL);

	header = (mqttsn_willmsg_header_t *)to_send;
	if (auto_str_len)
	{
		header->will_msg_length = (uint16_t)strlen(header->will_msg);
	}
	mqttsn_willmsg_encode(&buf, &size, header);
	printf("<< MQTTSN_WILLMSG: WillMsg=%.*s\n", header->will_msg_length, header->will_msg);
	msg_mqtt_udp_add_packet(&mqttsn_dummy_addr, buf, size);
}

//--------------------------------------------
// REGISTER
//--------------------------------------------
void mqttsn_command_register(void *to_send)
{
	unsigned char *buf;
	size_t size;
	mqttsn_register_header_t *header;

	assert(to_send != NULL);

	header = (mqttsn_register_header_t *)to_send;
	if (auto_msg_id)
	{
		msg_id = msg_id == 0xFFFF ? 0x0001 : ++msg_id;
		header->msg_id = msg_id;
	}
	if (auto_str_len)
	{
		header->name_length = (uint16_t)strlen(header->name);
	}
	mqttsn_register_encode(&buf, &size, header);
	printf("<< MQTTSN_REGISTER: TopicId=%d, MsgId=%d, TopicName=%.*s\n",
		header->topic_id,
		header->msg_id,
		header->name_length,
		header->name);
	msg_mqtt_udp_add_packet(&mqttsn_dummy_addr, buf, size);
}

int mqttsn_noresponse_command_register_regack(void)
{
	printf("MQTTSN_REGACK is not received\n");
	printf("MQTTSN_REGISTER: test failed\n");
	return 1;
}

//--------------------------------------------
// REGACK
//--------------------------------------------
int mqttsn_response_regack(void *received, void *expected)
{
	mqttsn_regack_header_t received_regack = { 0 };
	mqttsn_regack_header_t *expected_regack;
	mqttsn_msg_type_t msg_type;

	assert(received != NULL);
	assert(expected != NULL);

	if ((msg_type = *(mqttsn_msg_type_t *)received) != MQTTSN_REGACK)
	{
		return mqttsn_response_unexpected(msg_type);
	}

	mqttsn_regack_decode(received, &received_regack);
	expected_regack = (mqttsn_regack_header_t *)expected;
	printf(">> MQTTSN_REGACK: ReturnCode=%d, TopicID=%d, MsgID=%d\n",
		received_regack.return_code,
		received_regack.topic_id,
		received_regack.msg_id);
	if (auto_msg_id)
	{
		expected_regack->msg_id = msg_id;
	}
	if (!memcmp(&received_regack, expected_regack, sizeof(mqttsn_regack_header_t)))
	{
		printf("MQTTSN_REGISTER: test passed\n\n");
		return 0;
	}
	else
	{
		printf("MQTTSN_REGACK: the expected ReturnCode=%d, TopicID=%d, MsgID=%d are not equal to the received\n",
			expected_regack->return_code,
			expected_regack->topic_id,
			expected_regack->msg_id);
		printf("MQTTSN_REGISTER: test failed\n");
		return 1;
	}
}

//--------------------------------------------
// PUBLISH
//--------------------------------------------
void mqttsn_command_publish(void *to_send)
{
	unsigned char *buf;
	size_t size;
	mqttsn_publish_header_t *header;

	assert(to_send != NULL);

	header = (mqttsn_publish_header_t *)to_send;
	if (auto_msg_id)
	{
		msg_id = msg_id == 0xFFFF ? 0x0001 : ++msg_id;
		header->msg_id = msg_id;
	}
	if (auto_str_len)
	{
		header->data_length = (uint16_t)strlen(header->data);
	}
	mqttsn_publish_encode(&buf, &size, header);
	printf("<< MQTTSN_PUBLISH: DUP=%d, QoS=%d, Retain=%d, TopicIdType=%d, TopicId=%d, MsgId=%d, Data=%.*s\n",
		header->flags.dup,
		header->flags.qos,
		header->flags.retain,
		header->flags.topic_id_type,
		header->topic_id,
		header->msg_id,
		header->data_length,
		header->data);
	msg_mqtt_udp_add_packet(&mqttsn_dummy_addr, buf, size);
}

int mqttsn_noresponse_command_publish_puback(void)
{
	printf("MQTTSN_PUBACK is not received\n");
	printf("MQTTSN_PUBLISH: test failed\n");
	return 1;
}

int mqttsn_noresponse_command_publish_pubrec(void)
{
	printf("MQTTSN_PUBREC is not received\n");
	printf("MQTTSN_PUBLISH: test failed\n");
	return 1;
}

int mqttsn_response_publish(void *received, void *expected)
{
	mqttsn_publish_header_t received_publish = { 0 };
	mqttsn_publish_header_t *expected_publish;
	mqttsn_msg_type_t msg_type;

	assert(received != NULL);
	assert(expected != NULL);

	if ((msg_type = *(mqttsn_msg_type_t *)received) != MQTTSN_PUBLISH)
	{
		return mqttsn_response_unexpected(msg_type);
	}

	mqttsn_publish_decode(received, &received_publish);
	expected_publish = (mqttsn_publish_header_t *)expected;
	printf(">> MQTTSN_PUBLISH: DUP=%d, QoS=%d, Retain=%d, TopicIdType=%d, TopicId=%d, MsgId=%d, Data=%.*s\n",
		received_publish.flags.dup,
		received_publish.flags.qos,
		received_publish.flags.retain,
		received_publish.flags.topic_id_type,
		received_publish.topic_id,
		received_publish.msg_id,
		received_publish.data_length,
		received_publish.data);
	/*	if (auto_msg_id)
	{
	expected_puback->msg_id = msg_id;
	}*/
	if ((!memcmp(&received_publish, expected_publish,
		sizeof(mqttsn_publish_header_t) -
		sizeof(((mqttsn_publish_header_t *)0)->data) -
		sizeof(((mqttsn_publish_header_t *)0)->pub_list) -
		sizeof(((mqttsn_publish_header_t *)0)->pub_item))) &&
		(!strncmp(received_publish.data,
		expected_publish->data,
		expected_publish->data_length)))
	{
		return 0;
	}
	else
	{
		printf(">> MQTTSN_PUBLISH: the expected DUP=%d, QoS=%d, Retain=%d, TopicIdType=%d, TopicId=%d, MsgId=%d, Data=%.*s are not equal to the received\n",
			expected_publish->flags.dup,
			expected_publish->flags.qos,
			expected_publish->flags.retain,
			expected_publish->flags.topic_id_type,
			expected_publish->topic_id,
			expected_publish->msg_id,
			expected_publish->data_length,
			expected_publish->data);
		return 1;
	}
}

//--------------------------------------------
// PUBACK
//--------------------------------------------
int mqttsn_response_puback(void *received, void *expected)
{
	mqttsn_puback_header_t received_puback = { 0 };
	mqttsn_puback_header_t *expected_puback;
	mqttsn_msg_type_t msg_type;

	assert(received != NULL);
	assert(expected != NULL);

	if ((msg_type = *(mqttsn_msg_type_t *)received) != MQTTSN_PUBACK)
	{
		return mqttsn_response_unexpected(msg_type);
	}

	mqttsn_puback_decode(received, &received_puback);
	expected_puback = (mqttsn_puback_header_t *)expected;
	printf(">> MQTTSN_PUBACK: ReturnCode=%d, TopicID=%d, MsgID=%d\n",
		received_puback.return_code,
		received_puback.topic_id,
		received_puback.msg_id);
	if (auto_msg_id)
	{
		expected_puback->msg_id = msg_id;
	}
	if (!memcmp(&received_puback, expected_puback, sizeof(mqttsn_puback_header_t)))
	{
		printf("MQTTSN_PUBLISH: test passed\n\n");
		return 0;
	}
	else
	{
		printf("MQTTSN_PUBACK: the expected ReturnCode=%d, TopicID=%d, MsgID=%d are not equal to the received\n",
			expected_puback->return_code,
			expected_puback->topic_id,
			expected_puback->msg_id);
		printf("MQTTSN_PUBLISH: test failed\n");
		return 1;
	}
}

void mqttsn_command_puback(void *to_send)
{
	unsigned char *buf;
	size_t size;
	mqttsn_puback_header_t *header;

	assert(to_send != NULL);

	header = (mqttsn_puback_header_t *)to_send;
	mqttsn_puback_encode(&buf, &size, header);
	printf("<< MQTTSN_PUBACK: ReturnCode=%d, TopicID=%d, MsgID=%d\n",
		header->return_code,
		header->topic_id,
		header->msg_id);
	msg_mqtt_udp_add_packet(&mqttsn_dummy_addr, buf, size);
}


//--------------------------------------------
// PUBREC
//--------------------------------------------
int mqttsn_response_pubrec(void *received, void *expected)
{
	uint16_t received_msgid;
	uint16_t expected_msgid;
	mqttsn_msg_type_t msg_type;

	assert(received != NULL);
	assert(expected != NULL);

	if ((msg_type = *(mqttsn_msg_type_t *)received) != MQTTSN_PUBREC)
	{
		return mqttsn_response_unexpected(msg_type);
	}

	mqttsn_pubrec_decode(received, &received_msgid);
	expected_msgid = *((uint16_t *)expected);
	printf(">> MQTTSN_PUBREC: MsgID=%d\n", received_msgid);
	if (auto_msg_id)
	{
		expected_msgid = msg_id;
	}
	if (received_msgid == expected_msgid)
	{
		return 0;
	}
	else
	{
		printf("MQTTSN_PUBREC: the expected MsgID=%d is not equal to the received MsgID=%d\n",
			expected_msgid,
			received_msgid);
		printf("MQTTSN_PUBLISH: test failed\n");
		return 1;
	}
}

//--------------------------------------------
// PUBREL
//--------------------------------------------
void mqttsn_command_pubrel(void *to_send)
{
	unsigned char *buf;
	size_t size;
	uint16_t msgid;

	assert(to_send != NULL);

	msgid = *((uint16_t *)to_send);
	if (auto_msg_id)
	{
		msgid = msg_id;
	}
	mqttsn_pubrel_encode(&buf, &size, msgid);
	printf("<< MQTTSN_PUBREL: MsgId=%d\n", msgid);
	msg_mqtt_udp_add_packet(&mqttsn_dummy_addr, buf, size);
}

int mqttsn_noresponse_command_pubrel_pubcomp(void)
{
	printf("MQTTSN_PUBCOMP is not received\n");
	printf("MQTTSN_PUBREL: test failed\n");
	return 1;
}

//--------------------------------------------
// PUBCOMP
//--------------------------------------------
int mqttsn_response_pubcomp(void *received, void *expected)
{
	uint16_t received_msgid;
	uint16_t expected_msgid;
	mqttsn_msg_type_t msg_type;

	assert(received != NULL);
	assert(expected != NULL);

	if ((msg_type = *(mqttsn_msg_type_t *)received) != MQTTSN_PUBCOMP)
	{
		return mqttsn_response_unexpected(msg_type);
	}

	mqttsn_pubcomp_decode(received, &received_msgid);
	expected_msgid = *((uint16_t *)expected);
	printf(">> MQTTSN_PUBCOMP: MsgID=%d\n", received_msgid);
	if (auto_msg_id)
	{
		expected_msgid = msg_id;
	}
	if (received_msgid == expected_msgid)
	{
		printf("MQTTSN_PUBLISH: test passed\n\n");
		return 0;
	}
	else
	{
		printf("MQTTSN_PUBCOMP: the expected MsgID=%d is not equal to the received MsgID=%d\n",
			expected_msgid,
			received_msgid);
		printf("MQTTSN_PUBLISH: test failed\n");
		return 1;
	}
}

//--------------------------------------------
// SUBSCRIBE
//--------------------------------------------
void mqttsn_command_subscribe(void *to_send)
{
	unsigned char *buf;
	size_t size;
	mqttsn_subscribe_header_t *header;

	assert(to_send != NULL);

	header = (mqttsn_subscribe_header_t *)to_send;
	if (auto_msg_id)
	{
		msg_id = msg_id == 0xFFFF ? 0x0001 : ++msg_id;
		header->msg_id = msg_id;
	}
	mqttsn_subscribe_encode(&buf, &size, header);
	printf("<< MQTTSN_SUBSCRIBE: DUP=%d, QoS=%d, TopicIdType=%d, TopicId=%d, MsgId=%d\n",
		header->flags.dup,
		header->flags.qos,
		header->flags.topic_id_type,
		header->topic_id,
		header->msg_id);
	msg_mqtt_udp_add_packet(&mqttsn_dummy_addr, buf, size);
}

int mqttsn_noresponse_command_subscribe_suback(void)
{
	printf("MQTTSN_SUBACK is not received\n");
	printf("MQTTSN_SUBSCRIBE: test failed\n");
	return 1;
}

//--------------------------------------------
// SUBACK
//--------------------------------------------
int mqttsn_response_suback(void *received, void *expected)
{
	mqttsn_suback_header_t received_suback = { 0 };
	mqttsn_suback_header_t *expected_suback;
	mqttsn_msg_type_t msg_type;

	assert(received != NULL);
	assert(expected != NULL);

	if ((msg_type = *(mqttsn_msg_type_t *)received) != MQTTSN_SUBACK)
	{
		return mqttsn_response_unexpected(msg_type);
	}

	mqttsn_suback_decode(received, &received_suback);
	expected_suback = (mqttsn_suback_header_t *)expected;
	printf(">> MQTTSN_SUBACK: ReturnCode=%d, TopicID=%d, MsgID=%d, QoS=%d\n",
		received_suback.return_code,
		received_suback.topic_id,
		received_suback.msg_id,
		received_suback.flags.qos);
	if (auto_msg_id)
	{
		expected_suback->msg_id = msg_id;
	}
	if (!memcmp(&received_suback, expected_suback, sizeof(mqttsn_suback_header_t)))
	{
		printf("MQTTSN_SUBSCRIBE: test passed\n\n");
		return 0;
	}
	else
	{
		printf("MQTTSN_SUBACK: the expected ReturnCode=%d, TopicID=%d, MsgID=%d, QoS=%d are not equal to the received\n",
			expected_suback->return_code,
			expected_suback->topic_id,
			expected_suback->msg_id,
			expected_suback->flags.qos);
		printf("MQTTSN_SUBSCRIBE: test failed\n");
		return 1;
	}
}

//--------------------------------------------
// NOTHING
//--------------------------------------------
int mqttsn_noresponse_nothing(void)
{
	return 0;
}

int mqttsn_noresponse_nothing_publish(void)
{
	printf("MQTTSN_PUBLISH is not received\n");
	printf("MQTTSN_PUBLISH: test failed\n");
	return 1;
}

int mqttsn_response_nothing(void *received, void *expected)
{
	assert(received != NULL);

	return mqttsn_response_unexpected(*(mqttsn_msg_type_t *)received);
}


//--------------------------------------------
typedef struct test_list_funcs
{
	size_t (*get_size)(void);
	test_t *(*get_test)(size_t);
} test_list_funcs_t;

static test_list_funcs_t test_suite[] =
{
	{
		&test_mqttsn_list1_get_size,
		&test_mqttsn_list1_get_test,
	},
	{
		&test_mqttsn_list2_get_size,
		&test_mqttsn_list2_get_test,
	},
	{
		&test_mqttsn_list3_get_size,
		&test_mqttsn_list3_get_test,
	},
};

//--------------------------------------------
size_t test_mqttsn_get_size(void)
{
	return (sizeof(test_suite) / sizeof(test_list_funcs_t));
}

//--------------------------------------------
size_t test_mqttsn_list_get_size(void)
{
	return test_suite[current_test_list].get_size();
}

//--------------------------------------------
test_t *test_mqttsn_list_get_test(size_t number)
{
	return test_suite[current_test_list].get_test(number);
}

void test_mqttsn_set_list(size_t number)
{
	current_test_list = number;
}

