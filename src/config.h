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

#ifndef CONFIG_H_
#define CONFIG_H_

// TLS library
//#define OPENSSL_LIBRARY		// OpenSSL
#define AXTLS_LIBRARY			// axTLS
//#define SSL_LIBRARY_HEADERS	// TLS library headers

// sensor and MySQL support
#define SENSOR_DATA				// automatic decoding of the MQTT-SN message payload from the sensors
#define SENSOR_DATA_MYSQL		// storing decoded data from the sensors in the MySQL database (see parse_mqttsn_topic_name_to_mysql_query in sensor_data.c)
#define MQTT_DATA_MYSQL			// storing payload from the specific MQTT topics in the MySQL database (see parse_mqtt_topic_name_to_mysql_query in sensor_data.c)

#if defined SENSOR_DATA_MYSQL
#define SENSOR_DATA
#endif
#if defined SENSOR_DATA_MYSQL || defined MQTT_DATA_MYSQL
#define THREAD_MYSQL
#endif

// rules engine support
#define RULES_ENGINE

// MQTT-SN IPv4 network interface MTU size
// IEEE 802.15.4 + Contiki Rime => 127 - 9 - 2 - 6 = 110 bytes
// including IPv4 header(20) + UDP header(8) + MQTT-SN payload(82)
// (packet fragmentation is not supported)
#define MQTTSN_IF_MTU			110

#ifdef WIN32
#define CONFIG_FILE				"../conf/whsnbg.conf"
#define RULES_FILE				"../conf/whsnbg.json"
#else
#ifdef __UBUNTU__
// Host computer
#define CONFIG_FILE				"/home/shared/whsnbg/whsnbg.conf"
#define RULES_FILE				"/home/shared/whsnbg/whsnbg.json"
//#define LINUX_DAEMON_VERSION
#else
// OpenWRT
#define CONFIG_FILE				"/etc/whsnbg.conf"
#define RULES_FILE				"/etc/whsnbg.json"
//#define LINUX_DAEMON_VERSION
#endif
#endif

#endif /* CONFIG_H_ */
