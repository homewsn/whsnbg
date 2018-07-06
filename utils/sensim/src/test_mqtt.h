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

#ifndef TEST_MQTT_H_
#define TEST_MQTT_H_

#include "test.h"

//--------------------------------------------
void test_mqtt_setup(int auto_msgid, int auto_strlen);
void test_mqtt_destroy(void);
size_t test_mqtt_get_size(void);
size_t test_mqtt_list_get_size(void);
test_t *test_mqtt_list_get_test(size_t number);
void test_mqtt_set_list(size_t number);


//--------------------------------------------
// CONNECT
//--------------------------------------------
void mqtt_command_connect(void *to_send);
int mqtt_noresponse_command_connect_connack(void);
int mqtt_noresponse_command_connect_willtopicreq(void);
int mqtt_noresponse_command_connect_willmsgreq(void);
int mqtt_noresponse_command_connect_passed(void);

//--------------------------------------------
// CONNACK
//--------------------------------------------
int mqtt_response_connack(void *received, void *expected);

//--------------------------------------------
// DISCONNECT
//--------------------------------------------
void mqtt_command_disconnect(void *to_send);
int mqtt_noresponse_command_disconnect_passed(void);
int mqtt_response_disconnect(void *received, void *expected);

//--------------------------------------------
// PINGREQ
//--------------------------------------------
void mqtt_command_pingreq(void *to_send);
int mqtt_noresponse_command_pingreq_pingresp(void);

//--------------------------------------------
// PINGRESP
//--------------------------------------------
int mqtt_response_pingresp(void *received, void *expected);

//--------------------------------------------
// PUBLISH
//--------------------------------------------
void mqtt_command_publish(void *to_send);
int mqtt_noresponse_command_publish_puback(void);
int mqtt_noresponse_command_publish_pubrec(void);
int mqtt_response_publish(void *received, void *expected);

//--------------------------------------------
// PUBACK
//--------------------------------------------
int mqtt_response_puback(void *received, void *expected);
void mqtt_command_puback(void *to_send);

//--------------------------------------------
// PUBREC
//--------------------------------------------
int mqtt_response_pubrec(void *received, void *expected);

//--------------------------------------------
// PUBREL
//--------------------------------------------
void mqtt_command_pubrel(void *to_send);
int mqtt_noresponse_command_pubrel_pubcomp(void);

//--------------------------------------------
// PUBCOMP
//--------------------------------------------
int mqtt_response_pubcomp(void *received, void *expected);

//--------------------------------------------
// SUBSCRIBE
//--------------------------------------------
void mqtt_command_subscribe(void *to_send);
int mqtt_noresponse_command_subscribe_suback(void);

//--------------------------------------------
// SUBACK
//--------------------------------------------
int mqtt_response_suback(void *received, void *expected);

//--------------------------------------------
// NOTHING
//--------------------------------------------
int mqtt_noresponse_nothing(void);
int mqtt_response_nothing(void *received, void *expected);
int mqtt_noresponse_nothing_publish(void);

#endif /* TEST_MQTT_H_ */
