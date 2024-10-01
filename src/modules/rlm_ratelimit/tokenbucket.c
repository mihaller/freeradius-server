/*
 *  1. initialise the tokenbucket when first instantiated.
 *  2. add csid buckets to the bucket list
 *  3. ratelimit a csid
 *
 *  Ratelimiting:
 *  - increase a token count if time has expired
 *  - if token is available:
 *    - decrement token count
 *    - return OK
 */

#include <time.h>
#include "hashtable.h"
#include "rlm_ratelimit.h"

static bucketRef add_bucket(void *datastore, const char *id);
static long current_time_in_ms(void);
static bucketRef get_bucket(void *datastore, const char *id);
static int tokens_to_add(long elapsed);
static void update_used_bucket(bucket *b);
static void update_bucket_tokens(bucket *b);
static bool valid_bucket(bucket *b);


static int TOKENMAX;
static uint update_period; /* ms */


void *rlm_ratelimit_init(uint32_t tokenmax, uint32_t period, uint32_t hashmax) {
	TOKENMAX = (int)tokenmax;
	update_period = period;
	maxbuckets = 0;
	return hashtable_init(hashmax, sizeof(bucket));
}


/*
 *  add_bucket creates a new CSID token bucket and returns a reference to it.
 */
static bucketRef add_bucket(void *datastore, const char *id) {
	/* create a new CSID record */
	bucketRef csid = malloc(sizeof(bucket));
	csid->id = id;
	csid->tokens = TOKENMAX;
	csid->latest = current_time_in_ms();
	INFO("created bucket for %s. maxbuckets: %d", id, maxbuckets);

	insert(datastore, csid, csid->id);
	return csid;
}


/*
 *  update_used_bucket decrements the token count and updates last access time
 */
static void update_used_bucket(bucket *b) {
	if (!valid_bucket(b)) {
		ERROR("bucket index out of range");
	}
	b->tokens--;
	b->latest = current_time_in_ms();
}


/*
 *  update_bucket_tokens determines if the bucket's tokens can be replinished and
 *  replinishes them up to but not exceeding TOKENMAX.
 */
static void update_bucket_tokens(bucket *b) {
	int nTokens;
	int toks_to_add;

	if (!valid_bucket(b)) {
		WARN("update_bucket_tokens: invalid bucket");
		return;
	}

	INFO("update_bucket_tokens(bucket %s)", b->id);

	nTokens = tokens_to_add(current_time_in_ms() - b->latest);
	toks_to_add = (b->tokens+nTokens <= TOKENMAX) ? b->tokens+nTokens : TOKENMAX;
	DEBUG("setting tokens to %d to %s bucket", toks_to_add, b->id);
	b->tokens = toks_to_add;
}


/*
 *  tokens_to_add returns the number of tokens to add for the elapse time for the update_period
 */
static int tokens_to_add(long elapsed) {
    // uint update_period = 5000; /* ms */

	return elapsed / update_period;
}


/*
 *  valid_bucket return true if the bucketRef is valid
 */
static bool valid_bucket(bucket *b) {
	if (b != NULL) {
		return true;
	}
	return false;
}


/*
 *  get_bucket returns a reference to the token bucket with the specified id. If the id
 *  doesn't exist a new bucket is created and a reference to the new bucket is
 *  returned.
 */
static bucket* get_bucket(void *datastore, const char *id) {
	bucket *b = NULL;

	INFO("get_bucket(%s)", id);
	b = lookup(datastore, id);

	/* bucket for ID doesn't exist. Add one. */
	if (!b) {
		INFO("get_bucket: bucket not found. Creating: %s", id);
		b = add_bucket(datastore, id);
	}

	return b;
}


/*
 *  rlm_ratelimit_ok returns true if the rate limit for the session reference id hasn't
 *  been exceeded.
 */
bool rlm_ratelimit_ok(void *datastore, const char *id) {
	bucketRef b;

	INFO("Checking rate limit for %s", id);

	b = get_bucket(datastore, id);
	INFO("rlm_ratelimit_ok: calling update_bucket_tokens for %s", b->id);
	update_bucket_tokens(b);
	// if (buckets[b].tokens <= 0) {
	if (b->tokens <= 0) {
		INFO("rate-limit for %s exceeded", id);
		return false;
	}

	/* the request is within limits - update the bucket and return "OK" */
	update_used_bucket(b);
	return true;
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
