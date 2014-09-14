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

#include "utf8.h"

//--------------------------------------------
uint32_t utf8_strlen(char *utf8str, uint32_t size)
{
	uint32_t byte_cnt, char_cnt;
	for (byte_cnt = 0, char_cnt = 0; byte_cnt < size; byte_cnt++)
	{
		if ((utf8str[byte_cnt] & 0xc0) != 0x80)
			char_cnt++;
	}
	return char_cnt;
}
