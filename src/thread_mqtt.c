/*
* Copyright (c) 2013-2015 Vladimir Alemasov
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
#ifdef LINUX_DAEMON_VERSION
#include <unistd.h>
#include <syslog.h>
#endif

#include <string.h>			/* memset, memcmp, memcpy */
#include <assert.h>			/* assert */
#include <errno.h>			/* errno */
#include <stdio.h>			/* FILE etc. */
#include "mqtt.h"
#include "mqttsn.h"
#include "msg_tcp_mqtt.h"
#include "msg_mqtt_tcp.h"
#include "msg_udp_mqtt.h"
#include "msg_mqtt_udp.h"
#include "msg_trigger_rules.h"
#include "msg_rules_mqtt.h"
#include "list_mqtt_user.h"
#include "list_mqtt_conn.h"
#include "list_mqttsn_conn.h"
#include "mqtt_trigger.h"
#include "thread_state.h"
#include "thread_cron.h"
#include "thread_rules.h"
#include "parse_json.h"

#ifdef _MSC_VER
#pragma warning (disable:4996) // This function may be unsafe.
#endif

#ifndef NDEBUG
#ifdef LINUX_DAEMON_VERSION
#define dprintf(...) syslog(LOG_DEBUG, __VA_ARGS__)
#else
#define dprintf(...) printf(__VA_ARGS__)
#endif
#else
#define dprintf(...)
#endif

//--------------------------------------------
static list_mqtt_conn_t *mqtt_conns = NULL;
static list_mqttsn_conn_t *mqttsn_conns = NULL;
static list_mqtt_user_t *mqtt_users = NULL;
static list_pub_t *pub_list = NULL;
static mqtt_trigger_t *mqtt_triggers = NULL;
static uint16_t mqtt_msg_id = 1;
static uint16_t mqttsn_msg_id = 1;
static uint16_t mqttsn_topic_id = 1;
static volatile thread_state_t thread_state;


//--------------------------------------------
//** miscellaneous

//--------------------------------------------
// send buffered messages to MQTT-SN subscribers
static void mqttsn_send_buffered_msgs(list_mqttsn_conn_t *item)
{
	unsigned char *buf;
	size_t size;

	if (mqttsn_conn_register_msg_length(item) != 0)
	{
		mqttsn_register_header_t registerh = { 0 };
		list_msg_t *reg_msg_item;
		list_pub_t *pub_item;

		reg_msg_item = mqttsn_conn_register_msg_head(item);
		pub_item = list_pub_find_topic_id(&pub_list, reg_msg_item->topic_id);
		reg_msg_item->msg_id = registerh.msg_id = mqttsn_msg_id;
		mqttsn_msg_id = mqttsn_msg_id == 65535 ? 1 : ++mqttsn_msg_id;
		registerh.pub_item = pub_item;
		mqttsn_register_encode(&buf, &size, &registerh);
		dprintf("<MQTTSN_REGISTER\n");
		msg_mqtt_udp_add_packet(&item->addr, buf, size);
		return;
	}

	if (mqttsn_conn_publish_msg_length(item) != 0)
	{
		list_msg_t *pub_msg_item = mqttsn_conn_publish_msg_head(item);

		while (pub_msg_item != NULL)
		{
			mqttsn_publish_header_t publish = { 0 };
			list_pub_t *pub_item;

			pub_item = list_pub_find_topic_id(&pub_list, pub_msg_item->topic_id);
			pub_msg_item->msg_id = publish.msg_id = mqttsn_msg_id;
			mqttsn_msg_id = mqttsn_msg_id == 65535 ? 1 : ++mqttsn_msg_id;
			publish.pub_item = pub_item;
			publish.flags.qos = pub_msg_item->qos;
			publish.flags.topic_id_type = MQTTSN_PREDEF_TOPIC_ID;
			mqttsn_publish_encode(&buf, &size, &publish);
			dprintf("<MQTTSN_PUBLISH\n");
			dprintf("mqttsn conn:%s:%d, topic_id:%d, msg_id:%d\n",\
				inet_ntoa(item->addr.sin_addr),\
				(int)ntohs(item->addr.sin_port),\
				publish.topic_id,
				publish.msg_id);
			msg_mqtt_udp_add_packet(&item->addr, buf, size);
			if (pub_msg_item->qos == 0)
				pub_msg_item = mqttsn_conn_publish_msg_remove(item, pub_msg_item);
			else
				return;
		}
	}

	if (item->state == MQTTSN_CLIENT_AWAKE)
	{
		item->state = MQTTSN_CLIENT_ASLEEP;
		mqttsn_pingresp_encode(&buf, &size);
		dprintf("<MQTTSN_PINGRESP\n");
		msg_mqtt_udp_add_packet(&item->addr, buf, size);
		return;
	}

	if (item->state == MQTTSN_CLIENT_CONNECTED)
	{
		item->state = MQTTSN_CLIENT_ACTIVE;
		mqttsn_connack_encode(&buf, &size, MQTTSN_ACCEPTED);
		dprintf("<MQTTSN_CONNACK\n");
		msg_mqtt_udp_add_packet(&item->addr, buf, size);
		return;
	}
}

//--------------------------------------------
// send publish messages to subscribers
static void mqtt_publish_send(list_pub_t *pub_item)
{
	list_mqtt_conn_t *conn;
	unsigned char *buf;
	size_t size;
	int qos;

	conn = mqtt_conns;
	while (conn != NULL)
	{
		if ((qos = list_pub_sub_matches(pub_item, &conn->sub_list)) >= 0)
		{
			mqtt_publish_header_t publish = { 0 };

			dprintf("mqtt conn:%s:%d, qos:%d\n",
				inet_ntoa(conn->addr.sin_addr),
				(int)ntohs(conn->addr.sin_port),
				qos);

			publish.pub_item = pub_item;
			publish.qos = qos;
			publish.msg_id = mqtt_msg_id;
			mqtt_publish_encode(&buf, &size, &publish);
			mqtt_msg_id = mqtt_msg_id == 65535 ? 1 : ++mqtt_msg_id;
			dprintf("<MQTT_PUBLISH\n");
			msg_mqtt_tcp_add_packet(&conn->addr, buf, size);
		}
		conn = list_mqtt_conn_next(conn);
	}
}

//--------------------------------------------
// send publish messages to subscribers
static void mqttsn_publish_send(list_pub_t *pub_item)
{
	list_mqttsn_conn_t *conn;
	int qos;

	conn = mqttsn_conns;
	while (conn != NULL)
	{
		if ((qos = list_pub_sub_matches(pub_item, &conn->sub_list)) >= 0)
		{
			dprintf("mqttsn conn:%s:%d, qos:%d\n",
				inet_ntoa(conn->addr.sin_addr),
				(int)ntohs(conn->addr.sin_port),
				qos);

			if (mqttsn_conn_registered_topic_id_find(conn, pub_item->topic_id) == NULL)
				mqttsn_conn_register_msg_add(conn, pub_item, qos);
			else
				mqttsn_conn_publish_msg_add(conn, pub_item, qos);
			if (conn->state == MQTTSN_CLIENT_ACTIVE)
				mqttsn_send_buffered_msgs(conn);
		}
		conn = list_mqttsn_conn_next(conn);
	}
}

//--------------------------------------------
// send mqtt trigger message to rules thread
static void mqtt_trigger_send(list_pub_t *pub_item)
{
	mqtt_trigger_t *item;

	item = mqtt_trigger_head(&mqtt_triggers);
	while (item != NULL)
	{
		if ((pub_item->name_len == item->name_len) &&
			(memcmp(pub_item->name, item->name, pub_item->name_len) == 0))
			msg_mqtt_rules_add_packet(item);
		item = mqtt_trigger_next(item);
	}
}

//--------------------------------------------
static void mqtt_get_topic_data(list_data_t *list)
{
	list_data_t *item;
	list_data_t *next;
	list_pub_t *pub_item;

	item = list;

	while (item != NULL)
	{
		next = list_data_next(item);
		pub_item = list_pub_head(&pub_list);
		while (pub_item != NULL)
		{
			if ((pub_item->name_len == item->name_len) &&
				(memcmp(pub_item->name, item->name, pub_item->name_len) == 0))
			{
				list_data_replace_data(item, pub_item->data, pub_item->data_len);
				break;
			}
			pub_item = list_pub_next(pub_item);
		}
		item = next;
	}
}

//--------------------------------------------
static void mqtt_set_topic_data(list_data_t *list, uint8_t retain)
{
	list_data_t *item;
	list_data_t *next;
	list_pub_t *pub_item;

	item = list;
	next = list_data_next(item);

	if (next == NULL)
		pub_item = list_pub_add_replace(&pub_list, item->name, item->name_len, item->data, item->data_len, retain);
	else
	{
		mqtt_get_topic_data(next);
		pub_item = list_pub_add_replace(&pub_list, item->name, item->name_len, next->data, next->data_len, retain);
	}
#ifdef RULES_ENGINE
	mqtt_trigger_send(pub_item);
#endif
	mqtt_publish_send(pub_item);
	mqttsn_publish_send(pub_item);
}

//--------------------------------------------
static int mqtt_save_rules(list_pub_t *pub_item)
{
	if ((pub_item->name_len == sizeof(MQTT_SYSTOPIC_RULESENGINE_RULES) - 1) &&
		(memcmp(pub_item->name, MQTT_SYSTOPIC_RULESENGINE_RULES, sizeof(MQTT_SYSTOPIC_RULESENGINE_RULES) - 1) == 0))
	{
		FILE *fp = NULL;

		fp = fopen(RULES_FILE, "wb");
		if (fp == NULL)
		{
			dprintf("can't open %s file for writing: %s\n", RULES_FILE, strerror(errno));
			return -1;
		}
		fwrite(pub_item->data, pub_item->data_len, 1, fp);
		fclose(fp);

		msg_trigger_rules_close();
		msggap_rules_mqtt_close();
		msg_rules_mqtt_close();

		mqtt_trigger_remove_all(&mqtt_triggers);
		thread_rules_remove_all();
		thread_cron_remove_all();

		thread_cron_stop();
		thread_rules_stop();

		parse_json_file();

		thread_rules_start();
		thread_cron_start();

		return -1;
	}
	return 0;
}



//--------------------------------------------
//** connections

//--------------------------------------------
#define mqtt_conn_add_new(a) list_mqtt_conn_add_new(&mqtt_conns, a)
#define mqtt_conn_find_addr(a) list_mqtt_conn_find_addr(&mqtt_conns, a)
#if 0
#define mqtt_conn_find_client_id(a, b) list_mqtt_conn_find_client_id(&mqtt_conns, a, b)
#endif
#define mqtt_conn_remove(a) list_mqtt_conn_remove(&mqtt_conns, a)
#define mqtt_conn_remove_all() list_mqtt_conn_remove_all(&mqtt_conns)

//--------------------------------------------
static void mqtt_conn_close(list_mqtt_conn_t *conn)
{
	if (conn->will.name != NULL)
	{
		list_sub_t *sub_list;
		list_pub_t *pub_item;

		sub_list = conn->sub_list;
		conn->sub_list = NULL; // don't send willtopic message to the client itself
		pub_item = list_pub_add_replace(&pub_list, conn->will.name, conn->will.name_len, conn->will.data, conn->will.data_len, conn->will.retain);
		if (pub_item->topic_id == 0)
		{
			pub_item->topic_id = mqttsn_topic_id;
			mqttsn_topic_id = mqttsn_topic_id == 65535 ? 1 : ++mqttsn_topic_id;
		}
		mqtt_publish_send(pub_item);
		mqttsn_publish_send(pub_item);
		conn->sub_list = sub_list;
	}
}


//--------------------------------------------
#define mqttsn_conn_add_new(a) list_mqttsn_conn_add_new(&mqttsn_conns, a)
#define mqttsn_conn_find_addr(a) list_mqttsn_conn_find_addr(&mqttsn_conns, a)
#define mqttsn_conn_find_client_id(a, b) list_mqttsn_conn_find_client_id(&mqttsn_conns, a, b)
#define mqttsn_conn_remove(a) list_mqttsn_conn_remove(&mqttsn_conns, a)
#define mqttsn_conn_remove_all() list_mqttsn_conn_remove_all(&mqttsn_conns)

//--------------------------------------------
static void mqttsn_conn_close(list_mqttsn_conn_t *conn)
{
	if (conn->will.name != NULL)
	{
		list_sub_t *sub_list;
		list_pub_t *pub_item;

		sub_list = conn->sub_list;
		conn->sub_list = NULL; // don't send willtopic message to the client itself
		pub_item = list_pub_add_replace(&pub_list, conn->will.name, conn->will.name_len, conn->will.data, conn->will.data_len, conn->will.retain);
		if (pub_item->topic_id == 0)
		{
			pub_item->topic_id = mqttsn_topic_id;
			mqttsn_topic_id = mqttsn_topic_id == 65535 ? 1 : ++mqttsn_topic_id;
		}
		mqtt_publish_send(pub_item);
		mqttsn_publish_send(pub_item);
		conn->sub_list = sub_list;
	}
}



//--------------------------------------------
//** packets

//--------------------------------------------
static void mqtt_packet_handle(msg_tcp_mqtt_t *ms)
{
	list_mqtt_conn_t *conn;
	mqtt_fixed_header_t fixhdr;
	unsigned char *buf;
	size_t size;

	conn = mqtt_conn_find_addr(&ms->addr);

	// cross-thread internal service message
	if (ms->close == 1 && conn != NULL)
	{
		mqtt_conn_close(conn);
		mqtt_conn_remove(conn);
		return;
	}

	if (mqtt_fixed_header_decode(&fixhdr, ms->msg_buf, ms->msg_cnt) < 0)
		return;

	// the first command must be MQTT_CONNECT
	if (conn == NULL && fixhdr.msg_type != MQTT_CONNECT)
		return;

	if (fixhdr.msg_type == MQTT_CONNECT)
	{
		mqtt_connect_header_t connect = { 0 };
		mqtt_connack_return_code_t code;

		dprintf(">MQTT_CONNECT\n");

		if (conn == NULL)
		{
			if ((conn = mqtt_conn_add_new(&ms->addr)) == NULL)
				code = MQTT_REFUSED_SERVER_UNAVAILABLE;
			else
			{
				connect.mqtt_users = &mqtt_users;
				code = mqtt_connect_decode(&fixhdr, &connect);
				if (code == MQTT_ACCEPTED)
				{
					conn->keepalivesec = (size_t)connect.keep_alive_timer;
					conn->remainsec = (size_t)connect.keep_alive_timer;
					conn->publish_enable = connect.publish_enable;
					if (connect.flags.will == 1)
					{
						mqtt_conn_will_set(conn,
							connect.will_topic,
							connect.will_topic_length,
							connect.will_message,
							connect.will_message_length,
							connect.flags.will_retain);
					}
#if 0
					qos = connect.flags.will_qos; // for what? all qos in subtopic
#endif
					mqtt_connack_encode(&buf, &size, code);
					dprintf("<MQTT_CONNACK\n");
					msg_mqtt_tcp_add_packet(&ms->addr, buf, size);
					return;
				}
			}
		}
		else
			// do not accept multiple CONNECT commands
			code = MQTT_REFUSED_PROTOCOL_VERSION;
		mqtt_connack_encode(&buf, &size, code);
		dprintf("<MQTT_CONNACK\n");
		msg_mqtt_tcp_add_packet(&ms->addr, buf, size);
		msg_mqtt_tcp_add_close_conn(&ms->addr);
		return;
	}

	list_mqtt_conn_reset_remainsec(conn);

#ifndef NDEBUG
	if (fixhdr.msg_type == MQTT_CONNACK)
	{
		dprintf(">MQTT_CONNACK\n");
		return;
	}
#endif

	if (fixhdr.msg_type == MQTT_PUBLISH)
	{
		mqtt_publish_header_t publish = { 0 };

		dprintf(">MQTT_PUBLISH\n");

		if (conn->publish_enable == 0)
		{
			// If a Server implementation does not authorize a PUBLISH to be performed by a Client; it has no way of
			// informing that Client. It MUST either make a positive acknowledgement, according to the normal QoS
			// rules, or close the Network Connection [MQTT-3.3.5]
			msg_mqtt_tcp_add_close_conn(&ms->addr);
			return;
		}

		publish.pub_list = &pub_list;
		mqtt_publish_decode(&fixhdr, &publish);
		if (publish.pub_item->topic_id == 0)
		{
			publish.pub_item->topic_id = mqttsn_topic_id;
			mqttsn_topic_id = mqttsn_topic_id == 65535 ? 1 : ++mqttsn_topic_id;
		}
		if (publish.qos > 0)
		{
			if (publish.qos == 1)
			{
				mqtt_puback_encode(&buf, &size, publish.msg_id);
				dprintf("<MQTT_PUBACK\n");
			}
			else
			{
				mqtt_pubrec_encode(&buf, &size, publish.msg_id);
				dprintf("<MQTT_PUBREC\n");
			}
			msg_mqtt_tcp_add_packet(&ms->addr, buf, size);
		}

#ifdef RULES_ENGINE
		if (mqtt_save_rules(publish.pub_item) < 0)
			return;
		mqtt_trigger_send(publish.pub_item);
#endif
		mqtt_publish_send(publish.pub_item);
		mqttsn_publish_send(publish.pub_item);
		return;
	}

#ifndef NDEBUG
	if (fixhdr.msg_type == MQTT_PUBACK)
	{
		dprintf(">MQTT_PUBACK\n");
		return;
	}
#endif

	if (fixhdr.msg_type == MQTT_PUBREC)
	{
		mqtt_pubrec_header_t pubrec = { 0 };

		dprintf(">MQTT_PUBREC\n");
		mqtt_pubrec_decode(&fixhdr, &pubrec);
		mqtt_pubrel_encode(&buf, &size, pubrec.msg_id);
		dprintf("<MQTT_PUBREL\n");
		msg_mqtt_tcp_add_packet(&ms->addr, buf, size);
		return;
	}

	if (fixhdr.msg_type == MQTT_PUBREL)
	{
		mqtt_pubrel_header_t pubrel = { 0 };

		dprintf(">MQTT_PUBREL\n");
		mqtt_pubrel_decode(&fixhdr, &pubrel);
		mqtt_pubcomp_encode(&buf, &size, pubrel.msg_id);
		dprintf("<MQTT_PUBCOMP\n");
		msg_mqtt_tcp_add_packet(&ms->addr, buf, size);
		return;
	}

#ifndef NDEBUG
	if (fixhdr.msg_type == MQTT_PUBCOMP)
	{
		dprintf(">MQTT_PUBCOMP\n");
		return;
	}
#endif

	if (fixhdr.msg_type == MQTT_SUBSCRIBE)
	{
		mqtt_subscribe_header_t subscribe = { 0 };
		mqtt_suback_header_t suback = { 0 };
		uint8_t *qos_buf;
		list_link_t *link_list;
		list_link_t *link_item;
		list_sub_t *sub_item;
		size_t cnt;

		dprintf(">MQTT_SUBSCRIBE\n");

		subscribe.sub_list = &conn->sub_list;
		mqtt_subscribe_decode(&fixhdr, &subscribe);

		// qos as the client wants
		qos_buf = (uint8_t *)malloc(subscribe.qos_size);
		sub_item = subscribe.sub_item;
		for (cnt = 0; cnt < subscribe.qos_size; cnt++)
		{
			qos_buf[cnt] = sub_item->qos;
			sub_item = list_sub_next(sub_item);
		}

		suback.msg_id = subscribe.msg_id;
		suback.qos_buf = qos_buf;
		suback.qos_size = subscribe.qos_size;
		mqtt_suback_encode(&buf, &size, &suback);
		free(qos_buf);
		dprintf("<MQTT_SUBACK\n");
		msg_mqtt_tcp_add_packet(&ms->addr, buf, size);

		sub_item = subscribe.sub_item;
		while(sub_item != NULL)
		{
			list_sub_pub_matches(sub_item, &pub_list, &link_list);
			link_item = list_link_head(&link_list);
			while (link_item != NULL)
			{
				mqtt_publish_header_t publish = { 0 };

				publish.pub_item = link_item->pub_item;
				publish.qos = conn->sub_list->qos;
				publish.msg_id = mqtt_msg_id;
				mqtt_publish_encode(&buf, &size, &publish);
				mqtt_msg_id = mqtt_msg_id == 65535 ? 1 : ++mqtt_msg_id;
				dprintf("<MQTT_PUBLISH\n");
				msg_mqtt_tcp_add_packet(&ms->addr, buf, size);
				link_item = list_link_next(link_item);
			}
			list_link_remove_all(&link_list);
			sub_item = list_sub_next(sub_item);
		}
		return;
	}

#ifndef NDEBUG
	if (fixhdr.msg_type == MQTT_SUBACK)
	{
		dprintf(">MQTT_SUBACK\n");
		return;
	}
#endif

	if (fixhdr.msg_type == MQTT_UNSUBSCRIBE)
	{
		mqtt_unsubscribe_header_t unsubscribe = { 0 };

		dprintf(">MQTT_UNSUBSCRIBE\n");
		unsubscribe.sub_list = &conn->sub_list;
		mqtt_unsubscribe_decode(&fixhdr, &unsubscribe);
		mqtt_unsuback_encode(&buf, &size, unsubscribe.msg_id);
		dprintf("<MQTT_UNSUBACK\n");
		msg_mqtt_tcp_add_packet(&ms->addr, buf, size);
		return;
	}

#ifndef NDEBUG
	if (fixhdr.msg_type == MQTT_UNSUBACK)
	{
		dprintf(">MQTT_UNSUBACK\n");
		return;
	}
#endif

	if (fixhdr.msg_type == MQTT_PINGREQ)
	{
		dprintf(">MQTT_PINGREQ\n");
		mqtt_pingresp_encode(&buf, &size);
		dprintf("<MQTT_PINGRESP\n");
		msg_mqtt_tcp_add_packet(&ms->addr, buf, size);
		return;
	}

#ifndef NDEBUG
	if (fixhdr.msg_type == MQTT_PINGRESP)
	{
		dprintf(">MQTT_PINGRESP\n");
		return;
	}
#endif

	if (fixhdr.msg_type == MQTT_DISCONNECT)
	{
		dprintf(">MQTT_DISCONNECT\n");
		msg_mqtt_tcp_add_close_conn(&conn->addr);
		return;
	}
}

//--------------------------------------------
static void mqttsn_packet_handle(msg_udp_mqtt_t *ms)
{
	list_mqttsn_conn_t *conn;
	mqttsn_fixed_header_t fixhdr;
	unsigned char *buf;
	size_t size;

	conn = mqttsn_conn_find_addr(&ms->addr);

	if (mqttsn_fixed_header_decode(&fixhdr, ms->msg_buf, ms->msg_cnt) < 0)
		return;

	// the first command must be MQTTSN_CONNECT
	if (conn == NULL && fixhdr.msg_type != MQTTSN_CONNECT)
	{
		// A client may also receive an unsolicited DISCONNECT sent by the gateway.
		// This may happen for example when the gateway, due to an error,
		// cannot identify the client to which a received message belongs.
		uint16_t duration = 0;
		mqttsn_disconnect_encode(&buf, &size, duration);
		msg_mqtt_udp_add_packet(&ms->addr, buf, size);
		return;
	}

	if (fixhdr.msg_type == MQTTSN_CONNECT)
	{
		mqttsn_connect_header_t connect = { 0 };
		mqttsn_return_code_t code;

		dprintf(">MQTTSN_CONNECT\n");

		if ((code = mqttsn_connect_decode(&fixhdr, &connect)) != MQTTSN_ACCEPTED)
		{
			mqttsn_connack_encode(&buf, &size, code);
			dprintf("<MQTTSN_CONNACK\n");
			msg_mqtt_udp_add_packet(&ms->addr, buf, size);
			return;
		}

		if (conn == NULL)
		{
			if ((conn = mqttsn_conn_find_client_id((char *)connect.client_id, connect.client_id_length)) == NULL)
			{
				// client is connecting for the first time
				if (connect.flags.clean_session == 0)
				{
					// for the first time clean_session must be 1 because of possible problem with registered topic numbers
					// if whsnbg has been restarted (topic numbers are not stored)
					mqttsn_connack_encode(&buf, &size, MQTTSN_REFUSED_NOT_SUPPORTED);
					dprintf("<MQTTSN_CONNACK\n");
					dprintf("return_code:MQTTSN_REFUSED_NOT_SUPPORTED\n");
					msg_mqtt_udp_add_packet(&ms->addr, buf, size);
					return;
				}
				if ((conn = mqttsn_conn_add_new(&ms->addr)) == NULL)
				{
					mqttsn_connack_encode(&buf, &size, MQTTSN_REFUSED_CONGESTION);
					dprintf("<MQTTSN_CONNACK\n");
					dprintf("return_code:MQTTSN_REFUSED_CONGESTION\n");
					msg_mqtt_udp_add_packet(&ms->addr, buf, size);
					return;
				}
				list_mqttsn_conn_set_client_id(conn, connect.client_id, connect.client_id_length);
			}
			else
			{
				// client is reconnecting, use old record
				// an ip address and port number may be others
				memcpy(&conn->addr, &ms->addr, sizeof(struct sockaddr_in));
			}
		}

		conn->state = MQTTSN_CLIENT_CONNECTED;

		// MQTT-SN Protocol Specification Version 1.2
		// 7.2 “Best practice” values for timers and counters
		// The “tolerance” of the sleep and keep-alive timers at the server/gateway 
		// depends on the duration indicated by the clients.
		// For example, the timer values should be 10% higher than the indicated values 
		// for durations larger than 1 minute, and 50% higher if less.
		if (connect.duration > 60)
			conn->keepalivesec = (size_t)((double)connect.duration * 1.1);
		else
			conn->keepalivesec = (size_t)((double)connect.duration * 1.5);
		list_mqttsn_conn_reset_remainsec(conn);

		dprintf("mqttsn conn:%s:%d, client_id:%.*s, duration:%d, will:%d, clean_session:%d\n",\
			inet_ntoa(conn->addr.sin_addr),\
			(int)ntohs(conn->addr.sin_port),\
			conn->client_id_length,\
			conn->client_id,\
			connect.duration,\
			connect.flags.will,\
			connect.flags.clean_session);

		if (connect.flags.clean_session == 1)
			list_mqttsn_conn_topics_clear(conn);

		if (connect.flags.will == 1)
		{
			mqttsn_willtopicreq_encode(&buf, &size);
			dprintf("<MQTTSN_WILLROPICREQ\n");
			msg_mqtt_udp_add_packet(&ms->addr, buf, size);
		}
		else
			mqttsn_send_buffered_msgs(conn);
		return;
	}

	list_mqttsn_conn_reset_remainsec(conn);

	if (fixhdr.msg_type == MQTTSN_WILLTOPIC || fixhdr.msg_type == MQTTSN_WILLTOPICUPD)
	{
		mqttsn_willtopic_header_t willtopic = { 0 };

		mqttsn_willtopic_decode(&fixhdr, &willtopic);
		if (willtopic.will_topic_length == 0)
			mqttsn_conn_will_remove(conn);
		else
			mqttsn_conn_will_name_set(conn, willtopic.will_topic, willtopic.will_topic_length, willtopic.flags.retain);
#if 0
		qos = willtopic.flags.qos; // for what? all qos in subtopic
#endif

		if (fixhdr.msg_type == MQTTSN_WILLTOPIC)
		{
			dprintf(">MQTTSN_WILLTOPIC\n");
			mqttsn_willmsgreq_encode(&buf, &size);
			dprintf("<MQTTSN_WILLMSGREQ\n");
		}
		else
		{
			dprintf(">MQTTSN_WILLTOPICUPD\n");
			mqttsn_willtopicresp_encode(&buf, &size, MQTTSN_ACCEPTED);
			dprintf("<MQTTSN_WILLTOPICRESP\n");
		}
		msg_mqtt_udp_add_packet(&ms->addr, buf, size);
		return;
	}

	if (fixhdr.msg_type == MQTTSN_WILLMSG || fixhdr.msg_type == MQTTSN_WILLMSGUPD)
	{
		mqttsn_willmsg_header_t willmsg = { 0 };

		mqttsn_willmsg_decode(&fixhdr, &willmsg);
		mqttsn_conn_will_data_set(conn, willmsg.will_msg, willmsg.will_msg_length);

		if (fixhdr.msg_type == MQTTSN_WILLMSG)
		{
			dprintf(">MQTTSN_WILLMSG\n");
			mqttsn_send_buffered_msgs(conn);
		}
		else
		{
			dprintf(">MQTTSN_WILLMSGUPD\n");
			mqttsn_willmsgresp_encode(&buf, &size, MQTTSN_ACCEPTED);
			dprintf("<MQTTSN_WILLMSGRESP\n");
			msg_mqtt_udp_add_packet(&ms->addr, buf, size);
		}
		return;
	}

	if (fixhdr.msg_type == MQTTSN_PINGREQ)
	{
		dprintf(">MQTTSN_PINGREQ\n");
		if (conn->state == MQTTSN_CLIENT_ASLEEP)
		{
			conn->state = MQTTSN_CLIENT_AWAKE;
			// send postponed messages
			mqttsn_send_buffered_msgs(conn);
			return;
		}
		mqttsn_pingresp_encode(&buf, &size);
		dprintf("<MQTTSN_PINGRESP\n");
		msg_mqtt_udp_add_packet(&ms->addr, buf, size);
		return;
	}

#ifndef NDEBUG
	if (fixhdr.msg_type == MQTTSN_PINGRESP)
	{
		dprintf(">MQTTSN_PINGRESP\n");
		return;
	}
#endif

	if (fixhdr.msg_type == MQTTSN_DISCONNECT)
	{
		uint16_t duration;

		dprintf(">MQTTSN_DISCONNECT\n");

		mqttsn_disconnect_decode(&fixhdr, &duration);
		// is it needed to check unsent REGISTER and PUBLISH messages here?
		if (duration == 0)
		{
			conn->state = MQTTSN_CLIENT_DISCONNECTED; // is it really needed?
			mqttsn_conn_close(conn); // send will topic
			mqttsn_conn_remove(conn); // remove connection
		}
		else
		{
			conn->state = MQTTSN_CLIENT_ASLEEP;

			// MQTT-SN Protocol Specification Version 1.2
			// 7.2 “Best practice” values for timers and counters
			// The “tolerance” of the sleep and keep-alive timers at the server/gateway 
			// depends on the duration indicated by the clients.
			// For example, the timer values should be 10% higher than the indicated values 
			// for durations larger than 1 minute, and 50% higher if less.
			if (duration > 60)
				conn->keepalivesec = (size_t)((double)duration * 1.1);
			else
				conn->keepalivesec = (size_t)((double)duration * 1.5);
			list_mqttsn_conn_reset_remainsec(conn);
		}

		duration = 0;
		mqttsn_disconnect_encode(&buf, &size, duration);

		dprintf("<MQTTSN_DISCONNECT\n");

		msg_mqtt_udp_add_packet(&ms->addr, buf, size);
		return;
	}

	if (fixhdr.msg_type == MQTTSN_REGISTER)
	{
		mqttsn_register_header_t registerh = { 0 };
		mqttsn_return_code_t code;
		mqttsn_regack_header_t regack = { 0 };

		dprintf(">MQTTSN_REGISTER\n");

		registerh.pub_list = &pub_list;
		code = mqttsn_register_decode(&fixhdr, &registerh);
		if (registerh.pub_item != NULL && registerh.pub_item->topic_id == 0)
		{
			registerh.pub_item->topic_id = mqttsn_topic_id;
			mqttsn_topic_id = mqttsn_topic_id == 65535 ? 1 : ++mqttsn_topic_id;
		}

		if (code == MQTTSN_ACCEPTED)
		{
			regack.topic_id = registerh.pub_item->topic_id;
			mqttsn_conn_registered_topic_id_add(conn, regack.topic_id);
		}
		else
			regack.topic_id = 0;
		regack.msg_id = registerh.msg_id;
		regack.return_code = code;
		mqttsn_regack_encode(&buf, &size, &regack);

		dprintf("<MQTTSN_REGACK\n");
		dprintf("mqttsn conn:%s:%d, topic_id:%d, msg_id:%d, return_code:%d\n",\
			inet_ntoa(conn->addr.sin_addr),\
			(int)ntohs(conn->addr.sin_port),\
			regack.topic_id,
			regack.msg_id,
			regack.return_code);

		msg_mqtt_udp_add_packet(&ms->addr, buf, size);
		return;
	}

	if (fixhdr.msg_type == MQTTSN_REGACK)
	{
		mqttsn_regack_header_t regack = { 0 };

		dprintf(">MQTTSN_REGACK\n");

		mqttsn_regack_decode(&fixhdr, &regack);
		if (regack.return_code == MQTTSN_ACCEPTED)
		{
			list_msg_t *msg_item;

			msg_item = mqttsn_conn_register_msg_find(conn, regack.msg_id);
			if (msg_item != NULL)
			{
				list_pub_t *pub_item;

				mqttsn_conn_registered_topic_id_find(conn, regack.topic_id);
				pub_item = list_pub_find_topic_id(&pub_list, regack.topic_id);
				if (pub_item != NULL)
					mqttsn_conn_publish_msg_add(conn, pub_item, msg_item->qos);
				else
					assert(0); // something wrong

				mqttsn_conn_register_msg_remove_msg_id(conn, regack.msg_id);
				mqttsn_send_buffered_msgs(conn);
			}
			else
				assert(0); // something wrong
		}
		return;
	}

	if (fixhdr.msg_type == MQTTSN_PUBLISH)
	{
		mqttsn_publish_header_t publish = { 0 };
		mqttsn_return_code_t code;
		mqttsn_puback_header_t puback = { 0 };

		dprintf(">MQTTSN_PUBLISH\n");

		publish.pub_list = &pub_list;
		code = mqttsn_publish_decode(&fixhdr, &publish);
		puback.topic_id = publish.topic_id;
		puback.msg_id = publish.msg_id;
		puback.return_code = code;
		if (publish.flags.qos > 0)
		{
			if (publish.flags.qos == 1)
			{
				mqttsn_puback_encode(&buf, &size, &puback);
				dprintf("<MQTTSN_PUBACK\n");
			}
			else
			{
				mqttsn_pubrec_encode(&buf, &size, publish.msg_id);
				dprintf("<MQTTSN_PUBREC\n");
			}

			dprintf("mqttsn conn:%s:%d, topic_id:%d, msg_id:%d, return_code:%d\n",\
				inet_ntoa(conn->addr.sin_addr),\
				(int)ntohs(conn->addr.sin_port),\
				puback.topic_id,
				puback.msg_id,
				puback.return_code);

			msg_mqtt_udp_add_packet(&ms->addr, buf, size);
		}

		if (publish.pub_item != NULL)
		{
#ifdef RULES_ENGINE
			mqtt_trigger_send(publish.pub_item);
#endif
			mqtt_publish_send(publish.pub_item);
			mqttsn_publish_send(publish.pub_item);
		}

#ifdef SENSOR_DATA
		free(publish.data);
#endif

		return;
	}

	if (fixhdr.msg_type == MQTTSN_PUBACK)
	{
		mqttsn_puback_header_t puback = { 0 };

		dprintf(">MQTTSN_PUBACK\n");
		mqttsn_puback_decode(&fixhdr, &puback);

		dprintf("mqttsn conn:%s:%d, topic_id:%d, msg_id:%d, return_code:%d\n",\
			inet_ntoa(conn->addr.sin_addr),\
			(int)ntohs(conn->addr.sin_port),\
			puback.topic_id,
			puback.msg_id,
			puback.return_code);

		if (mqttsn_conn_publish_msg_find(conn, puback.msg_id) != NULL)
		{
			mqttsn_conn_publish_msg_remove_msg_id(conn, puback.msg_id);
			mqttsn_send_buffered_msgs(conn);
		}
		return;
	}

	if (fixhdr.msg_type == MQTTSN_PUBREC)
	{
		uint16_t msg_id;

		dprintf(">MQTTSN_PUBREC\n");
		mqttsn_pubrec_decode(&fixhdr, &msg_id);
		mqttsn_pubrel_encode(&buf, &size, msg_id);
		dprintf("<MQTTSN_PUBREL\n");
		msg_mqtt_udp_add_packet(&ms->addr, buf, size);
		return;
	}

	if (fixhdr.msg_type == MQTTSN_PUBREL)
	{
		uint16_t msg_id;

		dprintf(">MQTTSN_PUBREL\n");
		mqttsn_pubrel_decode(&fixhdr, &msg_id);
		mqttsn_pubcomp_encode(&buf, &size, msg_id);
		dprintf("<MQTTSN_PUBCOMP\n");
		msg_mqtt_udp_add_packet(&ms->addr, buf, size);
		return;
	}

	if (fixhdr.msg_type == MQTTSN_PUBCOMP)
	{
		uint16_t msg_id;

		dprintf(">MQTTSN_PUBCOMP\n");
		mqttsn_pubcomp_decode(&fixhdr, &msg_id);
		mqttsn_conn_publish_msg_remove_msg_id(conn, msg_id);
		mqttsn_send_buffered_msgs(conn);
		return;
	}

	if (fixhdr.msg_type == MQTTSN_SUBSCRIBE)
	{
		mqttsn_subscribe_header_t subscribe = { 0 };
		mqttsn_return_code_t code;
		mqttsn_suback_header_t suback = { 0 };
		list_link_t *link_list;
		list_link_t *link_item;

		dprintf(">MQTTSN_SUBSCRIBE\n");
		subscribe.sub_list = &conn->sub_list;
		subscribe.pub_list = &pub_list;
		code = mqttsn_subscribe_decode(&fixhdr, &subscribe);
		if (subscribe.pub_item != NULL)
		{
			if (subscribe.pub_item->topic_id == 0)
			{
				subscribe.pub_item->topic_id = mqttsn_topic_id;
				mqttsn_topic_id = mqttsn_topic_id == 65535 ? 1 : ++mqttsn_topic_id;
			}
			subscribe.sub_item->topic_id = subscribe.pub_item->topic_id;
		}

		suback.flags.qos = subscribe.flags.qos;
		suback.topic_id = subscribe.topic_id;
		suback.msg_id = subscribe.msg_id;
		suback.return_code = code;
		mqttsn_suback_encode(&buf, &size, &suback);
		dprintf("<MQTTSN_SUBACK\n");
		msg_mqtt_udp_add_packet(&ms->addr, buf, size);
		if (code != MQTTSN_ACCEPTED)
			return;

		list_sub_pub_matches(subscribe.sub_item, &pub_list, &link_list);
		link_item = list_link_head(&link_list);
		while (link_item != NULL)
		{
			if (mqttsn_conn_registered_topic_id_find(conn, link_item->pub_item->topic_id) == NULL)
				mqttsn_conn_register_msg_add(conn, link_item->pub_item, subscribe.sub_item->qos);
			else
				// the client already has the topic id
				mqttsn_conn_publish_msg_add(conn, link_item->pub_item, subscribe.sub_item->qos);
			link_item = list_link_next(link_item);
		}
		list_link_remove_all(&link_list);
		mqttsn_send_buffered_msgs(conn);
		return;
	}

	if (fixhdr.msg_type == MQTTSN_UNSUBSCRIBE)
	{
		mqttsn_unsubscribe_header_t unsubscribe = { 0 };
		mqttsn_return_code_t code;
		uint16_t msg_id;

		dprintf(">MQTTSN_UNSUBSCRIBE\n");
		unsubscribe.sub_list = &conn->sub_list;
		code = mqttsn_unsubscribe_decode(&fixhdr, &unsubscribe);
		msg_id = unsubscribe.msg_id;
		mqttsn_unsuback_encode(&buf, &size, msg_id);
		dprintf("<MQTTSN_UNSUBACK\n");
		msg_mqtt_udp_add_packet(&ms->addr, buf, size);
		return;
	}
}

//--------------------------------------------
static void timer_handle(void)
{
	list_mqtt_conn_t *mqtt_conn;
	list_mqttsn_conn_t *mqttsn_conn;

	mqtt_conn = mqtt_conns;
	while (mqtt_conn != NULL)
	{
		if (mqtt_conn->keepalivesec != 0)
		{
			if (mqtt_conn->remainsec == 0)
			{
				// send message to the tcp thread to close the tcp connection,
				// then send will topic and clear the connection information
				msg_mqtt_tcp_add_close_conn(&mqtt_conn->addr);
			}
			else
				--mqtt_conn->remainsec;
		}
		mqtt_conn = list_mqtt_conn_next(mqtt_conn);
	}

	mqttsn_conn = mqttsn_conns;
	while (mqttsn_conn != NULL)
	{
		if (mqttsn_conn->state == MQTTSN_CLIENT_ACTIVE ||
			mqttsn_conn->state == MQTTSN_CLIENT_ASLEEP)
		{
			if (mqttsn_conn->keepalivesec != 0)
			{
				if (mqttsn_conn->remainsec == 0)
				{
					// totally disconnect: send will topic and clear connection information
					// sensor MUST begin full connect procedure
					mqttsn_conn->state = MQTTSN_CLIENT_DISCONNECTED; //  is it really needed?
					mqttsn_conn_close(mqttsn_conn); // send will topic
					mqttsn_conn = mqttsn_conn_remove(mqttsn_conn); // clear connection
					continue;
				}
				else
					--mqttsn_conn->remainsec;
			}
		}
		mqttsn_conn = list_mqttsn_conn_next(mqttsn_conn);
	}
}



//--------------------------------------------
//** main thread

//--------------------------------------------
static void thread_run(void *param)
{
	size_t timer = 0;

	for (;;)
	{
		msg_tcp_mqtt_t *msg_tcp;
		msg_udp_mqtt_t *msg_udp;
		list_data_t *list;
		msg_rules_mqtt_t *msg_rules;

		if ((msg_tcp = msg_tcp_mqtt_get_first()) != NULL)
		{
			mqtt_packet_handle(msg_tcp);
			msg_tcp_mqtt_remove(msg_tcp);
		}
		if ((msg_udp = msg_udp_mqtt_get_first()) != NULL)
		{
			mqttsn_packet_handle(msg_udp);
			msg_udp_mqtt_remove(msg_udp);
		}
		if ((list = msggap_rules_mqtt_get_request()) != NULL)
		{
			mqtt_get_topic_data(list);
			msggap_rules_mqtt_reply_request();
		}
		if ((msg_rules = msg_rules_mqtt_get_first()) != NULL)
		{
			mqtt_set_topic_data(msg_rules->list, msg_rules->retain);
			msg_rules_mqtt_remove(msg_rules);
		}
		if (timer % 100 == 0)
			timer_handle(); // + 1 second
		if (thread_state == THREAD_STAYING)
			break;
		sleep(10);
		timer++;
	}

	mqttsn_conn_remove_all();
	mqtt_conn_remove_all();
	list_pub_remove_all(&pub_list);
	list_mqtt_user_remove_all(&mqtt_users);
	mqtt_trigger_remove_all(&mqtt_triggers);
	thread_state = THREAD_STOPPED;
}

//--------------------------------------------
#ifdef WIN32
static unsigned int __stdcall thread_launcher(void *param)
{
	thread_run(param);
	return 0;
}
#else
static void *thread_launcher(void *param)
{
	thread_run(param);
	return NULL;
}
#endif


//--------------------------------------------
void thread_mqtt_start(void)
{
	pthread_t thread;
	void *param = NULL;

	thread_state = THREAD_RUNNING;
	thread_begin(thread_launcher, param, &thread);
}

//--------------------------------------------
void thread_mqtt_stop(void)
{
	if (thread_state == THREAD_RUNNING)
		thread_state = THREAD_STAYING;
	while (thread_state != THREAD_STOPPED)
		sleep(10);
}

//--------------------------------------------
void thread_mqtt_user_add(const char *user_name, const char *password, unsigned char publish_enable)
{
	list_mqtt_user_add_new(&mqtt_users, user_name, password, publish_enable);
}

//--------------------------------------------
void thread_mqtt_trigger_add(const char *name, size_t next_id)
{
	mqtt_trigger_add_new(&mqtt_triggers, name, next_id);
}

//--------------------------------------------
void thread_mqtt_trigger_remove_all(void)
{
	mqtt_trigger_remove_all(&mqtt_triggers);
}

//--------------------------------------------
void thread_mqtt_set_rules_topic_data(const char *data, size_t data_len)
{
	list_pub_t *pub_item;

	pub_item = list_pub_add_replace(&pub_list, MQTT_SYSTOPIC_RULESENGINE_RULES, sizeof(MQTT_SYSTOPIC_RULESENGINE_RULES) - 1, (uint8_t *)data, (uint16_t)data_len, 1);
	mqtt_publish_send(pub_item);
}
