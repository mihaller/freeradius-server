#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hashtable.h"

typedef int hashi;

static bucketRef *buckets;

hashi hash(const char *key);

void hashtable_init(u_int32_t hashmax) {
    INFO("hashtable_init creating hashtable with size %d", hashmax);
    buckets = calloc(hashmax, sizeof(bucketRef));
}

void insert(bucketRef value) {
    int index = hash(value->id);

    INFO("insert into hashtable bucket with key %s at index %d", value->id, index);
	buckets[index] = value;
}


bucket* lookup(const char *key) {
    bucket *b = NULL;

    INFO("hashtable.c: lookup(key=%s)", key);
	for (int i=0; i<maxbuckets; i++) {
		if (strcmp(buckets[i]->id, key) == 0) {
			INFO("rate-limit bucket found for %s", key);
			b = buckets[i];
			break;
		}
	}
    return b;
}


hashi hash(const char *key) {
    INFO("hash(%s)", key);
	maxbuckets++;
	return maxbuckets-1;
}

// #define TABLE_SIZE 128

// typedef struct {
//     unsigned int key;
//     int value;
//     struct node* next;
// } node_t;

// node_t* table[TABLE_SIZE];

// unsigned int hash(unsigned int key) {
//     return key % TABLE_SIZE;
// }

// void insert(unsigned int key, int value) {
//     unsigned int index = hash(key);
//     node_t* node = malloc(sizeof(node_t));
//     node->key = key;
//     node->value = value;
//     node->next = table[index];
//     table[index] = node;
// }

// int lookup(unsigned int key) {
//     unsigned int index = hash(key);
//     node_t* current = table[index];
//     while (current != NULL) {
//         if (current->key == key) {
//             return current->value;
//         }
//         current = current->next;
//     }
//     return -1; // key not found
// }

// void delete(unsigned int key) {
//     unsigned int index = hash(key);
//     node_t** prev = &table[index];
//     node_t* current = *prev;
//     while (current != NULL) {
//         if (current->key == key) {
//             *prev = current->next;
//             free(current);
//             return;
//         }
//         prev = &current->next;
//         current = *prev;
//     }
// }
