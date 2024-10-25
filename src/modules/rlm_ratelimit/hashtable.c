#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hashtable.h"

typedef uint64_t hashindex;

static int maxbuckets; /* number of buckets */

static hashindex hash(const char *key);
static hashindex hash_key(const char *key);
static long current_time_in_ms(void);
static void delete(Hashentry *he);

/*
 * hashtable_init allocates a hashtable with hashmax size and returns a pointer to it.
 */
HashTable *hashtable_init(u_int32_t hashmax) {
	HashTable *h;

	DEBUG("ratelimit: hashtable_init creating hashtable with %d buckets", hashmax);
	h = talloc(NULL, HashTable);
	h->table = (Hashentry **) talloc_array_size(h, sizeof(Hashentry*), hashmax);
	maxbuckets = hashmax;
	for (u_int32_t i=0; i<hashmax; i++) {
		h->table[i] = NULL;
	}
	return h;
}

/*
 * insert adds the data to the datastore at the position hash(key).
 */
void insert(HashTable *datastore, void *data, const char *key) {
	HashTable *htable = datastore;
	Hashentry *h_entry;
	Hashentry *prev;
	Hashentry *curr;
	Hashentry **head;
	int index = hash(key);
	uint count = 0;
	uint rnp = 0;
	long age;

	/* allocate memory for the new hashentry */
	h_entry = talloc(htable, Hashentry);
	h_entry->key = talloc_strdup(htable->table, key);
	h_entry->value = talloc_steal(htable->table, data);
	h_entry->next = NULL;
	h_entry->accessed = current_time_in_ms();

	/* age the list */
	age = current_time_in_ms() - 500;
	head = &(htable->table[index]);
	curr = *head;
	while (curr != NULL) {
		INFO("ratelimit: insert() - age is %ld", curr->accessed);
		prev = curr->next;
		if (curr->accessed < age) {
			// prev = curr->next;
			// delete(curr);
			rnp++;
		}
		curr = prev;
		// head = &((*head)->next);
	}

	/* find where it should go */
	head = &(htable->table[index]);
	while (*head != NULL) {
		head = &((*head)->next);
		count++;
	}

	INFO("ratelimit: insert() - linked depth at index %d: %d for key %s", index, count, key);
	INFO("ratelimit: insert() - insert into hashtable bucket with key %s at index %d", key, index);

	/* add the entry */
	(*head) = h_entry;
}

/*
 * lookup returns the entry in the datastore with the given key or NULL if the datastore
 * doesn't contain an entry for key.
 */
void *lookup(HashTable *datastore, const char *key) {
	long index;
	HashTable *htable;
	Hashentry **head;
	void *data = NULL;

	INFO("ratelimit: hashtable.c: lookup(key=%s)", key);

	htable = datastore;
	index = hash(key);
	fr_assert(index >= 0 && index < maxbuckets);

	/* while not found and key != null */

	head = &(htable->table[index]);
	while (*head != NULL && (*head)->key != NULL) {
		if (strcmp((*head)->key, key) != 0) {
			WARN("ratelimit: lookup keys %s %s do not match", (*head)->key, key);
			head = &((*head)->next);
		} else {
			WARN("ratelimit: lookup key %s found", key);
			(*head)->accessed = current_time_in_ms();
			data = (*head)->value;
			break;
		}
	}

	if (data != NULL) {
		INFO("ratelimit: lookup data is not null");
	}
	return data;
}

void delete(Hashentry *he) {
	INFO("ratelimit: delete() - delete entry for key: %s", he->key);
	fr_assert(he != NULL);
	INFO("he is OK");
	fr_assert(he->value != NULL);
	talloc_free(he->value);
	INFO("value deleted OK");
	talloc_free(he);
	INFO("he deleted OK");
}

/*
 *  current_time_in_ms returns the current time in milliseconds
 */
static long current_time_in_ms(void) {
    struct timespec ts;
	long ms;
    clock_gettime(CLOCK_REALTIME, &ts);

    ms = ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
    return ms;
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
