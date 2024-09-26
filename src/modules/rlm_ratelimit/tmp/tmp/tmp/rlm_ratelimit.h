
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
	bool		boolean;
	uint32_t	value;
	char const	*string;
	fr_ipaddr_t	ipaddr;
} rlm_ratelimit_t;

#define BUCKETSIZEMAX 10
#define TOKENMAX 10


/* bucket holds the ID of the incoming request and the number of rate limit tokens */

void rlm_ratelimit_init(void);

bool rlm_ratelimit_ok(const char *id);


// rlm_yubikey_decrypt
