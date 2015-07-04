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

#ifndef CRON_TRIGGER_H_
#define CRON_TRIGGER_H_

#include "list.h"
#include <time.h>		/* time_t, time() */

#ifdef _MSC_VER
#include "stdint_msc.h"	/* uint8_t ... uint64_t */
#else
#include <stdint.h>		/* uint8_t ... uint64_t */
#include <stddef.h>		/* size_t */
#endif


/* --------------------------------------------
* The following structure has been taken from BusyBox crond.c
* Copyright 1994 Matthew Dillon (dillon@apollo.west.oic.com)
* (version 2.3.2)
* Vladimir Oleynik <dzo@simtreas.ru> (C) 2002
*
* Licensed under GPLv2.
*/
typedef struct cron_line
{
	/* ordered by size, not in natural order. makes code smaller: */
	char dow[7];                 /* 0-6, beginning sunday */
	char mons[12];               /* 0-11 */
	char hrs[24];                /* 0-23 */
	char days[32];               /* 1-31 */
	char mins[60];               /* 0-59 */
} cron_line_t;

//--------------------------------------------
typedef struct cron_trigger
{
	list_t next;
	cron_line_t cl;
	uint32_t next_id;
} cron_trigger_t;

unsigned int flag_starting_jobs(cron_line_t *cl);

cron_trigger_t *cron_trigger_add_new(cron_trigger_t **list, const char *str, uint32_t next_id);
#define cron_trigger_head(a) (cron_trigger_t *)list_head((list_t **)a)
#define cron_trigger_next(a) (cron_trigger_t *)list_next((list_t *)a)
void cron_trigger_remove_all(cron_trigger_t **list);

#endif /* CRON_TRIGGER_H_ */
