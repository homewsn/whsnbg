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
#include "test_mqtt_list1.h"
#include "test_mqtt_list2.h"
#include "test_mqtt_list3.h"

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
static struct sockaddr_in mqtt_dummy_addr;

//--------------------------------------------
// *** Message Ids ***
//--------------------------------------------
static size_t current_test_list = 0;
static int auto_msg_id = 0;
static int auto_str_len = 0;
static uint16_t msg_id = 0;
//list_clreg_t *reg_topics;

void test_mqtt_setup(int auto_msgid, int auto_strlen)
{
	auto_msg_id = auto_msgid;
	auto_str_len = auto_strlen;
/*	if (auto_msg_id)
	{
		list_clreg_init(&reg_topics);
	}*/
}
void test_mqtt_destroy(void)
{
/*	if (auto_msg_id)
	{
		list_clreg_remove_all(&reg_topics);
	}*/
}




//--------------------------------------------
// *** Test functions ***
//--------------------------------------------
static int mqtt_response_unexpected(mqtt_msg_type_t msg_type)
{
	printf(">> The unexpected MQTT command=%d was received.\n", msg_type);
	return 1;
}

//--------------------------------------------
// CONNECT
//--------------------------------------------
void mqtt_command_connect(void *to_send)
{
	unsigned char *buf;
	size_t size;
	mqtt_connect_header_t *header;

	assert(to_send != NULL);

	header = (mqtt_connect_header_t *)to_send;
	if (auto_str_len)
	{
		header->client_id_length = (uint16_t)strlen(header->client_id);
	}
	mqtt_connect_encode(&buf, &size, header);
	printf("<< MQTT_CONNECT: UserNameFlag=%d, PasswordFlag=%d, WillFlag=%d, WillQoSFlag=%d, WillRetainFlag=%d, CleanSessionFlag=%d, KeepAliveTimer=%d, ClientId=%.*s, WillTopic=%.*s, WillMessage=%.*s, UserName=%.*s, Password=%.*s\n",
		header->flags.user_name,
		header->flags.password,
		header->flags.will,
		header->flags.will_qos,
		header->flags.will_retain,
		header->flags.clean_session,
		header->keep_alive_timer,
		header->client_id_length,
		header->client_id,
		header->will_topic_length,
		header->will_topic,
		header->will_message_length,
		header->will_message,
		header->user_name_length,
		header->user_name,
		header->password_length,
		header->password);
	msg_mqtt_tcp_add_packet(&mqtt_dummy_addr, buf, size);
}

int mqtt_noresponse_command_connect_connack(void)
{
	printf("MQTT_CONNACK was not received\n");
	printf("MQTT_CONNECT: test failed\n");
	return 1;
}

int mqtt_noresponse_command_connect_willtopicreq(void)
{
	printf("MQTT_WILLTOPICREQ was not received\n");
	printf("MQTT_CONNECT: test failed\n");
	return 1;
}

int mqtt_noresponse_command_connect_willmsgreq(void)
{
	printf("MQTT_WILLMSGREQ was not received\n");
	printf("MQTT_CONNECT-WILLTOPIC: test failed\n");
	return 1;
}

int mqtt_noresponse_command_connect_passed(void)
{
	printf("MQTT_CONNACK was not received\n");
	printf("MQTT_CONNECT: test passed\n\n");
	return 0;
}

//--------------------------------------------
// CONNACK
//--------------------------------------------
int mqtt_response_connack(void *received, void *expected)
{
	mqtt_connack_return_code_t received_code;
	mqtt_connack_return_code_t expected_code;
	mqtt_msg_type_t msg_type;

	assert(received != NULL);

	if ((msg_type = *(mqtt_msg_type_t *)received) != MQTT_CONNACK)
	{
		return mqtt_response_unexpected(msg_type);
	}

	mqtt_connack_decode(received, &received_code);
	printf(">> MQTT_CONNACK: ReturnCode=%d\n", received_code);
	if (expected)
	{
		expected_code = *((mqtt_connack_return_code_t *)(expected));
		if (received_code != expected_code)
		{
			printf("MQTT_CONNACK: the expected ReturnCode=%d is not equal to the received ReturnCode=%d\n",
				expected_code,
				received_code);
			printf("MQTT_CONNECT: test failed\n");
			return 1;
		}
		printf("MQTT_CONNECT: test passed\n\n");
		return 0;
	}
	else
	{
		printf("MQTT_CONNACK: expected not to receive\n");
		printf("MQTT_CONNECT: test failed\n");
		return 1;
	}
}

//--------------------------------------------
// DISCONNECT
//--------------------------------------------
void mqtt_command_disconnect(void *to_send)
{
	unsigned char *buf;
	size_t size;

	mqtt_disconnect_encode(&buf, &size);
	printf("<< MQTT_DISCONNECT\n");
	msg_mqtt_tcp_add_packet(&mqtt_dummy_addr, buf, size);
}

int mqtt_noresponse_command_disconnect_passed(void)
{
	printf("MQTT_DISCONNECT: the server's response was not received\n");
	printf("MQTT_DISCONNECT: test passed\n");
	return 0;
}

int mqtt_response_disconnect(void *received, void *expected)
{
	printf("MQTT_DISCONNECT: the server's response was received, but not expected\n");
	printf("MQTT_DISCONNECT: test failed\n");
	return 1;
}

//--------------------------------------------
// PINGREQ
//--------------------------------------------
void mqtt_command_pingreq(void *to_send)
{
	unsigned char *buf;
	size_t size;

	mqtt_pingreq_encode(&buf, &size);
	printf("<< MQTT_PINGREQ\n");
	msg_mqtt_tcp_add_packet(&mqtt_dummy_addr, buf, size);
}

int mqtt_noresponse_command_pingreq_pingresp(void)
{
	printf("MQTT_PINGRESP is not received\n");
	printf("MQTT_PINGREQ: test failed\n");
	return 1;
}

//--------------------------------------------
// PINGRESP
//--------------------------------------------
int mqtt_response_pingresp(void *received, void *expected)
{
	mqtt_msg_type_t msg_type;

	assert(received != NULL);

	if ((msg_type = *(mqtt_msg_type_t *)received) != MQTT_PINGRESP)
	{
		return mqtt_response_unexpected(msg_type);
	}

	printf(">> MQTT_PINGRESP\n");
	printf("MQTT_PINGREQ: test passed\n\n");
	return 0;
}

//--------------------------------------------
// PUBLISH
//--------------------------------------------
void mqtt_command_publish(void *to_send)
{
	unsigned char *buf;
	size_t size;
	mqtt_publish_header_t *header;

	assert(to_send != NULL);

	header = (mqtt_publish_header_t *)to_send;
	if (auto_msg_id)
	{
		msg_id = msg_id == 0xFFFF ? 0x0001 : ++msg_id;
		header->msg_id = msg_id;
	}
	if (auto_str_len)
	{
		header->pub_item->name_len = (uint16_t)strlen(header->pub_item->name);
		header->pub_item->data_len = (uint16_t)strlen(header->pub_item->data);
	}
	mqtt_publish_encode(&buf, &size, header);
	printf("<< MQTT_PUBLISH: QoS=%d, Retain=%d, MsgId=%d, TopicName=%.*s, TopicData=%.*s\n",
		header->qos,
		header->pub_item->retain,
		header->msg_id,
		header->pub_item->name_len,
		header->pub_item->name,
		header->pub_item->data_len,
		header->pub_item->data);
	msg_mqtt_tcp_add_packet(&mqtt_dummy_addr, buf, size);
}

int mqtt_noresponse_command_publish_puback(void)
{
	printf("MQTT_PUBACK is not received\n");
	printf("MQTT_PUBLISH: test failed\n");
	return 1;
}

int mqtt_noresponse_command_publish_pubrec(void)
{
	printf("MQTT_PUBREC is not received\n");
	printf("MQTT_PUBLISH: test failed\n");
	return 1;
}

int mqtt_response_publish(void *received, void *expected)
{
	mqtt_publish_header_t received_publish = { 0 };
	mqtt_publish_header_t *expected_publish;
	list_pub_t *pub_list = NULL;
	mqtt_msg_type_t msg_type;

	assert(received != NULL);
	assert(expected != NULL);

	if ((msg_type = *(mqtt_msg_type_t *)received) != MQTT_PUBLISH)
	{
		return mqtt_response_unexpected(msg_type);
	}

	received_publish.pub_list = &pub_list;
	mqtt_publish_decode(received, &received_publish);
	expected_publish = (mqtt_publish_header_t *)expected;
	printf(">> MQTT_PUBLISH: QoS=%d, Retain=%d, MsgId=%d, TopicName=%.*s, TopicData=%.*s\n",
		received_publish.qos,
		received_publish.pub_item->retain,
		received_publish.msg_id,
		received_publish.pub_item->name_len,
		received_publish.pub_item->name,
		received_publish.pub_item->data_len,
		received_publish.pub_item->data);
	/*	if (auto_msg_id)
	{
	expected_puback->msg_id = msg_id;
	}*/
	if ((received_publish.msg_id == expected_publish->msg_id) &&
		(received_publish.pub_item->retain == expected_publish->pub_item->retain) &&
		(received_publish.qos == expected_publish->qos) &&
		(!strncmp(received_publish.pub_item->data,
		expected_publish->pub_item->data,
		expected_publish->pub_item->data_len)) &&
		(!strncmp(received_publish.pub_item->name,
		expected_publish->pub_item->name,
		expected_publish->pub_item->name_len)))
	{
		list_pub_remove_all(pub_list);
		return 0;
	}
	else
	{
		printf(">> MQTT_PUBLISH: the expected QoS=%d, Retain=%d, MsgId=%d, TopicName=%.*s, TopicData=%.*s are not equal to the received\n",
			expected_publish->qos,
			expected_publish->pub_item->retain,
			expected_publish->msg_id,
			expected_publish->pub_item->name_len,
			expected_publish->pub_item->name,
			expected_publish->pub_item->data_len,
			expected_publish->pub_item->data);
		list_pub_remove_all(pub_list);
		return 1;
	}
}

//--------------------------------------------
// PUBACK
//--------------------------------------------
int mqtt_response_puback(void *received, void *expected)
{
	mqtt_puback_header_t received_puback = { 0 };
	mqtt_puback_header_t *expected_puback;
	mqtt_msg_type_t msg_type;

	assert(received != NULL);
	assert(expected != NULL);

	if ((msg_type = *(mqtt_msg_type_t *)received) != MQTT_PUBACK)
	{
		return mqtt_response_unexpected(msg_type);
	}

	mqtt_puback_decode(received, &received_puback);
	expected_puback = (mqtt_puback_header_t *)expected;
	printf(">> MQTT_PUBACK: MsgID=%d\n",
		received_puback.msg_id);
	if (auto_msg_id)
	{
		expected_puback->msg_id = msg_id;
	}
	if (!memcmp(&received_puback, expected_puback, sizeof(mqtt_puback_header_t)))
	{
		printf("MQTT_PUBLISH: test passed\n\n");
		return 0;
	}
	else
	{
		printf("MQTT_PUBACK: the expected MsgID=%d are not equal to the received\n",
			expected_puback->msg_id);
		printf("MQTT_PUBLISH: test failed\n");
		return 1;
	}
}

void mqtt_command_puback(void *to_send)
{
	unsigned char *buf;
	size_t size;
	mqtt_puback_header_t *header;

	assert(to_send != NULL);

	header = (mqtt_puback_header_t *)to_send;
	if (auto_msg_id)
	{
		header->msg_id = msg_id;
	}
	mqtt_puback_encode(&buf, &size, header->msg_id);
	printf("<< MQTT_PUBACK: MsgId=%d\n", header->msg_id);
	msg_mqtt_tcp_add_packet(&mqtt_dummy_addr, buf, size);
}

int mqtt_noresponse_command_puback_passed(void)
{
	printf("MQTT_PUBACK: the server's response was not received\n");
	printf("MQTT_PUBACK: test passed\n");
	return 0;
}


//--------------------------------------------
// PUBREC
//--------------------------------------------
int mqtt_response_pubrec(void *received, void *expected)
{
	mqtt_pubrec_header_t received_pubrec = { 0 };
	mqtt_pubrec_header_t *expected_pubrec;
	mqtt_msg_type_t msg_type;

	assert(received != NULL);
	assert(expected != NULL);

	if ((msg_type = *(mqtt_msg_type_t *)received) != MQTT_PUBREC)
	{
		return mqtt_response_unexpected(msg_type);
	}

	mqtt_pubrec_decode(received, &received_pubrec);
	expected_pubrec = (mqtt_pubrec_header_t *)expected;
	printf(">> MQTT_PUBREC: MsgID=%d\n",
		received_pubrec.msg_id);
	if (auto_msg_id)
	{
		expected_pubrec->msg_id = msg_id;
	}
	if (!memcmp(&received_pubrec, expected_pubrec, sizeof(mqtt_pubrec_header_t)))
	{
		printf("MQTT_PUBLISH: test passed\n\n");
		return 0;
	}
	else
	{
		printf("MQTT_PUBREC: the expected MsgID=%d are not equal to the received\n",
			expected_pubrec->msg_id);
		printf("MQTT_PUBLISH: test failed\n");
		return 1;
	}
}

//--------------------------------------------
// PUBREL
//--------------------------------------------
void mqtt_command_pubrel(void *to_send)
{
	unsigned char *buf;
	size_t size;
	mqtt_pubrel_header_t *header;

	assert(to_send != NULL);

	header = (mqtt_pubrel_header_t *)to_send;
	if (auto_msg_id)
	{
		header->msg_id = msg_id;
	}
	mqtt_pubrel_encode(&buf, &size, header->msg_id);
	printf("<< MQTT_PUBREL: MsgId=%d\n", header->msg_id);
	msg_mqtt_tcp_add_packet(&mqtt_dummy_addr, buf, size);
}

int mqtt_noresponse_command_pubrel_pubcomp(void)
{
	printf("MQTT_PUBCOMP is not received\n");
	printf("MQTT_PUBREL: test failed\n");
	return 1;
}

//--------------------------------------------
// PUBCOMP
//--------------------------------------------
int mqtt_response_pubcomp(void *received, void *expected)
{
	mqtt_pubcomp_header_t received_pubcomp = { 0 };
	mqtt_pubcomp_header_t *expected_pubcomp;
	mqtt_msg_type_t msg_type;

	assert(received != NULL);
	assert(expected != NULL);

	if ((msg_type = *(mqtt_msg_type_t *)received) != MQTT_PUBCOMP)
	{
		return mqtt_response_unexpected(msg_type);
	}

	mqtt_pubcomp_decode(received, &received_pubcomp);
	expected_pubcomp = (mqtt_pubcomp_header_t *)expected;
	printf(">> MQTT_PUBCOMP: MsgID=%d\n",
		received_pubcomp.msg_id);
	if (auto_msg_id)
	{
		expected_pubcomp->msg_id = msg_id;
	}
	if (!memcmp(&received_pubcomp, expected_pubcomp, sizeof(mqtt_pubcomp_header_t)))
	{
		printf("MQTT_PUBLISH: test passed\n\n");
		return 0;
	}
	else
	{
		printf("MQTT_PUBCOMP: the expected MsgID=%d are not equal to the received\n",
			expected_pubcomp->msg_id);
		printf("MQTT_PUBLISH: test failed\n");
		return 1;
	}
}

//--------------------------------------------
// SUBSCRIBE
//--------------------------------------------
void mqtt_command_subscribe(void *to_send)
{
	unsigned char *buf;
	size_t size;
	mqtt_subscribe_header_t *header;

	assert(to_send != NULL);

	header = (mqtt_subscribe_header_t *)to_send;
	if (auto_msg_id)
	{
		msg_id = msg_id == 0xFFFF ? 0x0001 : ++msg_id;
		header->msg_id = msg_id;
	}
	if (auto_str_len)
	{
		header->sub_item->name_len = (uint16_t)strlen(header->sub_item->name);
	}
	mqtt_subscribe_encode(&buf, &size, header);
	printf("<< MQTT_SUBSCRIBE: QoS=%d, TopicName=%.*s\n",
		header->sub_item->qos,
		header->sub_item->name_len,
		header->sub_item->name);
	msg_mqtt_tcp_add_packet(&mqtt_dummy_addr, buf, size);
}

int mqtt_noresponse_command_subscribe_suback(void)
{
	printf("MQTT_SUBACK is not received\n");
	printf("MQTT_SUBSCRIBE: test failed\n");
	return 1;
}

//--------------------------------------------
// SUBACK
//--------------------------------------------
int mqtt_response_suback(void *received, void *expected)
{
	mqtt_suback_header_t received_suback = { 0 };
	mqtt_suback_header_t *expected_suback;
	uint8_t qos_buf[1] = { 0 };
	mqtt_msg_type_t msg_type;

	assert(received != NULL);
	assert(expected != NULL);

	if ((msg_type = *(mqtt_msg_type_t *)received) != MQTT_SUBACK)
	{
		return mqtt_response_unexpected(msg_type);
	}

	received_suback.qos_buf = qos_buf;
	mqtt_suback_decode(received, &received_suback);
	expected_suback = (mqtt_suback_header_t *)expected;
	assert(expected_suback->qos_size == 1);
	printf(">> MQTT_SUBACK: MsgID=%d, QoS=%d\n",
		received_suback.msg_id,
		received_suback.qos_buf[0]);
	if (auto_msg_id)
	{
		expected_suback->msg_id = msg_id;
	}
	if ((received_suback.msg_id == expected_suback->msg_id) &&
		(received_suback.qos_buf[0] == expected_suback->qos_buf[0]) &&
		(received_suback.qos_size == expected_suback->qos_size))
	{
		printf("MQTT_SUBSCRIBE: test passed\n\n");
		return 0;
	}
	else
	{
		printf("MQTT_SUBACK: the expected MsgID=%d, QoS=%d are not equal to the received\n",
			expected_suback->msg_id,
			expected_suback->qos_buf[0]);
		printf("MQTTSN_SUBSCRIBE: test failed\n");
		return 1;
	}
}

//--------------------------------------------
// NOTHING
//--------------------------------------------
int mqtt_noresponse_nothing(void)
{
	return 0;
}

int mqtt_response_nothing(void *received, void *expected)
{
	assert(received != NULL);

	return mqtt_response_unexpected(*(mqtt_msg_type_t *)received);
}

int mqtt_noresponse_nothing_publish(void)
{
	printf(">>? It was expected to receive the PUBLISH command, but it was not received.\n");
	return 1;
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
		&test_mqtt_list1_get_size,
		&test_mqtt_list1_get_test,
	},
	{
		&test_mqtt_list2_get_size,
		&test_mqtt_list2_get_test,
	},
	{
		&test_mqtt_list3_get_size,
		&test_mqtt_list3_get_test,
	},
};

//--------------------------------------------
size_t test_mqtt_get_size(void)
{
	return (sizeof(test_suite) / sizeof(test_list_funcs_t));
}


//--------------------------------------------
size_t test_mqtt_list_get_size(void)
{
	return test_suite[current_test_list].get_size();
}

//--------------------------------------------
test_t *test_mqtt_list_get_test(size_t number)
{
	return test_suite[current_test_list].get_test(number);
}

void test_mqtt_set_list(size_t number)
{
	current_test_list = number;
}
