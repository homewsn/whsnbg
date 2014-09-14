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

#include "lists.h"
#include <stdlib.h>		/* malloc */
#include <string.h>		/* memcpy, memset, memcmp */
#include <assert.h>		/* assert */


#ifndef NDEBUG
#include <stdio.h>
#ifdef LINUX_DAEMON_VERSION
#define dprintf(...) syslog(LOG_DEBUG, __VA_ARGS__)
#else
#define dprintf(...) printf(__VA_ARGS__)
#endif
#else
#define dprintf(...)
#endif



//--------------------------------------------
//** list_name_t functions

//--------------------------------------------
list_name_t *list_name_find_name(list_name_t **list, uint8_t *name, uint16_t name_len)
{
	list_name_t *item;

	assert(list != NULL);
	assert(name != NULL);
	assert(name_len != 0);

	item = *list;
	while (item != NULL)
	{
		if (item->name_len == name_len && memcmp(item->name, name, name_len) == 0)
			return item;
		item = list_name_next(item);
	}
	return NULL;
}

//--------------------------------------------
list_name_t *list_name_new(size_t topsize, int addzero, uint8_t *name, uint16_t name_len)
{
	list_name_t *item;

	assert(name != NULL);
	assert(name_len != 0);

	item = (list_name_t *)malloc(topsize);
	memset(item, 0, topsize);
	item->name_len = name_len;
	if (addzero == 1)
	{
		item->name = (uint8_t *)malloc(name_len + 1);
		item->name[item->name_len] = 0;
	}
	else
		item->name = (uint8_t *)malloc(name_len);
	memcpy(item->name, name, name_len);
	return item;
}

//--------------------------------------------
void list_name_remove(list_name_t **list, list_name_t *item)
{
	assert(list != NULL);
	assert(item != NULL);

	list_remove((list_t **)list, (list_t *)item);
	free(item->name);
	free(item);
}

//--------------------------------------------
void list_name_remove_all(list_name_t **list)
{
	list_name_t *next;
	list_name_t *item;

	assert(list != NULL);

	item = *list;
	while (item != NULL)
	{
		next = list_name_next(item);
		free(item->name);
		free(item);
		item = next;
	}
	*list = NULL;
}



//--------------------------------------------
//** list_sub_t (subscription list) functions

//--------------------------------------------
list_sub_t *list_sub_add_replace(list_sub_t **list, uint8_t *name, uint16_t name_len, uint8_t qos)
{
	list_sub_t *item;

	assert(list != NULL);
	assert(name != NULL);
	assert(name_len != 0);

	item = list_sub_find_name(list, name, name_len);
	if (item == NULL)
	{
		item = list_sub_add(list, name, name_len);
		item->qos = qos;
	}
	else
	{
		if (item->qos != qos)
			item->qos = qos;
	}
	return item;
}

//--------------------------------------------
void list_sub_remove_name(list_sub_t **list, uint8_t *name, uint16_t name_len)
{
	list_sub_t *item;

	assert(list != NULL);
	assert(name != NULL);
	assert(name_len != 0);

	item = list_sub_find_name(list, name, name_len);
	if (item != NULL)
		list_sub_remove(list, item);
}

//--------------------------------------------
void list_sub_remove_topic_id(list_sub_t **list, uint16_t topic_id)
{
	list_sub_t *item;

	assert(list != NULL);
	assert(topic_id != 0);

	item = *list;
	while (item != NULL)
	{
		if (item->topic_id == topic_id)
			break;
		item = list_sub_next(item);
	}
	if (item != NULL)
		list_sub_remove(list, item);
}



//--------------------------------------------
//** list_data (data list) functions

//--------------------------------------------
list_data_t *list_data_new(size_t topsize, uint8_t *name, uint16_t name_len, uint8_t *data, uint16_t data_len)
{
	list_data_t *item;

	assert(topsize != 0);
	assert(name != NULL);
	assert(name_len != 0);

	item = (list_data_t *)malloc(topsize);
	memset(item, 0, topsize);
	item->name_len = name_len;
	item->name = (uint8_t *)malloc(name_len);
	memcpy(item->name, name, name_len);
	if (data_len != 0 && data != NULL)
	{
		item->data_len = data_len;
		item->data = (uint8_t *)malloc(data_len);
		memcpy(item->data, data, data_len);
	}
	return item;
}

//--------------------------------------------
list_data_t *list_data_add_replace(list_data_t **list, size_t topsize, uint8_t *name, uint16_t name_len, uint8_t *data, uint16_t data_len, int *added)
{
	list_data_t *item;

	assert(list != NULL);
	assert(name != NULL);
	assert(name_len != 0);

	*added = 0;
	item = list_data_find_name(list, name, name_len);
	if (item == NULL)
	{
		item = list_data_add(list, topsize, name, name_len, data, data_len);
		*added = 1;
	}
	else
		list_data_replace_data(item, data, data_len);
	return item;
}

//--------------------------------------------
list_data_t *list_data_add_ignore(list_data_t **list, size_t topsize, uint8_t *name, uint16_t name_len, uint8_t *data, uint16_t data_len, int *added)
{
	list_data_t *item;

	assert(list != NULL);
	assert(name != NULL);
	assert(name_len != 0);

	*added = 0;
	item = list_data_find_name(list, name, name_len);
	if (item == NULL)
	{
		item = list_data_add(list, topsize, name, name_len, data, data_len);
		*added = 1;
	}
	return item;
}

//--------------------------------------------
void list_data_replace_data(list_data_t *item, uint8_t *data, uint16_t data_len)
{
	assert(item != NULL);

	if (item->data != NULL)
	{
		free(item->data);
		item->data = NULL;
	}
	item->data_len = data_len;
	if (data_len != 0)
	{
		item->data = (uint8_t *)malloc(data_len);
		memcpy(item->data, data, data_len);
	}
}

//--------------------------------------------
void list_data_remove_all(list_data_t **list)
{
	list_data_t *next;
	list_data_t *item;

	assert(list != NULL);

	item = *list;
	while (item != NULL)
	{
		next = list_data_next(item);
		if (item->name != NULL)
			free(item->name);
		if (item->data != NULL)
			free(item->data);
		free(item);
		item = next;
	}
	*list = NULL;
}

//--------------------------------------------
void list_data_remove(list_data_t *item)
{
	if (item->data != NULL)
		free(item->data);
	if (item->name != NULL)
		free(item->name);
	free(item);
}



//--------------------------------------------
//** will functions

//--------------------------------------------
void will_replace(will_t *will, uint8_t *name, uint16_t name_len, uint8_t *data, uint16_t data_len, uint8_t retain)
{
	assert(will != NULL);
	assert(name != NULL);
	assert(name_len != 0);

	will_name_replace(will, name, name_len, retain);
	will_data_replace(will, data, data_len);
}

//--------------------------------------------
void will_name_replace(will_t *will, uint8_t *name, uint16_t name_len, uint8_t retain)
{
	assert(will != NULL);
	assert(name != NULL);
	assert(name_len != 0);

	if (will->name != NULL)
		free(will->name);
	will->name_len = name_len;
	will->name = (uint8_t *)malloc(name_len);
	memcpy(will->name, name, name_len);
	will->retain = retain;
}

//--------------------------------------------
void will_data_replace(will_t *will, uint8_t *data, uint16_t data_len)
{
	assert(will != NULL);

	if (will->data != NULL)
	{
		free(will->data);
		will->data = NULL;
		will->data_len = 0;
	}
	if (data_len != 0 && data != NULL)
	{
		will->data = (uint8_t *)malloc(data_len);
		memcpy(will->data, data, data_len);
		will->data_len = data_len;
	}
}

//--------------------------------------------
void will_remove_all(will_t *will)
{
	assert(will != NULL);

	if (will->name != NULL)
	{
		free(will->name);
		will->name = NULL;
		will->name_len = 0;
	}
	if (will->data != NULL)
	{
		free(will->data);
		will->data = NULL;
		will->data_len = 0;
	}
}



//--------------------------------------------
//** list_pub_t (publish list) functions

//--------------------------------------------
list_pub_t *list_pub_add_replace(list_pub_t **list, uint8_t *name, uint16_t name_len, uint8_t *data, uint16_t data_len, uint8_t retain)
{
	list_pub_t *item;
	int added;

	assert(list != NULL);
	assert(name != NULL);
	assert(name_len != 0);

	item = (list_pub_t *)list_data_add_replace((list_data_t **)list, sizeof(list_pub_t), name, name_len, data, data_len, &added);
	item->retain = retain;
	return item;
}

//--------------------------------------------
list_pub_t *list_pub_add_ignore(list_pub_t **list, uint8_t *name, uint16_t name_len, uint8_t *data, uint16_t data_len, uint8_t retain)
{
	list_pub_t *item;
	int added;

	assert(list != NULL);
	assert(name != NULL);
	assert(name_len != 0);

	item = (list_pub_t *)list_data_add_ignore((list_data_t **)list, sizeof(list_pub_t), name, name_len, data, data_len, &added);
	if (added == 1)
		item->retain = retain;
	return item;
}

//--------------------------------------------
list_pub_t *list_pub_find_topic_id(list_pub_t **list, uint16_t topic_id)
{
	list_pub_t *item;

	assert(list != NULL);
	assert(topic_id != 0);

	item = *list;
	while (item != NULL)
	{
		if (item->topic_id == topic_id)
			break;
		item = list_pub_next(item);
	}
	return item;
}

//--------------------------------------------
void list_pub_data_replace(list_pub_t *item, uint8_t *data, uint16_t data_len,  uint8_t retain)
{
	assert(item != NULL);

	if (data != NULL && data_len != 0)
		list_data_replace_data((list_data_t *)item, data, data_len);
	item->retain = retain;
}



//--------------------------------------------
//** list_reg (register list) functions

//--------------------------------------------
static list_reg_t *list_reg_add_ignore(list_reg_t **list, size_t topsize, uint16_t topic_id)
{
	list_reg_t *item;

	assert(list != NULL);
	assert(topic_id != 0);

	item = *list;
	while (item != NULL)
	{
		if (item->topic_id == topic_id)
			break;
		item = list_reg_next(item);
	}
	if (item == NULL)
	{
		item = (list_reg_t *)malloc(topsize);
		memset(item, 0, topsize);
		item->topic_id = topic_id;
		list_add((list_t **)list, (list_t *)item);
	}
	return item;
}

//--------------------------------------------
list_reg_t *list_reg_add(list_reg_t **list, uint16_t topic_id)
{
	assert(list != NULL);
	assert(topic_id != 0);

	return list_reg_add_ignore(list, sizeof(list_reg_t), topic_id);
}

//--------------------------------------------
void list_reg_remove_all(list_reg_t **list)
{
	list_reg_t *next;
	list_reg_t *item;

	assert(list != NULL);

	item = *list;
	while (item != NULL)
	{
		next = list_reg_next(item);
		free(item);
		item = next;
	}
	*list = NULL;
}

//--------------------------------------------
list_reg_t *list_reg_find_topic_id(list_reg_t **list, uint16_t topic_id)
{
	list_reg_t *item;

	assert(list != NULL);
	assert(topic_id != 0);

	item = *list;
	while (item != NULL)
	{
		if (item->topic_id == topic_id)
			break;
		item = list_reg_next(item);
	}
	return item;
}



//--------------------------------------------
//** list_clreg (register list for client) functions

//--------------------------------------------
list_clreg_t *list_clreg_add(list_clreg_t **list, uint8_t *name, uint16_t name_len, uint16_t topic_id, uint16_t msg_id)
{
	list_clreg_t *item;

	assert(list != NULL);
	assert(name != NULL);
	assert(name_len != 0);
	assert(topic_id != 0);
	assert(msg_id != 0);

	item = list_clreg_find_name(list, name, name_len);
	if (item == NULL)
		item = (list_clreg_t *)list_add((list_t **)list, (list_t *)list_clreg_new(name, name_len));
	item->topic_id = topic_id;
	item->msg_id = msg_id;
	return item;
}

//--------------------------------------------
list_clreg_t *list_clreg_add_msg_id(list_clreg_t **list, uint8_t *name, uint16_t name_len, uint16_t msg_id)
{
	list_clreg_t *item;

	assert(list != NULL);
	assert(name != NULL);
	assert(name_len != 0);
	assert(msg_id != 0);

	item = list_clreg_find_name(list, name, name_len);
	if (item == NULL)
		item = (list_clreg_t *)list_add((list_t **)list, (list_t *)list_clreg_new(name, name_len));
	item->msg_id = msg_id;
	return item;
}

//--------------------------------------------
list_clreg_t *list_clreg_find_msg_id(list_clreg_t **list, uint16_t msg_id)
{
	list_clreg_t *item;

	assert(list != NULL);
	assert(msg_id != 0);

	item = *list;
	while (item != NULL)
	{
		if (item->msg_id == msg_id)
			break;
		item = list_clreg_next(item);
	}
	return item;
}



//--------------------------------------------
//** list_msg (message list) functions

//--------------------------------------------
list_msg_t *list_msg_add(list_msg_t **list, uint16_t topic_id, uint8_t qos)
{
	list_msg_t *item;

	assert(list != NULL);
	assert(topic_id != 0);

	item = (list_msg_t *)list_reg_add_ignore((list_reg_t **)list, sizeof(list_msg_t), topic_id);
	item->qos = qos;
	return item;
}

//--------------------------------------------
list_msg_t *list_msg_find_msg_id(list_msg_t **list, uint16_t msg_id)
{
	list_msg_t *item;

	assert(list != NULL);
	assert(msg_id != 0);

	item = *list;
	while (item != NULL)
	{
		if (item->msg_id == msg_id)
			break;
		item = list_msg_next(item);
	}
	return item;
}

//--------------------------------------------
void list_msg_remove_msg_id(list_msg_t **list, uint16_t msg_id)
{
	list_msg_t *item;

	assert(list != NULL);
	assert(msg_id != 0);

	item = list_msg_find_msg_id(list, msg_id);
	if (item != NULL)
		list_msg_remove(list, item);
}

//--------------------------------------------
list_msg_t *list_msg_remove(list_msg_t **list, list_msg_t *item)
{
	list_msg_t *next;

	assert(list != NULL);
	assert(item != NULL);

	next = (list_msg_t *)list_remove((list_t **)list, (list_t *)item);
	free(item);
	return next;
}



//--------------------------------------------
//** list_link

//--------------------------------------------
list_link_t *list_link_add_pubitem(list_link_t **list, list_pub_t *pub_item)
{
	list_link_t *item;

	assert(list != NULL);
	assert(pub_item != NULL);

	item = (list_link_t *)malloc(sizeof(list_link_t));
	memset(item, 0, sizeof(list_link_t));
	item->pub_item = pub_item;
	list_link_add(list, item);
	return item;
}



//--------------------------------------------
//** publish and subscription topics relations

//--------------------------------------------
static int pname_matches_sname(uint8_t *sname, uint16_t slen, uint8_t *pname, uint16_t plen)
{
	size_t spos;
	size_t ppos;
	int res;
	int wild = 0;

	assert(sname != NULL);
	assert(slen != 0);
	assert(pname != NULL);
	assert(plen != 0);

	spos = 0;
	ppos = 0;

	while (spos < slen && ppos < plen)
	{
		if (sname[spos] == pname[ppos])
		{
			spos++;
			ppos++;
			if (spos == slen && ppos == plen)
			{
				res = 1;
				break;
			}
		}
		else
		{
			if (sname[spos] == '+')
			{
				spos++;
				while (ppos < plen && pname[ppos] != '/')
				{
					ppos++;
				}
				if (ppos == plen && spos == slen)
				{
					res = 1;
					break;
				}
			}
			else if (sname[spos] == '#')
			{
				wild = 1;
				if(spos + 1 != slen)
				{
					res = 0;
					break;
				}
				else
				{
					res = 1;
					break;
				}
			}
			else
			{
				res = 0;
				break;
			}
		}
		if (ppos == plen - 1)
		{
			if(spos == slen - 3 && sname[spos + 1] == '/' && sname[spos + 2] == '#')
			{
				res = 1;
				wild = 1;
				break;
			}
		}
	}
	if (wild == 0 && (ppos < plen || spos < slen))
		res = 0;

	return res;
}

//--------------------------------------------
int name_has_wildcard(uint8_t *name, uint16_t len)
{
	assert(name != NULL);
	assert(len != 0);

	if (memchr((const void *)name, '#', (size_t)len) != NULL)
		return 1;
	if (memchr((const void *)name, '+', (size_t)len) != NULL)
		return 1;
	return 0;
}

//--------------------------------------------
int list_sub_has_wildcard(list_sub_t *sub_item)
{
	return name_has_wildcard(sub_item->name, sub_item->name_len);
}

//--------------------------------------------
void list_sub_pub_matches(list_sub_t *sub_item, list_pub_t **pub_list, list_link_t **link_list)
{
	list_pub_t *pub_item = list_pub_head(pub_list);
	list_link_init(link_list);
	while (pub_item != NULL)
	{
#if 0
		dprintf("subtopic:%.*s, pubtopic:%.*s, retain:%d\n", sub->name, sub->name_len, pub->name, pub->name_len, pub->retain);
#endif
		if (pname_matches_sname(sub_item->name, sub_item->name_len, pub_item->name, pub_item->name_len) == 1 &&
			pub_item->retain == 1) // only retained messages
			list_link_add_pubitem(link_list, pub_item);
		pub_item = list_pub_next(pub_item);
	}
}

//--------------------------------------------
int list_pub_sub_matches(list_pub_t *pub_item, list_sub_t **sub_list)
{
	list_sub_t *sub_item = list_sub_head(sub_list);
	while (sub_item != NULL)
	{
#if 0
		dprintf("pubtopic:%.*s, subtopic:%.*s\n", pub->name_len, pub->name, sub->name_len, sub->name);
#endif
		if (pname_matches_sname(sub_item->name, sub_item->name_len, pub_item->name, pub_item->name_len) == 1)
			return sub_item->qos;
		sub_item = list_sub_next(sub_item);
	}
	return -1;
}
