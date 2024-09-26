
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
	/* config */
	uint32_t tokenmax;
	uint32_t period;
	uint32_t hashsize;
} rlm_ratelimit_t;


/*
 *	A mapping of configuration file names to internal variables.
 */
static const CONF_PARSER module_config[] = {
	{ "tokenmax", FR_CONF_OFFSET(PW_TYPE_INTEGER, rlm_ratelimit_t, tokenmax), "10" },
	{ "period", FR_CONF_OFFSET(PW_TYPE_INTEGER, rlm_ratelimit_t, period), "5000" },
	{ "hashsize", FR_CONF_OFFSET(PW_TYPE_INTEGER, rlm_ratelimit_t, hashsize), "20" },
	CONF_PARSER_TERMINATOR
};

void rlm_ratelimit_init(uint32_t tokenmax, uint32_t period, uint32_t hashsize);
bool rlm_ratelimit_ok(const char *id);

