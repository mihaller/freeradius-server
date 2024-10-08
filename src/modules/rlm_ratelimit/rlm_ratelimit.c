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
 * @file rlm_ratelimit.c
 * @brief Allow FreeRADIUS to rate limit requests.
 *
 * @copyright 2024 The FreeRADIUS server project
 * @copyright 2024 your name <TODO>
 */
RCSID("$Id$")

#include "hashtable.h"
#include "rlm_ratelimit.h"

typedef struct bucket {
	const char *id;
	int tokens;
	long latest;
} bucket;

typedef bucket* bucketRef;

static void *ratelimit_init_datastore(rlm_ratelimit_t *instance);
static bool ratelimit_ok(rlm_ratelimit_t *inst, const char *id);
bucket* get_bucket(rlm_ratelimit_t *inst, const char *id);
static bucketRef add_bucket(rlm_ratelimit_t *inst, const char *id);
static long current_time_in_ms(void);
static int tokens_to_add(long elapsed, u_int32_t update_period);
static bool valid_bucket(bucket *b);
void update_bucket_tokens(bucket *b, u_int32_t maxtokens, u_int32_t period);
void update_used_bucket(bucket *b);

static uint numbuckets; /* TODO: used for debugging. Remove? */


/*
 * ratelimit_init_datastore
 */
static void *ratelimit_init_datastore(rlm_ratelimit_t *instance) {
	return hashtable_init(sizeof(bucket), instance->hashsize);
}


/*
 *  add_bucket creates a new CSID token bucket and returns a reference to it.
 */
static bucketRef add_bucket(rlm_ratelimit_t *inst, const char *id) {
	/* create a new CSID record */

	bucket *csid = talloc(NULL, bucket);
	csid->id = talloc_strdup(csid, id);
	csid->tokens = inst->tokenmax;
	csid->latest = current_time_in_ms();

	DEBUG("ratelimit: add_bucket() created bucket for ID %s. Total allocated buckets: %d", id, ++numbuckets);

	insert(inst->datastore, csid, csid->id);
	return csid;
}


/*
 *  update_used_bucket decrements the token count and updates last access time
 */
void update_used_bucket(bucket *b) {
	if (!valid_bucket(b)) {
		ERROR("bucket index out of range");
	}
	b->tokens--;
	b->latest = current_time_in_ms();
}


/*
 *  update_bucket_tokens determines if the bucket's tokens can be replenished. If so,
 *  it is replenished up to, but not exceeding, TOKENMAX.
 */
void update_bucket_tokens(bucket *b, u_int32_t maxtokens, u_int32_t period) {
	int nTokens;
	u_int32_t toks_to_add;

	if (!valid_bucket(b)) {
		WARN("update_bucket_tokens: invalid bucket");
		return;
	}

	INFO("update_bucket_tokens(bucket %s)", b->id);

	nTokens = tokens_to_add(current_time_in_ms() - b->latest, period);
	toks_to_add = (b->tokens+nTokens <= (int) maxtokens) ? b->tokens+nTokens : maxtokens;
	DEBUG("setting tokens to %d to %s bucket", toks_to_add, b->id);
	b->tokens = toks_to_add;
}


/*
 *  tokens_to_add returns the number of tokens to add for the elapse time for the update_period
 */
static int tokens_to_add(long elapsed, u_int32_t update_period) {
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
 *  current_time_in_ms returns the current time in milliseconds
 */
static long current_time_in_ms(void) {
    struct timespec ts;
	long ms;
    clock_gettime(CLOCK_REALTIME, &ts);

    ms = ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
    return ms;
}


/*
 *  get_bucket returns a reference to the token bucket with the specified id. If the id
 *  doesn't exist a new bucket is created and a reference to the new bucket is
 *  returned.
 */
bucket* get_bucket(rlm_ratelimit_t *inst, const char *id) {
	bucket *b = NULL;

	b = lookup(inst->datastore, id);

	/* bucket for ID doesn't exist. Add one. */
	if (!b) {
		DEBUG("get_bucket: bucket not found. Adding bucket: %s", id);
		b = add_bucket(inst, id);
	} else {
		// check if the datastore is already filled at that position
		// if so, replace the bucket with the new bucket
		// this means releasing the memory for the existing bucket at that index
		// and adding the new bucket.
		if (strcmp(b->id, id) != 0) {
			WARN("ratelimit: retrieved bucket (%s) doesn't match expected (%s)", b->id, id);
		}
	}

	return b;
}

/*
 *  ratelimit_ok returns true if the rate limit hasn't been exceeded.
 */
static bool ratelimit_ok(rlm_ratelimit_t *inst, const char *id) {
	// bucketRef b;
	bucket *b;

	DEBUG("ratelimit_ok(): Checking rate limit for %s", id);

	b = get_bucket(inst, id);
	update_bucket_tokens(b, inst->tokenmax, inst->period);
	if (b->tokens <= 0) {
		INFO("rate-limit for %s exceeded", id);
		return false;
	}

	/* the request is within limits - update the bucket and return "OK" */
	update_used_bucket(b);
	return true;
}

/*
 *	Do any per-module initialization that is separate to each
 *	configured instance of the module.  e.g. set up connections
 *	to external databases, read configuration files, set up
 *	dictionary entries, etc.
 *
 *	If configuration information is given in the config section
 *	that must be referenced in later calls, store a handle to it
 *	in *instance otherwise put a null pointer there.
 */
static int mod_instantiate(UNUSED CONF_SECTION *conf, void *instance) {
	rlm_ratelimit_t *inst = instance;

	/* TODO - remove the following after testing */
	INFO("This is a INFO message in mod_instantiate. Remove after tetsing.");
	WARN("This is a WARN message in mod_instantiate for when user is bob");
	ERROR("This is a ERROR message in mod_instantiate for when user is bob");
	DEBUG("This is a DEBUG message in mod_instantiate for when user is bob");
	DEBUG2("This is a DEBUG2 message in mod_instantiate for when user is bob");
	DEBUG3("This is a DEBUG3 message in mod_instantiate for when user is bob");

	/* trivial sanity check on config values passed in */
	rad_assert(inst->hashsize > 0);
	rad_assert(inst->tokenmax > 0);
	rad_assert(inst->period > 0);

	inst->datastore = ratelimit_init_datastore(inst);
	if (inst->datastore == NULL) {
		return -1;
	}

	// ratelimit_init(inst->tokenmax, inst->period, inst->hashsize);

	return 0;
}

#ifdef WITH_ACCOUNTING
/*
 *	Massage the request before recording it or proxying it
 */
static rlm_rcode_t CC_HINT(nonnull) mod_preacct(UNUSED void *instance, UNUSED REQUEST *request) {
	return RLM_MODULE_OK;
}

/*
 *	Write accounting information to this modules database.
 */
static rlm_rcode_t CC_HINT(nonnull) mod_accounting(UNUSED void *instance, UNUSED REQUEST *request) {
	return RLM_MODULE_OK;
}

/*
 *	See if a user is already logged in. Sets request->simul_count to the
 *	current session count for this user and sets request->simul_mpp to 2
 *	if it looks like a multilink attempt based on the requested IP
 *	address, otherwise leaves request->simul_mpp alone.
 *
 *	Check twice. If on the first pass the user exceeds his
 *	max. number of logins, do a second pass and validate all
 *	logins by querying the terminal server (using eg. SNMP).
 */
static rlm_rcode_t CC_HINT(nonnull) mod_checksimul(UNUSED void *instance, REQUEST *request) {
	request->simul_count=0;

	return RLM_MODULE_OK;
}
#endif


/*
 *	Only free memory we allocated.  The strings allocated via
 *	cf_section_parse() do not need to be freed.
 */
static int mod_detach(void *instance) {
	rlm_ratelimit_t *inst = instance;

	talloc_free(inst->datastore);

	/*
	 *  We need to explicitly free all children, so if the driver
	 *  parented any memory off the instance, their destructors
	 *  run before we unload the bytecode for them.
	 *
	 *  If we don't do this, we get a SEGV deep inside the talloc code
	 *  when it tries to call a destructor that no longer exists.
	 */
	talloc_free_children(inst);

	return 0;
}

/*
 *	Retrieve the calling_station_id from the request and return a RLM_MODULE_REJECT if the
 *  request for this session exceeds the rate limit.
 *  Return OK/NOP if the request doesn't contain a calling_station_id.
 */
static rlm_rcode_t CC_HINT(nonnull) mod_pre_proxy(void *instance, REQUEST *request) {
	rlm_ratelimit_t *inst = instance;
	VALUE_PAIR *vp;

	RDEBUG2("mod_pre_proxy()");

    /*
	 *  retrieve the calling_station_id from the request.
	 */
	if (request->packet->code == PW_CODE_ACCESS_REQUEST) {
		vp = fr_pair_find_by_num(request->packet->vps, PW_CALLING_STATION_ID, 0, TAG_ANY);
		if (vp) {
			RDEBUG("request CALLING_STATION_ID: %s", vp->vp_strvalue);
			// if (!ratelimit_ok(inst->datastore, vp->vp_strvalue)) {
			if (!ratelimit_ok(inst, vp->vp_strvalue)) {
				RINFO("access request for %s rejected due to rate-limiting", vp->vp_strvalue);
				return RLM_MODULE_REJECT;
			}
		} else {
			RDEBUG("calling_station_id not contained in the request");
		}
	}

	/* TODO: should this be a RLM_MODULE_NOP? */
	return RLM_MODULE_OK;
}

/*
 *	The module name should be the only globally exported symbol.
 *	That is, everything else should be 'static'.
 *
 *	If the module needs to temporarily modify it's instantiation
 *	data, the type should be changed to RLM_TYPE_THREAD_UNSAFE.
 *	The server will then take care of ensuring that the module
 *	is single-threaded.
 */
extern module_t rlm_ratelimit;
module_t rlm_ratelimit = {
	.magic		= RLM_MODULE_INIT,
	.name		= "ratelimit",
	.type		= RLM_TYPE_THREAD_SAFE,
	.inst_size	= sizeof(rlm_ratelimit_t),
	.config		= module_config,
	.instantiate	= mod_instantiate,
	.detach		= mod_detach,
	.methods = {
		// [MOD_AUTHENTICATE]	= mod_authenticate,
		// [MOD_AUTHORIZE]		= mod_authorize,
		[MOD_PRE_PROXY]		= mod_pre_proxy,
#ifdef WITH_ACCOUNTING
		[MOD_PREACCT]		= mod_preacct,
		[MOD_ACCOUNTING]	= mod_accounting,
		[MOD_SESSION]		= mod_checksimul
#endif
	},
};

