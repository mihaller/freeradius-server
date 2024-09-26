
#include "rlm_ratelimit.h"

static int maxbuckets; /* top of the buckets stack*/

typedef struct bucket {
	const char *id;
	int tokens;
} bucket;

static struct bucket buckets[BUCKETSIZEMAX];


void rate_limit_init(void) {
	maxbuckets = 0;
}

static void add_bucket(const char *id) {
	INFO("create bucket for %s. maxbuckets: %d", id, maxbuckets);
	if (maxbuckets >= BUCKETSIZEMAX) {
		ERROR("too many rate limit buckets");
	}
	buckets[maxbuckets].id = id;
	buckets[maxbuckets].tokens = TOKENMAX - 1;
	maxbuckets++;
}


/*
 *  Return true is rate limit for id hasn't been exceeded.
 *  The id is an index into a table of ids. Each entry in the table contains the
 *  following information:
 *      - ID (isn't needed but is handy perhaps)
 *      - number of token
 *  If a token exists for the ID, then the rate is OK. If not the rate has been exceeded.
 *  An initial version simply looks up the table and returns true if num token > 0 and
 *  decrements the counter,
 */
bool rate_limit_ok(const char *id) {
	bool found;
	INFO("Checking rate limit for %s", id);

	found = false;
	for (int i=0; i< maxbuckets; i++) {
		if (strcmp(buckets[i].id, id) == 0) {
			INFO("rate-limit bucket found for %s", id);
			found = true;
			if (buckets[i].tokens <= 0 ) {
				INFO("rate-limit for %s exceeded", id);
				return false;
			}
			buckets[i].tokens--;
			break;
		} else {
			ERROR("%s doesn't equal %s", buckets[i].id, id);
		}
	}

	/* bucket for ID doesn't exist. Add one. */
	if (!found) {
		add_bucket(id);
	}
	return true;
}
