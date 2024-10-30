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
 * @file fixedds.c
 * @brief Fixed length datastore for token bucket storage.
 *
 * @copyright 2024 The FreeRADIUS server project
 * @copyright 2024 your name <TODO>
 */
RCSID("$Id$")

#include "fixedds.h"

// static long current_time_in_ms(void);
static int index_from_key(const char *key, uint32_t *index);

void *datastore_init(uint32_t listlength) {
    BucketList *b;

    INFO("ratelimit: datastore_init(). Creating bucket store with capacity %d.", listlength);
	b = talloc(NULL, BucketList);
	b->buckets = (Bucket *) talloc_array_size(b, sizeof(Bucket), listlength);
    INFO("ratelimit: datastore_init() - storage allocated: %ld bytes", talloc_total_size(b));
	for (uint32_t i=0; i<listlength; i++) {
		b->buckets[i].accessed = 0;
	}
	return b;
}

/*
 * insert
 */
Bucket *insert(void *datastore, Bucket data, const char *key) {
    BucketList *list = datastore;
	uint32_t index;

    INFO("ratelimit: Hello from datastore insert()");
    list = datastore;

    if (index_from_key(key, &index) == -1) {
        return NULL;
    }

    INFO("ratelimit: value will be inserted at index %d", index);

    list->buckets[index].tokens = data.tokens;
    list->buckets[index].accessed = data.accessed;
    return &(list->buckets[index]);
}

/*
 * lookup returns the entry in the datastore with the given key or NULL if the datastore
 * doesn't contain an entry for key.
 */
Bucket *lookup(void *datastore, const char *key) {
    BucketList *list;
	uint32_t index;

    list = datastore;

    INFO("ratelimit: Hello from datastroe lookup()()");
	index_from_key(key, &index);
    INFO("ratelimit: return from index_from_key() %s %d %x", key, index, index);
    printf("ratelimt: index from key %s\n", key);
    return &(list->buckets[index]);
}

/*
 * index_from_key return the left most 24 bit from mac address
 */
static int index_from_key(const char *key, uint32_t *index) {
    uint values[6];
    uint64_t int_val;

    INFO("ratelimit: index_from_key()");

    /* 6 components to MAC but the last 3 (extension ID) is used as the index */
    if (6 == sscanf(key, "%x:%x:%x:%x:%x:%x", &values[0], &values[1], &values[2], &values[3], &values[4], &values[5])) {

        /* convert extension ID into an integer */
        int_val = values[3] << 16 | values[4] << 8 | values[5];
        (*index) = int_val;
        return 0; // Success
    }

    INFO("ratelimit: invalid MAC address");
    return -1; // Invalid MAC address string
}
