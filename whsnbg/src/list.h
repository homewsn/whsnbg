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

#ifndef __LIST_H__
#define __LIST_H__

#ifdef _MSC_VER
#include "stdint_msc.h"	/* uint8_t ... uint64_t */
#else
#include <stdint.h>		/* uint8_t ... uint64_t */
#include <stddef.h>		/* size_t */
#endif


//--------------------------------------------
typedef struct list
{
	struct list *next;
} list_t;

//--------------------------------------------
void list_init(list_t **list);
list_t *list_head(list_t **list);
list_t *list_remove(list_t **list, list_t *item);
list_t *list_add_item(list_t **list, list_t *item);
size_t list_get_length(list_t **list);
list_t *list_next(list_t *item);


#endif /* __LIST_H__ */
