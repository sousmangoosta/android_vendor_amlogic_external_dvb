#ifdef _FORTIFY_SOURCE
#undef _FORTIFY_SOURCE
#endif
/*
* section and descriptor parser
*
* Copyright (C) 2005 Andrew de Quincey (adq_dvb@lidskialf.net)
*
* This library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public
* License as published by the Free Software Foundation; either
* version 2.1 of the License, or (at your option) any later version.
*
* This library is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License along with this library; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "libucsi/endianops.h"
#include "libucsi/atsc/types.h"

#define HUFFTREE_LITERAL_MASK 0x80
#define HUFFSTRING_END 0x00
#define HUFFSTRING_ESCAPE 0x1b

#define DEST_ALLOC_DELTA 20

struct hufftree_entry {
	uint8_t left_idx;
	uint8_t right_idx;
} __ucsi_packed;

struct huffbuff {
	uint8_t *buf;
	uint32_t buf_len;

	uint32_t cur_byte;
	uint8_t cur_bit;
};


static struct hufftree_entry program_description_hufftree[][128] = {
	{ {0x14, 0x15}, {0x9b, 0xd6}, {0xc9, 0xcf}, {0xd7, 0xc7}, {0x01, 0xa2},
	{0xce, 0xcb}, {0x02, 0x03}, {0xc5, 0xcc}, {0xc6, 0xc8}, {0x04, 0xc4},
	{0x05, 0xc2}, {0x06, 0xc3}, {0xd2, 0x07}, {0xd3, 0x08}, {0xca, 0xd4},
	{0x09, 0xcd}, {0xd0, 0x0a}, {0xc1, 0x0b}, {0x0c, 0x0d}, {0x0e, 0x0f},
	{0x10, 0x11}, {0x12, 0x13}, },
	{ {0x9b, 0x9b}, },
	{ {0x9b, 0x9b}, },
	{ {0x9b, 0x9b}, },
	{ {0x9b, 0x9b}, },
	{ {0x9b, 0x9b}, },
	{ {0x9b, 0x9b}, },
	{ {0x9b, 0x9b}, },
	{ {0x9b, 0x9b}, },
	{ {0x9b, 0x9b}, },
	{ {0x9b, 0x9b}, },
	{ {0x9b, 0x9b}, },
	{ {0x9b, 0x9b}, },
	{ {0x9b, 0x9b}, },
	{ {0x9b, 0x9b}, },
	{ {0x9b, 0x9b}, },
	{ {0x9b, 0x9b}, },
	{ {0x9b, 0x9b}, },
	{ {0x9b, 0x9b}, },
	{ {0x9b, 0x9b}, },
	{ {0x9b, 0x9b}, },
	{ {0x9b, 0x9b}, },
	{ {0x9b, 0x9b}, },
	{ {0x9b, 0x9b}, },
	{ {0x9b, 0x9b}, },
	{ {0x9b, 0x9b}, },
	{ {0x9b, 0x9b}, },
	{ {0x9b, 0x9b}, },
	{ {0x9b, 0x9b}, },
	{ {0x9b, 0x9b}, },
	{ {0x9b, 0x9b}, },
	{ {0x9b, 0x9b}, },
	{ {0x38, 0x39}, {0xad, 0xaf}, {0xb7, 0xda}, {0xa8, 0xb3}, {0xb5, 0x01},
	{0x02, 0x9b}, {0xb4, 0xf1}, {0xa2, 0xd5}, {0xd6, 0xd9}, {0x03, 0x04},
	{0x05, 0xcf}, {0x06, 0xc9}, {0xf9, 0xea}, {0xeb, 0xf5}, {0xf6, 0x07},
	{0x08, 0x09}, {0xb2, 0xc5}, {0xc6, 0xb1}, {0x0a, 0xee}, {0xcb, 0x0b},
	{0xd4, 0x0c}, {0xc4, 0xc8}, {0xd2, 0x0d}, {0x0e, 0x0f}, {0xc7, 0xca},
	{0xce, 0xd0}, {0xd7, 0x10}, {0xc2, 0x11}, {0xcc, 0xec}, {0xe5, 0xe7},
	{0x12, 0xcd}, {0x13, 0x14}, {0xc3, 0x15}, {0x16, 0x17}, {0xed, 0x18},
	{0x19, 0xf2}, {0x1a, 0xd3}, {0x1b, 0x1c}, {0xe4, 0x1d}, {0xc1, 0xe3},
	{0x1e, 0xe9}, {0xf0, 0xe2}, {0xf7, 0x1f}, {0xf3, 0xe6}, {0x20, 0x21},
	{0x22, 0xe8}, {0xef, 0x23}, {0x24, 0x25}, {0x26, 0x27}, {0x28, 0x29},
	{0x2a, 0xf4}, {0x2b, 0x2c}, {0x2d, 0x2e}, {0x2f, 0xe1}, {0x30, 0x31},
	{0x32, 0x33}, {0x34, 0x35}, {0x36, 0x37}, },
	{ {0x9b, 0x9b}, },
	{ {0x03, 0x04}, {0x80, 0xae}, {0xc8, 0xd4}, {0x01, 0x02}, {0x9b, 0xa0}, },
	{ {0x9b, 0x9b}, },
	{ {0x9b, 0x9b}, },
	{ {0x9b, 0x9b}, },
	{ {0x9b, 0x9b}, },
	{ {0x02, 0xf3}, {0xa0, 0xf4}, {0x9b, 0x01}, },
	{ {0x9b, 0x9b}, },
	{ {0xac, 0x9b}, },
	{ {0x9b, 0x9b}, },
	{ {0x9b, 0x9b}, },
	{ {0x01, 0xa0}, {0x9b, 0xa2}, },
	{ {0x07, 0x08}, {0xe2, 0xe4}, {0xe5, 0xe6}, {0xa0, 0xf2}, {0xe1, 0x01},
	{0x02, 0xf3}, {0xe3, 0x03}, {0x04, 0x05}, {0x9b, 0x06}, },
	{ {0x04, 0x80}, {0xca, 0xd3}, {0xa2, 0x01}, {0x9b, 0x02}, {0x03, 0xa0}, },
	{ {0x9b, 0xa0}, },
	{ {0x03, 0x04}, {0x9b, 0xb7}, {0xf4, 0xa0}, {0xb0, 0xf3}, {0x01, 0x02}, },
	{ {0xb9, 0x02}, {0xb8, 0x9b}, {0xa0, 0x01}, },
	{ {0xae, 0x02}, {0xb6, 0x9b}, {0x01, 0xa0}, },
	{ {0xa0, 0x01}, {0x9b, 0xb0}, },
	{ {0xae, 0x01}, {0x9b, 0xa0}, },
	{ {0xae, 0x01}, {0xa0, 0x9b}, },
	{ {0x9b, 0x9b}, },
	{ {0x9b, 0x01}, {0xac, 0xae}, },
	{ {0x9b, 0x9b}, },
	{ {0x02, 0x03}, {0x9b, 0xa0}, {0xb5, 0xb6}, {0xb8, 0x01}, },
	{ {0x9b, 0xa0}, },
	{ {0x9b, 0xa0}, },
	{ {0x9b, 0x9b}, },
	{ {0x9b, 0x9b}, },
	{ {0x9b, 0x9b}, },
	{ {0x9b, 0xa0}, },
	{ {0x9b, 0x9b}, },
	{ {0x08, 0x09}, {0xe6, 0xf5}, {0xf3, 0xf4}, {0x9b, 0xe4}, {0x01, 0xed},
	{0x02, 0x03}, {0x04, 0xf2}, {0x05, 0x06}, {0xec, 0xee}, {0x07, 0xa0}, },
	{ {0x05, 0x06}, {0x9b, 0xec}, {0xf5, 0x01}, {0x02, 0xe1}, {0xef, 0xe5},
	{0xe9, 0xf2}, {0x03, 0x04}, },
	{ {0x06, 0x07}, {0x9b, 0xe9}, {0xf9, 0xf2}, {0xf5, 0x01}, {0x02, 0x03},
	{0xec, 0xef}, {0xe1, 0x04}, {0xe8, 0x05}, },
	{ {0x05, 0x06}, {0xf9, 0xf2}, {0xf5, 0x9b}, {0xe5, 0xef}, {0x01, 0x02},
	{0xe9, 0xe1}, {0x03, 0x04}, },
	{ {0x06, 0x07}, {0xe1, 0xe9}, {0xee, 0xf6}, {0xe4, 0xec}, {0xf3, 0x01},
	{0x02, 0xf2}, {0x03, 0x04}, {0x9b, 0x05}, },
	{ {0x02, 0x03}, {0xe5, 0xec}, {0x9b, 0xef}, {0x01, 0xf2}, },
	{ {0x05, 0x06}, {0xf5, 0xef}, {0x9b, 0xec}, {0xe9, 0x01}, {0xe1, 0xf2},
	{0x02, 0xe5}, {0x03, 0x04}, },
	{ {0x03, 0x04}, {0x9b, 0xe5}, {0xe9, 0xf5}, {0xe1, 0x01}, {0xef, 0x02}, },
	{ {0x04, 0x05}, {0xa0, 0xc9}, {0xf3, 0x9b}, {0xae, 0xf2}, {0x01, 0x02},
	{0x03, 0xee}, },
	{ {0xef, 0x05}, {0x9b, 0xae}, {0xe9, 0xe5}, {0x01, 0xf5}, {0x02, 0xe1},
	{0x03, 0x04}, },
	{ {0xe5, 0x03}, {0xe1, 0xe9}, {0xf2, 0x9b}, {0x01, 0x02}, },
	{ {0x03, 0x04}, {0x9b, 0xe9}, {0xf5, 0x01}, {0xe5, 0x02}, {0xef, 0xe1}, },
	{ {0xe1, 0x05}, {0x9b, 0xe3}, {0xef, 0x01}, {0xf5, 0xe5}, {0x02, 0x03},
	{0xe9, 0x04}, },
	{ {0xe5, 0x03}, {0x9b, 0xe9}, {0x01, 0xe1}, {0xef, 0x02}, },
	{ {0x03, 0x04}, {0xa7, 0xee}, {0xec, 0xf2}, {0xf3, 0x01}, {0x9b, 0x02}, },
	{ {0xe1, 0x06}, {0x9b, 0xe8}, {0xe9, 0x01}, {0xf2, 0xec}, {0x02, 0xef},
	{0x03, 0xe5}, {0x04, 0x05}, },
	{ {0x9b, 0x9b}, },
	{ {0x03, 0x04}, {0x9b, 0xae}, {0x01, 0xe9}, {0x02, 0xe1}, {0xe5, 0xef}, },
	{ {0x09, 0x0a}, {0xf6, 0xf9}, {0x01, 0xae}, {0xe3, 0xe9}, {0xf5, 0x9b},
	{0xe5, 0xef}, {0x02, 0x03}, {0xe1, 0x04}, {0xe8, 0x05}, {0x06, 0xf4},
	{0x07, 0x08}, },
	{ {0xe8, 0x07}, {0xe5, 0xf7}, {0xd6, 0xe1}, {0x9b, 0xe9}, {0xf2, 0x01},
	{0x02, 0x03}, {0x04, 0xef}, {0x05, 0x06}, },
	{ {0xae, 0x01}, {0x9b, 0xee}, },
	{ {0xe9, 0x02}, {0xe5, 0x9b}, {0xa0, 0x01}, },
	{ {0x03, 0x04}, {0x9b, 0xe8}, {0xe5, 0xe1}, {0xef, 0x01}, {0xe9, 0x02}, },
	{ {0x9b, 0x9b}, },
	{ {0x9b, 0xef}, },
	{ {0x9b, 0x9b}, },
	{ {0x9b, 0x9b}, },
	{ {0x9b, 0x9b}, },
	{ {0x9b, 0x9b}, },
	{ {0x9b, 0x9b}, },
	{ {0x9b, 0x9b}, },
	{ {0x9b, 0x9b}, },
	{ {0x18, 0x19}, {0xe8, 0xef}, {0xf8, 0x9b}, {0xa7, 0xf7}, {0xfa, 0x01},
	{0x02, 0x03}, {0x04, 0xe5}, {0xae, 0x05}, {0xe6, 0xe2}, {0x06, 0xf6},
	{0xeb, 0xf5}, {0xe9, 0x07}, {0xf0, 0xf9}, {0xe7, 0x08}, {0x09, 0xe4},
	{0x0a, 0xe3}, {0x0b, 0xed}, {0x0c, 0xf3}, {0x0d, 0x0e}, {0x0f, 0xec},
	{0x10, 0xf4}, {0x11, 0x12}, {0xf2, 0xa0}, {0x13, 0x14}, {0x15, 0xee},
	{0x16, 0x17}, },
	{ {0x0b, 0x0c}, {0xe4, 0xf3}, {0x9b, 0xae}, {0xe2, 0x01}, {0x02, 0x03},
	{0xec, 0xa0}, {0x04, 0xe9}, {0xf2, 0xf5}, {0x05, 0xf9}, {0xe1, 0x06},
	{0xef, 0x07}, {0xe5, 0x08}, {0x09, 0x0a}, },
	{ {0x0f, 0x10}, {0xf1, 0xae}, {0xc4, 0xf9}, {0xac, 0x01}, {0xe3, 0x02},
	{0x9b, 0xf2}, {0x03, 0x04}, {0xa0, 0xec}, {0xf5, 0x05}, {0x06, 0xe9},
	{0x07, 0xeb}, {0x08, 0xf4}, {0x09, 0xe5}, {0x0a, 0xef}, {0xe1, 0xe8},
	{0x0b, 0x0c}, {0x0d, 0x0e}, },
	{ {0x13, 0x14}, {0xa7, 0xbb}, {0xe6, 0xed}, {0xf7, 0xe7}, {0xf6, 0x01},
	{0x02, 0x9b}, {0xee, 0x03}, {0x04, 0xec}, {0x05, 0xf5}, {0x06, 0xac},
	{0xe4, 0xf9}, {0xf2, 0x07}, {0x08, 0x09}, {0xae, 0x0a}, {0xef, 0x0b},
	{0xe1, 0xf3}, {0x0c, 0xe9}, {0x0d, 0x0e}, {0x0f, 0x10}, {0xe5, 0x11},
	{0x12, 0xa0}, },
	{ {0x1d, 0x1e}, {0xa9, 0xe8}, {0xf5, 0x9b}, {0x01, 0xad}, {0xbb, 0xeb},
	{0xfa, 0x02}, {0xa7, 0xe6}, {0xe2, 0xe7}, {0x03, 0x04}, {0x05, 0x06},
	{0xe9, 0xf8}, {0x07, 0xac}, {0xef, 0xf0}, {0x08, 0xed}, {0xf6, 0xf9},
	{0x09, 0xf7}, {0x0a, 0x0b}, {0xae, 0x0c}, {0xe3, 0x0d}, {0xe5, 0xf4},
	{0x0e, 0x0f}, {0xe4, 0x10}, {0xec, 0x11}, {0xe1, 0x12}, {0x13, 0x14},
	{0x15, 0x16}, {0xee, 0xf3}, {0x17, 0x18}, {0xf2, 0xa0}, {0x19, 0x1a},
	{0x1b, 0x1c}, },
	{ {0x09, 0x0a}, {0xae, 0x9b}, {0xec, 0x01}, {0xf5, 0x02}, {0xf4, 0xe6},
	{0x03, 0xe1}, {0xe5, 0xe9}, {0x04, 0xf2}, {0xef, 0x05}, {0x06, 0x07},
	{0xa0, 0x08}, },
	{ {0x0e, 0x0f}, {0xad, 0xe7}, {0x9b, 0xa7}, {0xf9, 0x01}, {0xec, 0x02},
	{0xac, 0xf2}, {0x03, 0xae}, {0xf3, 0xf5}, {0x04, 0x05}, {0xef, 0x06},
	{0x07, 0xe9}, {0xe1, 0x08}, {0x09, 0xe8}, {0x0a, 0x0b}, {0xe5, 0x0c},
	{0xa0, 0x0d}, },
	{ {0x0d, 0x0e}, {0xa7, 0xac}, {0xf3, 0xad}, {0x01, 0x02}, {0x9b, 0xf9},
	{0xf5, 0xae}, {0x03, 0xee}, {0x04, 0xf2}, {0x05, 0x06}, {0xf4, 0x07},
	{0x08, 0x09}, {0xef, 0xe1}, {0xa0, 0x0a}, {0xe9, 0x0b}, {0x0c, 0xe5}, },
	{ {0x14, 0x15}, {0xac, 0xe2}, {0xf8, 0x9b}, {0xae, 0xfa}, {0x01, 0xeb},
	{0x02, 0xa0}, {0x03, 0x04}, {0xf0, 0x05}, {0x06, 0xe6}, {0xf6, 0x07},
	{0xe4, 0xed}, {0xe7, 0x08}, {0xe1, 0xef}, {0xf2, 0x09}, {0x0a, 0x0b},
	{0xec, 0x0c}, {0xe5, 0xe3}, {0x0d, 0xf4}, {0x0e, 0xf3}, {0x0f, 0x10},
	{0x11, 0xee}, {0x12, 0x13}, },
	{ {0x03, 0xef}, {0x9b, 0xe1}, {0xe5, 0xf5}, {0x01, 0x02}, },
	{ {0x08, 0x09}, {0xec, 0xf9}, {0xa7, 0xee}, {0x01, 0xac}, {0x9b, 0xae},
	{0x02, 0x03}, {0x04, 0xf3}, {0x05, 0xe9}, {0x06, 0xa0}, {0x07, 0xe5}, },
	{ {0x16, 0x17}, {0xa7, 0xad}, {0xee, 0xe3}, {0xeb, 0xf2}, {0x9b, 0xe2},
	{0x01, 0x02}, {0xf5, 0x03}, {0xf4, 0xac}, {0x04, 0x05}, {0xe6, 0xed},
	{0xf6, 0x06}, {0xae, 0xf0}, {0x07, 0x08}, {0xf3, 0x09}, {0x0a, 0xe4},
	{0x0b, 0x0c}, {0xf9, 0x0d}, {0xef, 0x0e}, {0xe1, 0x0f}, {0x10, 0xe9},
	{0xec, 0x11}, {0xa0, 0xe5}, {0x12, 0x13}, {0x14, 0x15}, },
	{ {0x0c, 0x0d}, {0xa7, 0xbb}, {0x9b, 0x01}, {0xf9, 0xae}, {0xe2, 0x02},
	{0xed, 0xf3}, {0x03, 0xf5}, {0xef, 0xf0}, {0x04, 0x05}, {0xe9, 0x06},
	{0x07, 0x08}, {0x09, 0xa0}, {0xe1, 0xe5}, {0x0a, 0x0b}, },
	{ {0x19, 0x1a}, {0xad, 0xbb}, {0xe2, 0xea}, {0xed, 0xf2}, {0xfa, 0xe6},
	{0xec, 0x01}, {0x02, 0x03}, {0x9b, 0xf5}, {0x04, 0xa7}, {0xf6, 0xf9},
	{0x05, 0x06}, {0xeb, 0xef}, {0x07, 0x08}, {0x09, 0x0a}, {0xac, 0x0b},
	{0x0c, 0xe3}, {0xae, 0x0d}, {0xee, 0xe9}, {0x0e, 0xe1}, {0x0f, 0xf3},
	{0x10, 0x11}, {0xf4, 0x12}, {0xe7, 0xe5}, {0x13, 0x14}, {0xe4, 0x15},
	{0x16, 0x17}, {0xa0, 0x18}, },
	{ {0x1a, 0x1b}, {0xc2, 0x9b}, {0xad, 0xac}, {0xf8, 0x01}, {0xae, 0x02},
	{0x03, 0xe5}, {0xe7, 0xe8}, {0xf9, 0xe9}, {0xeb, 0x04}, {0xe3, 0xe1},
	{0x05, 0xf6}, {0x06, 0xe4}, {0x07, 0xe2}, {0xf0, 0x08}, {0x09, 0xf3},
	{0xf4, 0xf7}, {0xef, 0x0a}, {0x0b, 0x0c}, {0x0d, 0xec}, {0x0e, 0x0f},
	{0x10, 0xf5}, {0xed, 0x11}, {0xe6, 0xa0}, {0x12, 0xf2}, {0x13, 0x14},
	{0x15, 0xee}, {0x16, 0x17}, {0x18, 0x19}, },
	{ {0x0e, 0x0f}, {0xad, 0xed}, {0xf9, 0x9b}, {0xae, 0x01}, {0xf3, 0x02},
	{0x03, 0xf5}, {0xf4, 0xf0}, {0x04, 0xef}, {0x05, 0xe9}, {0x06, 0xe8},
	{0xa0, 0xe1}, {0xec, 0x07}, {0xf2, 0x08}, {0xe5, 0x09}, {0x0a, 0x0b},
	{0x0c, 0x0d}, },
	{ {0x9b, 0xf5}, },
	{ {0x19, 0x1a}, {0xa9, 0xbb}, {0xf6, 0xe6}, {0x01, 0x9b}, {0xad, 0xe2},
	{0xf0, 0x02}, {0xa7, 0x03}, {0x04, 0x05}, {0xf5, 0xe3}, {0xac, 0xe7},
	{0xf2, 0x06}, {0xeb, 0x07}, {0xec, 0xed}, {0xee, 0xf9}, {0x08, 0xae},
	{0x09, 0x0a}, {0xe4, 0x0b}, {0x0c, 0xf4}, {0x0d, 0xf3}, {0x0e, 0x0f},
	{0x10, 0xe1}, {0xef, 0x11}, {0xe9, 0x12}, {0x13, 0xe5}, {0x14, 0xa0},
	{0x15, 0x16}, {0x17, 0x18}, },
	{ {0xa0, 0x16}, {0xa2, 0xa7}, {0xe2, 0xeb}, {0xed, 0xee}, {0x9b, 0xf7},
	{0x01, 0x02}, {0x03, 0xbb}, {0xf9, 0xf0}, {0x04, 0x05}, {0xec, 0x06},
	{0x07, 0x08}, {0xf5, 0xe1}, {0x09, 0xac}, {0xe3, 0x0a}, {0xe8, 0x0b},
	{0xe9, 0x0c}, {0xef, 0xf3}, {0xae, 0x0d}, {0x0e, 0xe5}, {0x0f, 0x10},
	{0x11, 0xf4}, {0x12, 0x13}, {0x14, 0x15}, },
	{ {0x14, 0x15}, {0xbb, 0xe2}, {0xad, 0xed}, {0x01, 0x9b}, {0xa7, 0xe3},
	{0xac, 0xec}, {0xee, 0x02}, {0xf7, 0x03}, {0x04, 0xf9}, {0x05, 0x06},
	{0x07, 0x08}, {0xf4, 0xae}, {0xf5, 0x09}, {0x0a, 0xf2}, {0xe1, 0xf3},
	{0x0b, 0x0c}, {0x0d, 0xe9}, {0x0e, 0x0f}, {0xef, 0xe5}, {0x10, 0xa0},
	{0xe8, 0x11}, {0x12, 0x13}, },
	{ {0x11, 0x12}, {0xef, 0xf6}, {0x9b, 0xeb}, {0xf9, 0x01}, {0xa0, 0xe2},
	{0x02, 0xe1}, {0x03, 0xed}, {0x04, 0xe3}, {0xe9, 0x05}, {0xe4, 0xe5},
	{0xe7, 0x06}, {0xec, 0xf0}, {0x07, 0x08}, {0x09, 0x0a}, {0x0b, 0xf3},
	{0x0c, 0xf4}, {0xee, 0x0d}, {0xf2, 0x0e}, {0x0f, 0x10}, },
	{ {0x05, 0xe5}, {0xf3, 0xf9}, {0x9b, 0x01}, {0xef, 0x02}, {0x03, 0xe1},
	{0x04, 0xe9}, },
	{ {0x0a, 0x0b}, {0xae, 0x9b}, {0xec, 0xed}, {0x01, 0x02}, {0xf3, 0xee},
	{0xf2, 0x03}, {0xe5, 0x04}, {0xe8, 0xa0}, {0xe1, 0x05}, {0xef, 0x06},
	{0x07, 0x08}, {0xe9, 0x09}, },
	{ {0x05, 0x06}, {0xa0, 0xac}, {0xad, 0xf4}, {0xe9, 0x01}, {0x02, 0xe1},
	{0xe5, 0x03}, {0x9b, 0x04}, },
	{ {0x11, 0xa0}, {0xbf, 0xe1}, {0xe2, 0xe6}, {0xed, 0xe4}, {0xe9, 0xf7},
	{0xa7, 0x01}, {0x02, 0xbb}, {0x03, 0x04}, {0xec, 0x05}, {0x9b, 0xee},
	{0x06, 0xef}, {0x07, 0xac}, {0xe5, 0xf3}, {0x08, 0x09}, {0x0a, 0xae},
	{0x0b, 0x0c}, {0x0d, 0x0e}, {0x0f, 0x10}, },
	{ {0x06, 0x07}, {0xa0, 0xae}, {0xe1, 0xe5}, {0xec, 0xfa}, {0x9b, 0xef},
	{0xe9, 0x01}, {0x02, 0x03}, {0x04, 0x05}, },
	{ {0x9b, 0x9b}, },
	{ {0x9b, 0x9b}, },
	{ {0x9b, 0x9b}, },
	{ {0x9b, 0x9b}, },
};

static struct hufftree_entry program_title_hufftree[][128] = {
	{ {0x1b, 0x1c}, {0xb4, 0xa4}, {0xb2, 0xb7}, {0xda, 0x01}, {0xd1, 0x02},
	{0x03, 0x9b}, {0x04, 0xd5}, {0xd9, 0x05}, {0xcb, 0xd6}, {0x06, 0xcf},
	{0x07, 0x08}, {0xca, 0x09}, {0xc9, 0xc5}, {0xc6, 0x0a}, {0xd2, 0xc4},
	{0xc7, 0xcc}, {0xd0, 0xc8}, {0xd7, 0xce}, {0x0b, 0xc1}, {0x0c, 0xc2},
	{0xcd, 0xc3}, {0x0d, 0x0e}, {0x0f, 0x10}, {0xd3, 0x11}, {0xd4, 0x12},
	{0x13, 0x14}, {0x15, 0x16}, {0x17, 0x18}, {0x19, 0x1a}, },
	{ {0x9b, 0x9b}, },
	{ {0x9b, 0x9b}, },
	{ {0x9b, 0x9b}, },
	{ {0x9b, 0x9b}, },
	{ {0x9b, 0x9b}, },
	{ {0x9b, 0x9b}, },
	{ {0x9b, 0x9b}, },
	{ {0x9b, 0x9b}, },
	{ {0x9b, 0x9b}, },
	{ {0x9b, 0x9b}, },
	{ {0x9b, 0x9b}, },
	{ {0x9b, 0x9b}, },
	{ {0x9b, 0x9b}, },
	{ {0x9b, 0x9b}, },
	{ {0x9b, 0x9b}, },
	{ {0x9b, 0x9b}, },
	{ {0x9b, 0x9b}, },
	{ {0x9b, 0x9b}, },
	{ {0x9b, 0x9b}, },
	{ {0x9b, 0x9b}, },
	{ {0x9b, 0x9b}, },
	{ {0x9b, 0x9b}, },
	{ {0x9b, 0x9b}, },
	{ {0x9b, 0x9b}, },
	{ {0x9b, 0x9b}, },
	{ {0x9b, 0x9b}, },
	{ {0x9b, 0x9b}, },
	{ {0x9b, 0x9b}, },
	{ {0x9b, 0x9b}, },
	{ {0x9b, 0x9b}, },
	{ {0x9b, 0x9b}, },
	{ {0x29, 0x2a}, {0xd8, 0xe5}, {0xb9, 0x01}, {0xa7, 0xb1}, {0xec, 0xd1},
	{0x02, 0xad}, {0xb2, 0xda}, {0xe3, 0xb3}, {0x03, 0xe4}, {0xe6, 0x04},
	{0x9b, 0xe2}, {0x05, 0x06}, {0x07, 0x08}, {0x09, 0xd5}, {0x0a, 0xd6},
	{0x0b, 0xd9}, {0x0c, 0xa6}, {0xe9, 0xcb}, {0xc5, 0xcf}, {0x0d, 0x0e},
	{0xca, 0xc9}, {0x0f, 0xc7}, {0x10, 0x11}, {0xe1, 0x12}, {0x13, 0xc6},
	{0xd2, 0xc8}, {0xce, 0xc1}, {0xc4, 0xd0}, {0xcc, 0x14}, {0x15, 0xef},
	{0xc2, 0xd7}, {0x16, 0xcd}, {0x17, 0xf4}, {0xd4, 0x18}, {0x19, 0x1a},
	{0xc3, 0xd3}, {0x1b, 0x1c}, {0x1d, 0x1e}, {0x1f, 0x20}, {0x21, 0x22},
	{0x23, 0x24}, {0x25, 0x26}, {0x27, 0x28}, },
	{ {0x01, 0x80}, {0xa0, 0x9b}, },
	{ {0x9b, 0x9b}, },
	{ {0x9b, 0x9b}, },
	{ {0xb1, 0x9b}, },
	{ {0x9b, 0x9b}, },
	{ {0x9b, 0xa0}, },
	{ {0x04, 0xf3}, {0xe4, 0xb9}, {0x01, 0xf4}, {0xa0, 0x9b}, {0x02, 0x03}, },
	{ {0x9b, 0x9b}, },
	{ {0x9b, 0x9b}, },
	{ {0x01, 0x02}, {0x9b, 0xc1}, {0xc8, 0xd3}, },
	{ {0x9b, 0x9b}, },
	{ {0x9b, 0xa0}, },
	{ {0x07, 0x08}, {0xb1, 0xd2}, {0xd3, 0xd4}, {0xd5, 0xad}, {0xcd, 0xc1},
	{0x01, 0x02}, {0x03, 0xa0}, {0x04, 0x9b}, {0x05, 0x06}, },
	{ {0xa0, 0x05}, {0xc9, 0xd7}, {0xd3, 0x01}, {0x02, 0x9b}, {0xae, 0x80},
	{0x03, 0x04}, },
	{ {0x9b, 0x9b}, },
	{ {0x02, 0x03}, {0xad, 0x9b}, {0x01, 0x80}, {0xa0, 0xb0}, },
	{ {0x04, 0x05}, {0x80, 0x9b}, {0xb1, 0xb2}, {0xa0, 0xb0}, {0xb9, 0x01},
	{0x02, 0x03}, },
	{ {0x02, 0x03}, {0xb1, 0xba}, {0x01, 0xb0}, {0x9b, 0x80}, },
	{ {0x80, 0x01}, {0xb0, 0x9b}, },
	{ {0x9b, 0xb8}, },
	{ {0x9b, 0x9b}, },
	{ {0x9b, 0x9b}, },
	{ {0x9b, 0xb0}, },
	{ {0x9b, 0xa0}, },
	{ {0x02, 0x03}, {0xb1, 0xb3}, {0xb9, 0xb0}, {0x01, 0x9b}, },
	{ {0x9b, 0xa0}, },
	{ {0x9b, 0x9b}, },
	{ {0x9b, 0x9b}, },
	{ {0x9b, 0x9b}, },
	{ {0x9b, 0x9b}, },
	{ {0x9b, 0x80}, },
	{ {0x9b, 0x9b}, },
	{ {0x13, 0x14}, {0xaa, 0xad}, {0xae, 0xf6}, {0xe7, 0xf4}, {0xe2, 0xe9},
	{0x01, 0x02}, {0xc2, 0xf0}, {0x9b, 0xf3}, {0xe3, 0xe6}, {0xf7, 0x03},
	{0xf5, 0x04}, {0x05, 0x06}, {0xf2, 0x07}, {0x08, 0x09}, {0x0a, 0x0b},
	{0x0c, 0xe4}, {0xa0, 0x0d}, {0xec, 0xee}, {0x0e, 0xed}, {0x0f, 0x10},
	{0x11, 0x12}, },
	{ {0x08, 0x09}, {0xc1, 0xd3}, {0x9b, 0x01}, {0xc3, 0x02}, {0xe9, 0xec},
	{0x03, 0xf2}, {0xf5, 0x04}, {0xef, 0xe1}, {0x05, 0xe5}, {0x06, 0x07}, },
	{ {0x0b, 0x0c}, {0xc1, 0xf9}, {0x01, 0xc2}, {0xcf, 0xe5}, {0xf5, 0x9b},
	{0xe9, 0x02}, {0xa0, 0x03}, {0x04, 0x05}, {0xf2, 0x06}, {0xec, 0x07},
	{0xe1, 0x08}, {0x09, 0xe8}, {0x0a, 0xef}, },
	{ {0x05, 0x06}, {0xf9, 0x9b}, {0x01, 0xf5}, {0x02, 0xf2}, {0xe9, 0xe5},
	{0xef, 0x03}, {0xe1, 0x04}, },
	{ {0x0a, 0x0b}, {0xf1, 0xf5}, {0xf3, 0x01}, {0xed, 0xf9}, {0xc3, 0x02},
	{0xec, 0xee}, {0xe4, 0xf8}, {0x03, 0x9b}, {0xf6, 0x04}, {0x05, 0xe1},
	{0x06, 0x07}, {0x08, 0x09}, },
	{ {0x07, 0x08}, {0xa0, 0x9b}, {0xcc, 0x01}, {0xe5, 0x02}, {0xec, 0xf5},
	{0xef, 0x03}, {0xe9, 0xf2}, {0x04, 0x05}, {0xe1, 0x06}, },
	{ {0x09, 0x0a}, {0xae, 0xec}, {0xf9, 0xc1}, {0xe8, 0x01}, {0x9b, 0x02},
	{0x03, 0x04}, {0xe1, 0xf5}, {0xe9, 0x05}, {0xe5, 0x06}, {0xf2, 0xef},
	{0x07, 0x08}, },
	{ {0xef, 0x05}, {0x80, 0x9b}, {0xf5, 0x01}, {0x02, 0xe9}, {0xe1, 0x03},
	{0xe5, 0x04}, },
	{ {0xee, 0x0b}, {0xba, 0xd4}, {0xae, 0xf2}, {0xe3, 0x01}, {0xa0, 0x02},
	{0x80, 0x9b}, {0xed, 0x03}, {0xc9, 0xf3}, {0xf4, 0x04}, {0x05, 0x06},
	{0x07, 0x08}, {0x09, 0x0a}, },
	{ {0x02, 0x03}, {0x9b, 0xf5}, {0x01, 0xe1}, {0xef, 0xe5}, },
	{ {0x05, 0xe9}, {0xe1, 0xef}, {0xf5, 0xee}, {0x9b, 0xe5}, {0x01, 0x02},
	{0x03, 0x04}, },
	{ {0x04, 0x05}, {0xa0, 0x9b}, {0x01, 0xf5}, {0x02, 0xe5}, {0xef, 0x03},
	{0xe1, 0xe9}, },
	{ {0x08, 0x09}, {0xaa, 0xd4}, {0x01, 0x9b}, {0xe3, 0x02}, {0xf2, 0x03},
	{0xe5, 0x04}, {0xf5, 0xf9}, {0xe9, 0x05}, {0xef, 0x06}, {0x07, 0xe1}, },
	{ {0xe5, 0x08}, {0xce, 0xa0}, {0xc6, 0xf5}, {0x01, 0x02}, {0x9b, 0xc2},
	{0x03, 0xe1}, {0x04, 0xef}, {0x05, 0xe9}, {0x06, 0x07}, },
	{ {0x09, 0x0a}, {0xe4, 0xf3}, {0xe6, 0xf6}, {0xf7, 0xf0}, {0xf2, 0x01},
	{0xec, 0x02}, {0x03, 0xa0}, {0x9b, 0x04}, {0x05, 0xf5}, {0x06, 0x07},
	{0xee, 0x08}, },
	{ {0x0b, 0x0c}, {0xa0, 0xf3}, {0xf9, 0xae}, {0xd2, 0xc7}, {0x01, 0x9b},
	{0x02, 0xf5}, {0x03, 0x04}, {0x05, 0xe9}, {0xec, 0x06}, {0xe5, 0x07},
	{0xef, 0x08}, {0xe1, 0x09}, {0xf2, 0x0a}, },
	{ {0x01, 0xf5}, {0x9b, 0xd6}, },
	{ {0x04, 0x05}, {0xe8, 0x9b}, {0x01, 0xf5}, {0x02, 0xe1}, {0xe9, 0xef},
	{0x03, 0xe5}, },
	{ {0x10, 0x11}, {0xaa, 0xec}, {0xf1, 0xae}, {0xa0, 0xf7}, {0xed, 0xee},
	{0x01, 0x02}, {0x9b, 0xeb}, {0x03, 0x04}, {0x05, 0x06}, {0xe3, 0x07},
	{0xef, 0x08}, {0xe9, 0xf5}, {0x09, 0xe1}, {0xe5, 0xf0}, {0xe8, 0x0a},
	{0x0b, 0x0c}, {0x0d, 0xf4}, {0x0e, 0x0f}, },
	{ {0xe8, 0x0a}, {0xad, 0xce}, {0x9b, 0x01}, {0xd6, 0x02}, {0xf5, 0xf7},
	{0x03, 0x04}, {0xe1, 0xe5}, {0xe9, 0x05}, {0xf2, 0x06}, {0xef, 0x07},
	{0x08, 0x09}, },
	{ {0xee, 0x03}, {0xec, 0xae}, {0x01, 0x9b}, {0x02, 0xf0}, },
	{ {0x06, 0xe9}, {0xa0, 0xc3}, {0xef, 0x9b}, {0xe5, 0x01}, {0x80, 0x02},
	{0x03, 0xe1}, {0x04, 0x05}, },
	{ {0x06, 0x07}, {0xc6, 0xd7}, {0x01, 0x9b}, {0xf2, 0x02}, {0x03, 0xe8},
	{0xe5, 0xe1}, {0x04, 0xe9}, {0xef, 0x05}, },
	{ {0x9b, 0x9b}, },
	{ {0x02, 0xef}, {0xe1, 0x9b}, {0x01, 0xe5}, },
	{ {0x01, 0xef}, {0x9b, 0xe1}, },
	{ {0x9b, 0x9b}, },
	{ {0x9b, 0x9b}, },
	{ {0x9b, 0x9b}, },
	{ {0x9b, 0x9b}, },
	{ {0x9b, 0x9b}, },
	{ {0x9b, 0x9b}, },
	{ {0x19, 0x1a}, {0x9b, 0xba}, {0xe5, 0xea}, {0xf8, 0x01}, {0x02, 0xe6},
	{0xa7, 0x03}, {0xfa, 0xe8}, {0x04, 0xf7}, {0x05, 0xf5}, {0xe2, 0x06},
	{0xeb, 0x07}, {0xf0, 0x08}, {0x80, 0xf6}, {0xe7, 0x09}, {0xe4, 0x0a},
	{0xa0, 0xe9}, {0x0b, 0xe3}, {0xf9, 0x0c}, {0x0d, 0xed}, {0x0e, 0x0f},
	{0xf3, 0x10}, {0x11, 0xec}, {0x12, 0xf4}, {0xf2, 0x13}, {0xee, 0x14},
	{0x15, 0x16}, {0x17, 0x18}, },
	{ {0x0a, 0x0b}, {0xf3, 0x9b}, {0xf5, 0xe2}, {0x01, 0x80}, {0xa0, 0x02},
	{0xe5, 0xf2}, {0xe9, 0x03}, {0xec, 0x04}, {0xf9, 0x05}, {0xef, 0x06},
	{0xe1, 0x07}, {0x08, 0x09}, },
	{ {0x10, 0x11}, {0xc3, 0xcc}, {0xc7, 0x9b}, {0xe3, 0x01}, {0x80, 0xec},
	{0xf9, 0x02}, {0xf3, 0x03}, {0xf5, 0x04}, {0x05, 0xf2}, {0x06, 0xe9},
	{0xa0, 0x07}, {0x08, 0xef}, {0xf4, 0x09}, {0x0a, 0xe1}, {0x0b, 0xe8},
	{0xeb, 0xe5}, {0x0c, 0x0d}, {0x0e, 0x0f}, },
	{ {0x0e, 0x0f}, {0xae, 0xf5}, {0xf7, 0x01}, {0xec, 0x02}, {0xe4, 0xe7},
	{0xf2, 0x03}, {0x9b, 0xef}, {0x04, 0xf6}, {0x05, 0x06}, {0xf9, 0xf3},
	{0x07, 0xe9}, {0xe1, 0x08}, {0x09, 0x80}, {0x0a, 0x0b}, {0xe5, 0x0c},
	{0x0d, 0xa0}, },
	{ {0x1e, 0x1f}, {0x9b, 0xa1}, {0xad, 0xe8}, {0xea, 0xf1}, {0xf5, 0xfa},
	{0x01, 0x02}, {0x03, 0x04}, {0xba, 0xf8}, {0xa7, 0xe2}, {0xe9, 0x05},
	{0x06, 0x07}, {0xe6, 0xed}, {0xe7, 0xeb}, {0x08, 0x09}, {0xf6, 0xf0},
	{0x0a, 0xef}, {0x0b, 0xe3}, {0x0c, 0x0d}, {0x0e, 0xf9}, {0x0f, 0xe4},
	{0xec, 0x10}, {0xe5, 0x11}, {0xf4, 0xf7}, {0x12, 0x13}, {0xe1, 0x14},
	{0x15, 0x16}, {0xee, 0xf3}, {0x17, 0x80}, {0x18, 0x19}, {0xf2, 0x1a},
	{0x1b, 0xa0}, {0x1c, 0x1d}, },
	{ {0xa0, 0x0b}, {0xf5, 0x9b}, {0x01, 0xec}, {0xf3, 0xf2}, {0x80, 0xe1},
	{0x02, 0x03}, {0xf4, 0xe9}, {0xef, 0xe6}, {0x04, 0x05}, {0x06, 0x07},
	{0xe5, 0x08}, {0x09, 0x0a}, },
	{ {0x0f, 0x10}, {0xba, 0xf9}, {0xa7, 0xf4}, {0x9b, 0x01}, {0xe7, 0xec},
	{0x02, 0xee}, {0x03, 0xef}, {0xf5, 0x04}, {0xf2, 0x05}, {0x06, 0xe9},
	{0x07, 0xf3}, {0xe1, 0x08}, {0x09, 0x0a}, {0x0b, 0xe5}, {0x80, 0x0c},
	{0xe8, 0xa0}, {0x0d, 0x0e}, },
	{ {0xe5, 0x0d}, {0xe2, 0xf5}, {0xf7, 0x9b}, {0xec, 0x01}, {0xf9, 0xee},
	{0x02, 0x03}, {0x04, 0xf2}, {0x05, 0x80}, {0x06, 0xa0}, {0xe1, 0xef},
	{0x07, 0xf4}, {0xe9, 0x08}, {0x09, 0x0a}, {0x0b, 0x0c}, },
	{ {0x15, 0x16}, {0xa1, 0xf8}, {0xe9, 0xeb}, {0x01, 0x80}, {0x9b, 0xfa},
	{0xe2, 0x02}, {0x03, 0x04}, {0xa0, 0xf0}, {0x05, 0x06}, {0x07, 0xe1},
	{0x08, 0xe6}, {0xf2, 0xed}, {0xf6, 0x09}, {0xe4, 0x0a}, {0xef, 0xf4},
	{0xec, 0xf3}, {0xe7, 0xe5}, {0x0b, 0xe3}, {0x0c, 0x0d}, {0x0e, 0x0f},
	{0x10, 0x11}, {0x12, 0x13}, {0xee, 0x14}, },
	{ {0xef, 0x01}, {0x9b, 0xe1}, },
	{ {0x0b, 0x0c}, {0xd4, 0xef}, {0xe6, 0xec}, {0xf7, 0xe1}, {0x01, 0xba},
	{0x02, 0x9b}, {0xf9, 0x03}, {0x04, 0x05}, {0xf3, 0x06}, {0x07, 0x08},
	{0xe9, 0xa0}, {0x09, 0x80}, {0xe5, 0x0a}, },
	{ {0x15, 0x16}, {0xa7, 0xba}, {0xe3, 0xf7}, {0xf2, 0xad}, {0xe2, 0x01},
	{0x02, 0x9b}, {0xe6, 0x03}, {0xed, 0xf6}, {0x04, 0xeb}, {0x05, 0xf4},
	{0x06, 0x07}, {0x08, 0xf3}, {0x09, 0xf5}, {0x0a, 0xef}, {0x0b, 0x0c},
	{0x80, 0xf9}, {0xe1, 0x0d}, {0xe4, 0xe9}, {0xa0, 0x0e}, {0x0f, 0xec},
	{0xe5, 0x10}, {0x11, 0x12}, {0x13, 0x14}, },
	{ {0x0a, 0x0b}, {0xf9, 0x9b}, {0xf5, 0xf3}, {0x01, 0x02}, {0xe2, 0xed},
	{0x80, 0x03}, {0xf0, 0xef}, {0x04, 0xa0}, {0x05, 0xe9}, {0x06, 0xe1},
	{0x07, 0x08}, {0x09, 0xe5}, },
	{ {0x18, 0x19}, {0xe2, 0xea}, {0xf2, 0xe8}, {0xec, 0xed}, {0xfa, 0x9b},
	{0x01, 0xf5}, {0x02, 0x03}, {0xf6, 0x04}, {0xba, 0xe6}, {0x05, 0x06},
	{0xeb, 0xef}, {0x07, 0xa7}, {0xf9, 0x08}, {0x09, 0x0a}, {0x0b, 0xe3},
	{0x0c, 0xee}, {0xe1, 0x0d}, {0xf3, 0x0e}, {0xe9, 0x0f}, {0x10, 0xf4},
	{0x80, 0xe4}, {0xe5, 0x11}, {0x12, 0xe7}, {0xa0, 0x13}, {0x14, 0x15},
	{0x16, 0x17}, },
	{ {0x1b, 0x1c}, {0xae, 0xfa}, {0xbf, 0x01}, {0xa7, 0x9b}, {0x02, 0xe9},
	{0xf8, 0xf9}, {0x03, 0xe5}, {0xe8, 0x04}, {0xe1, 0xeb}, {0x05, 0xe2},
	{0x06, 0x07}, {0xe3, 0x08}, {0xe7, 0xf4}, {0x09, 0x80}, {0xf6, 0xf0},
	{0x0a, 0xe4}, {0x0b, 0xf3}, {0xf7, 0x0c}, {0x0d, 0xef}, {0xec, 0xa0},
	{0x0e, 0x0f}, {0xed, 0xe6}, {0x10, 0xf5}, {0x11, 0x12}, {0x13, 0x14},
	{0x15, 0xf2}, {0x16, 0xee}, {0x17, 0x18}, {0x19, 0x1a}, },
	{ {0x0e, 0x0f}, {0xed, 0xa7}, {0x9b, 0xe4}, {0x01, 0xf9}, {0xf3, 0xf2},
	{0xf4, 0x02}, {0xe8, 0x03}, {0xec, 0xf0}, {0x04, 0xe1}, {0xe9, 0x05},
	{0x06, 0x80}, {0xa0, 0x07}, {0x08, 0x09}, {0x0a, 0xe5}, {0xef, 0x0b},
	{0x0c, 0x0d}, },
	{ {0x9b, 0xf5}, },
	{ {0x18, 0x19}, {0xba, 0xac}, {0xf6, 0x9b}, {0xf0, 0xe2}, {0x01, 0xe6},
	{0x02, 0xa7}, {0xae, 0xe7}, {0x03, 0xe3}, {0xf5, 0x04}, {0xed, 0x05},
	{0x06, 0x07}, {0xeb, 0x08}, {0x09, 0xee}, {0xf2, 0x0a}, {0xe4, 0x0b},
	{0xf9, 0xec}, {0x0c, 0x0d}, {0xf4, 0x80}, {0x0e, 0xef}, {0xf3, 0xa0},
	{0xe1, 0x0f}, {0xe9, 0x10}, {0x11, 0xe5}, {0x12, 0x13}, {0x14, 0x15},
	{0x16, 0x17}, },
	{ {0x19, 0x1a}, {0xa7, 0xac}, {0xbf, 0xc3}, {0xc8, 0xe4}, {0xe6, 0xed},
	{0xf2, 0xae}, {0xec, 0xee}, {0xf9, 0x01}, {0x02, 0x03}, {0x04, 0xba},
	{0x05, 0x9b}, {0xf5, 0x06}, {0x07, 0x08}, {0x09, 0xeb}, {0xf0, 0x0a},
	{0x0b, 0x0c}, {0xe1, 0xe3}, {0x0d, 0xe8}, {0x0e, 0x0f}, {0xef, 0x10},
	{0x11, 0xf3}, {0x12, 0xe9}, {0x13, 0xe5}, {0x14, 0x15}, {0xf4, 0x16},
	{0x17, 0xa0}, {0x18, 0x80}, },
	{ {0x14, 0x15}, {0xba, 0xbf}, {0xe4, 0xf7}, {0x9b, 0xa7}, {0x01, 0xee},
	{0x02, 0x03}, {0x04, 0xe3}, {0xe2, 0xed}, {0x05, 0xf9}, {0x06, 0xf4},
	{0x07, 0xec}, {0x08, 0xf5}, {0xf2, 0x09}, {0xe1, 0xf3}, {0x0a, 0xef},
	{0x0b, 0x0c}, {0x0d, 0xe9}, {0x80, 0xe5}, {0x0e, 0xa0}, {0x0f, 0xe8},
	{0x10, 0x11}, {0x12, 0x13}, },
	{ {0x11, 0x12}, {0xeb, 0xfa}, {0x80, 0xe6}, {0x9b, 0x01}, {0xa0, 0x02},
	{0x03, 0xe9}, {0xe1, 0x04}, {0xe4, 0xf0}, {0xed, 0xe2}, {0xe3, 0xe7},
	{0xec, 0x05}, {0xe5, 0x06}, {0x07, 0x08}, {0x09, 0xf4}, {0x0a, 0x0b},
	{0x0c, 0xf3}, {0xee, 0x0d}, {0x0e, 0xf2}, {0x0f, 0x10}, },
	{ {0x04, 0xe5}, {0xf3, 0xef}, {0x9b, 0x01}, {0xe1, 0x02}, {0x03, 0xe9}, },
	{ {0x0b, 0x0c}, {0xa7, 0xe2}, {0xec, 0xe3}, {0xf2, 0x01}, {0x9b, 0x02},
	{0x03, 0x04}, {0xe9, 0xef}, {0xee, 0xe5}, {0xe1, 0x80}, {0x05, 0xa0},
	{0x06, 0x07}, {0x08, 0x09}, {0xf3, 0x0a}, },
	{ {0x05, 0x06}, {0x9b, 0xa0}, {0xe1, 0xe5}, {0xe9, 0x01}, {0x80, 0xf0},
	{0x02, 0xf4}, {0x03, 0x04}, },
	{ {0xa0, 0x13}, {0xe3, 0xad}, {0xe4, 0xe9}, {0xee, 0xef}, {0xf0, 0xf4},
	{0xf6, 0xa1}, {0xe1, 0xed}, {0x01, 0xe2}, {0x02, 0x03}, {0x04, 0xa7},
	{0x05, 0x06}, {0xf7, 0x07}, {0x9b, 0xec}, {0x08, 0xe5}, {0x09, 0x0a},
	{0x0b, 0x0c}, {0x0d, 0x0e}, {0xf3, 0x0f}, {0x10, 0x11}, {0x80, 0x12}, },
	{ {0x05, 0x06}, {0xe5, 0xfa}, {0xa0, 0xf9}, {0x9b, 0x01}, {0x80, 0xe9},
	{0x02, 0xe1}, {0x03, 0x04}, },
	{ {0x9b, 0x9b}, },
	{ {0x9b, 0x9b}, },
	{ {0x9b, 0x9b}, },
	{ {0x9b, 0x9b}, },
};



static inline void huffbuff_init(struct huffbuff *hbuf, uint8_t *buf, uint32_t buf_len)
{
	memset(hbuf, 0, sizeof(struct huffbuff));
	hbuf->buf = buf;
	hbuf->buf_len = buf_len;
}

static inline int huffbuff_bits(struct huffbuff *hbuf, uint8_t nbits)
{
	uint8_t result = 0;

	if (nbits > 8)
		return -1;

	while (nbits--) {
		if (hbuf->cur_byte >= hbuf->buf_len) {
			return -1;
		}

		result <<= 1;
		if (hbuf->buf[hbuf->cur_byte] & (0x80 >> hbuf->cur_bit))
			result |= 1;

		if (++hbuf->cur_bit > 7) {
			hbuf->cur_byte++;
			hbuf->cur_bit = 0;
		}
	}

	return result;
}

static inline int append_unicode_char(uint8_t **destbuf, size_t *destbuflen, size_t *destbufpos,
				      uint32_t c)
{
	uint8_t tmp[3];
	int tmplen = 0;

	// encode the unicode character first of all
	if (c < 0x80) {
		tmp[0] = c;
		tmplen = 1;
	} else if (c < 0x800) {
		tmp[0] = 0xc0 | ((c >> 6) & 0x1f);
		tmp[1] = 0x80 | (c & 0x3f);
		tmplen = 2;
	} else if (c < 0x10000) {
		tmp[0] = 0xe0 | ((c >> 12) & 0x0f);
		tmp[1] = 0x80 | ((c >> 6) & 0x3f);
		tmp[2] = 0x80 | (c & 0x3f);
		tmplen = 3;
	} else {
		return -1;
	}

	// do we have enough buffer space?
	if ((*destbufpos + tmplen) >= *destbuflen) {
		uint8_t *new_dest = realloc(*destbuf, *destbuflen + DEST_ALLOC_DELTA);
		if (new_dest == NULL)
			return -ENOMEM;
		*destbuf = new_dest;
		*destbuflen += DEST_ALLOC_DELTA;
	}

	// copy it into position
	memcpy(*destbuf + *destbufpos, tmp, tmplen);
	*destbufpos += tmplen;

	return 0;
}

static inline int unicode_decode(uint8_t *srcbuf, size_t srcbuflen, int mode,
				 uint8_t **destbuf, size_t *destbuflen, size_t *destbufpos)
{
	size_t i;
	uint32_t msb = mode << 8;

	for (i=0; i< srcbuflen; i++) {
		if (append_unicode_char(destbuf, destbuflen, destbufpos, msb + srcbuf[i]))
			return -1;
	}

	return *destbufpos;
}

static int huffman_decode_uncompressed(struct huffbuff *hbuf,
				       uint8_t **destbuf, size_t *destbuflen, size_t *destbufpos)
{
	int c;

	while (hbuf->cur_byte < hbuf->buf_len) {
		// get next byte
		if ((c = huffbuff_bits(hbuf, 8)) < 0)
			return -1;

		switch (c) {
		case HUFFSTRING_END:
			return 0;

		case HUFFSTRING_ESCAPE:
			return HUFFSTRING_ESCAPE;

		default:
			if (append_unicode_char(destbuf, destbuflen, destbufpos, c))
				return -1;

			// if it is 7 bit, we swap back to the compressed context
			if ((c & 0x80) == 0)
				return c;

			// characters following an 8 bit uncompressed char are uncompressed as well
			break;
		}
	}

	// ran out of string; pretend we saw an end of string char
	return HUFFSTRING_END;
}

static int huffman_decode(uint8_t *src, size_t srclen,
			  uint8_t **destbuf, size_t *destbuflen, size_t *destbufpos,
			  struct hufftree_entry hufftree[][128])
{
	struct huffbuff hbuf;
	int bit;
	struct hufftree_entry *tree = hufftree[0];
	uint8_t treeidx = 0;
	uint8_t treeval;
	int tmp;

	huffbuff_init(&hbuf, src, srclen);

	while (hbuf.cur_byte < hbuf.buf_len) {
		// get the next bit
		if ((bit = huffbuff_bits(&hbuf, 1)) < 0)
			return *destbufpos;

		if (!bit) {
			treeval = tree[treeidx].left_idx;
		} else {
			treeval = tree[treeidx].right_idx;
		}

		if (treeval & HUFFTREE_LITERAL_MASK) {
			switch (treeval & ~HUFFTREE_LITERAL_MASK) {
			case HUFFSTRING_END:
				return 0;

			case HUFFSTRING_ESCAPE:
				if ((tmp =
					huffman_decode_uncompressed(&hbuf,
							destbuf, destbuflen, destbufpos)) < 0)
					return tmp;
				if (tmp == 0)
					return *destbufpos;

				tree = hufftree[tmp];
				treeidx = 0;
				break;

			default:
				// stash it
				if (append_unicode_char(destbuf, destbuflen, destbufpos,
								treeval & ~HUFFTREE_LITERAL_MASK))
					return -1;
				tree = hufftree[treeval & ~HUFFTREE_LITERAL_MASK];
				treeidx = 0;
				break;
			}
		} else {
			treeidx = treeval;
		}
	}

	return *destbufpos;
}

int atsc_text_segment_decode(struct atsc_text_string_segment *segment,
			     uint8_t **destbuf, size_t *destbufsize, size_t *destbufpos)
{
	if (segment->mode > ATSC_TEXT_SEGMENT_MODE_UNICODE_RANGE_MAX)
		return -1;

	// mode==0 MUST be used for compressed text
	if ((segment->mode) && (segment->compression_type))
		return -1;

	uint8_t *buf = atsc_text_string_segment_bytes(segment);

	switch (segment->compression_type) {
	case ATSC_TEXT_COMPRESS_NONE:
		return unicode_decode(buf, segment->number_bytes, segment->mode,
				      destbuf, destbufsize, destbufpos);

	case ATSC_TEXT_COMPRESS_PROGRAM_TITLE:
		return huffman_decode(buf, segment->number_bytes,
				      destbuf, destbufsize, destbufpos,
				      program_title_hufftree);

	case ATSC_TEXT_COMPRESS_PROGRAM_DESCRIPTION:
		return huffman_decode(buf, segment->number_bytes,
				      destbuf, destbufsize, destbufpos,
				      program_description_hufftree);
	}

	return -1;
}