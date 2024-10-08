/*
 *   This program is is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or (at
 *   your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

/**
 * $Id$
 *
 * @file hastable.c
 * @brief Hash table for use with freeRADIUS rate limiting.
 *
 * @copyright 2024 The FreeRADIUS server project
 * @copyright 2024 your name \<your address\>
 */
RCSIDH(hashtable_h, "$Id$")

#include <freeradius-devel/radiusd.h>

typedef void* hashtable;

void *hashtable_init(size_t element_size, u_int32_t hashmax);
void insert(void *datastore, void *value, const char *key);
void *lookup(void *datastore, const char *key);
void delete(void *datastore, const char *key);
