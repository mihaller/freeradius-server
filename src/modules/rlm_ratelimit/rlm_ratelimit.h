
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
 * @copyright 2024 your name \<your address\>
 */
RCSIDH(ratelimit_h, "$Id$")

#include <freeradius-devel/radiusd.h>
#include <freeradius-devel/modules.h>
#include <freeradius-devel/rad_assert.h>

/*
 *	Define a structure for our module configuration.
 *
 *	These variables do not need to be in a structure, but it's
 *	a lot cleaner to do so, and a pointer to the structure can
 *	be used as the instance handle.
 */
typedef struct rlm_ratelimit_t {
	uint32_t tokenmax;
	uint32_t period;
	uint32_t hashsize;

	void *datastore;
} rlm_ratelimit_t;


/**
 *	A mapping of configuration parameter to internal variables.
 */
static const CONF_PARSER module_config[] = {
	{ "tokenmax", FR_CONF_OFFSET(PW_TYPE_INTEGER, rlm_ratelimit_t, tokenmax), "10" },
	{ "period", FR_CONF_OFFSET(PW_TYPE_INTEGER, rlm_ratelimit_t, period), "5000" },
	{ "hashsize", FR_CONF_OFFSET(PW_TYPE_INTEGER, rlm_ratelimit_t, hashsize), "20" },
	CONF_PARSER_TERMINATOR
};

void *rlm_ratelimit_init(uint32_t tokenmax, uint32_t period, uint32_t hashsize);
bool rlm_ratelimit_ok(void *datastore, const char *id);

