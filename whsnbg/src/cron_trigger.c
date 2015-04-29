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

#include <stdio.h>		/* sscanf */
#include <string.h>		/* memset, memcmp, memcpy */
#include <stdlib.h>		/* malloc, strtol */
#include <ctype.h>		/* isdigit */
#include <assert.h>		/* assert */
#include "cron_trigger.h"
#include "os_port.h"

#ifdef _MSC_VER
#pragma warning (disable:4996) // This function may be unsafe.
#endif


#pragma pack(push, 1)

static const char DowAry[] = "sun""mon""tue""wed""thu""fri""sat";
static const char MonAry[] = "jan""feb""mar""apr""may""jun""jul""aug""sep""oct""nov""dec";

#pragma pack(pop)

/* --------------------------------------------
* The following code has been partially taken from BusyBox crond.c
* Copyright 1994 Matthew Dillon (dillon@apollo.west.oic.com)
* (version 2.3.2)
* Vladimir Oleynik <dzo@simtreas.ru> (C) 2002
*
* Licensed under GPLv2.
*/

//--------------------------------------------
static int parse_field(char *ary, int modvalue, int off, const char *names, char *ptr)
					   /* 'names' is a pointer to a set of 3-char abbreviations */
{
	char *base = ptr;
	int n1 = -1;
	int n2 = -1;

	// this can't happen due to config_read()
	/*if (base == NULL)
	return;*/

	while (1) {
		int skip = 0;

		/* Handle numeric digit or symbol or '*' */
		if (*ptr == '*') {
			n1 = 0;  /* everything will be filled */
			n2 = modvalue - 1;
			skip = 1;
			++ptr;
		} else if (isdigit(*ptr)) {
			char *endp;
			if (n1 < 0) {
				n1 = strtol(ptr, &endp, 10) + off;
			} else {
				n2 = strtol(ptr, &endp, 10) + off;
			}
			ptr = endp; /* gcc likes temp var for &endp */
			skip = 1;
		} else if (names) {
			int i;

			for (i = 0; names[i]; i += 3) {
				if (strncasecmp(ptr, &names[i], 3) == 0) {
					ptr += 3;
					if (n1 < 0) {
						n1 = i / 3;
					} else {
						n2 = i / 3;
					}
					skip = 1;
					break;
				}
			}
		}

		/* handle optional range '-' */
		if (skip == 0) {
			goto err;
		}
		if (*ptr == '-' && n2 < 0) {
			++ptr;
			continue;
		}

		/*
		* collapse single-value ranges, handle skipmark, and fill
		* in the character array appropriately.
		*/
		if (n2 < 0) {
			n2 = n1;
		}
		if (*ptr == '/') {
			char *endp;
			skip = strtol(ptr + 1, &endp, 10);
			ptr = endp; /* gcc likes temp var for &endp */
		}

		/*
		* fill array, using a failsafe is the easiest way to prevent
		* an endless loop
		*/
		{
			int s0 = 1;
			int failsafe = 1024;

			--n1;
			do {
				n1 = (n1 + 1) % modvalue;

				if (--s0 == 0) {
					ary[n1 % modvalue] = 1;
					s0 = skip;
				}
				if (--failsafe == 0) {
					goto err;
				}
			} while (n1 != n2);
		}
		if (*ptr != ',') {
			break;
		}
		++ptr;
		n1 = -1;
		n2 = -1;
	}
	if (*ptr) {
err:
		return -1;
	}
	return 0;
}

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

static void fix_day_dow(cron_line_t *cl)
{
	unsigned i;
	int weekUsed = 0;
	int daysUsed = 0;

	for (i = 0; i < ARRAY_SIZE(cl->dow); ++i) {
		if (cl->dow[i] == 0) {
			weekUsed = 1;
			break;
		}
	}
	for (i = 0; i < ARRAY_SIZE(cl->days); ++i) {
		if (cl->days[i] == 0) {
			daysUsed = 1;
			break;
		}
	}
	if (weekUsed != daysUsed) {
		if (weekUsed)
			memset(cl->days, 0, sizeof(cl->days));
		else /* daysUsed */
			memset(cl->dow, 0, sizeof(cl->dow));
	}
}



//--------------------------------------------
#define MAX_MINS_LENGTH		170		// 0,1, ... 58,59
#define MAX_HRS_LENGTH		62		// 0,1, ... 22,23
#define MAX_DAYS_LENGTH		84		// 1,2, ... 30,31
#define MAX_MONS_LENGTH		48		// jan,feb ... nov,dec
#define MAX_DOW_LENGTH		28		// sun,mon ... fri,sat

static int parse_cronstring(cron_trigger_t *item, const char *str)
{
	char mins[MAX_MINS_LENGTH];
	char hrs[MAX_HRS_LENGTH];
	char days[MAX_DAYS_LENGTH];
	char mons[MAX_MONS_LENGTH];
	char dow[MAX_DOW_LENGTH];

	if (sscanf(str, " %"STRINGIFY(MAX_MINS_LENGTH)"[^ ] %"STRINGIFY(MAX_HRS_LENGTH)"[^ ] %"STRINGIFY(MAX_DAYS_LENGTH)"[^ ] %"STRINGIFY(MAX_MONS_LENGTH)"[^ ] %"STRINGIFY(MAX_DOW_LENGTH)"[^ \r\n]",
		(char *)&mins,
		(char *)&hrs,
		(char *)&days,
		(char *)&mons,
		(char *)&dow) == 5)
	{
		if (parse_field(item->cl.mins, 60, 0, NULL, (char *)&mins) != 0)
			return -1;
		if (parse_field(item->cl.hrs, 24, 0, NULL, (char *)&hrs) != 0)
			return -1;
		if (parse_field(item->cl.days, 32, 0, NULL, (char *)&days) != 0)
			return -1;
		if (parse_field(item->cl.mons, 12, -1, MonAry, (char *)&mons) != 0)
			return -1;
		if (parse_field(item->cl.dow, 7, 0, DowAry, (char *)&dow) != 0)
			return -1;
		fix_day_dow(&item->cl);
		return 0;
	}
	return -1;
}

//--------------------------------------------
unsigned int flag_starting_jobs(cron_line_t *cl)
{
	time_t t;
	struct tm *ptm;

	t = time(NULL);
	ptm = localtime(&t);

	if (cl->mins[ptm->tm_min] &&
		cl->hrs[ptm->tm_hour] &&
		(cl->days[ptm->tm_mday] || cl->dow[ptm->tm_wday]) && cl->mons[ptm->tm_mon])
		return 1;
	return 0;
}

//--------------------------------------------
cron_trigger_t *cron_trigger_add_new(cron_trigger_t **list, const char *str, size_t next_id)
{
	cron_trigger_t *item;

	assert(list != NULL);
	assert(str != NULL);

	item = (cron_trigger_t *)malloc(sizeof(cron_trigger_t));
	memset(item, 0, sizeof(cron_trigger_t));
	parse_cronstring(item, str);
	item->next_id = next_id;
	list_add((list_t **)list, (list_t *)item);
	return item;
}

//--------------------------------------------
void cron_trigger_remove_all(cron_trigger_t **list)
{
	cron_trigger_t *next;
	cron_trigger_t *item;

	assert(list != NULL);

	item = *list;
	while (item != NULL)
	{
		next = cron_trigger_next(item);
		free(item);
		item = next;
	}
	*list = NULL;
}

