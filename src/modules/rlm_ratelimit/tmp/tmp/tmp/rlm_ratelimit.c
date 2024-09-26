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
 * @file rlm_ratelimit.c
 * @brief test module code.
 *
 * @copyright 2013 The FreeRADIUS server project
 * @copyright 2013 your name \<your address\>
 */
RCSID("$Id$")

#include "rlm_ratelimit.h"

/*
 *	A mapping of configuration file names to internal variables.
 */
static const CONF_PARSER module_config[] = {
	{ "integer", FR_CONF_OFFSET(PW_TYPE_INTEGER, rlm_ratelimit_t, value), "1" },
	{ "boolean", FR_CONF_OFFSET(PW_TYPE_BOOLEAN, rlm_ratelimit_t, boolean), "no" },
	{ "string", FR_CONF_OFFSET(PW_TYPE_STRING, rlm_ratelimit_t, string), NULL },
	{ "ipaddr", FR_CONF_OFFSET(PW_TYPE_IPV4_ADDR, rlm_ratelimit_t, ipaddr), "*" },
	CONF_PARSER_TERMINATOR
};

static int rlm_ratelimit_cmp(UNUSED void *instance, REQUEST *request, UNUSED VALUE_PAIR *thing, VALUE_PAIR *check,
			UNUSED VALUE_PAIR *check_pairs, UNUSED VALUE_PAIR **reply_pairs)
{
	rad_assert(check->da->type == PW_TYPE_STRING);

	RINFO("ratelimit-Paircmp called with \"%s\"", check->vp_strvalue);

	if (strcmp(check->vp_strvalue, "yes") == 0) return 0;
	return 1;
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
static int mod_instantiate(UNUSED CONF_SECTION *conf, void *instance)
{
	rlm_ratelimit_t *inst = instance;
	ATTR_FLAGS flags;

	DEBUG("^^^^^^^^  instantiated module ratelimit  ^^^^^^^^^^^^");

	memset(&flags, 0, sizeof(flags));

	if (dict_addattr("test-Paircmp", -1, 0, PW_TYPE_STRING, flags) < 0) {
		ERROR("Failed creating paircmp attribute: %s", fr_strerror());

		return -1;
	}

	paircompare_register(dict_attrbyname("test-Paircmp"), dict_attrbyvalue(PW_USER_NAME, 0), false,
			     rlm_ratelimit_cmp, inst);

	/*
	 *	Log some messages
	 */
	INFO("rlm_ratelimit: Informational message");
	WARN("rlm_ratelimit: Warning message");
	ERROR("rlm_ratelimit: Error message");
	DEBUG("rlm_ratelimit: Debug message");
	DEBUG2("rlm_ratelimit: Debug2 message");
	DEBUG3("rlm_ratelimit: Debug3 message");
	DEBUG4("rlm_ratelimit: Debug4 message");
	AUTH("rlm_ratelimit: Auth message");
	ACCT("rlm_ratelimit: Acct message");
	PROXY("rlm_ratelimit: Proxy message");

	/* do something useful */
	rate_limit_init();

	return 0;
}

/****************************************************************************************
 *  Below here is implementation of ratelimiting algorithm. It should be moved into its
 *  own file(s).
 ****************************************************************************************/






/**************************************
 * END - rate limiting althorithm code
 **************************************/


/*
 *	Retrieve the calling_station_id from the request and return a RLM_MODULE_REJECT if the
 *  request for this session exceeds the rate limit.
 *  Return OK/NOP if the request doesn't contain a calling_station_id.
 */
static rlm_rcode_t CC_HINT(nonnull) mod_authorize(UNUSED void *instance, REQUEST *request)
{
	VALUE_PAIR *vp;
	char const *caller = request->module;

	RINFO("RINFO message");
	RDEBUG("RDEBUG message");
	RDEBUG2("RDEBUG2 message");

	RWARN("RWARN message");
	RWDEBUG("RWDEBUG message");
	RWDEBUG("RWDEBUG2 message");

	RAUTH("RAUTH message");
	RACCT("RACCT message");
	RPROXY("RPROXY message");

	/*
	 *	 Should appear wavy
	 */
	RERROR("RERROR error message");
	RINDENT();
	REDEBUG("RDEBUG error message");
	REXDENT();
	REDEBUG2("RDEBUG2 error message");
	RINDENT();
	REDEBUG3("RDEBUG3 error message");
	REXDENT();
	REDEBUG4("RDEBUG4 error message");

	RDEBUG2("Calling submodule %s to process data", caller);

    /*
	 * retrieve the calling_station_id from the request.
	 */
	if (request->packet->code == PW_CODE_ACCESS_REQUEST) {
		vp = fr_pair_find_by_num(request->packet->vps, PW_CALLING_STATION_ID, 0, TAG_ANY);
		if (vp) {
			RDEBUG2("request CALLING_STATION_ID: %s", vp->vp_strvalue);
			if (!rate_limit_ok(vp->vp_strvalue)) {
				RERROR("rejecting for now - rate-limit should return a REJECT");
				return RLM_MODULE_REJECT;
			}
		} else {
			RDEBUG2("calling_station_id not contained in the request");
		}
	}

/*	// mac   = fr_pair_find_by_num(request->vps, PW_CALLING_STATION_ID, 0, TAG_ANY); */

/*   if (!request->calling-station-id) {
           	 RERROR("No User-Password attribute in the request.  Cannot do ratelimit");
           	 return RLM_MODULE_NOOP;
     }
*/

	/* TODO: should this be a RLM_MODULE_NOP? */
	// return RLM_MODULE_OK;
	return RLM_MODULE_OK;
}

/*
 *	Authenticate the user with the given password.
 */
/*static rlm_rcode_t CC_HINT(nonnull) mod_authenticate(UNUSED void *instance, UNUSED REQUEST *request)*/
static rlm_rcode_t CC_HINT(nonnull) mod_authenticate(UNUSED void *instance, REQUEST *request)
{
	RINFO("RINFO this is mod_authenticate");
	return RLM_MODULE_OK;
}

#ifdef WITH_ACCOUNTING
/*
 *	Massage the request before recording it or proxying it
 */
static rlm_rcode_t CC_HINT(nonnull) mod_preacct(UNUSED void *instance, UNUSED REQUEST *request)
{
	return RLM_MODULE_OK;
}

/*
 *	Write accounting information to this modules database.
 */
static rlm_rcode_t CC_HINT(nonnull) mod_accounting(UNUSED void *instance, UNUSED REQUEST *request)
{
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
static rlm_rcode_t CC_HINT(nonnull) mod_checksimul(UNUSED void *instance, REQUEST *request)
{
	request->simul_count=0;

	return RLM_MODULE_OK;
}
#endif


/*
 *	Only free memory we allocated.  The strings allocated via
 *	cf_section_parse() do not need to be freed.
 */
static int mod_detach(UNUSED void *instance)
{
	/* free things here */
	return 0;
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
		[MOD_AUTHENTICATE]	= mod_authenticate,
		[MOD_AUTHORIZE]		= mod_authorize,
#ifdef WITH_ACCOUNTING
		[MOD_PREACCT]		= mod_preacct,
		[MOD_ACCOUNTING]	= mod_accounting,
		[MOD_SESSION]		= mod_checksimul
#endif
	},
};

