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

#include <stdlib.h>		/* malloc, strtol, strtof */
#include <stdio.h>		/* sscanf, sprintf */
#include <errno.h>		/* errno */
#include <string.h>		/* memcpy, strlen */
#include <assert.h>		/* assert */
#include "config.h"
#include "sensor_data.h"
#include "msg_mqtt_mysql.h"

#ifdef WIN32
#include <winsock2.h>	/* htonl */
#else
#include <arpa/inet.h>	/* htonl */
#endif

#ifdef _MSC_VER
#pragma warning (disable:4996) // This function may be unsafe.
#endif

#if defined SENSOR_DATA
#if MQTTSN_IF_MTU < 54
#error "MQTTSN_IF_MTU must not be less than 54"
#elif MQTTSN_IF_MTU > 292
#error "MQTTSN_IF_MTU must not be more than 292"
#endif
#endif

#define MAX_FLOAT_STRING		17	// for scientific notation "±x.yyyyyyyyye±zzz"
#define MAX_INT32_STRING		11
#define MAX_IPV4_STRING			15
// all headers = IPv4 header(20) + UDP header(8) + MQTT-SN publish header(7) + data type (1) + data length (1) = 37 bytes
#define MAX_UTF8_STRING			MQTTSN_IF_MTU - 37

// data types
#define SENSOR_DATA_RESERVED	0x00
#define SENSOR_DATA_INT32		0x01	// long (32-bit)
#define SENSOR_DATA_FLOAT		0x02	// float (32-bit)
#define SENSOR_DATA_UTF8STR		0x03	// length (8-bit), UTF-8 string ('length' bytes)
#define SENSOR_DATA_IPV4		0x04	// long (32-bit) => x.x.x.x string

// http://en.wikipedia.org/wiki/Endianness#Floating-point_and_endianness

// Serialization of floating-point values:
// 1. both systems must use IEEE754 32-bit floating-point representation (1 sign bit, 8 bits for exponent and 23 bits for the mantissa)
// 2. both systems must use the same endianness and size for float as they do for long

// CC2530:
// SDCC: long (32-bit, little-endian), float (IEEE754, 32-bit, little-endian)
// IAR: long (32-bit, little-endian), float (IEEE754, 32-bit, little-endian)

// Intel, Windows: long (32-bit, little-endian), float (IEEE754, 32-bit, little-endian)
// TP-Link TL-WR703N, OpenWRT: ????????????

//--------------------------------------------
static unsigned long _inet_addr(const char *data, size_t data_len)
{
	char *buf;
	unsigned long res;

	buf = (char *)malloc(data_len + 1);
	memcpy(buf, data, data_len);
	buf[data_len] = '\0';
	res = inet_addr(buf);
	free(buf);
	return res;
}

//--------------------------------------------
void encode_mqttsn_sensor_data(uint8_t **buf, size_t *size, const char *data, size_t data_len)
{
	uint32_t *data_ptr;
	float float_data;
	int32_t long_data;
	char *end;

	assert(data_len < 50);

	errno = 0;
	end = NULL;
	long_data = strtol(data, &end, 10);
	if (errno == 0 && end == (char *)data + data_len)
	{
		*size = 5;
		*buf = (uint8_t *)malloc(*size);
		*(*buf) = SENSOR_DATA_INT32;
		data_ptr = (uint32_t *)(*buf + 1);
		*data_ptr = htonl(long_data);
		return;
	}

	errno = 0;
#ifdef _MSC_VER
	float_data = (float)strtod(data, &end);
#else
	float_data = strtof(data, &end);
#endif
	if (errno == 0 && end == (char *)data + data_len)
	{
		*size = 5;
		*buf = (uint8_t *)malloc(*size);
		*(*buf) = SENSOR_DATA_FLOAT;
		data_ptr = (uint32_t *)(*buf + 1);
		*data_ptr = htonl(*(uint32_t *)&float_data);
		return;
	}

	long_data = _inet_addr(data, data_len);
	if (long_data != INADDR_NONE)
	{
		*size = 5;
		*buf = (uint8_t *)malloc(*size);
		*(*buf) = SENSOR_DATA_IPV4;
		data_ptr = (uint32_t *)(*buf + 1);
		*data_ptr = long_data;
		return;
	}

	*size = 2 + data_len;
	*buf = (uint8_t *)malloc(*size);
	*(*buf) = SENSOR_DATA_UTF8STR;
	*(*buf + 1) = (uint8_t)data_len < MAX_UTF8_STRING ? (uint8_t)data_len : MAX_UTF8_STRING;

	memcpy(*buf + 2, data, *(*buf + 1));
}

//--------------------------------------------
void decode_mqttsn_sensor_data(char **buf, size_t *size, uint8_t *data)
{
	uint32_t long_data;

	if (*data == SENSOR_DATA_INT32)
	{
		*size = MAX_INT32_STRING + 1;
		*buf = (char *)malloc(*size);
		long_data = ntohl(*(uint32_t *)(data + 1));
		sprintf(*buf, "%ld", (long)long_data);
		*size = strlen(*buf);
		return;
	}

	if (*data == SENSOR_DATA_FLOAT)
	{
		*size = MAX_FLOAT_STRING + 1;
		*buf = (char *)malloc(*size);
		long_data = ntohl(*(uint32_t *)(data + 1));
		// http://hbfs.wordpress.com/2010/05/11/failed-experiment/
		// use scientific notation:
		sprintf(*buf, "%.8e", *(float *)&long_data);
		*size = strlen(*buf);
		return;
	}

	if (*data == SENSOR_DATA_IPV4)
	{
		struct in_addr addr;

		*size = MAX_IPV4_STRING + 1;
		*buf = (char *)malloc(*size);
		addr.s_addr = *(uint32_t *)(data + 1);
		memcpy(*buf, inet_ntoa(addr), MAX_IPV4_STRING);
		*size = strlen(*buf);
		return;
	}

	*size = *(data + 1);
	*buf = (char *)malloc(*size);
	memcpy(*buf, data + 2, *size);
}


//--------------------------------------------
// from sensors (MQTT-SN):
// initial publish messages:
// 1. sensors/sensor_id = "online" ("offline") => no query
// 2. sensors/sensor_id/param_id/unit = "%" => MYSQL_QUERY_ADD_SENSOR_PARAMETER
// 3. sensors/sensor_id/sleeptimeduration = "60" => MYSQL_QUERY_ADD_SENSOR_TIMEOUT
// 4. sensors/sensor_id/ip = "x.x.x.x" => MYSQL_QUERY_ADD_SENSOR_IP
// subscribe message:
// 5. sensors/sensor_id/sleeptimeduration
// periodic publish messages:
// 6. sensors/sensor_id/param_id = "long" => MYSQL_QUERY_ADD_LONG_DATA

//--------------------------------------------
void parse_mqttsn_topic_name_to_mysql_query(char *name, size_t name_len, uint8_t *data)
{
	char seps[] = "//";
	char *token;
	size_t len = name_len;
	char *buf = (char *)malloc(len + 1);

	memcpy(buf, name, len);
	buf[len] = '\0';

	token = strtok(buf, seps);
	for (;;)
	{
		long sensor_id;
		long sensor_param;
		char *end;

		if (strcmp(token, "sensors") == 0)
		{
			token = strtok(NULL, seps);
			if (token == NULL)
				break;

			errno = 0;
			end = NULL;
			sensor_id = strtol(token, &end, 10);
			if (errno == 0 && end == (char *)token + strlen(token))
			{
				token = strtok(NULL, seps);
				if (token == NULL)
					break;

				errno = 0;
				end = NULL;
				sensor_param = strtol(token, &end, 10);
				if (errno == 0 && end == (char *)token + strlen(token))
				{
					token = strtok(NULL, seps);
					if (token == NULL)
					{
						if (*data == SENSOR_DATA_INT32)
						{
							// MYSQL_ADD_LONG_DATA
							long long_data;

							long_data = ntohl(*(uint32_t *)(data + 1));
							msg_mqtt_mysql_add_long_data(sensor_id, sensor_param, long_data);
						}
						else if (*data == SENSOR_DATA_FLOAT)
						{
							// MYSQL_ADD_FLOAT_DATA
							long long_data;
							float float_data;

							long_data = ntohl(*(uint32_t *)(data + 1));
							float_data = *(float *)&long_data;
							msg_mqtt_mysql_add_float_data(sensor_id, sensor_param, float_data);
						}
						else
						{
							// MYSQL_ADD_UTF8STR_DATA
							size_t size;
							char *utf8str_data;

							size = *(data + 1);
							utf8str_data = (char *)malloc(size + 1);
							memcpy(utf8str_data, data + 2, size);
							utf8str_data[size] = '\0';
							msg_mqtt_mysql_add_utf8str_data(sensor_id, sensor_param, utf8str_data);
						}
						break;
					}
					if (strcmp(token, "unit") == 0)
					{
						// MYSQL_UPDATE_SENSOR_PARAM
						size_t size;
						char *utf8str_data;

						size = *(data + 1);
						utf8str_data = (char *)malloc(size + 1);
						memcpy(utf8str_data, data + 2, size);
						utf8str_data[size] = '\0';
						msg_mqtt_mysql_update_sensor_param(sensor_id, sensor_param, utf8str_data);
						break;
					}
					break;
				}
				if (strcmp(token, "ip") == 0)
				{
					// MYSQL_UPDATE_SENSOR_IP
					struct in_addr addr;

					addr.s_addr = *(uint32_t *)(data + 1);
					msg_mqtt_mysql_update_sensor_ip(sensor_id, inet_ntoa(addr));
					break;
				}
				break;
			}
		}
		break;
	}

	free(buf);
}


//--------------------------------------------
// from human (MQTT):
// sensors/sensor_id/sleeptimeduration = "10-255" => MYSQL_QUERY_UPDATE_SENSOR_TIMEOUT
// sensors/sensor_id/location = "location" => MYSQL_QUERY_UPDATE_SENSOR_LOCATION

//--------------------------------------------
void parse_mqtt_topic_name_to_mysql_query(char *name, size_t name_len, char *data, size_t data_len)
{
	char seps[] = "//";
	char *token;
	size_t len = name_len;
	char *buf = (char *)malloc(len + 1);

	memcpy(buf, name, len);
	buf[len] = '\0';

	token = strtok(buf, seps);
	for (;;)
	{
		long sensor_id;
		char *end;

		if (strcmp(token, "sensors") == 0)
		{
			token = strtok(NULL, seps);
			if (token == NULL)
				break;

			errno = 0;
			end = NULL;
			sensor_id = strtol(token, &end, 10);
			if (errno == 0 && end == (char *)token + strlen(token))
			{
				token = strtok(NULL, seps);
				if (token == NULL)
					break;

				if (strcmp(token, "sleeptimeduration") == 0)
				{
					// MYSQL_UPDATE_SENSOR_SLEEPTIMEDURATION
					long st_duration;

					errno = 0;
					end = NULL;
					st_duration = strtol(data, &end, 10);
					if (errno == 0 && end == (char *)data + data_len)
						msg_mqtt_mysql_update_sensor_sleeptimeduration(sensor_id, st_duration);
					break;
				}
				if (strcmp(token, "location") == 0)
				{
					// MYSQL_UPDATE_SENSOR_LOCATION
					char *utf8str_data;

					utf8str_data = (char *)malloc(data_len + 1);
					memcpy(utf8str_data, data, data_len);
					utf8str_data[data_len] = '\0';
					msg_mqtt_mysql_update_sensor_location(sensor_id, utf8str_data);
					break;
				}
				break;
			}
		}
		break;
	}

	free(buf);
}
