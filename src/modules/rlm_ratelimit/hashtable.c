#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hashtable.h"

typedef uint64_t hashindex;

static int maxbuckets; /* number of buckets */

static hashindex hash(const char *key);
static hashindex hash_key(const char* key);

HashTable *hashtable_init(size_t element_size, u_int32_t hashmax) {
	HashTable *h;

	DEBUG("ratelimit: hashtable_init creating hashtable with %d buckets", hashmax);
	h = talloc(NULL,HashTable);
	h->table = talloc_array_size(h, element_size, hashmax);
	maxbuckets = hashmax;
	return h;
}

/*
 * insert adds the value to the datastore at the position hash(key).
 */
void insert(HashTable *datastore, void *value, const char *key) {
	HashTable *htable = datastore;
	int index = hash(key);

	INFO("insert into hashtable bucket with key %s at index %d", key, index);
	htable->table[index].value = talloc_steal(htable->table, value);
}

/*
 * lookup returns the entry in the datastore with the given key or NULL if the datastore
 * doesn't contain an entry for key.
 *
 *
 */
void *lookup(HashTable *datastore, const char *key) {
	long hash_index;
	HashTable *htable;
	INFO("hashtable.c: lookup(key=%s)", key);

	htable = datastore;
	hash_index = hash(key);
	fr_assert(hash_index >= 0 && hash_index < maxbuckets);
	return htable->table[hash_index].value;
}

void delete(UNUSED HashTable *datastore, UNUSED const char *key) {
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
