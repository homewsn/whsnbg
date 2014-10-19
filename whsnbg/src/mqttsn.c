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

#include "config.h"
#include "mqttsn.h"
#ifdef SENSOR_DATA
#include "sensor_data.h"
#endif
#include <stdlib.h>		/* malloc */
#include <string.h>		/* memcpy, strlen */
#include <assert.h>		/* assert */

#ifdef WIN32 /* Windows */
#include <winsock2.h>	/* ntohs, htons */
#else /* Linux, OpenWRT */
#include <arpa/inet.h>	/* ntohs, htons */
#endif

#ifndef NDEBUG
#ifdef LINUX_DAEMON_VERSION
#include <syslog.h>
#define dprintf(...) syslog(LOG_DEBUG, __VA_ARGS__)
#else
#include <stdio.h>
#define dprintf(...) printf(__VA_ARGS__)
#endif
#else
#define dprintf(...)
#endif

extern unsigned int mysql_enable;

//--------------------------------------------
//** decode command functions

//--------------------------------------------
int mqttsn_fixed_header_decode(mqttsn_fixed_header_t *fixhdr, uint8_t *buf, size_t size)
{
	if (buf[0] == 1)
	{
		fixhdr->rem_len = ntohs(*((uint16_t *)&buf[1]));
		fixhdr->msg_type = (mqttsn_msg_type_t)buf[3];
	}
	else
	{
		fixhdr->rem_len = buf[0];
		fixhdr->msg_type = (mqttsn_msg_type_t)buf[1];
	}

	// Note that because MQTT-SN does not support message fragmentation and reassembly, the maximum message length
	// that could be used in a network is governed by the maximum packet size that is supported by that network,
	// and not by the maximum length that could be encoded by MQTT-SN.
	if (fixhdr->rem_len != size)
		return -1;

	if (buf[0] == 1)
	{
		fixhdr->rem_len -= 4;
		fixhdr->rem_buf = buf + 4;
	}
	else
	{
		fixhdr->rem_len -= 2;
		fixhdr->rem_buf = buf + 2;
	}

	return 0;
}

//--------------------------------------------
void mqttsn_flags_decode(mqttsn_flags_t *flags, uint8_t *byte)
{
	flags->topic_id_type = (mqttsn_topic_id_type_t)(*byte & 0x03);
	flags->clean_session = *byte & 0x04 ? 1 : 0;
	flags->will = *byte & 0x08 ? 1 : 0;
	flags->retain = *byte & 0x10 ? 1 : 0;
	flags->qos = (*byte & 0x60) >> 5;
	flags->dup = *byte & 0x80 ? 1 : 0;
}

//--------------------------------------------
void mqttsn_return_command_decode(mqttsn_fixed_header_t *fixhdr, mqttsn_return_code_t *code)
{
	*code = (mqttsn_return_code_t)fixhdr->rem_buf[0];
}

//--------------------------------------------
mqttsn_return_code_t mqttsn_connect_decode(mqttsn_fixed_header_t *fixhdr, mqttsn_connect_header_t *connect)
{
//	uint32_t len;

	mqttsn_flags_decode(&connect->flags, &fixhdr->rem_buf[0]);
	connect->protocol_id = fixhdr->rem_buf[1];
	if (connect->protocol_id != 1)
		return MQTTSN_REFUSED_NOT_SUPPORTED;
	connect->duration = ntohs(*((uint16_t *)&fixhdr->rem_buf[2]));
	connect->client_id_length = fixhdr->rem_len - 4;
	if (connect->client_id_length == 0)
		return MQTTSN_REFUSED_NOT_SUPPORTED;
	connect->client_id = &fixhdr->rem_buf[4];

#if 0
	// The first UTF-encoded string. The client identifier (Client ID) is between 1 and 23
	// characters long, and uniquely identifies the client to the server.
	len = utf8_strlen(connect->client_id, connect->client_id_length);
	if (len > 23)
		return MQTTSN_REFUSED_NOT_SUPPORTED;
#endif

	return MQTTSN_ACCEPTED;
}

//--------------------------------------------
void mqttsn_willtopic_decode(mqttsn_fixed_header_t *fixhdr, mqttsn_willtopic_header_t *willtopic)
{
	// An empty WILLTOPIC message is a WILLTOPIC message without Flags andWillTopic field (i.e. it is exactly 2 octets long).
	// It is used by a client to delete the Will topic and the Will message stored in the broker, see Section 6.4.
	if (fixhdr->rem_len == 0)
	{
		willtopic->will_topic_length = 0;
		willtopic->will_topic = NULL;
		return;
	}

	mqttsn_flags_decode(&willtopic->flags, &fixhdr->rem_buf[0]);
	willtopic->will_topic_length = fixhdr->rem_len - 1;
	willtopic->will_topic = &fixhdr->rem_buf[1];
}

//--------------------------------------------
void mqttsn_willmsg_decode(mqttsn_fixed_header_t *fixhdr, mqttsn_willmsg_header_t *willmsg)
{
	willmsg->will_msg_length = fixhdr->rem_len;
	willmsg->will_msg = &fixhdr->rem_buf[0];
}

//--------------------------------------------
mqttsn_return_code_t mqttsn_register_decode(mqttsn_fixed_header_t *fixhdr, mqttsn_register_header_t *registerh)
{
	registerh->topic_id = ntohs(*((uint16_t *)&fixhdr->rem_buf[0]));
	registerh->msg_id = ntohs(*((uint16_t *)&fixhdr->rem_buf[2]));
	registerh->name_length = fixhdr->rem_len - 4;
	registerh->name = &fixhdr->rem_buf[4];
	if (registerh->name_length != 0)
	{
		if (name_has_wildcard(registerh->name, registerh->name_length) == 1)
		{
			dprintf("wildcard pubtopic, topic_id:0, msg_id:%d\n", registerh->msg_id);
			return MQTTSN_REFUSED_NOT_SUPPORTED;
		}
		if (registerh->pub_list != NULL)
			// server only
			registerh->pub_item = list_pub_add_ignore(registerh->pub_list, registerh->name, registerh->name_length, NULL, 0, 0);
		dprintf("pubtopic:%.*s, topic_id:%d, msg_id:%d\n", registerh->name_length, registerh->name, registerh->topic_id, registerh->msg_id);
		return MQTTSN_ACCEPTED;
	}
	else
	{
		dprintf("no pubtopic, topic_id:0, msg_id:%d\n", registerh->msg_id);
		return MQTTSN_REFUSED_NOT_SUPPORTED;
	}
}

//--------------------------------------------
void mqttsn_xxxack_decode(mqttsn_fixed_header_t *fixhdr, mqttsn_xxxack_header_t *xxxack)
{
	xxxack->topic_id = ntohs(*((uint16_t *)&fixhdr->rem_buf[0]));
	xxxack->msg_id = ntohs(*((uint16_t *)&fixhdr->rem_buf[2]));
	xxxack->return_code = (mqttsn_return_code_t)fixhdr->rem_buf[4];
}

//--------------------------------------------
mqttsn_return_code_t mqttsn_publish_decode(mqttsn_fixed_header_t *fixhdr, mqttsn_publish_header_t *publish)
{
#ifdef SENSOR_DATA
	char *dec_buf;
	size_t dec_size;
#endif

	mqttsn_flags_decode(&publish->flags, &fixhdr->rem_buf[0]);
	publish->topic_id = ntohs(*((uint16_t *)&fixhdr->rem_buf[1]));
	publish->msg_id = ntohs(*((uint16_t *)&fixhdr->rem_buf[3]));

#ifdef SENSOR_DATA
	decode_mqttsn_sensor_data(&dec_buf, &dec_size, &fixhdr->rem_buf[5]);
	publish->data_length = (uint16_t)dec_size;
	publish->data = (uint8_t *)dec_buf;
#else
	publish->data_length = fixhdr->rem_len - 5;
	publish->data = &fixhdr->rem_buf[5];
#endif

	if (publish->flags.topic_id_type == MQTTSN_TOPIC_NAME || publish->flags.topic_id_type == MQTTSN_PREDEF_TOPIC_ID)
	{
		if (publish->pub_list != NULL)
		{
			// server only
			publish->pub_item = list_pub_find_topic_id(publish->pub_list, publish->topic_id);
			if (publish->pub_item == NULL)
			{
				dprintf("invalid pubtopic, topic_id:%d, qos:%d, msg_id:%d\n", publish->topic_id, publish->flags.qos, publish->msg_id);
				return MQTTSN_REFUSED_INVALID_TOPIC_ID;
			}
			list_pub_data_replace(publish->pub_item, publish->data, publish->data_length, publish->data_length == 0 ? 0 : publish->flags.retain);

#if defined SENSOR_DATA && defined SENSOR_DATA_MYSQL
			if (mysql_enable == 1)
				parse_mqttsn_topic_name_to_mysql_query(publish->pub_item->name, publish->pub_item->name_len, &fixhdr->rem_buf[5]);
#endif

		}
		dprintf("retain:%d, data:%.*s, qos:%d, topic_id:%d, msg_id:%d\n", publish->flags.retain, publish->data_length, publish->data, publish->flags.qos, publish->topic_id, publish->msg_id);
		return MQTTSN_ACCEPTED;
	}
	return MQTTSN_REFUSED_NOT_SUPPORTED;
}

//--------------------------------------------
void mqttsn_pubxxx_decode(mqttsn_fixed_header_t *fixhdr, uint16_t *msg_id)
{
	*msg_id = ntohs(*((uint16_t *)(&fixhdr->rem_buf[0])));
}

//--------------------------------------------
mqttsn_return_code_t mqttsn_subscribe_decode(mqttsn_fixed_header_t *fixhdr, mqttsn_subscribe_header_t *subscribe)
{
	mqttsn_flags_decode(&subscribe->flags, &fixhdr->rem_buf[0]);
	subscribe->msg_id = ntohs(*((uint16_t *)&fixhdr->rem_buf[1]));
	if (subscribe->flags.topic_id_type == MQTTSN_TOPIC_NAME)
	{
		subscribe->name_length = fixhdr->rem_len - 3;
		subscribe->name = &fixhdr->rem_buf[3];
		subscribe->sub_item = list_sub_add_replace(subscribe->sub_list, subscribe->name, subscribe->name_length, subscribe->flags.qos);
		if (list_sub_has_wildcard(subscribe->sub_item) == 0)
			subscribe->pub_item = list_pub_add_ignore(subscribe->pub_list, subscribe->name, subscribe->name_length, NULL, 0, 0);
#if 0
		// maybe it's not needed
		else
			subscribe->topic_id = 0;
#endif
	}
	if (subscribe->flags.topic_id_type == MQTTSN_PREDEF_TOPIC_ID)
	{
		subscribe->topic_id = ntohs(*((unsigned short *)&fixhdr->rem_buf[3]));
		subscribe->pub_item = list_pub_find_topic_id(subscribe->pub_list, subscribe->topic_id);
		if (subscribe->pub_item == NULL)
		{
			dprintf("invalid subtopic, topic_id:%d, qos:%d, msg_id:%d\n", subscribe->topic_id, subscribe->flags.qos, subscribe->msg_id);
			return MQTTSN_REFUSED_INVALID_TOPIC_ID;
		}
		subscribe->sub_item = list_sub_add_replace(subscribe->sub_list, subscribe->pub_item->name, subscribe->pub_item->name_len, subscribe->flags.qos);
	}
	if (subscribe->flags.topic_id_type == MQTTSN_SHORT_TOPIC_NAME)
		return MQTTSN_REFUSED_NOT_SUPPORTED;
	dprintf("subtopic:%.*s, topic_id:%d, qos:%d, msg_id:%d\n", subscribe->sub_item->name_len, subscribe->sub_item->name, subscribe->topic_id, subscribe->flags.qos, subscribe->msg_id);
	return MQTTSN_ACCEPTED;
}

//--------------------------------------------
mqttsn_return_code_t mqttsn_unsubscribe_decode(mqttsn_fixed_header_t *fixhdr, mqttsn_unsubscribe_header_t *unsubscribe)
{
	mqttsn_flags_decode(&unsubscribe->flags, &fixhdr->rem_buf[0]);
	unsubscribe->msg_id = ntohs(*((uint16_t *)&fixhdr->rem_buf[1]));
	if (unsubscribe->flags.topic_id_type == MQTTSN_TOPIC_NAME)
	{
		unsubscribe->name_length = fixhdr->rem_len - 3;
		unsubscribe->name = &fixhdr->rem_buf[3];
		list_sub_remove_name(unsubscribe->sub_list, unsubscribe->name, unsubscribe->name_length);
	}
	if (unsubscribe->flags.topic_id_type == MQTTSN_PREDEF_TOPIC_ID)
	{
		unsubscribe->topic_id = ntohs(*((unsigned short *)&fixhdr->rem_buf[3]));
		list_sub_remove_topic_id(unsubscribe->sub_list, unsubscribe->topic_id);
	}
	if (unsubscribe->flags.topic_id_type == MQTTSN_SHORT_TOPIC_NAME)
		return MQTTSN_REFUSED_NOT_SUPPORTED;
	return MQTTSN_ACCEPTED;
}

//--------------------------------------------
void mqttsn_suback_decode(mqttsn_fixed_header_t *fixhdr, mqttsn_suback_header_t *suback)
{
	mqttsn_flags_decode(&suback->flags, &fixhdr->rem_buf[0]);
	suback->topic_id = ntohs(*((uint16_t *)&fixhdr->rem_buf[1]));
	suback->msg_id = ntohs(*((uint16_t *)&fixhdr->rem_buf[3]));
	suback->return_code = (mqttsn_return_code_t)fixhdr->rem_buf[5];
}

//--------------------------------------------
void mqttsn_pingreq_decode(mqttsn_fixed_header_t *fixhdr, mqttsn_pingreq_header_t *pingreq)
{
	pingreq->client_id_length = fixhdr->rem_len - 2;
	pingreq->client_id = &fixhdr->rem_buf[2];
}

//--------------------------------------------
void mqttsn_disconnect_decode(mqttsn_fixed_header_t *fixhdr, uint16_t *duration)
{
	if (fixhdr->rem_len == 0)
		*duration = 0;
	else
		*duration = ntohs(*((uint16_t *)&fixhdr->rem_buf[0]));
	dprintf("duration:%d\n", *duration);
}





//--------------------------------------------
//** encode command functions

//--------------------------------------------
void mqttsn_flags_encode(mqttsn_flags_t *flags, uint8_t *byte)
{
	*byte = (flags->dup << 7) +
		(flags->qos << 5) +
		(flags->retain << 4) +
		(flags->will << 3) +
		(flags->clean_session << 2) +
		(flags->topic_id_type);
}

//--------------------------------------------
void mqttsn_simple_command_encode(mqttsn_msg_type_t msg_type, uint8_t **buf, size_t *size)
{
	*size = 2;
	*buf = (uint8_t *)malloc(*size);

	*(*buf) = (uint8_t)*size;
	*(*buf + 1) = msg_type;
}

//--------------------------------------------
void mqttsn_return_command_encode(mqttsn_msg_type_t msg_type, uint8_t **buf, size_t *size, mqttsn_return_code_t code)
{
	*size = 3;
	*buf = (uint8_t *)malloc(*size);

	*(*buf) = (uint8_t)*size;
	*(*buf + 1) = msg_type;
	*(*buf + 2) = (uint8_t)code;
}

//--------------------------------------------
void mqttsn_connect_encode(uint8_t **buf, size_t *size, mqttsn_connect_header_t *connect)
{
	*size = 6 + connect->client_id_length;
	*buf = (uint8_t *)malloc(*size);

	*(*buf) = (uint8_t)*size;
	*(*buf + 1) = MQTTSN_CONNECT;
	mqttsn_flags_encode(&connect->flags, (*buf + 2));
	*(*buf + 3) = 1;
	*(uint16_t *)(*buf + 4) = htons(connect->duration);
	if (connect->client_id_length > 0)
		memcpy(*buf + 6, connect->client_id, connect->client_id_length);
}

//--------------------------------------------
void mqttsn_willtopicxxx_encode(mqttsn_msg_type_t msg_type, uint8_t **buf, size_t *size, mqttsn_willtopic_header_t *willtopic)
{
	if (willtopic->will_topic_length == 0)
		*size = 2;
	else
		*size = 3 + willtopic->will_topic_length;
	*buf = (uint8_t *)malloc(*size);

	*(*buf) = (uint8_t)*size;
	*(*buf + 1) = msg_type;

	if (willtopic->will_topic_length > 0)
	{
		mqttsn_flags_encode(&willtopic->flags, (*buf + 2));
		memcpy(*buf + 3, willtopic->will_topic, willtopic->will_topic_length);
	}
}

//--------------------------------------------
void mqttsn_willmsgxxx_encode(mqttsn_msg_type_t msg_type, uint8_t **buf, size_t *size, mqttsn_willmsg_header_t *willmsg)
{
	*size = 2 + willmsg->will_msg_length;
	*buf = (uint8_t *)malloc(*size);

	*(*buf) = (uint8_t)*size;
	*(*buf + 1) = msg_type;
	memcpy(*buf + 2, willmsg->will_msg, willmsg->will_msg_length);
}

//--------------------------------------------
void mqttsn_register_encode(uint8_t **buf, size_t *size, mqttsn_register_header_t *registerh)
{
	if (registerh->pub_item != NULL)
	{
		registerh->topic_id = registerh->pub_item->topic_id;
		registerh->name = registerh->pub_item->name; 
		registerh->name_length = (uint16_t)registerh->pub_item->name_len;
	}

	*size = 6 + registerh->name_length;
	*buf = (uint8_t *)malloc(*size);

	*(*buf) = (uint8_t)*size;
	*(*buf + 1) = MQTTSN_REGISTER;
	*(uint16_t *)(*buf + 2) = htons(registerh->topic_id);
	*(uint16_t *)(*buf + 4) = htons(registerh->msg_id);
	memcpy(*buf + 6, registerh->name, registerh->name_length);
}

//--------------------------------------------
void mqttsn_xxxack_encode(mqttsn_msg_type_t msg_type, uint8_t **buf, size_t *size, mqttsn_xxxack_header_t *xxxack)
{
	*size = 7;
	*buf = (uint8_t *)malloc(*size);

	*(*buf) = (uint8_t)*size;
	*(*buf + 1) = msg_type;
	*(uint16_t *)(*buf + 2) = htons(xxxack->topic_id);
	*(uint16_t *)(*buf + 4) = htons(xxxack->msg_id);
	*(*buf + 6) = xxxack->return_code;
}

//--------------------------------------------
void mqttsn_publish_encode(uint8_t **buf, size_t *size, mqttsn_publish_header_t *publish)
{
#ifdef SENSOR_DATA
	uint8_t *en_buf;
	size_t en_size;
#endif

	if (publish->pub_item != NULL)
	{
		publish->topic_id = publish->pub_item->topic_id;
		publish->data = publish->pub_item->data;
		publish->data_length = publish->pub_item->data_len;
		publish->flags.retain = publish->pub_item->retain;
	}

#ifdef SENSOR_DATA
	encode_mqttsn_sensor_data(&en_buf, &en_size, (const char *)publish->data, publish->data_length);
	*size = 7 + en_size;
#else
	*size = 7 + publish->data_length;
#endif

	*buf = (uint8_t *)malloc(*size);

	*(*buf) = (uint8_t)*size;
	*(*buf + 1) = MQTTSN_PUBLISH;
	mqttsn_flags_encode(&publish->flags, (*buf + 2));
	*(uint16_t *)(*buf + 3) = htons(publish->topic_id);
	*(uint16_t *)(*buf + 5) = htons(publish->msg_id);

#ifdef SENSOR_DATA
	memcpy((*buf + 7), en_buf, en_size);
	free(en_buf);
#else
	memcpy((*buf + 7), publish->data, publish->data_length);
#endif
}

//--------------------------------------------
void mqttsn_pubxxx_encode(mqttsn_msg_type_t msg_type, uint8_t **buf, size_t *size, uint16_t msg_id)
{
	*size = 4;
	*buf = (uint8_t *)malloc(*size);

	*(*buf) = (uint8_t)*size;
	*(*buf + 1) = msg_type;
	*(uint16_t *)(*buf + 2) = htons(msg_id);
}

//--------------------------------------------
void mqttsn_xxsubscribe_encode(mqttsn_msg_type_t msg_type, uint8_t **buf, size_t *size, mqttsn_xxsubscribe_header_t *xxsubscribe)
{
	*size = 5 + (xxsubscribe->flags.topic_id_type == MQTTSN_TOPIC_NAME ? xxsubscribe->name_length : 2);
	*buf = (uint8_t *)malloc(*size);

	*(*buf) = (uint8_t)*size;
	*(*buf + 1) = msg_type;
	mqttsn_flags_encode(&xxsubscribe->flags, (*buf + 2));
	*(uint16_t *)(*buf + 3) = htons(xxsubscribe->msg_id);
	if (xxsubscribe->flags.topic_id_type == MQTTSN_TOPIC_NAME)
		memcpy((*buf + 5), xxsubscribe->name, xxsubscribe->name_length);
	else
		*(uint16_t *)(*buf + 5) = htons(xxsubscribe->topic_id);
}

//--------------------------------------------
void mqttsn_suback_encode(uint8_t **buf, size_t *size, mqttsn_suback_header_t *suback)
{
	*size = 8;
	*buf = (uint8_t *)malloc(*size);

	*(*buf) = (uint8_t)*size;
	*(*buf + 1) = MQTTSN_SUBACK;
	mqttsn_flags_encode(&suback->flags, (*buf + 2));
	*(uint16_t *)(*buf + 3) = htons(suback->topic_id);
	*(uint16_t *)(*buf + 5) = htons(suback->msg_id);
	*(*buf + 7) = suback->return_code;
}

//--------------------------------------------
void mqttsn_pingreq_encode(uint8_t **buf, size_t *size, mqttsn_pingreq_header_t *pingreq)
{
	*size = 2 + pingreq->client_id_length;
	*buf = (uint8_t *)malloc(*size);

	*(*buf) = (uint8_t)*size;
	*(*buf + 1) = MQTTSN_PINGREQ;
	if (pingreq->client_id_length != 0)
		memcpy((*buf + 2), pingreq->client_id, pingreq->client_id_length);
}

//--------------------------------------------
void mqttsn_disconnect_encode(uint8_t **buf, size_t *size, uint16_t duration)
{
	if (duration == 0)
		*size = 2;
	else
		*size = 4;

	*buf = (uint8_t *)malloc(*size);

	*(*buf) = (uint8_t)*size;
	*(*buf + 1) = MQTTSN_DISCONNECT;

	if (duration > 0)
		*(uint16_t *)(*buf + 2) = htons(duration);
}

