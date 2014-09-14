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

#ifndef __SENSOR_DATA_H__
#define __SENSOR_DATA_H__

#ifdef _MSC_VER
#include "stdint_msc.h"	/* uint8_t ... uint64_t */
#else
#include <stdint.h>		/* uint8_t ... uint64_t */
#include <stddef.h>		/* size_t */
#endif

void encode_mqttsn_sensor_data(uint8_t **buf, size_t *size, const char *data, size_t data_len);
void decode_mqttsn_sensor_data(char **buf, size_t *size, uint8_t *data);
void parse_mqttsn_topic_name_to_mysql_query(char *name, size_t name_len, uint8_t *data);
void parse_mqtt_topic_name_to_mysql_query(char *name, size_t name_len, char *data, size_t data_len);

#endif /* __SENSOR_DATA_H__ */
