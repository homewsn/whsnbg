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

#ifndef TEST_MQTTSN_H_
#define TEST_MQTTSN_H_

#include "test.h"

//--------------------------------------------
void test_mqttsn_setup(int auto_msgid, int auto_strlen);
void test_mqttsn_destroy(void);
size_t test_mqttsn_get_size(void);
size_t test_mqttsn_list_get_size(void);
test_t *test_mqttsn_list_get_test(size_t number);
void test_mqttsn_set_list(size_t number);

//--------------------------------------------
// CONNECT
//--------------------------------------------
void mqttsn_command_connect(void *to_send);
int mqttsn_noresponse_command_connect_connack(void);
int mqttsn_noresponse_command_connect_publish(void);
int mqttsn_noresponse_command_connect_willtopicreq(void);
int mqttsn_noresponse_command_connect_willmsgreq(void);
int mqttsn_noresponse_command_connect_passed(void);

//--------------------------------------------
// CONNACK
//--------------------------------------------
int mqttsn_response_connack(void *received, void *expected);

//--------------------------------------------
// DISCONNECT
//--------------------------------------------
void mqttsn_command_disconnect(void *to_send);
int mqttsn_noresponse_command_disconnect_disconnect(void);
int mqttsn_response_disconnect(void *received, void *expected);

//--------------------------------------------
// PINGREQ
//--------------------------------------------
void mqttsn_command_pingreq(void *to_send);
int mqttsn_noresponse_command_pingreq_pingresp(void);
int mqttsn_noresponse_command_pingreq_publish(void);

//--------------------------------------------
// PINGRESP
//--------------------------------------------
int mqttsn_response_pingresp(void *received, void *expected);

//--------------------------------------------
// WILLTOPICREQ
//--------------------------------------------
int mqttsn_response_willtopicreq(void *received, void *expected);

//--------------------------------------------
// WILLTOPIC
//--------------------------------------------
void mqttsn_command_willtopic(void *to_send);

//--------------------------------------------
// WILLMSGREQ
//--------------------------------------------
int mqttsn_response_willmsgreq(void *received, void *expected);

//--------------------------------------------
// WILLMSG
//--------------------------------------------
void mqttsn_command_willmsg(void *to_send);

//--------------------------------------------
// REGISTER
//--------------------------------------------
void mqttsn_command_register(void *to_send);
int mqttsn_noresponse_command_register_regack(void);

//--------------------------------------------
// REGACK
//--------------------------------------------
int mqttsn_response_regack(void *received, void *expected);

//--------------------------------------------
// PUBLISH
//--------------------------------------------
void mqttsn_command_publish(void *to_send);
int mqttsn_noresponse_command_publish_puback(void);
int mqttsn_noresponse_command_publish_pubrec(void);
int mqttsn_response_publish(void *received, void *expected);

//--------------------------------------------
// PUBACK
//--------------------------------------------
int mqttsn_response_puback(void *received, void *expected);
void mqttsn_command_puback(void *to_send);

//--------------------------------------------
// PUBREC
//--------------------------------------------
int mqttsn_response_pubrec(void *received, void *expected);

//--------------------------------------------
// PUBREL
//--------------------------------------------
void mqttsn_command_pubrel(void *to_send);
int mqttsn_noresponse_command_pubrel_pubcomp(void);

//--------------------------------------------
// PUBCOMP
//--------------------------------------------
int mqttsn_response_pubcomp(void *received, void *expected);

//--------------------------------------------
// SUBSCRIBE
//--------------------------------------------
void mqttsn_command_subscribe(void *to_send);
int mqttsn_noresponse_command_subscribe_suback(void);

//--------------------------------------------
// SUBACK
//--------------------------------------------
int mqttsn_response_suback(void *received, void *expected);

//--------------------------------------------
// NOTHING
//--------------------------------------------
int mqttsn_noresponse_nothing(void);
int mqttsn_noresponse_nothing_publish(void);
int mqttsn_response_nothing(void *received, void *expected);

#endif /* TEST_MQTTSN_H_ */
