#include <freeradius-devel/radiusd.h>

#define BUCKETSIZEMAX 10

typedef struct bucket {
	const char *id;
	int tokens;
	long latest;
} bucket;

static int maxbuckets; /* top of the buckets stack*/
typedef bucket* bucketRef;

void hashtable_init(u_int32_t hashmax);
void insert(bucketRef value);
bucket* lookup(const char *key);
