/*
* Copyright (c) 2013-2015, 2018 Vladimir Alemasov
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

#include <stdlib.h>		/* malloc */
#include <string.h>		/* memcpy, strlen */
#include <assert.h>		/* assert */
#include "config.h"
#include "mqtt.h"
#ifdef MQTT_DATA_MYSQL
#include "sensor_data.h"
#endif
#include "utf8.h"

#ifdef WIN32 /* Windows */
#include <winsock2.h>	/* ntohs */
#else /* Linux, OpenWRT */
#include <arpa/inet.h>	/* ntohs */
#endif

#ifndef NDPRINTF
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


//--------------------------------------------
int mqtt_packet_check_length(unsigned char *buf, size_t size, uint32_t *len, unsigned char **rem_buf)
{
	uint8_t digit;
	uint8_t cnt = 1;
	uint32_t mult = 1;

	if (size < 2)
		return -1;

	*len = 0;

	do
	{
		if (cnt >= size)
			return -1;
		if (cnt > 4)
			return -2;
		digit = buf[cnt];
		*len += (digit & 127) * mult;
		mult *= 128;
		++cnt;
	}
	while ((digit & 128) != 0);

	*rem_buf = buf + cnt;
	return 0;
}

//--------------------------------------------
int mqtt_fixed_header_decode(mqtt_fixed_header_t *fixhdr, unsigned char *buf, size_t size)
{
	if (mqtt_packet_check_length(buf, size, &fixhdr->rem_len, &fixhdr->rem_buf) < 0)
		return -1;

	fixhdr->msg_type = (buf[0] & 0xF0) >> 4;
	fixhdr->dup_flag = buf[0] & 0x08 ? 1 : 0;
	fixhdr->qos_level = (buf[0] & 0x06) >> 1;
	fixhdr->retain = buf[0] & 0x01;

	return 0;
}



//--------------------------------------------
//** decode command functions

//--------------------------------------------
mqtt_connack_return_code_t mqtt_connect_decode(mqtt_fixed_header_t *fixhdr, mqtt_connect_header_t *connect)
{
	// MQTT protocol and version    0     6     M     Q     I     s     d     p     3
	unsigned char proto_id[9] = {0x00, 0x06, 0x4D, 0x51, 0x49, 0x73, 0x64, 0x70, 0x03};
	uint32_t len;

	if (memcmp(&fixhdr->rem_buf[0], proto_id, sizeof(proto_id) != 0))
		return MQTT_REFUSED_PROTOCOL_VERSION;

	// clean session flag = 0 is not supported, every session is new
	connect->flags.clean_session = fixhdr->rem_buf[9] & 0x02 ? 1 : 0;
	connect->flags.will = fixhdr->rem_buf[9] & 0x04 ? 1 : 0;
	connect->flags.will_qos = (fixhdr->rem_buf[9] & 0x18) >> 3;
	connect->flags.will_retain = fixhdr->rem_buf[9] & 0x20 ? 1 : 0;
	connect->flags.password = fixhdr->rem_buf[9] & 0x40 ? 1 : 0;
	connect->flags.user_name = fixhdr->rem_buf[9] & 0x80 ? 1 : 0;

	connect->keep_alive_timer = ntohs(*((unsigned short *)&fixhdr->rem_buf[10]));

	connect->client_id_length = ntohs(*((unsigned short *)&fixhdr->rem_buf[12]));
	if (connect->client_id_length == 0)
		return MQTT_REFUSED_IDENTIFIER_REJECTED;
	connect->client_id = (char *)&fixhdr->rem_buf[14];

	// The first UTF-encoded string. The client identifier (Client ID) is between 1 and 23
	// characters long, and uniquely identifies the client to the server.
	len = utf8_strlen(connect->client_id, connect->client_id_length);
	if (len > 23)
		return MQTT_REFUSED_IDENTIFIER_REJECTED;

	len = 14 + connect->client_id_length;
	assert(len <= fixhdr->rem_len);

	if (connect->flags.will == 1)
	{
		connect->will_topic_length = ntohs(*((unsigned short *)&fixhdr->rem_buf[len]));
		assert(connect->will_topic_length > 0);
		connect->will_topic = (char *)&fixhdr->rem_buf[len + 2];
		len += 2 + connect->will_topic_length;

		connect->will_message_length = ntohs(*((unsigned short *)&fixhdr->rem_buf[len]));
		assert(connect->will_message_length > 0);
		connect->will_message = (char *)&fixhdr->rem_buf[len + 2];
		len += 2 + connect->will_message_length;
	}

	if (connect->flags.user_name == 1 && len < fixhdr->rem_len)
	{
		connect->user_name_length = ntohs(*((unsigned short *)&fixhdr->rem_buf[len]));
		assert(connect->user_name_length > 0);
		connect->user_name = (char *)&fixhdr->rem_buf[len + 2];
		len += 2 + connect->user_name_length;
	}

	if (connect->flags.password == 1 && len < fixhdr->rem_len)
	{
		connect->password_length = ntohs(*((unsigned short *)&fixhdr->rem_buf[len]));
		assert(connect->password_length > 0);
		connect->password = (char *)&fixhdr->rem_buf[len + 2];
		len += 2 + connect->password_length;
	}

	if (*connect->mqtt_users != NULL)
	{
		list_mqtt_user_t *lmu;

		if (connect->user_name_length == 0 || connect->password_length == 0)
			return MQTT_REFUSED_BAD_USER_NAME_PASSWORD;
		if ((lmu = list_mqtt_user_find_user(connect->mqtt_users, connect->user_name, connect->user_name_length, connect->password, connect->password_length)) != NULL)
			connect->publish_enable = lmu->publish_enable;
		else
			return MQTT_REFUSED_BAD_USER_NAME_PASSWORD;
	}
	else
		connect->publish_enable = 1;

	assert(len == fixhdr->rem_len);

	return MQTT_ACCEPTED;
}

//--------------------------------------------
void mqtt_connack_decode(mqtt_fixed_header_t *fixhdr, mqtt_connack_return_code_t *code)
{
	*code = fixhdr->rem_buf[1];
}

//--------------------------------------------
void mqtt_subscribe_decode(mqtt_fixed_header_t *fixhdr, mqtt_subscribe_header_t *subscribe)
{
	size_t len;

	subscribe->qos_size = 0;
	subscribe->msg_id = ntohs(*((unsigned short *)&fixhdr->rem_buf[0]));

	for (len = 0; len < fixhdr->rem_len - 2;)
	{
		uint16_t name_len = ntohs(*((unsigned short *)(&fixhdr->rem_buf[2] + len)));
		uint8_t *name = fixhdr->rem_buf + 2 + len + 2;
		uint8_t qos = *(fixhdr->rem_buf + 2 + len + 2 + name_len);
		list_sub_add_replace(subscribe->sub_list, name, name_len, qos/*, &changed*/);
		list_sub_add_replace(&subscribe->sub_item, name, name_len, qos/*, &changed*/);
		dprintf("subtopic:%.*s, qos:%d\n", name_len, name, qos);
		subscribe->qos_size++;
		len += name_len + 3;
	}

	assert(2 + len == fixhdr->rem_len);
}

//--------------------------------------------
void mqtt_suback_decode(mqtt_fixed_header_t *fixhdr, mqtt_suback_header_t *suback)
{
	size_t len;

	suback->msg_id = ntohs(*((unsigned short *)&fixhdr->rem_buf[0]));
	suback->qos_size = fixhdr->rem_len - 2;

	for (len = 0; len < fixhdr->rem_len - 2;)
	{
		suback->qos_buf[len] = *((uint8_t *)(&fixhdr->rem_buf[2] + len));
		len += 1;
	}

	assert(2 + len == fixhdr->rem_len);
}

//--------------------------------------------
void mqtt_unsubscribe_decode(mqtt_fixed_header_t *fixhdr, mqtt_unsubscribe_header_t *unsubscribe)
{
	size_t len;

	unsubscribe->msg_id = ntohs(*((unsigned short *)&fixhdr->rem_buf[0]));

	for (len = 0; len < fixhdr->rem_len - 2;)
	{
		uint16_t name_len = ntohs(*((unsigned short *)(&fixhdr->rem_buf[2] + len)));
		uint8_t *name = fixhdr->rem_buf + 2 + len + 2;
		list_sub_remove_name(unsubscribe->sub_list, name, name_len);
		dprintf("subtopic:%.*s\n", name_len, name);
		len += name_len + 2;
	}

	assert(2 + len == fixhdr->rem_len);
}

//--------------------------------------------
void mqtt_publish_decode(mqtt_fixed_header_t *fixhdr, mqtt_publish_header_t *publish)
{
	uint16_t msg_id_len = fixhdr->qos_level > 0 ? 2 : 0;
	uint16_t name_len = ntohs(*((unsigned short *)&fixhdr->rem_buf[0]));
	uint16_t data_len = fixhdr->rem_len - name_len - 2 - msg_id_len;
	uint8_t *name = fixhdr->rem_buf + 2;
	uint8_t *data = fixhdr->rem_buf + 2 + name_len + msg_id_len;

	publish->pub_item = list_pub_add_replace(publish->pub_list, name, name_len, data, data_len, data_len == 0 ? 0 : fixhdr->retain);

#if defined MQTT_DATA_MYSQL
	parse_mqtt_topic_name_to_mysql_query(name, name_len, data, data_len);
#endif

	dprintf("pubtopic:%.*s, retain:%d, data:%.*s, qos:%d\n", name_len, name, publish->pub_item->retain, data_len, data, fixhdr->qos_level);
	if (fixhdr->qos_level > 0)
		publish->msg_id = ntohs(*((unsigned short *)(&fixhdr->rem_buf[2] + name_len)));
	publish->qos = fixhdr->qos_level;
}

//--------------------------------------------
void mqtt_pubxxx_decode(mqtt_fixed_header_t *fixhdr, mqtt_pubxxx_header_t *pubxxx)
{
	pubxxx->msg_id = ntohs(*((unsigned short *)(&fixhdr->rem_buf[0])));
}



//--------------------------------------------
//** encode command functions

//--------------------------------------------
void mqtt_packet_encode(mqtt_fixed_header_t *fixhdr, unsigned char **buf, size_t *size)
{
	uint8_t digit[4];
	uint8_t cnt = 0;
	uint32_t len = fixhdr->rem_len;

	do
	{
		digit[cnt] = len % 128;
		len = len / 128;
		// if there are more digits to encode, set the top bit of this digit
		if (len > 0)
			digit[cnt] = digit[cnt] | 0x80;
		++cnt;
	}
	while (len > 0);

	*size = 1 + cnt + fixhdr->rem_len;
	*buf = (unsigned char *)malloc(*size);
	*buf[0] = (fixhdr->msg_type << 4) + (fixhdr->dup_flag << 3) + (fixhdr->qos_level << 1) + fixhdr->retain;
	memcpy(*buf + 1, (uint8_t *)&digit, cnt);
	if (fixhdr->rem_len > 0)
	{
		memcpy(*buf + 1 + cnt, fixhdr->rem_buf, fixhdr->rem_len);
		free(fixhdr->rem_buf);
	}
}

//--------------------------------------------
void mqtt_connect_encode(unsigned char **buf, size_t *size, mqtt_connect_header_t *connect)
{
	mqtt_fixed_header_t fixhdr;
	// MQTT protocol and version    0     6     M     Q     I     s     d     p     3
	unsigned char proto_id[9] = {0x00, 0x06, 0x4D, 0x51, 0x49, 0x73, 0x64, 0x70, 0x03};
	uint16_t client_id_length = connect->client_id_length;
	uint16_t will_topic_length = connect->will_topic_length;
	uint16_t will_message_length = connect->will_message_length;
	uint16_t user_name_length = connect->user_name_length;
	uint16_t password_length = connect->password_length;
	uint8_t *client_id = connect->client_id;
	uint8_t *will_topic = connect->will_topic;
	uint8_t *will_message = connect->will_message;
	uint8_t *user_name = connect->user_name;
	uint8_t *password = connect->password;
	uint8_t *rem_buf;

	fixhdr.msg_type = MQTT_CONNECT;
	fixhdr.dup_flag = 0;
	fixhdr.qos_level = 0;
	fixhdr.retain = 0;
	fixhdr.rem_len = (uint32_t)(sizeof(proto_id) + 1 + 2 + 
		(client_id_length ? (2 + client_id_length) : 0) +
		((connect->flags.will && will_topic_length) ? (2 + will_topic_length) : 0) +
		((connect->flags.will && will_message_length) ? (2 + will_message_length) : 0) +
		((connect->flags.user_name && user_name_length) ? (2 + user_name_length) : 0) +
		((connect->flags.password && password_length) ? (2 + password_length) : 0));
	fixhdr.rem_buf = (unsigned char *)malloc(fixhdr.rem_len);
	rem_buf = fixhdr.rem_buf;
	memcpy(rem_buf, proto_id, sizeof(proto_id));
	rem_buf += sizeof(proto_id);
	*rem_buf = (connect->flags.clean_session << 1) |
		(connect->flags.will << 2) |
		(connect->flags.will_qos << 3) |
		(connect->flags.will_retain << 5) |
		(connect->flags.password << 6) |
		(connect->flags.user_name << 7);
	rem_buf += 1;
	*(uint16_t *)rem_buf = htons(connect->keep_alive_timer);
	rem_buf += 2;
	if (client_id_length)
	{
		*(uint16_t *)rem_buf = htons(client_id_length);
		rem_buf += 2;
		memcpy(rem_buf, client_id, client_id_length);
		rem_buf += client_id_length;
	}
	if (connect->flags.will && will_topic_length)
	{
		*(uint16_t *)rem_buf = htons(will_topic_length);
		rem_buf += 2;
		memcpy(rem_buf, will_topic, will_topic_length);
		rem_buf += will_topic_length;
	}
	if (connect->flags.will && will_message_length)
	{
		*(uint16_t *)rem_buf = htons(will_message_length);
		rem_buf += 2;
		memcpy(rem_buf, will_message, will_message_length);
		rem_buf += will_message_length;
	}
	if (connect->flags.user_name && user_name_length)
	{
		*(uint16_t *)rem_buf = htons(user_name_length);
		rem_buf += 2;
		memcpy(rem_buf, user_name, user_name_length);
		rem_buf += user_name_length;
	}
	if (connect->flags.password && password_length)
	{
		*(uint16_t *)rem_buf = htons(password_length);
		rem_buf += 2;
		memcpy(rem_buf, password, password_length);
		rem_buf += password_length;
	}

	mqtt_packet_encode(&fixhdr, buf, size);
}

//--------------------------------------------
void mqtt_connack_encode(unsigned char **buf, size_t *size, mqtt_connack_return_code_t code)
{
	mqtt_fixed_header_t fixhdr;

	fixhdr.msg_type = MQTT_CONNACK;
	fixhdr.dup_flag = 0;
	fixhdr.qos_level = 0;
	fixhdr.retain = 0;
	fixhdr.rem_len = 2;
	fixhdr.rem_buf = (unsigned char *)malloc(fixhdr.rem_len);
	fixhdr.rem_buf[0] = 0; // unused
	fixhdr.rem_buf[1] = code;

	mqtt_packet_encode(&fixhdr, buf, size);
}

//--------------------------------------------
void mqtt_suback_encode(unsigned char **buf, size_t *size, mqtt_suback_header_t *suback)
{
	mqtt_fixed_header_t fixhdr;

	fixhdr.msg_type = MQTT_SUBACK;
	fixhdr.dup_flag = 0;
	fixhdr.qos_level = 0;
	fixhdr.retain = 0;
	fixhdr.rem_len = (uint32_t)(2 + suback->qos_size);
	fixhdr.rem_buf = (unsigned char *)malloc(fixhdr.rem_len);
	*(uint16_t *)&fixhdr.rem_buf[0] = htons(suback->msg_id);
	memcpy(&fixhdr.rem_buf[2], suback->qos_buf, suback->qos_size);

	mqtt_packet_encode(&fixhdr, buf, size);
}

//--------------------------------------------
void mqtt_pingreq_encode(unsigned char **buf, size_t *size)
{
	mqtt_fixed_header_t fixhdr;

	fixhdr.msg_type = MQTT_PINGREQ;
	fixhdr.dup_flag = 0;
	fixhdr.qos_level = 0;
	fixhdr.retain = 0;
	fixhdr.rem_len = 0;

	mqtt_packet_encode(&fixhdr, buf, size);
}

//--------------------------------------------
void mqtt_pingresp_encode(unsigned char **buf, size_t *size)
{
	mqtt_fixed_header_t fixhdr;

	fixhdr.msg_type = MQTT_PINGRESP;
	fixhdr.dup_flag = 0;
	fixhdr.qos_level = 0;
	fixhdr.retain = 0;
	fixhdr.rem_len = 0;

	mqtt_packet_encode(&fixhdr, buf, size);
}

//--------------------------------------------
void mqtt_publish_encode(unsigned char **buf, size_t *size, mqtt_publish_header_t *publish)
{
	mqtt_fixed_header_t fixhdr;
	uint16_t name_len = publish->pub_item->name_len;
	uint16_t data_len = publish->pub_item->data_len;
	uint8_t *name = publish->pub_item->name;
	uint8_t *data = publish->pub_item->data;
	uint16_t msg_id_len = publish->qos > 0 ? 2 : 0;

	fixhdr.msg_type = MQTT_PUBLISH;
	fixhdr.dup_flag = 0;
	fixhdr.qos_level = publish->qos;
	fixhdr.retain = publish->pub_item->retain;
	fixhdr.rem_len = (uint32_t)(2 + name_len + msg_id_len + data_len);
	fixhdr.rem_buf = (uint8_t *)malloc(fixhdr.rem_len);
	*(uint16_t *)&fixhdr.rem_buf[0] = htons(name_len);
	memcpy(&fixhdr.rem_buf[2], name, name_len);
	if (fixhdr.qos_level > 0)
		*(uint16_t *)&fixhdr.rem_buf[2 + name_len] = htons(publish->msg_id);
	memcpy(&fixhdr.rem_buf[2 + name_len + msg_id_len], data, data_len);

	mqtt_packet_encode(&fixhdr, buf, size);
}

//--------------------------------------------
void mqtt_pubxxx_encode(mqtt_msg_type_t msg_type, unsigned char **buf, size_t *size, uint16_t msg_id)
{
	mqtt_fixed_header_t fixhdr;

	fixhdr.msg_type = msg_type;
	fixhdr.dup_flag = 0;
	if (msg_type == MQTT_PUBREL)
		fixhdr.qos_level = 1;
	else
		fixhdr.qos_level = 0;
	fixhdr.retain = 0;
	fixhdr.rem_len = 2;
	fixhdr.rem_buf = (unsigned char *)malloc(fixhdr.rem_len);
	*(uint16_t *)&fixhdr.rem_buf[0] = htons(msg_id);

	mqtt_packet_encode(&fixhdr, buf, size);
}

//--------------------------------------------
void mqtt_subscribe_encode(unsigned char **buf, size_t *size, mqtt_subscribe_header_t *subscribe)
{
	mqtt_fixed_header_t fixhdr;
	uint16_t name_len = subscribe->sub_item->name_len;
	uint8_t *name = subscribe->sub_item->name;

	assert(subscribe->qos_size == 1);

	fixhdr.msg_type = MQTT_SUBSCRIBE;
	fixhdr.dup_flag = 0;
	fixhdr.qos_level = 1;
	fixhdr.retain = 0;
	fixhdr.rem_len = (uint32_t)(2 + 2 + name_len + 1);
	fixhdr.rem_buf = (uint8_t *)malloc(fixhdr.rem_len);
	*(uint16_t *)&fixhdr.rem_buf[0] = htons(subscribe->msg_id);
	*(uint16_t *)&fixhdr.rem_buf[2] = htons(name_len);
	memcpy(&fixhdr.rem_buf[4], name, name_len);
	fixhdr.rem_buf[2 + 2 + name_len] = subscribe->sub_item->qos;

	mqtt_packet_encode(&fixhdr, buf, size);
}

//--------------------------------------------
void mqtt_disconnect_encode(unsigned char **buf, size_t *size)
{
	mqtt_fixed_header_t fixhdr;

	fixhdr.msg_type = MQTT_DISCONNECT;
	fixhdr.dup_flag = 0;
	fixhdr.qos_level = 0;
	fixhdr.retain = 0;
	fixhdr.rem_len = 0;

	mqtt_packet_encode(&fixhdr, buf, size);
}
