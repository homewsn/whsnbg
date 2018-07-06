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

#ifndef TICK_COUNTER_H_
#define TICK_COUNTER_H_

#define TICKS_IN_SECOND (uint32_t)(10)
#define TICK_TIME (uint32_t)(CLOCKS_PER_SEC / TICKS_IN_SECOND)

//--------------------------------------------
typedef struct alarm_timer
{
	uint32_t start_counter;
	uint32_t running;
} alarm_timer_t;

//--------------------------------------------
void tick_counter_init(void);
void tick_counter_increment(void);
uint32_t tick_counter_get(void);
void tick_counter_destroy(void);
void tick_counter_set_mqtt_mode(int mode);
void tick_counter_set_mqttsn_mode(int mode);
int tick_counter_get_mode(void);
uint32_t alarm_timer_check(alarm_timer_t *timer_name, uint32_t interval);

#endif /* TICK_COUNTER_H_ */
