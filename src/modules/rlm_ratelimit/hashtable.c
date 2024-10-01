#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hashtable.h"

typedef uint64_t hashindex;

static bucketRef *buckets;

hashindex hash(const char *key);
static hashindex hash_key(const char* key);

void *hashtable_init(u_int32_t hashmax, size_t bucket_size) {
	INFO("hashtable_init creating hashtable with %d buckets", hashmax);
	buckets = calloc(hashmax, bucket_size);
	maxbuckets = hashmax;
	return buckets;
}

void insert(void *datastore, void *value, const char *key) {
	bucketRef *hashtable = datastore;
	int index = hash(key);

	INFO("insert into hashtable bucket with key %s at index %d", key, index);
	hashtable[index] = value;
}

void *lookup(void *datastore, const char *key) {
	long hash_index;
	bucketRef *hashtable;
	INFO("hashtable.c: lookup(key=%s)", key);

	hashtable = datastore;
	hash_index = hash(key);
	fr_assert(hash_index >= 0 && hash_index < maxbuckets);

	return hashtable[hash_index];
}

void delete(UNUSED void *datastore, UNUSED const char *key) {
	ERROR("hashtable->delete: not implemented");
}

hashindex hash(const char *key) {
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
