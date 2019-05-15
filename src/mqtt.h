/*
* Copyright (c) 2013-2015, 2018, 2019 Vladimir Alemasov
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

#ifndef MQTT_H_
#define MQTT_H_

#ifdef _MSC_VER
#include "stdint_msc.h"	/* uint8_t ... uint64_t */
#else
#include <stdint.h>		/* uint8_t ... uint64_t */
#include <stddef.h>		/* size_t */
#endif

#include "lists.h"
#include "list_mqtt_user.h"

//--------------------------------------------
#define MQTT_SYSTOPIC_RULESENGINE_RULES		"$SYS/rulesengine/rules"
#define MQTT_SYSTOPIC_RULESENGINE_ERROR		"$SYS/rulesengine/error"
#define MQTT_SYSTOPIC_RULESENGINE_VERSION	"$SYS/rulesengine/version"
#define MQTT_SYSTOPIC_RULESENGINE_VARIABLES	"$SYS/rulesengine/variables"

//--------------------------------------------
typedef enum mqtt_msg_type
{
	MQTT_CONNECT		= 1,	/* Client request to connect to Server */
	MQTT_CONNACK		= 2,	/* Connect Acknowledgment */
	MQTT_PUBLISH		= 3,	/* Publish message */
	MQTT_PUBACK			= 4,	/* Publish Acknowledgment */
	MQTT_PUBREC			= 5,	/* Publish Received (assured delivery part 1) */
	MQTT_PUBREL			= 6,	/* Publish Release (assured delivery part 2) */
	MQTT_PUBCOMP		= 7,	/* Publish Complete (assured delivery part 3) */
	MQTT_SUBSCRIBE		= 8,	/* Client Subscribe request */
	MQTT_SUBACK			= 9,	/* Subscribe Acknowledgment */
	MQTT_UNSUBSCRIBE	= 10,	/* Client Unsubscribe request */
	MQTT_UNSUBACK		= 11,	/* Unsubscribe Acknowledgment */
	MQTT_PINGREQ		= 12,	/* PING Request */
	MQTT_PINGRESP		= 13,	/* PING Response */
	MQTT_DISCONNECT		= 14	/* Client is Disconnecting */
} mqtt_msg_type_t;

typedef enum mqtt_connack_return_code
{
	MQTT_ACCEPTED						= 0,	/* Connection Accepted */
	MQTT_REFUSED_PROTOCOL_VERSION		= 1,	/* Connection Refused: unacceptable protocol version */
	MQTT_REFUSED_IDENTIFIER_REJECTED	= 2,	/* Connection Refused: identifier rejected */
	MQTT_REFUSED_SERVER_UNAVAILABLE		= 3,	/* Connection Refused: server unavailable */
	MQTT_REFUSED_BAD_USER_NAME_PASSWORD = 4,	/* Connection Refused: bad user name or password */
	MQTT_REFUSED_NOT_AUTHORIZED			= 5		/* Connection Refused: not authorized */
} mqtt_connack_return_code_t;

//--------------------------------------------
typedef struct mqtt_fixed_header
{
	mqtt_msg_type_t msg_type;
	uint8_t dup_flag;
	uint8_t qos_level;
	uint8_t retain;
	uint32_t rem_len;
	uint8_t *rem_buf;
} mqtt_fixed_header_t;

typedef struct mqtt_connect_flags
{
	uint8_t user_name;
	uint8_t password;
	uint8_t will_retain;
	uint8_t will_qos;
	uint8_t will;
	uint8_t clean_session;
} mqtt_connect_flags_t;

typedef struct mqtt_connect_header
{
	mqtt_connect_flags_t flags;
	uint16_t keep_alive_timer;
	uint16_t client_id_length;
	uint8_t *client_id;
	uint16_t will_topic_length;
	uint8_t *will_topic;
	uint16_t will_message_length;
	uint8_t *will_message;
	uint16_t user_name_length;
	uint8_t *user_name;
	uint16_t password_length;
	uint8_t *password;
	list_mqtt_user_t **mqtt_users;
	unsigned char publish_enable;
} mqtt_connect_header_t;

typedef struct mqtt_topic
{
	struct mqtt_topic *next;
	uint16_t length;
	uint8_t *name;
	uint8_t qos;
} mqtt_topic_t;

typedef struct mqtt_subscribe_header
{
	uint16_t msg_id;
	list_sub_t **sub_list;
	list_sub_t *sub_item;
	size_t qos_size;
} mqtt_subscribe_header_t;

typedef struct mqtt_suback_header
{
	uint16_t msg_id;
	uint8_t *qos_buf;
	size_t qos_size;
} mqtt_suback_header_t;

#define mqtt_unsubscribe_header_t mqtt_subscribe_header_t

typedef struct mqtt_publish_header
{
	uint16_t msg_id;
	uint8_t qos;
	list_pub_t **pub_list;
	list_pub_t *pub_item;
} mqtt_publish_header_t;

typedef struct mqtt_pubxxx_header
{
	uint16_t msg_id;
} mqtt_pubxxx_header_t;

#define mqtt_puback_header_t mqtt_pubxxx_header_t
#define mqtt_pubrec_header_t mqtt_pubxxx_header_t
#define mqtt_pubrel_header_t mqtt_pubxxx_header_t
#define mqtt_pubcomp_header_t mqtt_pubxxx_header_t
#define mqtt_unsuback_header_t mqtt_pubxxx_header_t

//--------------------------------------------
int mqtt_packet_check_length(unsigned char *buf, size_t size, uint32_t *len, unsigned char **rem_buf, size_t *proc_size);
int mqtt_packets_buffer_check(unsigned char *buf, size_t size);
int mqtt_fixed_header_decode(mqtt_fixed_header_t *fixhdr, unsigned char *buf, size_t size, size_t *proc_size);

//--------------------------------------------
mqtt_connack_return_code_t mqtt_connect_decode(mqtt_fixed_header_t *fixhdr, mqtt_connect_header_t *connect);
void mqtt_connack_decode(mqtt_fixed_header_t *fixhdr, mqtt_connack_return_code_t *code);
void mqtt_subscribe_decode(mqtt_fixed_header_t *fixhdr, mqtt_subscribe_header_t *subscribe);
void mqtt_suback_decode(mqtt_fixed_header_t *fixhdr, mqtt_suback_header_t *suback);
void mqtt_unsubscribe_decode(mqtt_fixed_header_t *fixhdr, mqtt_unsubscribe_header_t *unsubscribe);
void mqtt_publish_decode(mqtt_fixed_header_t *fixhdr, mqtt_publish_header_t *publish);
void mqtt_pubxxx_decode(mqtt_fixed_header_t *fixhdr, mqtt_pubxxx_header_t *pubxxx);
#define mqtt_puback_decode(a, b) mqtt_pubxxx_decode(a, b)
#define mqtt_pubrec_decode(a, b) mqtt_pubxxx_decode(a, b)
#define mqtt_pubrel_decode(a, b) mqtt_pubxxx_decode(a, b)
#define mqtt_pubcomp_decode(a, b) mqtt_pubxxx_decode(a, b)

//--------------------------------------------
void mqtt_packet_encode(mqtt_fixed_header_t *fixhdr, unsigned char **buf, size_t *size);
void mqtt_connect_encode(unsigned char **buf, size_t *size, mqtt_connect_header_t *connect);
void mqtt_connack_encode(unsigned char **buf, size_t *size, mqtt_connack_return_code_t code);
void mqtt_suback_encode(unsigned char **buf, size_t *size, mqtt_suback_header_t *suback);
void mqtt_pingresp_encode(unsigned char **buf, size_t *size);
void mqtt_publish_encode(unsigned char **buf, size_t *size, mqtt_publish_header_t *publish);
void mqtt_pubxxx_encode(mqtt_msg_type_t msg_type, unsigned char **buf, size_t *size, uint16_t msg_id);
#define mqtt_puback_encode(a, b, c) mqtt_pubxxx_encode(MQTT_PUBACK, a, b, c)
#define mqtt_pubrec_encode(a, b, c) mqtt_pubxxx_encode(MQTT_PUBREC, a, b, c)
#define mqtt_pubrel_encode(a, b, c) mqtt_pubxxx_encode(MQTT_PUBREL, a, b, c)
#define mqtt_pubcomp_encode(a, b, c) mqtt_pubxxx_encode(MQTT_PUBCOMP, a, b, c)
#define mqtt_unsuback_encode(a, b, c) mqtt_pubxxx_encode(MQTT_UNSUBACK, a, b, c)
void mqtt_subscribe_encode(unsigned char **buf, size_t *size, mqtt_subscribe_header_t *subscribe);
void mqtt_disconnect_encode(unsigned char **buf, size_t *size);

#endif /* MQTT_H_ */
