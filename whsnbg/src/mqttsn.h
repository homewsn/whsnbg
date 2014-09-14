/*
* Copyright (c) 2013-2014 Vladimir Alemasov
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

#ifndef __MQTTSN_H__
#define __MQTTSN_H__

#ifdef _MSC_VER
#include "stdint_msc.h"	/* uint8_t ... uint64_t */
#else
#include <stdint.h>		/* uint8_t ... uint64_t */
#include <stddef.h>		/* size_t */
#endif

#include "lists.h"

//--------------------------------------------
typedef enum mqttsn_msg_type
{
	MQTTSN_ADVERTISE		= 0x00,
	MQTTSN_SEARCHGW			= 0x01,
	MQTTSN_GWINFO			= 0x02,
	MQTTSN_CONNECT			= 0x04,
	MQTTSN_CONNACK			= 0x05,
	MQTTSN_WILLTOPICREQ		= 0x06,
	MQTTSN_WILLTOPIC		= 0x07,
	MQTTSN_WILLMSGREQ		= 0x08,
	MQTTSN_WILLMSG			= 0x09,
	MQTTSN_REGISTER			= 0x0A,
	MQTTSN_REGACK			= 0x0B,
	MQTTSN_PUBLISH			= 0x0C,
	MQTTSN_PUBACK			= 0x0D,
	MQTTSN_PUBCOMP			= 0x0E,
	MQTTSN_PUBREC			= 0x0F,
	MQTTSN_PUBREL			= 0x10,
	MQTTSN_SUBSCRIBE		= 0x12,
	MQTTSN_SUBACK			= 0x13,
	MQTTSN_UNSUBSCRIBE		= 0x14,
	MQTTSN_UNSUBACK			= 0x15,
	MQTTSN_PINGREQ			= 0x16,
	MQTTSN_PINGRESP			= 0x17,
	MQTTSN_DISCONNECT		= 0x18,
	MQTTSN_WILLTOPICUPD		= 0x1A,
	MQTTSN_WILLTOPICRESP	= 0x1B,
	MQTTSN_WILLMSGUPD		= 0x1C,
	MQTTSN_WILLMSGRESP		= 0x1D
} mqttsn_msg_type_t;

typedef enum mqttsn_return_code
{
	MQTTSN_ACCEPTED					= 0,	/* accepted */
	MQTTSN_REFUSED_CONGESTION		= 1,	/* refused: congestion */
	MQTTSN_REFUSED_INVALID_TOPIC_ID	= 2,	/* refused: invalid topic ID */
	MQTTSN_REFUSED_NOT_SUPPORTED	= 3		/* refused: not supported */
} mqttsn_return_code_t;

typedef enum mqttsn_topic_id_type
{
	MQTTSN_TOPIC_NAME		= 0,			/* topic name */
	MQTTSN_PREDEF_TOPIC_ID	= 1,			/* pre-defined topic id */
	MQTTSN_SHORT_TOPIC_NAME	= 2				/* short topic name */
} mqttsn_topic_id_type_t;

typedef enum mqttsn_client_state
{
	MQTTSN_CLIENT_DISCONNECTED = 0,
	MQTTSN_CLIENT_CONNECTED,
	MQTTSN_CLIENT_ACTIVE,
	MQTTSN_CLIENT_ASLEEP,
	MQTTSN_CLIENT_AWAKE,
	MQTTSN_CLIENT_LOST
} mqttsn_client_state_t;

//--------------------------------------------
//** command structures

//--------------------------------------------
typedef struct mqttsn_fixed_header
{
	mqttsn_msg_type_t msg_type;
	uint16_t rem_len;
	uint8_t *rem_buf;
} mqttsn_fixed_header_t;

typedef struct mqttsn_flags
{
	uint8_t dup;
	uint8_t qos;
	uint8_t retain;
	uint8_t will;
	uint8_t clean_session;
	mqttsn_topic_id_type_t topic_id_type;
} mqttsn_flags_t;

typedef struct mqttsn_connect_header
{
	mqttsn_flags_t flags;
	uint8_t protocol_id;
	uint16_t duration;
	uint16_t client_id_length;
	uint8_t *client_id;
} mqttsn_connect_header_t;

typedef struct mqttsn_willtopic_header
{
	mqttsn_flags_t flags;
	uint16_t will_topic_length;
	uint8_t *will_topic;
} mqttsn_willtopic_header_t;

typedef struct mqttsn_willmsg_header
{
	uint16_t will_msg_length;
	uint8_t *will_msg;
} mqttsn_willmsg_header_t;

typedef struct mqttsn_register_header
{
	uint16_t topic_id;
	uint16_t msg_id;
	uint16_t name_length;
	uint8_t *name;
	list_pub_t **pub_list;
	list_pub_t *pub_item;
} mqttsn_register_header_t;

typedef struct mqttsn_xxxack_header
{
	uint16_t topic_id;
	uint16_t msg_id;
	mqttsn_return_code_t return_code;
} mqttsn_xxxack_header_t;

#define mqttsn_regack_header_t mqttsn_xxxack_header_t

typedef struct mqttsn_publish_header
{
	mqttsn_flags_t flags;
	uint16_t topic_id;
	uint16_t msg_id;
	uint16_t data_length;
	uint8_t *data;
	list_pub_t **pub_list;
	list_pub_t *pub_item;
} mqttsn_publish_header_t;

#define mqttsn_puback_header_t mqttsn_xxxack_header_t

typedef struct mqttsn_xxsubscribe_header
{
	mqttsn_flags_t flags;
	uint16_t msg_id;
	uint16_t topic_id;
	uint16_t name_length;
	uint8_t *name;
	list_pub_t **pub_list;
	list_pub_t *pub_item;
	list_sub_t **sub_list;
	list_sub_t *sub_item;
} mqttsn_xxsubscribe_header_t;

#define mqttsn_subscribe_header_t mqttsn_xxsubscribe_header_t

typedef struct mqttsn_suback_header
{
	mqttsn_flags_t flags;
	uint16_t topic_id;
	uint16_t msg_id;
	mqttsn_return_code_t return_code;
} mqttsn_suback_header_t;

#define mqttsn_unsubscribe_header_t mqttsn_xxsubscribe_header_t

typedef struct mqttsn_pingreq_header
{
	uint16_t client_id_length;
	uint8_t *client_id;
} mqttsn_pingreq_header_t;

#define mqttsn_willtopicupd_header_t mqttsn_willtopic_header_t
#define mqttsn_willmsgupd_header_t mqttsn_willmsg_header_t



//--------------------------------------------
//** decode command functions

//--------------------------------------------
int mqttsn_fixed_header_decode(mqttsn_fixed_header_t *fixhdr, uint8_t *buf, size_t size);
void mqttsn_flags_decode(mqttsn_flags_t *flags, uint8_t *byte);
void mqttsn_return_command_decode(mqttsn_fixed_header_t *fixhdr, mqttsn_return_code_t *code);

//--------------------------------------------
mqttsn_return_code_t mqttsn_connect_decode(mqttsn_fixed_header_t *fixhdr, mqttsn_connect_header_t *connect);
#define mqttsn_connack_decode(a, b) mqttsn_return_command_decode(a, b)
void mqttsn_willtopic_decode(mqttsn_fixed_header_t *fixhdr, mqttsn_willtopic_header_t *willtopic);
void mqttsn_willmsg_decode(mqttsn_fixed_header_t *fixhdr, mqttsn_willmsg_header_t *willmsg);
mqttsn_return_code_t mqttsn_register_decode(mqttsn_fixed_header_t *fixhdr, mqttsn_register_header_t *registerh);
void mqttsn_xxxack_decode(mqttsn_fixed_header_t *fixhdr, mqttsn_regack_header_t *xxxack);
#define mqttsn_regack_decode(a, b) mqttsn_xxxack_decode(a, b)
mqttsn_return_code_t mqttsn_publish_decode(mqttsn_fixed_header_t *fixhdr, mqttsn_publish_header_t *publish);
#define mqttsn_puback_decode(a, b) mqttsn_xxxack_decode(a, b)
void mqttsn_pubxxx_decode(mqttsn_fixed_header_t *fixhdr, uint16_t *msg_id);
#define mqttsn_pubrec_decode(a, b) mqttsn_pubxxx_decode(a, b)
#define mqttsn_pubrel_decode(a, b) mqttsn_pubxxx_decode(a, b)
#define mqttsn_pubcomp_decode(a, b) mqttsn_pubxxx_decode(a, b)
void mqttsn_xxsubscribe_decode(mqttsn_fixed_header_t *fixhdr, mqttsn_subscribe_header_t *xxsubscribe);
mqttsn_return_code_t mqttsn_subscribe_decode(mqttsn_fixed_header_t *fixhdr, mqttsn_subscribe_header_t *subscribe);
void mqttsn_suback_decode(mqttsn_fixed_header_t *fixhdr, mqttsn_suback_header_t *suback);
mqttsn_return_code_t mqttsn_unsubscribe_decode(mqttsn_fixed_header_t *fixhdr, mqttsn_unsubscribe_header_t *unsubscribe);
#define mqttsn_unsuback_decode(a, b) mqttsn_pubxxx_decode(a, b)
void mqttsn_pingreq_decode(mqttsn_fixed_header_t *fixhdr, mqttsn_pingreq_header_t *pingreq);
void mqttsn_disconnect_decode(mqttsn_fixed_header_t *fixhdr, uint16_t *duration);
#define mqttsn_willtopicupd_decode(a, b) mqttsn_willtopic_decode(a, b)
#define mqttsn_willmsgupd_decode(a, b) mqttsn_willmsg_decode(a, b)
#define mqttsn_willtopicresp_decode(a, b) mqttsn_return_command_decode(a, b)
#define mqttsn_willmsgresp_decode(a, b) mqttsn_return_command_decode(a, b)


//--------------------------------------------
//** encode command functions

//--------------------------------------------
void mqttsn_flags_encode(mqttsn_flags_t *flags, uint8_t *byte);
void mqttsn_simple_command_encode(mqttsn_msg_type_t msg_type, uint8_t **buf, size_t *size);
void mqttsn_return_command_encode(mqttsn_msg_type_t msg_type, uint8_t **buf, size_t *size, mqttsn_return_code_t code);

//--------------------------------------------
void mqttsn_connect_encode(uint8_t **buf, size_t *size, mqttsn_connect_header_t *connect);
#define mqttsn_connack_encode(a, b, c) mqttsn_return_command_encode(MQTTSN_CONNACK, a, b, c)
#define mqttsn_willtopicreq_encode(a, b) mqttsn_simple_command_encode(MQTTSN_WILLTOPICREQ, a, b)
void mqttsn_willtopicxxx_encode(mqttsn_msg_type_t msg_type, uint8_t **buf, size_t *size, mqttsn_willtopic_header_t *willtopic);
#define mqttsn_willtopic_encode(a, b, c) mqttsn_willtopicxxx_encode(MQTTSN_WILLTOPIC, a, b, c);
#define mqttsn_willmsgreq_encode(a, b) mqttsn_simple_command_encode(MQTTSN_WILLMSGREQ, a, b)
void mqttsn_willmsgxxx_encode(mqttsn_msg_type_t msg_type, uint8_t **buf, size_t *size, mqttsn_willmsg_header_t *willmsg);
#define mqttsn_willmsg_encode(a, b, c) mqttsn_willmsgxxx_encode(MQTTSN_WILLMSG, a, b, c);
void mqttsn_register_encode(uint8_t **buf, size_t *size, mqttsn_register_header_t *registerh);
void mqttsn_xxxack_encode(mqttsn_msg_type_t msg_type, uint8_t **buf, size_t *size, mqttsn_regack_header_t *xxxack);
#define mqttsn_regack_encode(a, b, c) mqttsn_xxxack_encode(MQTTSN_REGACK, a, b, c)
void mqttsn_publish_encode(uint8_t **buf, size_t *size, mqttsn_publish_header_t *publish);
#define mqttsn_puback_encode(a, b, c) mqttsn_xxxack_encode(MQTTSN_PUBACK, a, b, c)
void mqttsn_pubxxx_encode(mqttsn_msg_type_t msg_type, uint8_t **buf, size_t *size, uint16_t msg_id);
#define mqttsn_pubrec_encode(a, b, c) mqttsn_pubxxx_encode(MQTTSN_PUBREC, a, b, c)
#define mqttsn_pubrel_encode(a, b, c) mqttsn_pubxxx_encode(MQTTSN_PUBREL, a, b, c)
#define mqttsn_pubcomp_encode(a, b, c) mqttsn_pubxxx_encode(MQTTSN_PUBCOMP, a, b, c)
void mqttsn_xxsubscribe_encode(mqttsn_msg_type_t msg_type, uint8_t **buf, size_t *size, mqttsn_xxsubscribe_header_t *xxsubscribe);
#define mqttsn_subscribe_encode(a, b, c) mqttsn_xxsubscribe_encode(MQTTSN_SUBSCRIBE, a, b, c)
void mqttsn_suback_encode(uint8_t **buf, size_t *size, mqttsn_suback_header_t *suback);
#define mqttsn_unsubscribe_encode(a, b, c) mqttsn_xxsubscribe_encode(MQTTSN_UNSUBSCRIBE, a, b, c)
#define mqttsn_unsuback_encode(a, b, c) mqttsn_pubxxx_encode(MQTTSN_UNSUBACK, a, b, c)
void mqttsn_pingreq_encode(uint8_t **buf, size_t *size, mqttsn_pingreq_header_t *pingreq);
#define mqttsn_pingresp_encode(a, b) mqttsn_simple_command_encode(MQTTSN_PINGRESP, a, b)
void mqttsn_disconnect_encode(uint8_t **buf, size_t *size, uint16_t duration);
#define mqttsn_willtopicupd_encode(a, b, c) mqttsn_willtopicxxx_encode(MQTTSN_WILLTOPICUPD, a, b, c);
#define mqttsn_willmsgupd_encode(a, b, c) mqttsn_willmsgxxx_encode(MQTTSN_WILLMSGUPD, a, b, c);
#define mqttsn_willtopicresp_encode(a, b, c) mqttsn_return_command_encode(MQTTSN_WILLTOPICRESP, a, b, c)
#define mqttsn_willmsgresp_encode(a, b, c) mqttsn_return_command_encode(MQTTSN_WILLMSGRESP, a, b, c)


#endif /* __MQTTSN_H__ */
