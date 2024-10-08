#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hashtable.h"

typedef uint64_t hashindex;

static int maxbuckets; /* number of buckets */

static hashindex hash(const char *key);
static hashindex hash_key(const char* key);

hashtable hashtable_init(size_t element_size, u_int32_t hashmax) {
	hashtable h;

	DEBUG("ratelimit: hashtable_init creating hashtable with %d buckets", hashmax);
	h = talloc_array_size(NULL, element_size, hashmax);
	maxbuckets = hashmax;
	return h;
}

void insert(void *datastore, void *value, const char *key) {
	hashtable *htable = datastore;
	int index = hash(key);

	INFO("insert into hashtable bucket with key %s at index %d", key, index);
	DEBUG("This is a debug message for when user is bob");
	htable[index] = talloc_steal(datastore, value);
}

void *lookup(void *datastore, const char *key) {
	long hash_index;
	hashtable *htable;
	INFO("hashtable.c: lookup(key=%s)", key);

	htable = datastore;
	hash_index = hash(key);
	fr_assert(hash_index >= 0 && hash_index < maxbuckets);
	return htable[hash_index];
}

void delete(UNUSED void *datastore, UNUSED const char *key) {
	ERROR("hashtable->delete: not implemented");
}

static hashindex hash(const char *key) {
	uint64_t h_key;
	INFO("hash(%s)", key);

	h_key = hash_key(key);
	return h_key % maxbuckets;
}

#define FNV_OFFSET 14695981039346656037UL
#define FNV_PRIME 1099511628211UL

/*
 * Return 64-bit FNV-1a hash for key (NUL-terminated). See description:
 * https://en.wikipedia.org/wiki/Fowler–Noll–Vo_hash_function
 */
static hashindex hash_key(const char* key) {
	hashindex hash = FNV_OFFSET;
	for (const char* p = key; *p; p++) {
		hash ^= (uint64_t)(unsigned char)(*p);
		hash *= FNV_PRIME;
	}
	return hash;
}
