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
#if defined SENSOR_DATA_MYSQL
#include "msg_mqtt_mysql.h"
#endif
#define MQTTSN_IF_MTU 110

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
#define SENSOR_DATA_FLOAT		0x02	// float (32-bit), scientific notation ("%.8e", float)
#define SENSOR_DATA_UTF8STR		0x03	// length (8-bit), UTF-8 string ('length' bytes)
#define SENSOR_DATA_IPV4		0x04	// long (32-bit) => x.x.x.x string
#define SENSOR_DATA_FLOAT2		0x05	// float (32-bit), width and precision fields are used ("%*.*f", wf, pf, float)

// sensor_data_float_2 structure
typedef struct sensor_data_float2
{
	float data;	// float type data (32-bit)
	uint8_t wf;	// width field, see https://en.wikipedia.org/wiki/Printf_format_string#Width_field
	uint8_t pf;	// precision field, see https://en.wikipedia.org/wiki/Printf_format_string#Precision_field
} sensor_data_float2_t;

// http://en.wikipedia.org/wiki/Endianness#Floating-point_and_endianness

// Serialization of floating-point values:
// 1. both systems must use IEEE754 32-bit floating-point representation (1 sign bit, 8 bits for exponent and 23 bits for the mantissa)
// 2. both systems must use the same endianness and size for float as they do for long

// CC2530:
// SDCC: long (32-bit, little-endian), float (IEEE754, 32-bit, little-endian)
// IAR: long (32-bit, little-endian), float (IEEE754, 32-bit, little-endian)

// Intel:
// msvc: long (32-bit, little-endian), float (IEEE754, 32-bit, little-endian)
// MIPS 24Kc (TP-Link TL-WR703N):
// gcc: long (32-bit, big-endian), float (IEEE754, 32-bit, big-endian)

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

	switch (*data)
	{
	case SENSOR_DATA_INT32:
		*size = MAX_INT32_STRING + 1;
		*buf = (char *)malloc(*size);
		long_data = ntohl(*(uint32_t *)(data + 1));
		sprintf(*buf, "%ld", (long)long_data);
		*size = strlen(*buf);
		break;

	case SENSOR_DATA_FLOAT:
		*size = MAX_FLOAT_STRING + 1;
		*buf = (char *)malloc(*size);
		long_data = ntohl(*(uint32_t *)(data + 1));
		// use scientific notation:
		sprintf(*buf, "%.8e", *(float *)&long_data);
		*size = strlen(*buf);
		break;

	case SENSOR_DATA_UTF8STR:
		*size = *(data + 1);
		*buf = (char *)malloc(*size);
		memcpy(*buf, data + 2, *size);
		break;

	case SENSOR_DATA_IPV4:
		{
			struct in_addr addr;

			*size = MAX_IPV4_STRING + 1;
			*buf = (char *)malloc(*size);
			addr.s_addr = *(uint32_t *)(data + 1);
			memcpy(*buf, inet_ntoa(addr), MAX_IPV4_STRING);
			*size = strlen(*buf);
			break;
		}

	case SENSOR_DATA_FLOAT2:
		{
			sensor_data_float2_t *float2;

			float2 = (sensor_data_float2_t *)(data + 1);
			*size = float2->wf + 1;
			*buf = (char *)malloc(*size);
			long_data = ntohl(*(uint32_t *)(&float2->data));
			// use width and precision fields
			sprintf(*buf, "%*.*f", float2->wf, float2->pf, *(float *)&long_data);
			*size = strlen(*buf);
			break;
		}
	}
}

#if defined SENSOR_DATA_MYSQL
//--------------------------------------------
// from devices (MQTT-SN):
// initial publish messages:
// 1. sensors/id = "online" ("offline") => no query
// 2. sensors/id/ip = "x.x.x.x" => MYSQL_QUERY_UPDATE_SENSOR_IP
// 3. sensors/id/param/unit = "%" => MYSQL_QUERY_UPDATE_PARAM_UNIT
// 4. sensors/id/param/type = "long" => MYSQL_QUERY_UPDATE_PARAM_TYPE
// 5. actuators/id = "online" ("offline") => no query
// 6. actuators/id/ip = "x.x.x.x" => MYSQL_QUERY_UPDATE_ACTUATOR_IP
// 7. actuators/id/param/unit = "%" => MYSQL_QUERY_UPDATE_PARAM_UNIT
// 8. actuators/id/param/type = "long" => MYSQL_QUERY_UPDATE_PARAM_TYPE
// periodic publish messages:
// 9. sensors/id/param = "long_data" => MYSQL_QUERY_ADD_LONG_DATA
// 10. actuators/id/param = "long_data" => MYSQL_QUERY_ADD_LONG_DATA
// 11. sensors/id/param = "float_data" => MYSQL_QUERY_ADD_FLOAT_DATA
// 12. actuators/id/param = "float_data" => MYSQL_QUERY_ADD_FLOAT_DATA
// 13. sensors/id/param = "utf8str_data" => MYSQL_QUERY_ADD_UTF8STR_DATA
// 14. actuators/id/param = "utf8str_data" => MYSQL_QUERY_ADD_UTF8STR_DATA

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
		long id;
		long param;
		char *end;
		int sensor = 0;
		int actuator = 0;

		if (strcmp(token, "sensors") == 0)
			sensor = 1;
		if (strcmp(token, "actuators") == 0)
			actuator = 1;
		if (sensor == 1 || actuator == 1)
		{
			token = strtok(NULL, seps);
			if (token == NULL)
				break;

			errno = 0;
			end = NULL;
			id = strtol(token, &end, 10);
			if (errno == 0 && end == (char *)token + strlen(token))
			{
				token = strtok(NULL, seps);
				if (token == NULL)
					break;

				errno = 0;
				end = NULL;
				param = strtol(token, &end, 10);
				if (errno == 0 && end == (char *)token + strlen(token))
				{
					token = strtok(NULL, seps);
					if (token == NULL)
					{
						switch (*data)
						{
						case SENSOR_DATA_INT32:
							{
								// MYSQL_ADD_LONG_DATA
								long long_data;

								long_data = ntohl(*(uint32_t *)(data + 1));
								msg_mqtt_mysql_add_long_data(id, param, long_data);
								break;
							}
						case SENSOR_DATA_FLOAT:
						case SENSOR_DATA_FLOAT2:
							{
								// MYSQL_ADD_FLOAT_DATA
								long long_data;
								float float_data;

								long_data = ntohl(*(uint32_t *)(data + 1));
								float_data = *(float *)&long_data;
								msg_mqtt_mysql_add_float_data(id, param, float_data);
								break;
							}
						case SENSOR_DATA_UTF8STR:
							{
								// MYSQL_ADD_UTF8STR_DATA
								size_t size;
								char *utf8str_data;

								size = *(data + 1);
								utf8str_data = (char *)malloc(size + 1);
								memcpy(utf8str_data, data + 2, size);
								utf8str_data[size] = '\0';
								msg_mqtt_mysql_add_utf8str_data(id, param, utf8str_data);
								break;
							}
						}
						break;
					}
					if (strcmp(token, "unit") == 0)
					{
						// MYSQL_UPDATE_PARAM_UNIT
						size_t size;
						char *utf8str_data;

						size = *(data + 1);
						utf8str_data = (char *)malloc(size + 1);
						memcpy(utf8str_data, data + 2, size);
						utf8str_data[size] = '\0';
						if (sensor == 1)
							msg_mqtt_mysql_update_sensor_param_unit(id, param, utf8str_data);
						if (actuator == 1)
							msg_mqtt_mysql_update_actuator_param_unit(id, param, utf8str_data);
						break;
					}
					if (strcmp(token, "type") == 0)
					{
						// MYSQL_UPDATE_PARAM_TYPE
						size_t size;
						char *utf8str_data;

						size = *(data + 1);
						utf8str_data = (char *)malloc(size + 1);
						memcpy(utf8str_data, data + 2, size);
						utf8str_data[size] = '\0';
						if (sensor == 1)
							msg_mqtt_mysql_update_sensor_param_type(id, param, utf8str_data);
						if (actuator == 1)
							msg_mqtt_mysql_update_actuator_param_type(id, param, utf8str_data);
						break;
					}
					break;
				}
				if (strcmp(token, "ip") == 0)
				{
					// MYSQL_UPDATE_SENSOR_IP
					struct in_addr addr;
					char *inet_ntoa_str;
					char *utf8str_data;
					size_t size;

					addr.s_addr = *(uint32_t *)(data + 1);
					inet_ntoa_str = inet_ntoa(addr);
					size = strlen(inet_ntoa_str);
					utf8str_data = (char *)malloc(size + 1);
					memcpy(utf8str_data, inet_ntoa_str, size + 1);
					if (sensor == 1)
						msg_mqtt_mysql_update_sensor_ip(id, utf8str_data);
					if (actuator == 1)
						msg_mqtt_mysql_update_actuator_ip(id, utf8str_data);
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
// sensors/id/sleeptimeduration = "10-255" => MYSQL_QUERY_UPDATE_SENSOR_SLEEPTIMEDURATION

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
				break;
			}
		}
		break;
	}

	free(buf);
}
#endif

//--------------------------------------------
int check_for_actuators_id_topic(char *name, size_t name_len)
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
		long id;
		char *end;

		if (strcmp(token, "actuators") == 0)
		{
			token = strtok(NULL, seps);
			if (token == NULL)
				break;

			errno = 0;
			end = NULL;
			id = strtol(token, &end, 10);
			if (errno == 0 && end == (char *)token + strlen(token))
			{
				token = strtok(NULL, seps);
				if (token == NULL)
					return (int)id;
			}
			break;
		}
		break;
	}
	return -1;
}
