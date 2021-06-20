/*
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
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
 * @file tls/base.c
 * @brief Initialise OpenSSL
 *
 * @copyright 2001 hereUare Communications, Inc. (raghud@hereuare.com)
 * @copyright 2003 Alan DeKok (aland@freeradius.org)
 * @copyright 2006-2016 The FreeRADIUS server project
 */
RCSID("$Id$")
USES_APPLE_DEPRECATED_API	/* OpenSSL API has been deprecated by Apple */

#ifdef WITH_TLS
#define LOG_PREFIX "tls - "

#include <openssl/conf.h>

#include <freeradius-devel/server/base.h>
#include <freeradius-devel/tls/attrs.h>
#include <freeradius-devel/tls/base.h>
#include <freeradius-devel/tls/engine.h>
#include <freeradius-devel/util/atexit.h>
#include <freeradius-devel/util/debug.h>

#include "log.h"
#include "bio.h"

static uint32_t instance_count = 0;

/** The context which holds any memory OpenSSL allocates
 *
 * This should be used to work around memory leaks in the OpenSSL.
 */
_Thread_local TALLOC_CTX 	*ssl_talloc_ctx;

/** Used to control freeing of thread local OpenSSL resources
 *
 */
static _Thread_local bool	*async_pool_init;

fr_dict_t const *dict_freeradius;
fr_dict_t const *dict_radius;
fr_dict_t const *dict_tls;

extern fr_dict_autoload_t tls_dict[];
fr_dict_autoload_t tls_dict[] = {
	{ .out = &dict_freeradius, .proto = "freeradius" },
	{ .out = &dict_radius, .proto = "radius" },
	{ .out = &dict_tls, .proto = "tls" },
	{ NULL }
};

fr_dict_attr_t const *attr_allow_session_resumption;
fr_dict_attr_t const *attr_session_resumed;

fr_dict_attr_t const *attr_tls_cert_common_name;
fr_dict_attr_t const *attr_tls_cert_expiration;
fr_dict_attr_t const *attr_tls_cert_issuer;
fr_dict_attr_t const *attr_tls_cert_serial;
fr_dict_attr_t const *attr_tls_cert_subject;
fr_dict_attr_t const *attr_tls_cert_subject_alt_name_dns;
fr_dict_attr_t const *attr_tls_cert_subject_alt_name_email;
fr_dict_attr_t const *attr_tls_cert_subject_alt_name_upn;

fr_dict_attr_t const *attr_tls_client_cert_common_name;
fr_dict_attr_t const *attr_tls_client_cert_expiration;
fr_dict_attr_t const *attr_tls_client_cert_issuer;
fr_dict_attr_t const *attr_tls_client_cert_serial;
fr_dict_attr_t const *attr_tls_client_cert_subject;
fr_dict_attr_t const *attr_tls_client_cert_subject_alt_name_dns;
fr_dict_attr_t const *attr_tls_client_cert_subject_alt_name_email;
fr_dict_attr_t const *attr_tls_client_cert_subject_alt_name_upn;

fr_dict_attr_t const *attr_tls_client_cert_filename;
fr_dict_attr_t const *attr_tls_client_error_code;
fr_dict_attr_t const *attr_tls_ocsp_cert_valid;
fr_dict_attr_t const *attr_tls_ocsp_next_update;
fr_dict_attr_t const *attr_tls_ocsp_response;
fr_dict_attr_t const *attr_tls_psk_identity;

fr_dict_attr_t const *attr_tls_session_cert_file;
fr_dict_attr_t const *attr_tls_session_cipher_suite;
fr_dict_attr_t const *attr_tls_session_version;

fr_dict_attr_t const *attr_framed_mtu;

fr_dict_attr_t const *attr_tls_packet_type;
fr_dict_attr_t const *attr_tls_session_data;
fr_dict_attr_t const *attr_tls_session_id;

extern fr_dict_attr_autoload_t tls_dict_attr[];
fr_dict_attr_autoload_t tls_dict_attr[] = {
	{ .out = &attr_allow_session_resumption, .name = "Allow-Session-Resumption", .type = FR_TYPE_BOOL, .dict = &dict_freeradius },
	{ .out = &attr_session_resumed, .name = "EAP-Session-Resumed", .type = FR_TYPE_BOOL, .dict = &dict_freeradius },

	{ .out = &attr_tls_cert_common_name, .name = "TLS-Cert-Common-Name", .type = FR_TYPE_STRING, .dict = &dict_freeradius },
	{ .out = &attr_tls_cert_expiration, .name = "TLS-Cert-Expiration", .type = FR_TYPE_DATE, .dict = &dict_freeradius },
	{ .out = &attr_tls_cert_issuer, .name = "TLS-Cert-Issuer", .type = FR_TYPE_STRING, .dict = &dict_freeradius },
	{ .out = &attr_tls_cert_serial, .name = "TLS-Cert-Serial", .type = FR_TYPE_STRING, .dict = &dict_freeradius },
	{ .out = &attr_tls_cert_subject, .name = "TLS-Cert-Subject", .type = FR_TYPE_STRING, .dict = &dict_freeradius },
	{ .out = &attr_tls_cert_subject_alt_name_dns, .name = "TLS-Cert-Subject-Alt-Name-Dns", .type = FR_TYPE_STRING, .dict = &dict_freeradius },
	{ .out = &attr_tls_cert_subject_alt_name_email, .name = "TLS-Cert-Subject-Alt-Name-Email", .type = FR_TYPE_STRING, .dict = &dict_freeradius },
	{ .out = &attr_tls_cert_subject_alt_name_upn, .name = "TLS-Cert-Subject-Alt-Name-Upn", .type = FR_TYPE_STRING, .dict = &dict_freeradius },

	{ .out = &attr_tls_client_cert_common_name, .name = "TLS-Client-Cert-Common-Name", .type = FR_TYPE_STRING, .dict = &dict_freeradius },
	{ .out = &attr_tls_client_cert_expiration, .name = "TLS-Client-Cert-Expiration", .type = FR_TYPE_DATE, .dict = &dict_freeradius },
	{ .out = &attr_tls_client_cert_issuer, .name = "TLS-Client-Cert-Issuer", .type = FR_TYPE_STRING, .dict = &dict_freeradius },
	{ .out = &attr_tls_client_cert_serial, .name = "TLS-Client-Cert-Serial", .type = FR_TYPE_STRING, .dict = &dict_freeradius },
	{ .out = &attr_tls_client_cert_subject, .name = "TLS-Client-Cert-Subject", .type = FR_TYPE_STRING, .dict = &dict_freeradius },
	{ .out = &attr_tls_client_cert_subject_alt_name_dns, .name = "TLS-Client-Cert-Subject-Alt-Name-Dns", .type = FR_TYPE_STRING, .dict = &dict_freeradius },
	{ .out = &attr_tls_client_cert_subject_alt_name_email, .name = "TLS-Client-Cert-Subject-Alt-Name-Email", .type = FR_TYPE_STRING, .dict = &dict_freeradius },
	{ .out = &attr_tls_client_cert_subject_alt_name_upn, .name = "TLS-Client-Cert-Subject-Alt-Name-Upn", .type = FR_TYPE_STRING, .dict = &dict_freeradius },

	{ .out = &attr_tls_client_cert_filename, .name = "TLS-Client-Cert-Filename", .type = FR_TYPE_STRING, .dict = &dict_freeradius },
	{ .out = &attr_tls_client_error_code, .name = "TLS-Client-Error-Code", .type = FR_TYPE_UINT8, .dict = &dict_freeradius },
	{ .out = &attr_tls_ocsp_cert_valid, .name = "TLS-OCSP-Cert-Valid", .type = FR_TYPE_UINT32, .dict = &dict_freeradius },
	{ .out = &attr_tls_ocsp_next_update, .name = "TLS-OCSP-Next-Update", .type = FR_TYPE_UINT32, .dict = &dict_freeradius },
	{ .out = &attr_tls_ocsp_response, .name = "TLS-OCSP-Response", .type = FR_TYPE_OCTETS, .dict = &dict_freeradius },
	{ .out = &attr_tls_psk_identity, .name = "TLS-PSK-Identity", .type = FR_TYPE_STRING, .dict = &dict_freeradius },

	{ .out = &attr_tls_session_cert_file, .name = "TLS-Session-Cert-File", .type = FR_TYPE_STRING, .dict = &dict_freeradius },
	{ .out = &attr_tls_session_cipher_suite, .name = "TLS-Session-Cipher-Suite", .type = FR_TYPE_STRING, .dict = &dict_freeradius },
	{ .out = &attr_tls_session_version, .name = "TLS-Session-Version", .type = FR_TYPE_STRING, .dict = &dict_freeradius },

	{ .out = &attr_framed_mtu, .name = "Framed-MTU", .type = FR_TYPE_UINT32, .dict = &dict_radius },

	/*
	 *	Eventually all TLS attributes will be in the TLS dictionary
	 */
	{ .out = &attr_tls_packet_type, .name = "Packet-Type", .type = FR_TYPE_UINT32, .dict = &dict_tls },
	{ .out = &attr_tls_session_data, .name = "Session-Data", .type = FR_TYPE_OCTETS, .dict = &dict_tls },
	{ .out = &attr_tls_session_id, .name = "Session-Id", .type = FR_TYPE_OCTETS, .dict = &dict_tls },
	{ NULL }
};

/*
 *	request types
 */
fr_value_box_t const	*enum_tls_packet_type_session_load;
fr_value_box_t const	*enum_tls_packet_type_session_store;
fr_value_box_t const	*enum_tls_packet_type_session_clear;
fr_value_box_t const	*enum_tls_packet_type_certificate_validate;

/*
 *	response types
 */
fr_value_box_t const	*enum_tls_packet_type_success;
fr_value_box_t const	*enum_tls_packet_type_failure;
fr_value_box_t const	*enum_tls_packet_type_notfound;

extern fr_dict_enum_autoload_t tls_dict_enum[];
fr_dict_enum_autoload_t tls_dict_enum[] = {
	{ .out = &enum_tls_packet_type_session_load, .name = "Session-Load", .attr = &attr_tls_packet_type },
	{ .out = &enum_tls_packet_type_session_store, .name = "Session-Store", .attr = &attr_tls_packet_type },
	{ .out = &enum_tls_packet_type_session_clear, .name = "Session-Clear", .attr = &attr_tls_packet_type },
	{ .out = &enum_tls_packet_type_certificate_validate, .name = "Certificate-Validate", .attr = &attr_tls_packet_type },

	{ .out = &enum_tls_packet_type_success, .name = "Success", .attr = &attr_tls_packet_type },
	{ .out = &enum_tls_packet_type_failure, .name = "Failure", .attr = &attr_tls_packet_type },
	{ .out = &enum_tls_packet_type_notfound, .name = "Notfound", .attr = &attr_tls_packet_type },
	{ NULL }
};

/*
 *	Updated by threads.c in the server, and left alone for everyone else.
 */
int fr_tls_max_threads = 1;

#ifdef ENABLE_OPENSSL_VERSION_CHECK
typedef struct {
	uint64_t	high;		//!< The last version number this defect affected.
	uint64_t	low;		//!< The first version this defect affected.

	char const	*id;		//!< CVE (or other ID)
	char const	*name;		//!< As known in the media...
	char const	*comment;	//!< Where to get more information.
} fr_openssl_defect_t;

#undef VM
#undef Vm
#define VM(_a,_b,_c) (((((_a) << 24) | ((_b) << 16) | ((_c) << 8)) << 4) | 0x0f)
#define Vm(_a,_b,_c,_d) (((((_a) << 24) | ((_b) << 16) | ((_c) << 8) | ((_d) - 'a' + 1)) << 4) | 0x0f)

/* Record critical defects in libssl here, new versions of OpenSSL to older versions of OpenSSL.  */
static fr_openssl_defect_t fr_openssl_defects[] =
{
	{
		.low		= Vm(1,1,0,'a'),		/* 1.1.0a */
		.high		= Vm(1,1,0,'a'),		/* 1.1.0a */
		.id		= "CVE-2016-6309",
		.name		= "OCSP status request extension",
		.comment	= "For more information see https://www.openssl.org/news/secadv/20160926.txt"
	},
	{
		.low		= VM(1,1,0),			/* 1.1.0  */
		.high		= VM(1,1,0),			/* 1.1.0  */
		.id		= "CVE-2016-6304",
		.name		= "OCSP status request extension",
		.comment	= "For more information see https://www.openssl.org/news/secadv/20160922.txt"
	}
};
#endif /* ENABLE_OPENSSL_VERSION_CHECK */

#ifdef ENABLE_OPENSSL_VERSION_CHECK
/** Check for vulnerable versions of libssl
 *
 * @param acknowledged The highest CVE number a user has confirmed is not present in the system's
 *	libssl.
 * @return 0 if the CVE specified by the user matches the most recent CVE we have, else -1.
 */
int fr_openssl_version_check(char const *acknowledged)
{
	uint64_t v;
	bool bad = false;
	size_t i;

	/*
	 *	Didn't get passed anything, that's an error.
	 */
	if (!acknowledged || !*acknowledged) {
		ERROR("Refusing to start until 'allow_vulnerable_openssl' is given a value");
		return -1;
	}

	if (strcmp(acknowledged, "yes") == 0) return 0;

	/* Check for bad versions */
	v = (uint64_t) SSLeay();

	for (i = 0; i < (NUM_ELEMENTS(fr_openssl_defects)); i++) {
		fr_openssl_defect_t *defect = &fr_openssl_defects[i];

		if ((v >= defect->low) && (v <= defect->high)) {
			/*
			 *	If the CVE is acknowledged, allow it.
			 */
			if (!bad && (strcmp(acknowledged, defect->id) == 0)) return 0;

			ERROR("Refusing to start with libssl version %s (in range %s)",
			      ssl_version(), ssl_version_range(defect->low, defect->high));
			ERROR("Security advisory %s (%s)", defect->id, defect->name);
			ERROR("%s", defect->comment);

			/*
			 *	Only warn about the first one...
			 */
			if (!bad) {
				INFO("Once you have verified libssl has been correctly patched, "
				     "set security.allow_vulnerable_openssl = '%s'", defect->id);
				bad = true;
			}
		}
	}

	if (bad) return -1;

	return 0;
}
#endif

/** Allocate memory for OpenSSL in the NULL context
 *
 * @param len to alloc.
 * @return realloc.
 */
static void *fr_openssl_talloc(size_t len, NDEBUG_UNUSED char const *file, NDEBUG_UNUSED int line)
{
	void *chunk;

	chunk = talloc_array(ssl_talloc_ctx, uint8_t, len);
#ifndef NDEBUG
	talloc_set_name(chunk, "%s:%u", file, line);
#endif
	return chunk;
}

/** Reallocate memory for OpenSSL in the NULL context
 *
 * @param old memory to realloc.
 * @param len to extend to.
 * @return realloced memory.
 */
static void *fr_openssl_talloc_realloc(void *old, size_t len, NDEBUG_UNUSED char const *file, NDEBUG_UNUSED int line)
{
	void *chunk;

	chunk = talloc_realloc_size(ssl_talloc_ctx, old, len);
#ifndef NDEBUG
	talloc_set_name(chunk, "%s:%u", file, line);
#endif
	return chunk;
}

/** Free memory allocated by OpenSSL
 *
 * @param to_free memory to free.
 */
#ifdef NDEBUG
/*
 *	If we're not debugging, use only the filename.  Otherwise the
 *	cost of snprintf() is too large.
 */
static void fr_openssl_talloc_free(void *to_free, char const *file, UNUSED int line)
{
	(void)_talloc_free(to_free, file);
}
#else
static void fr_openssl_talloc_free(void *to_free, char const *file, int line)
{
	char buffer[256];

	snprintf(buffer, sizeof(buffer), "%s:%i", file, line);
	(void)_talloc_free(to_free, buffer);
}
#endif

/** Cleanup async pools if the thread exits
 *
 */
static void _openssl_thread_free(void *init)
{
	ASYNC_cleanup_thread();
	talloc_free(init);
}

/** Perform thread-specific initialisation for OpenSSL
 *
 * Async contexts are what OpenSSL uses to track
 *
 * @param[in] async_pool_size_init	The initial number of async contexts
 *					we keep in the pool.
 * @param[in] async_pool_size_max	The maximum number of async contexts
 *					we keep in the thread-local pool.
 * @return
 *	- 0 on success.
 *      - -1 on failure.
 */
int fr_openssl_thread_init(size_t async_pool_size_init, size_t async_pool_size_max)
{
	/*
	 *	Hack to use thread local destructor code
	 */
	if (!async_pool_init) {
		bool *init = talloc_zero(NULL, bool);

		if (ASYNC_init_thread(async_pool_size_max, async_pool_size_init) != 1) {
			fr_tls_log_error(NULL, "Failed initialising OpenSSL async context pool");
			return -1;
		}

		fr_atexit_thread_local(async_pool_init, _openssl_thread_free, init);
	}

	return 0;
}

/** Free any memory alloced by libssl
 *
 * OpenSSL >= 1.1.0 uses an atexit handler to automatically free
 * memory. However, we need to call OPENSSL_cleanup manually because
 * some of the SSL ctx is parented to the main config which will get
 * freed before the atexit handler, causing a segfault on exit.
 */
void fr_openssl_free(void)
{
	if (--instance_count > 0) return;

	fr_tls_engine_free_all();

	OPENSSL_cleanup();

	fr_dict_autofree(tls_dict);

	fr_tls_log_free();

	fr_tls_bio_free();
}

/** Add all the default ciphers and message digests to our context.
 *
 * This should be called exactly once from main, before reading the main config
 * or initialising any modules.
 */
int fr_openssl_init(void)
{
	if (instance_count > 0) {
		instance_count++;
		return 0;
	}

	/*
	 *	This will only fail if memory has already been allocated
	 *	by OpenSSL.
	 */
	if (CRYPTO_set_mem_functions(fr_openssl_talloc, fr_openssl_talloc_realloc, fr_openssl_talloc_free) != 1) {
		fr_tls_log_error(NULL, "Failed to set OpenSSL memory allocation functions.  fr_openssl_init() called too late");
		return -1;
	}

	/*
	 *	SHA256 is in all versions of OpenSSL, but isn't
	 *	initialized by default.  It's needed for WiMAX
	 *	certificates.
	 */
	EVP_add_digest(EVP_sha256());

	OPENSSL_init_crypto(OPENSSL_INIT_LOAD_CONFIG, NULL);

	/*
	 *	FIXME - This should be done _after_
	 *	running any engine controls.
	 */
	fr_tls_engine_load_builtin();

	fr_tls_log_init();

	fr_tls_bio_init();

	instance_count++;

	return 0;
}

/** Load dictionary attributes
 *
 * This is a separate function because of ordering issues.
 * OpenSSL may need to be initialised before anything else
 * including the dictionary loader.
 *
 * fr_openssl_free will unload both the dictionary and the
 * OpenSSL library.
 */
int fr_tls_dict_init(void)
{
	if (fr_dict_autoload(tls_dict) < 0) {
		PERROR("Failed initialising protocol library");
		fr_openssl_free();
		return -1;
	}

	if (fr_dict_attr_autoload(tls_dict_attr) < 0) {
		PERROR("Failed resolving attributes");
		fr_openssl_free();
		return -1;
	}

	if (fr_dict_enum_autoload(tls_dict_enum) < 0) {
		PERROR("Failed resolving enums");
		fr_openssl_free();
		return -1;
	}
	return 0;
}
#endif /* WITH_TLS */
