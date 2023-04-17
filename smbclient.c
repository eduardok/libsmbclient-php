/* ------------------------------------------------------------------
 * This file is part of libsmbclient-php: Samba bindings for PHP.
 * Libsmbclient-php is licensed under the BSD 2-clause license:
 * ------------------------------------------------------------------
 *
 * Copyright (c) 2003,        Matthew Sachs
 *               2009 - 2014, Eduardo Bacchi Kienetz
 *               2013 - 2015, Alfred Klomp
 *               2015,        Remi Collet
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
 * CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * ------------------------------------------------------------------
 */

#define IS_EXT_MODULE

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "ext/standard/info.h"
#include "php_smbclient.h"

ZEND_DECLARE_MODULE_GLOBALS(smbclient)

#define PHP_SMBCLIENT_STATE_NAME "smbclient state"
#define PHP_SMBCLIENT_FILE_NAME "smbclient file"

static int le_smbclient_state;
static int le_smbclient_file;

#if PHP_MAJOR_VERSION >= 7
typedef size_t strsize_t;
#else
typedef int strsize_t;
typedef long zend_long;
#endif

enum {
	SMBCLIENT_OPT_OPEN_SHAREMODE = 1,
	SMBCLIENT_OPT_ENCRYPT_LEVEL = 2,
	SMBCLIENT_OPT_CASE_SENSITIVE = 3,
	SMBCLIENT_OPT_BROWSE_MAX_LMB_COUNT = 4,
	SMBCLIENT_OPT_URLENCODE_READDIR_ENTRIES = 5,
	/* Ignore OneSharePerServer, not relevant to us. */
	SMBCLIENT_OPT_USE_KERBEROS = 6,
	SMBCLIENT_OPT_FALLBACK_AFTER_KERBEROS = 7,
	/* Reverse the sense of this option, the original
	 * is the confusing "NoAutoAnonymousLogin": */
	SMBCLIENT_OPT_AUTO_ANONYMOUS_LOGIN = 8,
	SMBCLIENT_OPT_USE_CCACHE = 9,
	SMBCLIENT_OPT_USE_NT_HASH = 10,
	SMBCLIENT_OPT_NETBIOS_NAME = 11,
	SMBCLIENT_OPT_WORKGROUP = 12,
	SMBCLIENT_OPT_USER = 13,
	SMBCLIENT_OPT_PORT = 14,
	SMBCLIENT_OPT_TIMEOUT = 15,
}
php_smbclient_options;

static char *
find_char (char *start, char *last, char q)
{
	char *c;
	for (c = start; c <= last; c++) {
		if (*c == q) {
			return c;
		}
	}
	return NULL;
}

static char *
find_second_char (char *start, char *last, char q)
{
	char *c;
	if ((c = find_char(start, last, q)) == NULL) {
		return NULL;
	}
	return find_char(c + 1, last, q);
}

static void
astfill (char *start, char *last)
{
	char *c;
	for (c = start; c <= last; c++) {
		*c = '*';
	}
}

static void
hide_password (char *url, int len)
{
	/* URL should have the form:
	 *   smb://[[[domain;]user[:password@]]server[/share[/path[/file]]]]
	 * Replace everything after the second colon and before the next @
	 * with asterisks. */
	char *last = (url + len) - 1;
	char *second_colon;
	char *slash;
	char *at_sign;

	if (len <= 0) {
		return;
	}
	if ((second_colon = find_second_char(url, last, ':')) == NULL) {
		return;
	}
	if ((slash = find_char(second_colon + 1, last, '/')) == NULL) {
		slash = last + 1;
	}
	if ((at_sign = find_char(second_colon + 1, last, '@')) == NULL) {
		astfill(second_colon + 1, slash - 1);
		return;
	}
	if (at_sign > slash) {
		at_sign = slash;
	}
	astfill(second_colon + 1, at_sign - 1);
}

#if PHP_VERSION_ID < 80000
#include "smbclient_legacy_arginfo.h"
#else
#include "smbclient_arginfo.h"
#endif

zend_module_entry smbclient_module_entry =
	{ STANDARD_MODULE_HEADER
	, "smbclient"		/* name                  */
	, ext_functions		/* functions             */
	, PHP_MINIT(smbclient)		/* module_startup_func   */
	, PHP_MSHUTDOWN(smbclient)	/* module_shutdown_func  */
	, PHP_RINIT(smbclient)		/* request_startup_func  */
	, PHP_RSHUTDOWN(smbclient)	/* request_shutdown_func */
	, PHP_MINFO(smbclient)		/* info_func             */
	, PHP_SMBCLIENT_VERSION		/* version               */
	, PHP_MODULE_GLOBALS(smbclient)
	, PHP_GINIT(smbclient)		/* globals ctor */
	, NULL						/* globals dtor */
	, NULL						/* post_deactivate_func */
	, STANDARD_MODULE_PROPERTIES_EX
	} ;

zend_module_entry old_module_entry = {
	STANDARD_MODULE_HEADER,
	"libsmbclient",
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	PHP_SMBCLIENT_VERSION,
	STANDARD_MODULE_PROPERTIES,
};


#ifdef COMPILE_DL_SMBCLIENT
ZEND_GET_MODULE(smbclient)
#endif

static void
auth_copy (char *dst, char *src, size_t srclen, size_t maxlen)
{
	if (dst == NULL || maxlen == 0) {
		return;
	}
	if (src == NULL || srclen == 0) {
		*dst = '\0';
		return;
	}
	if (srclen < maxlen) {
		memcpy(dst, src, srclen);
		dst[srclen] = '\0';
		return;
	}
	memcpy(dst, src, maxlen - 1);
	dst[maxlen - 1] = '\0';
}

static void
smbclient_auth_func (SMBCCTX *ctx, const char *server, const char *share, char *wrkg, int wrkglen, char *user, int userlen, char *pass, int passlen)
{
	/* Given context, server and share, return workgroup, username and password.
	 * String lengths are the max allowable lengths. */

	php_smbclient_state *state;

	if (ctx == NULL || (state = smbc_getOptionUserData(ctx)) == NULL) {
		return;
	}
	auth_copy(wrkg, state->wrkg, (size_t)state->wrkglen, (size_t)wrkglen);
	auth_copy(user, state->user, (size_t)state->userlen, (size_t)userlen);
	auth_copy(pass, state->pass, (size_t)state->passlen, (size_t)passlen);
}

void
php_smbclient_state_free (php_smbclient_state *state TSRMLS_DC)
{
	/* TODO: if smbc_free_context() returns 0, PHP will leak the handle: */
	if (state->ctx != NULL && smbc_free_context(state->ctx, 1) != 0) {
		switch (errno) {
			case EBUSY: php_error(E_WARNING, "Couldn't destroy SMB context: connection in use"); break;
			case EBADF: php_error(E_WARNING, "Couldn't destroy SMB context: invalid handle"); break;
			default: php_error(E_WARNING, "Couldn't destroy SMB context: unknown error (%d)", errno); break;
		}
	}
	if (state->wrkg != NULL) {
		memset(state->wrkg, 0, state->wrkglen);
		efree(state->wrkg);
	}
	if (state->user != NULL) {
		memset(state->user, 0, state->userlen);
		efree(state->user);
	}
	if (state->pass != NULL) {
		memset(state->pass, 0, state->passlen);
		efree(state->pass);
	}
	efree(state);
}

static inline void
smbclient_state_dtor (
#if PHP_MAJOR_VERSION >= 7
	zend_resource *rsrc
#else
	zend_rsrc_list_entry *rsrc
#endif
	TSRMLS_DC)
{
	php_smbclient_state_free((php_smbclient_state *)rsrc->ptr TSRMLS_CC);
}

static void
smbclient_file_dtor (
#if PHP_VERSION_ID >= 70000
	zend_resource *rsrc
#else
	zend_rsrc_list_entry *rsrc
#endif
	TSRMLS_DC)
{
	/* Because libsmbclient's file/dir close functions require a pointer to
	 * a context which we don't have, we cannot reliably destroy a file
	 * resource. One way of obtaining the context pointer could be to save
	 * it in a structure along with the file context, but the pointer could
	 * grow stale or otherwise spark a race condition. So it seems that the
	 * best we can do is nothing. The PHP programmer can either manually
	 * free the file resources, or wait for them to be cleaned up when the
	 * associated context is destroyed. */
}

PHP_GINIT_FUNCTION(smbclient)
{
	smbclient_globals->pool_first = NULL;
}

PHP_MINIT_FUNCTION(smbclient)
{
	/* Constants for smbclient_setxattr: */
	REGISTER_LONG_CONSTANT("SMBCLIENT_XATTR_CREATE", SMBC_XATTR_FLAG_CREATE, CONST_PERSISTENT | CONST_CS);
	REGISTER_LONG_CONSTANT("SMBCLIENT_XATTR_REPLACE", SMBC_XATTR_FLAG_REPLACE, CONST_PERSISTENT | CONST_CS);

	/* Constants for getting/setting options: */
	#define SMBCLIENT_CONST(x) REGISTER_LONG_CONSTANT(#x, x, CONST_PERSISTENT | CONST_CS);
	SMBCLIENT_CONST(SMBCLIENT_OPT_OPEN_SHAREMODE);
	SMBCLIENT_CONST(SMBCLIENT_OPT_ENCRYPT_LEVEL);
	SMBCLIENT_CONST(SMBCLIENT_OPT_CASE_SENSITIVE);
	SMBCLIENT_CONST(SMBCLIENT_OPT_BROWSE_MAX_LMB_COUNT);
	SMBCLIENT_CONST(SMBCLIENT_OPT_URLENCODE_READDIR_ENTRIES);
	SMBCLIENT_CONST(SMBCLIENT_OPT_USE_KERBEROS);
	SMBCLIENT_CONST(SMBCLIENT_OPT_FALLBACK_AFTER_KERBEROS);
	SMBCLIENT_CONST(SMBCLIENT_OPT_AUTO_ANONYMOUS_LOGIN);
	SMBCLIENT_CONST(SMBCLIENT_OPT_USE_CCACHE);
	SMBCLIENT_CONST(SMBCLIENT_OPT_USE_NT_HASH);
	SMBCLIENT_CONST(SMBCLIENT_OPT_NETBIOS_NAME);
	SMBCLIENT_CONST(SMBCLIENT_OPT_WORKGROUP);
	SMBCLIENT_CONST(SMBCLIENT_OPT_USER);
	SMBCLIENT_CONST(SMBCLIENT_OPT_PORT);
	SMBCLIENT_CONST(SMBCLIENT_OPT_TIMEOUT);
	#undef SMBCLIENT_CONST

	/* Constants for use with SMBCLIENT_OPT_OPENSHAREMODE: */
	REGISTER_LONG_CONSTANT("SMBCLIENT_SHAREMODE_DENY_DOS", SMBC_SHAREMODE_DENY_DOS, CONST_PERSISTENT | CONST_CS);
	REGISTER_LONG_CONSTANT("SMBCLIENT_SHAREMODE_DENY_ALL", SMBC_SHAREMODE_DENY_ALL, CONST_PERSISTENT | CONST_CS);
	REGISTER_LONG_CONSTANT("SMBCLIENT_SHAREMODE_DENY_WRITE", SMBC_SHAREMODE_DENY_WRITE, CONST_PERSISTENT | CONST_CS);
	REGISTER_LONG_CONSTANT("SMBCLIENT_SHAREMODE_DENY_READ", SMBC_SHAREMODE_DENY_READ, CONST_PERSISTENT | CONST_CS);
	REGISTER_LONG_CONSTANT("SMBCLIENT_SHAREMODE_DENY_NONE", SMBC_SHAREMODE_DENY_NONE, CONST_PERSISTENT | CONST_CS);
	REGISTER_LONG_CONSTANT("SMBCLIENT_SHAREMODE_DENY_FCB", SMBC_SHAREMODE_DENY_FCB, CONST_PERSISTENT | CONST_CS);

	/* Constants for use with SMBCLIENT_OPT_ENCRYPTLEVEL: */
	REGISTER_LONG_CONSTANT("SMBCLIENT_ENCRYPTLEVEL_NONE", SMBC_ENCRYPTLEVEL_NONE, CONST_PERSISTENT | CONST_CS);
	REGISTER_LONG_CONSTANT("SMBCLIENT_ENCRYPTLEVEL_REQUEST", SMBC_ENCRYPTLEVEL_REQUEST, CONST_PERSISTENT | CONST_CS);
	REGISTER_LONG_CONSTANT("SMBCLIENT_ENCRYPTLEVEL_REQUIRE", SMBC_ENCRYPTLEVEL_REQUIRE, CONST_PERSISTENT | CONST_CS);

	/* Constants for the VFS functions: */
	REGISTER_LONG_CONSTANT("SMBCLIENT_VFS_RDONLY", SMBC_VFS_FEATURE_RDONLY, CONST_PERSISTENT | CONST_CS);
	REGISTER_LONG_CONSTANT("SMBCLIENT_VFS_DFS", SMBC_VFS_FEATURE_DFS, CONST_PERSISTENT | CONST_CS);
	REGISTER_LONG_CONSTANT("SMBCLIENT_VFS_CASE_INSENSITIVE", SMBC_VFS_FEATURE_CASE_INSENSITIVE, CONST_PERSISTENT | CONST_CS);
	REGISTER_LONG_CONSTANT("SMBCLIENT_VFS_NO_UNIXCIFS", SMBC_VFS_FEATURE_NO_UNIXCIFS, CONST_PERSISTENT | CONST_CS);

	le_smbclient_state = zend_register_list_destructors_ex(smbclient_state_dtor, NULL, PHP_SMBCLIENT_STATE_NAME, module_number);
	le_smbclient_file = zend_register_list_destructors_ex(smbclient_file_dtor, NULL, PHP_SMBCLIENT_FILE_NAME, module_number);

	php_register_url_stream_wrapper("smb", &php_stream_smb_wrapper TSRMLS_CC);

	/* register with old name for code using extension_loaded(libsmbclient) */
	zend_register_internal_module(&old_module_entry TSRMLS_CC);

	return SUCCESS;
}

PHP_RINIT_FUNCTION(smbclient)
{
	return SUCCESS;
}

PHP_MSHUTDOWN_FUNCTION(smbclient)
{
	return SUCCESS;
}

PHP_RSHUTDOWN_FUNCTION(smbclient)
{
	php_smb_pool_cleanup(TSRMLS_C);
	return SUCCESS;
}

PHP_MINFO_FUNCTION(smbclient)
{
	php_info_print_table_start();
	php_info_print_table_row(2, "smbclient Support", "enabled");
	php_info_print_table_row(2, "smbclient extension Version", PHP_SMBCLIENT_VERSION);
	php_info_print_table_row(2, "libsmbclient library Version", smbc_version());
	php_info_print_table_end();
}

static int
ctx_init_getauth (zval *z, char **dest, int *destlen, char *varname)
{
	if (*dest != NULL) {
		efree(*dest);
		*dest = NULL;
	}
	*destlen = 0;

	if (z == NULL) {
		return 1;
	}
	switch (Z_TYPE_P(z))
	{
		case IS_NULL:
			return 1;

#if PHP_MAJOR_VERSION >= 7
		case IS_TRUE:
			php_error(E_WARNING, "invalid value for %s", varname);
			return 0;

		case IS_FALSE:
			return 1;
#else
		case IS_BOOL:
			if (Z_LVAL_P(z) == 1) {
				php_error(E_WARNING, "invalid value for %s", varname);
				return 0;
			}
			return 1;
#endif

		case IS_STRING:
			*destlen = Z_STRLEN_P(z);
			*dest = estrndup(Z_STRVAL_P(z), *destlen);
			return 1;

		default:
			php_error(E_WARNING, "invalid datatype for %s", varname);
			return 0;
	}
}

php_smbclient_state *
php_smbclient_state_new (php_stream_context *context, int init TSRMLS_DC)
{
	php_smbclient_state *state;
	SMBCCTX *ctx;

	if ((ctx = smbc_new_context()) == NULL) {
		switch (errno) {
			case ENOMEM: php_error(E_WARNING, "Couldn't create smbclient state: insufficient memory"); break;
			default: php_error(E_WARNING, "Couldn't create smbclient state: unknown error (%d)", errno); break;
		};
		return NULL;
	}
	state = emalloc(sizeof(php_smbclient_state));
	state->ctx = ctx;
	state->wrkg = NULL;
	state->user = NULL;
	state->pass = NULL;
	state->wrkglen = 0;
	state->userlen = 0;
	state->passlen = 0;
	state->err = 0;

	smbc_setFunctionAuthDataWithContext(state->ctx, smbclient_auth_func);

	/* Must also save a pointer to the state object inside the context, to
	 * find the state from the context in the auth function: */
	smbc_setOptionUserData(ctx, (void *)state);

	/* Force full, modern timenames when getting xattrs: */
	smbc_setOptionFullTimeNames(state->ctx, 1);

	if (context) {
#if PHP_MAJOR_VERSION >= 7
		zval *tmpzval;

		if (NULL != (tmpzval = php_stream_context_get_option(context, "smb", "workgroup"))) {
			if (ctx_init_getauth(tmpzval, &state->wrkg, &state->wrkglen, "workgroup") == 0) {
				php_smbclient_state_free(state);
				return NULL;
			}
		}
		if (NULL != (tmpzval = php_stream_context_get_option(context, "smb", "username"))) {
			if (ctx_init_getauth(tmpzval, &state->user, &state->userlen, "username") == 0) {
				php_smbclient_state_free(state);
				return NULL;
			}
		}
		if (NULL != (tmpzval = php_stream_context_get_option(context, "smb", "password"))) {
			if (ctx_init_getauth(tmpzval, &state->pass, &state->passlen, "password") == 0) {
				php_smbclient_state_free(state);
				return NULL;
			}
		}
#if HAVE_SMBC_SETOPTIONPROTOCOLS
		if (NULL != (tmpzval = php_stream_context_get_option(context, "smb", "minproto"))) {
			smbc_setOptionProtocols(state->ctx, Z_STRVAL_P(tmpzval), NULL);
		}
		if (NULL != (tmpzval = php_stream_context_get_option(context, "smb", "maxproto"))) {
			smbc_setOptionProtocols(state->ctx, NULL, Z_STRVAL_P(tmpzval));
		}
#endif
#else
		zval **tmpzval;

		if (php_stream_context_get_option(context, "smb", "workgroup", &tmpzval) == SUCCESS) {
			if (ctx_init_getauth(*tmpzval, &state->wrkg, &state->wrkglen, "workgroup") == 0) {
				php_smbclient_state_free(state TSRMLS_CC);
				return NULL;
			}
		}
		if (php_stream_context_get_option(context, "smb", "username", &tmpzval) == SUCCESS) {
			if (ctx_init_getauth(*tmpzval, &state->user, &state->userlen, "username") == 0) {
				php_smbclient_state_free(state TSRMLS_CC);
				return NULL;
			}
		}
		if (php_stream_context_get_option(context, "smb", "password", &tmpzval) == SUCCESS) {
			if (ctx_init_getauth(*tmpzval, &state->pass, &state->passlen, "password") == 0) {
				php_smbclient_state_free(state TSRMLS_CC);
				return NULL;
			}
		}
#if HAVE_SMBC_SETOPTIONPROTOCOLS
		if (php_stream_context_get_option(context, "smb", "minproto", &tmpzval) == SUCCESS) {
			smbc_setOptionProtocols(state->ctx, Z_STRVAL_PP(tmpzval), NULL);
		}
		if (php_stream_context_get_option(context, "smb", "maxproto", &tmpzval) == SUCCESS) {
			smbc_setOptionProtocols(state->ctx, NULL, Z_STRVAL_PP(tmpzval));
		}
#endif
#endif
	}
	if (init) {
		if (php_smbclient_state_init(state TSRMLS_CC)) {
			php_smbclient_state_free(state TSRMLS_CC);
			return NULL;
		}
	}
	return state;
}

PHP_FUNCTION(smbclient_state_new)
{
	php_smbclient_state *state;

	if (zend_parse_parameters_none() == FAILURE) {
		RETURN_FALSE;
	}
	if ((state = php_smbclient_state_new(NULL, 0 TSRMLS_CC)) == NULL) {
		RETURN_FALSE;
	}
#if PHP_MAJOR_VERSION >= 7
	ZVAL_RES(return_value, zend_register_resource(state, le_smbclient_state));
#else
	ZEND_REGISTER_RESOURCE(return_value, state, le_smbclient_state);
#endif
}

PHP_FUNCTION(smbclient_version)
{
	if (zend_parse_parameters_none() == FAILURE) {
		RETURN_FALSE;
	}
#if PHP_MAJOR_VERSION >= 7
	RETURN_STRING(PHP_SMBCLIENT_VERSION);
#else
	RETURN_STRING(PHP_SMBCLIENT_VERSION, 1);
#endif
}

PHP_FUNCTION(smbclient_library_version)
{
	if (zend_parse_parameters_none() == FAILURE) {
		RETURN_FALSE;
	}
#if PHP_MAJOR_VERSION >= 7
	RETURN_STRING(smbc_version());
#else
	RETURN_STRING(smbc_version(), 1);
#endif
}

int
php_smbclient_state_init (php_smbclient_state *state TSRMLS_DC)
{
	SMBCCTX *ctx;

	if ((ctx = smbc_init_context(state->ctx)) != NULL) {
		state->ctx = ctx;
		return 0;
	}
	switch (state->err = errno) {
		case EBADF: php_error(E_WARNING, "Couldn't init SMB context: null context given"); break;
		case ENOMEM: php_error(E_WARNING, "Couldn't init SMB context: insufficient memory"); break;
		case ENOENT: php_error(E_WARNING, "Couldn't init SMB context: cannot load smb.conf"); break;
		default: php_error(E_WARNING, "Couldn't init SMB context: unknown error (%d)", errno); break;
	}
	return 1;
}

#if PHP_MAJOR_VERSION >= 7

#define SMB_FETCH_RESOURCE(_state, _type, _zval, _name, _le) \
	if ((_state = (_type)zend_fetch_resource(Z_RES_P(*_zval), _name, _le)) == NULL) { \
		RETURN_FALSE; \
	}
#else

#define SMB_FETCH_RESOURCE(_state, _type, _zval, _name, _le) \
	ZEND_FETCH_RESOURCE(_state, _type, _zval, -1, _name, _le);

#endif

#define STATE_FROM_ZSTATE \
	SMB_FETCH_RESOURCE(state, php_smbclient_state *, &zstate, PHP_SMBCLIENT_STATE_NAME, le_smbclient_state); \
	if (state == NULL || state->ctx == NULL) { \
		php_error(E_WARNING, PHP_SMBCLIENT_STATE_NAME " not found"); \
		RETURN_FALSE; \
	}

#define FILE_FROM_ZFILE \
	SMB_FETCH_RESOURCE(file, SMBCFILE *, &zfile, PHP_SMBCLIENT_FILE_NAME, le_smbclient_file); \
	if (file == NULL) { \
		php_error(E_WARNING, PHP_SMBCLIENT_FILE_NAME " not found"); \
		RETURN_FALSE; \
	}

PHP_FUNCTION(smbclient_state_init)
{
	zval *zstate;
	zval *zwrkg = NULL;
	zval *zuser = NULL;
	zval *zpass = NULL;
	php_smbclient_state *state;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r|zzz", &zstate, &zwrkg, &zuser, &zpass) != SUCCESS) {
		RETURN_FALSE;
	}
	SMB_FETCH_RESOURCE(state, php_smbclient_state *, &zstate, PHP_SMBCLIENT_STATE_NAME, le_smbclient_state);

	if (state->ctx == NULL) {
		php_error(E_WARNING, "Couldn't init SMB context: context is null");
		RETURN_FALSE;
	}
	if (ctx_init_getauth(zwrkg, &state->wrkg, &state->wrkglen, "workgroup") == 0) {
		RETURN_FALSE;
	}
	if (ctx_init_getauth(zuser, &state->user, &state->userlen, "username") == 0) {
		RETURN_FALSE;
	}
	if (ctx_init_getauth(zpass, &state->pass, &state->passlen, "password") == 0) {
		RETURN_FALSE;
	}
	if (php_smbclient_state_init(state TSRMLS_CC)) {
		RETURN_FALSE;
	}
	RETURN_TRUE;
}

PHP_FUNCTION(smbclient_state_errno)
{
	zval *zstate;
	php_smbclient_state *state;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r", &zstate) != SUCCESS) {
		RETURN_LONG(0);
	}
	SMB_FETCH_RESOURCE(state, php_smbclient_state *, &zstate, PHP_SMBCLIENT_STATE_NAME, le_smbclient_state);
	RETURN_LONG(state->err);
}

PHP_FUNCTION(smbclient_state_free)
{
	zval *zstate;
	php_smbclient_state *state;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r", &zstate) != SUCCESS) {
		RETURN_FALSE;
	}
	SMB_FETCH_RESOURCE(state, php_smbclient_state *, &zstate, PHP_SMBCLIENT_STATE_NAME, le_smbclient_state);

	if (state->ctx == NULL) {
#if PHP_MAJOR_VERSION >= 7
		zend_list_close(Z_RES_P(zstate));
#else
		zend_list_delete(Z_LVAL_P(zstate));
#endif
		RETURN_TRUE;
	}
	if (smbc_free_context(state->ctx, 1) == 0) {
		state->ctx = NULL;
#if PHP_MAJOR_VERSION >= 7
		zend_list_close(Z_RES_P(zstate));
#else
		zend_list_delete(Z_LVAL_P(zstate));
#endif
		RETURN_TRUE;
	}
	switch (state->err = errno) {
		case EBUSY: php_error(E_WARNING, "Couldn't destroy smbclient state: connection in use"); break;
		case EBADF: php_error(E_WARNING, "Couldn't destroy smbclient state: invalid handle"); break;
		default: php_error(E_WARNING, "Couldn't destroy smbclient state: unknown error (%d)", errno); break;
	}
	RETURN_FALSE;
}

PHP_FUNCTION(smbclient_opendir)
{
	char *path;
	strsize_t path_len;
	zval *zstate;
	SMBCFILE *dir;
	smbc_opendir_fn smbc_opendir;
	php_smbclient_state *state;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rs", &zstate, &path, &path_len) == FAILURE) {
		return;
	}
	STATE_FROM_ZSTATE;

	if ((smbc_opendir = smbc_getFunctionOpendir(state->ctx)) == NULL) {
		RETURN_FALSE;
	}
	if ((dir = smbc_opendir(state->ctx, path)) != NULL) {
#if PHP_MAJOR_VERSION >= 7
		ZVAL_RES(return_value, zend_register_resource(dir, le_smbclient_file));
#else
		ZEND_REGISTER_RESOURCE(return_value, dir, le_smbclient_file);
#endif
		return;
	}
	hide_password(path, path_len);
	switch (state->err = errno) {
		case EACCES: php_error(E_WARNING, "Couldn't open SMB directory %s: Permission denied", path); break;
		case EINVAL: php_error(E_WARNING, "Couldn't open SMB directory %s: Invalid URL", path); break;
		case ENOENT: php_error(E_WARNING, "Couldn't open SMB directory %s: Path does not exist", path); break;
		case ENOMEM: php_error(E_WARNING, "Couldn't open SMB directory %s: Insufficient memory", path); break;
		case ENOTDIR: php_error(E_WARNING, "Couldn't open SMB directory %s: Not a directory", path); break;
		case EPERM: php_error(E_WARNING, "Couldn't open SMB directory %s: Workgroup not found", path); break;
		case ENODEV: php_error(E_WARNING, "Couldn't open SMB directory %s: Workgroup or server not found", path); break;
		default: php_error(E_WARNING, "Couldn't open SMB directory %s: unknown error (%d)", path, errno); break;
	}
	RETURN_FALSE;
}

static char *
type_to_string (unsigned int type)
{
	switch (type) {
		case SMBC_WORKGROUP: return "workgroup";
		case SMBC_SERVER: return "server";
		case SMBC_FILE_SHARE: return "file share";
		case SMBC_PRINTER_SHARE: return "printer share";
		case SMBC_COMMS_SHARE: return "communication share";
		case SMBC_IPC_SHARE: return "IPC share";
		case SMBC_DIR: return "directory";
		case SMBC_FILE: return "file";
		case SMBC_LINK: return "link";
	}
	return "unknown";
}

PHP_FUNCTION(smbclient_readdir)
{
	struct smbc_dirent *dirent;
	zval *zstate;
	zval *zfile;
	SMBCFILE *file;
	smbc_readdir_fn smbc_readdir;
	php_smbclient_state *state;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rr", &zstate, &zfile) == FAILURE) {
		return;
	}
	STATE_FROM_ZSTATE;
	FILE_FROM_ZFILE;

	if ((smbc_readdir = smbc_getFunctionReaddir(state->ctx)) == NULL) {
		RETURN_FALSE;
	}
	errno = 0;
	if ((dirent = smbc_readdir(state->ctx, file)) == NULL) {
		switch (state->err = errno) {
			case 0: RETURN_FALSE;
			case EBADF: php_error(E_WARNING, "Couldn't read " PHP_SMBCLIENT_FILE_NAME ": Not a directory resource"); break;
			case EINVAL: php_error(E_WARNING, "Couldn't read " PHP_SMBCLIENT_FILE_NAME ": State resource not initialized"); break;
			default: php_error(E_WARNING, "Couldn't read " PHP_SMBCLIENT_FILE_NAME ": unknown error (%d)", errno); break;
		}
		RETURN_FALSE;
	}
	array_init(return_value);
#if PHP_MAJOR_VERSION >= 7
	add_assoc_string(return_value, "type", type_to_string(dirent->smbc_type));
	add_assoc_stringl(return_value, "comment", dirent->comment, dirent->commentlen);
	add_assoc_stringl(return_value, "name", dirent->name, dirent->namelen);
#else
	add_assoc_string(return_value, "type", type_to_string(dirent->smbc_type), 1);
	add_assoc_stringl(return_value, "comment", dirent->comment, dirent->commentlen, 1);
	add_assoc_stringl(return_value, "name", dirent->name, dirent->namelen, 1);
#endif
}

PHP_FUNCTION(smbclient_closedir)
{
	zval *zstate;
	zval *zfile;
	SMBCFILE *file;
	smbc_closedir_fn smbc_closedir;
	php_smbclient_state *state;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rr", &zstate, &zfile) == FAILURE) {
		return;
	}
	STATE_FROM_ZSTATE;
	FILE_FROM_ZFILE;

	if ((smbc_closedir = smbc_getFunctionClosedir(state->ctx)) == NULL) {
		RETURN_FALSE;
	}
	if (smbc_closedir(state->ctx, file) == 0) {
#if PHP_MAJOR_VERSION >= 7
		zend_list_close(Z_RES_P(zfile));
#else
		zend_list_delete(Z_LVAL_P(zfile));
#endif
		RETURN_TRUE;
	}
	switch (state->err = errno) {
		case EBADF: php_error(E_WARNING, "Couldn't close " PHP_SMBCLIENT_FILE_NAME ": Not a directory resource"); break;
		default: php_error(E_WARNING, "Couldn't close " PHP_SMBCLIENT_FILE_NAME ": unknown error (%d)", errno); break;
	}
	RETURN_FALSE;
}

PHP_FUNCTION(smbclient_rename)
{
	char *ourl, *nurl;
	strsize_t ourl_len, nurl_len;
	zval *zstate_old;
	zval *zstate_new;
	smbc_rename_fn smbc_rename;
	php_smbclient_state *state_old;
	php_smbclient_state *state_new;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rsrs", &zstate_old, &ourl, &ourl_len, &zstate_new, &nurl, &nurl_len) == FAILURE) {
		return;
	}
	SMB_FETCH_RESOURCE(state_old, php_smbclient_state *, &zstate_old, PHP_SMBCLIENT_STATE_NAME, le_smbclient_state);
	SMB_FETCH_RESOURCE(state_new, php_smbclient_state *, &zstate_new, PHP_SMBCLIENT_STATE_NAME, le_smbclient_state);

	if (state_old == NULL || state_old->ctx == NULL) {
		php_error(E_WARNING, "old " PHP_SMBCLIENT_STATE_NAME " is null");
		RETURN_FALSE;
	}
	if (state_new == NULL || state_new->ctx == NULL) {
		php_error(E_WARNING, "new " PHP_SMBCLIENT_STATE_NAME " is null");
		RETURN_FALSE;
	}
	if ((smbc_rename = smbc_getFunctionRename(state_old->ctx)) == NULL) {
		RETURN_FALSE;
	}
	if (smbc_rename(state_old->ctx, ourl, state_new->ctx, nurl) == 0) {
		RETURN_TRUE;
	}
	hide_password(ourl, ourl_len);
	switch (state_old->err = errno) {
		case EISDIR: php_error(E_WARNING, "Couldn't rename SMB directory %s: existing url is not a directory", ourl); break;
		case EACCES: php_error(E_WARNING, "Couldn't open SMB directory %s: Permission denied", ourl); break;
		case EINVAL: php_error(E_WARNING, "Couldn't open SMB directory %s: Invalid URL", ourl); break;
		case ENOENT: php_error(E_WARNING, "Couldn't open SMB directory %s: Path does not exist", ourl); break;
		case ENOMEM: php_error(E_WARNING, "Couldn't open SMB directory %s: Insufficient memory", ourl); break;
		case ENOTDIR: php_error(E_WARNING, "Couldn't open SMB directory %s: Not a directory", ourl); break;
		case EPERM: php_error(E_WARNING, "Couldn't open SMB directory %s: Workgroup not found", ourl); break;
		case EXDEV: php_error(E_WARNING, "Couldn't open SMB directory %s: Workgroup or server not found", ourl); break;
		case EEXIST: php_error(E_WARNING, "Couldn't rename SMB directory %s: new name already exists", ourl); break;
		default: php_error(E_WARNING, "Couldn't open SMB directory %s: unknown error (%d)", ourl, errno); break;
	}
	RETURN_FALSE;
}

PHP_FUNCTION(smbclient_unlink)
{
	char *url;
	strsize_t url_len;
	zval *zstate;
	smbc_unlink_fn smbc_unlink;
	php_smbclient_state *state;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rs", &zstate, &url, &url_len) == FAILURE) {
		return;
	}
	STATE_FROM_ZSTATE;

	if ((smbc_unlink = smbc_getFunctionUnlink(state->ctx)) == NULL) {
		RETURN_FALSE;
	}
	if (smbc_unlink(state->ctx, url) == 0) {
		RETURN_TRUE;
	}
	hide_password(url, url_len);
	switch (state->err = errno) {
		case EACCES: php_error(E_WARNING, "Couldn't delete %s: Permission denied", url); break;
		case EINVAL: php_error(E_WARNING, "Couldn't delete %s: Invalid URL", url); break;
		case ENOENT: php_error(E_WARNING, "Couldn't delete %s: Path does not exist", url); break;
		case ENOMEM: php_error(E_WARNING, "Couldn't delete %s: Insufficient memory", url); break;
		case EPERM: php_error(E_WARNING, "Couldn't delete %s: Workgroup not found", url); break;
		case EISDIR: php_error(E_WARNING, "Couldn't delete %s: It is a Directory (use rmdir instead)", url); break;
		case EBUSY: php_error(E_WARNING, "Couldn't delete %s: Device or resource busy", url); break;
		default: php_error(E_WARNING, "Couldn't delete %s: unknown error (%d)", url, errno); break;
	}
	RETURN_FALSE;
}

PHP_FUNCTION(smbclient_mkdir)
{
	char *path = NULL;
	strsize_t path_len;
	zend_long mode = 0777;	/* Same as PHP's native mkdir() */
	zval *zstate;
	smbc_mkdir_fn smbc_mkdir;
	php_smbclient_state *state;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rs|l", &zstate, &path, &path_len, &mode) == FAILURE) {
		return;
	}
	STATE_FROM_ZSTATE;

	if ((smbc_mkdir = smbc_getFunctionMkdir(state->ctx)) == NULL) {
		RETURN_FALSE;
	}
	if (smbc_mkdir(state->ctx, path, (mode_t)mode) == 0) {
		RETURN_TRUE;
	}
	hide_password(path, path_len);
	switch (state->err = errno) {
		case EACCES: php_error(E_WARNING, "Couldn't create SMB directory %s: Permission denied", path); break;
		case EINVAL: php_error(E_WARNING, "Couldn't create SMB directory %s: Invalid URL", path); break;
		case ENOENT: php_error(E_WARNING, "Couldn't create SMB directory %s: Path does not exist", path); break;
		case ENOMEM: php_error(E_WARNING, "Couldn't create SMB directory %s: Insufficient memory", path); break;
		case EEXIST: php_error(E_WARNING, "Couldn't create SMB directory %s: Directory already exists", path); break;
		default: php_error(E_WARNING, "Couldn't create SMB directory %s: unknown error (%d)", path, errno); break;
	}
	RETURN_FALSE;
}

PHP_FUNCTION(smbclient_rmdir)
{
	char *url;
	strsize_t url_len;
	zval *zstate;
	smbc_rmdir_fn smbc_rmdir;
	php_smbclient_state *state;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rs", &zstate, &url, &url_len) == FAILURE) {
		return;
	}
	STATE_FROM_ZSTATE;

	if ((smbc_rmdir = smbc_getFunctionRmdir(state->ctx)) == NULL) {
		RETURN_FALSE;
	}
	if (smbc_rmdir(state->ctx, url) == 0) {
		RETURN_TRUE;
	}
	hide_password(url, url_len);
	switch (state->err = errno) {
		case EACCES: php_error(E_WARNING, "Couldn't delete %s: Permission denied", url); break;
		case EINVAL: php_error(E_WARNING, "Couldn't delete %s: Invalid URL", url); break;
		case ENOENT: php_error(E_WARNING, "Couldn't delete %s: Path does not exist", url); break;
		case ENOMEM: php_error(E_WARNING, "Couldn't delete %s: Insufficient memory", url); break;
		case EPERM: php_error(E_WARNING, "Couldn't delete %s: Workgroup not found", url); break;
		case ENOTEMPTY: php_error(E_WARNING, "Couldn't delete %s: It is not empty", url); break;
		default: php_error(E_WARNING, "Couldn't delete %s: unknown error (%d)", url, errno); break;
	}
	RETURN_FALSE;
}

PHP_FUNCTION(smbclient_stat)
{
	char *file;
	struct stat statbuf;
	strsize_t file_len;
	zval *zstate;
	smbc_stat_fn smbc_stat;
	php_smbclient_state *state;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rs", &zstate, &file, &file_len) == FAILURE) {
		return;
	}
	STATE_FROM_ZSTATE;

	if ((smbc_stat = smbc_getFunctionStat(state->ctx)) == NULL) {
		RETURN_FALSE;
	}
	if (smbc_stat(state->ctx, file, &statbuf) < 0) {
		hide_password(file, file_len);
		switch (state->err = errno) {
			case ENOENT: php_error(E_WARNING, "Couldn't stat %s: Does not exist", file); break;
			case EINVAL: php_error(E_WARNING, "Couldn't stat: null URL or smbc_init failed"); break;
			case EACCES: php_error(E_WARNING, "Couldn't stat %s: Permission denied", file); break;
			case ENOMEM: php_error(E_WARNING, "Couldn't stat %s: Out of memory", file); break;
			case ENOTDIR: php_error(E_WARNING, "Couldn't stat %s: Not a directory", file); break;
			default: php_error(E_WARNING, "Couldn't stat %s: unknown error (%d)", file, errno); break;
		}
		RETURN_FALSE;
	}
	array_init(return_value);
	add_index_long(return_value, 0, statbuf.st_dev);
	add_index_long(return_value, 1, statbuf.st_ino);
	add_index_long(return_value, 2, statbuf.st_mode);
	add_index_long(return_value, 3, statbuf.st_nlink);
	add_index_long(return_value, 4, statbuf.st_uid);
	add_index_long(return_value, 5, statbuf.st_gid);
	add_index_long(return_value, 6, statbuf.st_rdev);
	add_index_long(return_value, 7, statbuf.st_size);
	add_index_long(return_value, 8, statbuf.st_atime);
	add_index_long(return_value, 9, statbuf.st_mtime);
	add_index_long(return_value, 10, statbuf.st_ctime);
	add_index_long(return_value, 11, statbuf.st_blksize);
	add_index_long(return_value, 12, statbuf.st_blocks);

	add_assoc_long(return_value, "dev", statbuf.st_dev);
	add_assoc_long(return_value, "ino", statbuf.st_ino);
	add_assoc_long(return_value, "mode", statbuf.st_mode);
	add_assoc_long(return_value, "nlink", statbuf.st_nlink);
	add_assoc_long(return_value, "uid", statbuf.st_uid);
	add_assoc_long(return_value, "gid", statbuf.st_gid);
	add_assoc_long(return_value, "rdev", statbuf.st_rdev);
	add_assoc_long(return_value, "size", statbuf.st_size);
	add_assoc_long(return_value, "atime", statbuf.st_atime);
	add_assoc_long(return_value, "mtime", statbuf.st_mtime);
	add_assoc_long(return_value, "ctime", statbuf.st_ctime);
	add_assoc_long(return_value, "blksize", statbuf.st_blksize);
	add_assoc_long(return_value, "blocks", statbuf.st_blocks);
}

PHP_FUNCTION(smbclient_fstat)
{
	struct stat statbuf;
	zval *zstate;
	zval *zfile;
	SMBCFILE *file;
	smbc_fstat_fn smbc_fstat;
	php_smbclient_state *state;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rr", &zstate, &zfile) == FAILURE) {
		return;
	}
	STATE_FROM_ZSTATE;
	FILE_FROM_ZFILE;

	if ((smbc_fstat = smbc_getFunctionFstat(state->ctx)) == NULL) {
		RETURN_FALSE;
	}
	if (smbc_fstat(state->ctx, file, &statbuf) < 0) {
		switch (state->err = errno) {
			case ENOENT: php_error(E_WARNING, "Couldn't fstat " PHP_SMBCLIENT_FILE_NAME ": Does not exist"); break;
			case EINVAL: php_error(E_WARNING, "Couldn't fstat: null resource or smbc_init failed"); break;
			case EACCES: php_error(E_WARNING, "Couldn't fstat " PHP_SMBCLIENT_FILE_NAME ": Permission denied"); break;
			case ENOMEM: php_error(E_WARNING, "Couldn't fstat " PHP_SMBCLIENT_FILE_NAME ": Out of memory"); break;
			case ENOTDIR: php_error(E_WARNING, "Couldn't fstat " PHP_SMBCLIENT_FILE_NAME ": Not a directory"); break;
			default: php_error(E_WARNING, "Couldn't fstat " PHP_SMBCLIENT_FILE_NAME ": unknown error (%d)", errno); break;
		}
		RETURN_FALSE;
	}
	array_init(return_value);
	add_index_long(return_value, 0, statbuf.st_dev);
	add_index_long(return_value, 1, statbuf.st_ino);
	add_index_long(return_value, 2, statbuf.st_mode);
	add_index_long(return_value, 3, statbuf.st_nlink);
	add_index_long(return_value, 4, statbuf.st_uid);
	add_index_long(return_value, 5, statbuf.st_gid);
	add_index_long(return_value, 6, statbuf.st_rdev);
	add_index_long(return_value, 7, statbuf.st_size);
	add_index_long(return_value, 8, statbuf.st_atime);
	add_index_long(return_value, 9, statbuf.st_mtime);
	add_index_long(return_value, 10, statbuf.st_ctime);
	add_index_long(return_value, 11, statbuf.st_blksize);
	add_index_long(return_value, 12, statbuf.st_blocks);

	add_assoc_long(return_value, "dev", statbuf.st_dev);
	add_assoc_long(return_value, "ino", statbuf.st_ino);
	add_assoc_long(return_value, "mode", statbuf.st_mode);
	add_assoc_long(return_value, "nlink", statbuf.st_nlink);
	add_assoc_long(return_value, "uid", statbuf.st_uid);
	add_assoc_long(return_value, "gid", statbuf.st_gid);
	add_assoc_long(return_value, "rdev", statbuf.st_rdev);
	add_assoc_long(return_value, "size", statbuf.st_size);
	add_assoc_long(return_value, "atime", statbuf.st_atime);
	add_assoc_long(return_value, "mtime", statbuf.st_mtime);
	add_assoc_long(return_value, "ctime", statbuf.st_ctime);
	add_assoc_long(return_value, "blksize", statbuf.st_blksize);
	add_assoc_long(return_value, "blocks", statbuf.st_blocks);
}

int
flagstring_to_smbflags (const char *flags, int flags_len, int *retval TSRMLS_DC)
{
	/* Returns 0 on failure, or 1 on success with *retval filled. */
	if (flags_len != 1 && flags_len != 2) {
		goto err;
	}
	if (flags_len == 2 && flags[1] != '+') {
		goto err;
	}
	/* For both lengths, add the "core business" flags.
	 * See php_stream_parse_fopen_modes() in PHP's /main/streams/plain_wrapper.c
	 * for how PHP's native fopen() translates these flags: */
	switch (flags[0]) {
		case 'r': *retval = 0; break;
		case 'w': *retval = O_CREAT | O_TRUNC; break;
		case 'a': *retval = O_CREAT | O_APPEND; break;
		case 'x': *retval = O_CREAT | O_EXCL; break;
		case 'c': *retval = O_CREAT; break;
		default: goto err;
	}
	/* If length is 1, enforce read-only or write-only: */
	if (flags_len == 1) {
		*retval |= (*retval == 0) ? O_RDONLY : O_WRONLY;
		return 1;
	}
	/* Length is 2 and this is a '+' mode, so read/write everywhere: */
	*retval |= O_RDWR;
	return 1;

err:
	php_error_docref(NULL TSRMLS_CC, E_WARNING, "Invalid flag string '%s'", flags);
	return 0;
}

PHP_FUNCTION(smbclient_open)
{
	char *file, *flags;
	strsize_t file_len, flags_len;
	int smbflags;
	zend_long mode = 0666;
	zval *zstate;
	SMBCFILE *handle;
	smbc_open_fn smbc_open;
	php_smbclient_state *state;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rss|l", &zstate, &file, &file_len, &flags, &flags_len, &mode) == FAILURE) {
		return;
	}
	STATE_FROM_ZSTATE;

	/* The flagstring is in the same format as the native fopen() uses, so
	 * one of the characters r, w, a, x, c, optionally followed by a plus.
	 * Need to translate this to an integer value for smbc_open: */
	if (flagstring_to_smbflags(flags, flags_len, &smbflags TSRMLS_CC) == 0) {
		RETURN_FALSE;
	}
	if ((smbc_open = smbc_getFunctionOpen(state->ctx)) == NULL) {
		RETURN_FALSE;
	}
	if ((handle = smbc_open(state->ctx, file, smbflags, mode)) != NULL) {
#if PHP_MAJOR_VERSION >= 7
		ZVAL_RES(return_value, zend_register_resource(handle, le_smbclient_file));
#else
		ZEND_REGISTER_RESOURCE(return_value, handle, le_smbclient_file);
#endif
		return;
	}
	hide_password(file, file_len);
	switch (state->err = errno) {
		case ENOMEM: php_error(E_WARNING, "Couldn't open %s: Out of memory", file); break;
		case EINVAL: php_error(E_WARNING, "Couldn't open %s: No file?", file); break;
		case EEXIST: php_error(E_WARNING, "Couldn't open %s: Pathname already exists and O_CREAT and O_EXECL were specified", file); break;
		case EISDIR: php_error(E_WARNING, "Couldn't open %s: Can't write to a directory", file); break;
		case EACCES: php_error(E_WARNING, "Couldn't open %s: Access denied", file); break;
		case ENODEV: php_error(E_WARNING, "Couldn't open %s: Requested share does not exist", file); break;
		case ENOTDIR: php_error(E_WARNING, "Couldn't open %s: Path component isn't a directory", file); break;
		case ENOENT: php_error(E_WARNING, "Couldn't open %s: Directory in path doesn't exist", file); break;
		default: php_error(E_WARNING, "Couldn't open %s: unknown error (%d)", file, errno); break;
	}
	RETURN_FALSE;
}

PHP_FUNCTION(smbclient_creat)
{
	char *file;
	strsize_t file_len;
	zend_long mode = 0666;
	zval *zstate;
	SMBCFILE *handle;
	smbc_creat_fn smbc_creat;
	php_smbclient_state *state;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rs|l", &zstate, &file, &file_len, &mode) == FAILURE) {
		return;
	}
	STATE_FROM_ZSTATE;

	if ((smbc_creat = smbc_getFunctionCreat(state->ctx)) == NULL) {
		RETURN_FALSE;
	}
	if ((handle = smbc_creat(state->ctx, file, (mode_t)mode)) != NULL) {
#if PHP_MAJOR_VERSION >= 7
		ZVAL_RES(return_value, zend_register_resource(handle, le_smbclient_file));
#else
		ZEND_REGISTER_RESOURCE(return_value, handle, le_smbclient_file);
#endif

		return;
	}
	hide_password(file, file_len);
	switch (state->err = errno) {
		case ENOMEM: php_error(E_WARNING, "Couldn't create %s: Out of memory", file); break;
		case EINVAL: php_error(E_WARNING, "Couldn't create %s: No file?", file); break;
		case EEXIST: php_error(E_WARNING, "Couldn't create %s: Pathname already exists and O_CREAT and O_EXECL were specified", file); break;
		case EISDIR: php_error(E_WARNING, "Couldn't create %s: Can't write to a directory", file); break;
		case EACCES: php_error(E_WARNING, "Couldn't create %s: Access denied", file); break;
		case ENODEV: php_error(E_WARNING, "Couldn't create %s: Requested share does not exist", file); break;
		case ENOENT: php_error(E_WARNING, "Couldn't create %s: Directory in path doesn't exist", file); break;
		default: php_error(E_WARNING, "Couldn't create %s: unknown error (%d)", file, errno); break;
	}
	RETURN_FALSE;
}

PHP_FUNCTION(smbclient_read)
{
	zend_long count;
	zval *zstate;
	zval *zfile;
	SMBCFILE *file;
	smbc_read_fn smbc_read;
	php_smbclient_state *state;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rrl", &zstate, &zfile, &count) == FAILURE) {
		return;
	}
	if (count < 0) {
		php_error(E_WARNING, "Negative byte count: %ld", count);
		RETURN_FALSE;
	}
	STATE_FROM_ZSTATE;
	FILE_FROM_ZFILE;

	if ((smbc_read = smbc_getFunctionRead(state->ctx)) == NULL) {
		RETURN_FALSE;
	}
#if PHP_MAJOR_VERSION >= 7
	zend_string *buf = zend_string_alloc(count, 0);

	if ((ZSTR_LEN(buf) = smbc_read(state->ctx, file, ZSTR_VAL(buf), count)) >= 0) {
		RETURN_STR(buf);
	}
	zend_string_release(buf);
#else
	void *buf = emalloc(count);
	ssize_t nbytes;

	if ((nbytes = smbc_read(state->ctx, file, buf, count)) >= 0) {
		RETURN_STRINGL(buf, nbytes, 0);
	}
	efree(buf);
#endif
	switch (state->err = errno) {
		case EISDIR: php_error(E_WARNING, "Read error: Is a directory"); break;
		case EBADF: php_error(E_WARNING, "Read error: Not a valid file resource or not open for reading"); break;
		case EINVAL: php_error(E_WARNING, "Read error: Object not suitable for reading or bad buffer"); break;
		default: php_error(E_WARNING, "Read error: unknown error (%d)", errno); break;
	}
	RETURN_FALSE;
}

PHP_FUNCTION(smbclient_write)
{
	zend_long count = 0;
	strsize_t str_len;
	char * str;
	size_t nwrite;
	ssize_t nbytes;
	zval *zstate;
	zval *zfile;
	SMBCFILE *file;
	smbc_write_fn smbc_write;
	php_smbclient_state *state;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rrs|l", &zstate, &zfile, &str, &str_len, &count) == FAILURE) {
		return;
	}
	if (count < 0) {
		php_error(E_WARNING, "Negative byte count: %ld", count);
		RETURN_FALSE;
	}
	if (count == 0 || count > str_len) {
		nwrite = str_len;
	}
	else {
		nwrite = count;
	}
	STATE_FROM_ZSTATE;
	FILE_FROM_ZFILE;

	if ((smbc_write = smbc_getFunctionWrite(state->ctx)) == NULL) {
		RETURN_FALSE;
	}
	if ((nbytes = smbc_write(state->ctx, file, str, nwrite)) >= 0) {
		RETURN_LONG(nbytes);
	}
	switch (state->err = errno) {
		case EISDIR: php_error(E_WARNING, "Write error: Is a directory"); break;
		case EBADF: php_error(E_WARNING, "Write error: Not a valid file resource or not open for reading"); break;
		case EINVAL: php_error(E_WARNING, "Write error: Object not suitable for reading or bad buffer"); break;
		case EACCES: php_error(E_WARNING, "Write error: Permission denied"); break;
		default: php_error(E_WARNING, "Write error: unknown error (%d)", errno); break;
	}
	RETURN_FALSE;
}

PHP_FUNCTION(smbclient_lseek)
{
	zend_long offset, whence, ret;
	zval *zstate;
	zval *zfile;
	SMBCFILE *file;
	smbc_lseek_fn smbc_lseek;
	php_smbclient_state *state;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rrll", &zstate, &zfile, &offset, &whence) == FAILURE) {
		return;
	}
	if ((int)whence != SEEK_SET && (int)whence != SEEK_CUR && (int)whence != SEEK_END) {
		php_error(E_WARNING, "Invalid argument for whence");
		RETURN_FALSE;
	}
	STATE_FROM_ZSTATE;
	FILE_FROM_ZFILE;

	if ((smbc_lseek = smbc_getFunctionLseek(state->ctx)) == NULL) {
		RETURN_FALSE;
	}
	if ((ret = smbc_lseek(state->ctx, file, (off_t)offset, (int)whence)) > -1) {
		RETURN_LONG(ret);
	}
	switch (state->err = errno) {
		case EBADF: php_error(E_WARNING, "Couldn't lseek: resource is invalid"); break;
		case EINVAL: php_error(E_WARNING, "Couldn't lseek: invalid parameters or not initialized"); break;
		default: php_error(E_WARNING, "Couldn't lseek: unknown error (%d)", errno); break;
	}
	RETURN_FALSE;
}

PHP_FUNCTION(smbclient_ftruncate)
{
	zend_long offset;
	zval *zstate;
	zval *zfile;
	SMBCFILE *file;
	smbc_ftruncate_fn smbc_ftruncate;
	php_smbclient_state *state;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rrl", &zstate, &zfile, &offset) == FAILURE) {
		return;
	}
	STATE_FROM_ZSTATE;
	FILE_FROM_ZFILE;

	if ((smbc_ftruncate = smbc_getFunctionFtruncate(state->ctx)) == NULL) {
		RETURN_FALSE;
	}
	if (smbc_ftruncate(state->ctx, file, offset) == 0) {
		RETURN_TRUE;
	}
	switch (state->err = errno) {
		case EBADF: php_error(E_WARNING, "Couldn't ftruncate: resource is invalid"); break;
		case EACCES: php_error(E_WARNING, "Couldn't ftruncate: permission denied"); break;
		case EINVAL: php_error(E_WARNING, "Couldn't ftruncate: invalid parameters or not initialized"); break;
		case ENOMEM: php_error(E_WARNING, "Couldn't ftruncate: out of memory"); break;
		default: php_error(E_WARNING, "Couldn't ftruncate: unknown error (%d)", errno); break;
	}
	RETURN_FALSE;
}

PHP_FUNCTION(smbclient_close)
{
	zval *zstate;
	zval *zfile;
	SMBCFILE *file;
	smbc_close_fn smbc_close;
	php_smbclient_state *state;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rr", &zstate, &zfile) == FAILURE) {
		return;
	}
	STATE_FROM_ZSTATE;
	FILE_FROM_ZFILE;

	if ((smbc_close = smbc_getFunctionClose(state->ctx)) == NULL) {
		RETURN_FALSE;
	}
	if (smbc_close(state->ctx, file) == 0) {
#if PHP_MAJOR_VERSION >= 7
		zend_list_close(Z_RES_P(zfile));
#else
		zend_list_delete(Z_LVAL_P(zfile));
#endif
		RETURN_TRUE;
	}
	switch (state->err = errno) {
		case EBADF: php_error(E_WARNING, "Close error: Not a valid file resource or not open for reading"); break;
		case EINVAL: php_error(E_WARNING, "Close error: State resource not initialized"); break;
		default: php_error(E_WARNING, "Close error: unknown error (%d)", errno); break;
	}
	RETURN_FALSE;
}

PHP_FUNCTION(smbclient_chmod)
{
	char *file;
	strsize_t file_len;
	zend_long mode;
	zval *zstate;
	smbc_chmod_fn smbc_chmod;
	php_smbclient_state *state;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rsl", &zstate, &file, &file_len, &mode) == FAILURE) {
		return;
	}
	STATE_FROM_ZSTATE;

	if ((smbc_chmod = smbc_getFunctionChmod(state->ctx)) == NULL) {
		RETURN_FALSE;
	}
	if (smbc_chmod(state->ctx, file, (mode_t)mode) == 0) {
		RETURN_TRUE;
	}
	hide_password(file, file_len);
	switch (state->err = errno) {
		case EPERM: php_error(E_WARNING, "Couldn't chmod %s: the effective UID does not match the owner of the file, and is not zero", file); break;
		case ENOENT: php_error(E_WARNING, "Couldn't chmod %s: file or directory does not exist", file); break;
		case ENOMEM: php_error(E_WARNING, "Couldn't chmod %s: insufficient memory", file); break;
		default: php_error(E_WARNING, "Couldn't chmod %s: unknown error (%d)", file, errno); break;
	}
	RETURN_FALSE;
}

PHP_FUNCTION(smbclient_utimes)
{
	char *file;
	strsize_t file_len;
	zend_long mtime = -1, atime = -1;
	zval *zstate;
	struct timeval times[2];
	smbc_utimes_fn smbc_utimes;
	php_smbclient_state *state;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rs|ll", &zstate, &file, &file_len, &mtime, &atime) == FAILURE) {
		return;
	}
	STATE_FROM_ZSTATE;

	times[0].tv_usec = 0;	/* times[0] = access time (atime) */
	times[1].tv_usec = 0;	/* times[1] = write time (mtime) */

	/* TODO: we are a bit lazy here about the optional arguments and assume
	 * that if they are negative, they were omitted. This makes it
	 * impossible to use legitimate negative timestamps - a rare use-case. */
	times[1].tv_sec = (mtime < 0) ? time(NULL) : mtime;

	/* If not given, atime defaults to value of mtime: */
	times[0].tv_sec = (atime < 0) ? times[1].tv_sec : atime;

	if ((smbc_utimes = smbc_getFunctionUtimes(state->ctx)) == NULL) {
		RETURN_FALSE;
	}
	if (smbc_utimes(state->ctx, file, times) == 0) {
		RETURN_TRUE;
	}
	hide_password(file, file_len);
	switch (state->err = errno) {
		case EINVAL: php_error(E_WARNING, "Couldn't set times on %s: the client library is not properly initialized", file); break;
		case EPERM: php_error(E_WARNING, "Couldn't set times on %s: permission was denied", file); break;
		default: php_error(E_WARNING, "Couldn't set times on %s: unknown error (%d)", file, errno); break;
	}
	RETURN_FALSE;
}

PHP_FUNCTION(smbclient_listxattr)
{
	char *url, *s, *c;
	strsize_t url_len;
	char values[1000];
	zval *zstate;
	smbc_listxattr_fn smbc_listxattr;
	php_smbclient_state *state;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rs", &zstate, &url, &url_len) == FAILURE) {
		return;
	}
	STATE_FROM_ZSTATE;

	if ((smbc_listxattr = smbc_getFunctionListxattr(state->ctx)) == NULL) {
		RETURN_FALSE;
	}
	/* This is a bit of a bogus function. Looking in the Samba source, it
	 * always returns all possible attribute names, regardless of what the
	 * file system supports or which attributes the file actually has.
	 * Because this list is static, we can get away with using a fixed
	 * buffer size.*/
	if (smbc_listxattr(state->ctx, url, values, sizeof(values)) >= 0) {
		array_init(return_value);
		/* Each attribute is null-separated, the list itself terminates
		 * with an empty element (i.e. two null bytes in a row). */
		for (s = c = values; c < values + sizeof(values); c++) {
			if (*c != '\0') {
				continue;
			}
			/* c and s identical: last element */
			if (s == c) {
				break;
			}
#if PHP_MAJOR_VERSION >= 7
			add_next_index_stringl(return_value, s, c - s);
#else
			add_next_index_stringl(return_value, s, c - s, 1);
#endif
			s = c + 1;
		}
		return;
	}
	switch (state->err = errno) {
		case EINVAL: php_error(E_WARNING, "Couldn't get xattrs: library not initialized"); break;
		case ENOMEM: php_error(E_WARNING, "Couldn't get xattrs: out of memory"); break;
		case EPERM: php_error(E_WARNING, "Couldn't get xattrs: permission denied"); break;
		case ENOTSUP: php_error(E_WARNING, "Couldn't get xattrs: file system does not support extended attributes"); break;
		default: php_error(E_WARNING, "Couldn't get xattrs: unknown error (%d)", errno); break;
	}
	RETURN_FALSE;
}


/* loop from 16K to 256M */
#define DEFAULT_BUFFER_SIZE   (16 << 10)
#define MAXIMUM_BUFFER_SIZE  (256 << 20)

PHP_FUNCTION(smbclient_getxattr)
{
	char *url, *name;
	strsize_t url_len, name_len;
	int retsize;
	int xattr_size;
	zval *zstate;
	smbc_getxattr_fn smbc_getxattr;
	php_smbclient_state *state;
	char *values = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rss", &zstate, &url, &url_len, &name, &name_len) == FAILURE) {
		return;
	}
	STATE_FROM_ZSTATE;

	if ((smbc_getxattr = smbc_getFunctionGetxattr(state->ctx)) == NULL) {
		RETURN_FALSE;
	}

	xattr_size = smbc_getxattr(state->ctx, url, name, NULL, 0);

	if (xattr_size < 0) {
		goto fail;
	}

	if (xattr_size == 0) {
		/* since version 4.16.9 and 4.17.5 this means success :(
		 * so there is no way to compute the buffer size
		 * see https://bugzilla.samba.org/show_bug.cgi?id=14808
		 */
		xattr_size = DEFAULT_BUFFER_SIZE;
		do {
			if (values) {
				efree(values);
				xattr_size *= 4;
			}
			values = emalloc(xattr_size + 1);
			retsize = smbc_getxattr(state->ctx, url, name, values, xattr_size + 1);
		} while (retsize < 0 && xattr_size < MAXIMUM_BUFFER_SIZE);
	} else {
		values = emalloc(xattr_size + 1);
		retsize = smbc_getxattr(state->ctx, url, name, values, xattr_size + 1);
	}

	if (retsize == 0) { /* success, since 4.16.9 and 4.17.5 */
		retsize = strlen(values);
	} else if (retsize > xattr_size) { /* time-of-check, time-of-use error, never happen as recent versions return -1 */
		retsize = xattr_size;
	} else if (retsize < 0) {
		efree(values);
		goto fail;
	}
	/* realloc the string to its real size */
#if PHP_MAJOR_VERSION >= 7
	RETVAL_STRINGL(values, retsize);
#else
	RETVAL_STRINGL(values, retsize, 1);
#endif
	efree(values);
	return;

fail:
	hide_password(url, url_len);
	switch (state->err = errno) {
		case EINVAL: php_error(E_WARNING, "Couldn't get xattr for %s: library not initialized or incorrect parameter", url); break;
		case ENOMEM: php_error(E_WARNING, "Couldn't get xattr for %s: out of memory", url); break;
		case EPERM: php_error(E_WARNING, "Couldn't get xattr for %s: permission denied", url); break;
		case ENOTSUP: php_error(E_WARNING, "Couldn't get xattr for %s: file system does not support extended attributes", url); break;
		default:
			if (xattr_size == MAXIMUM_BUFFER_SIZE) {
				php_error(E_WARNING, "Couldn't get xattr for %s: internal buffer is too small", url); break;
			} else {
				php_error(E_WARNING, "Couldn't get xattr for %s: unknown error (%d)", url, errno); break;
			}
	}
	RETURN_FALSE;
}

PHP_FUNCTION(smbclient_setxattr)
{
	char *url, *name, *val;
	strsize_t url_len, name_len, val_len;
	zend_long flags = 0;
	zval *zstate;
	smbc_setxattr_fn smbc_setxattr;
	php_smbclient_state *state;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rsss|l", &zstate, &url, &url_len, &name, &name_len, &val, &val_len, &flags) == FAILURE) {
		return;
	}
	STATE_FROM_ZSTATE;

	if ((smbc_setxattr = smbc_getFunctionSetxattr(state->ctx)) == NULL) {
		RETURN_FALSE;
	}
	if (smbc_setxattr(state->ctx, url, name, val, val_len, flags) == 0) {
		RETURN_TRUE;
	}
	hide_password(url, url_len);
	switch (state->err = errno) {
		case EINVAL: php_error(E_WARNING, "Couldn't set attribute on %s: client library not properly initialized", url); break;
		case ENOMEM: php_error(E_WARNING, "Couldn't set attribute on %s: out of memory", url); break;
		case EEXIST: php_error(E_WARNING, "Couldn't set attribute on %s: attribute already exists", url); break;
		case ENOATTR: php_error(E_WARNING, "Couldn't set attribute on %s: attribute does not exist", url); break;
		case ENOTSUP: php_error(E_WARNING, "Couldn't set attribute on %s: not supported by filesystem", url); break;
		case EPERM: php_error(E_WARNING, "Couldn't set attribute on %s: permission denied", url); break;
		default: php_error(E_WARNING, "Couldn't set attribute on %s: unknown error (%d)", url, errno); break;
	}
	RETURN_FALSE;
}

PHP_FUNCTION(smbclient_removexattr)
{
	char *url, *name;
	strsize_t url_len, name_len;
	zval *zstate;
	smbc_removexattr_fn smbc_removexattr;
	php_smbclient_state *state;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rss", &zstate, &url, &url_len, &name, &name_len) == FAILURE) {
		return;
	}
	STATE_FROM_ZSTATE;

	if ((smbc_removexattr = smbc_getFunctionRemovexattr(state->ctx)) == NULL) {
		RETURN_FALSE;
	}
	if (smbc_removexattr(state->ctx, url, name) == 0) {
		RETURN_TRUE;
	}
	hide_password(url, url_len);
	switch (state->err = errno) {
		case EINVAL: php_error(E_WARNING, "Couldn't remove attribute on %s: client library not properly initialized", url); break;
		case ENOMEM: php_error(E_WARNING, "Couldn't remove attribute on %s: out of memory", url); break;
		case ENOTSUP: php_error(E_WARNING, "Couldn't remove attribute on %s: not supported by filesystem", url); break;
		case EPERM: php_error(E_WARNING, "Couldn't remove attribute on %s: permission denied", url); break;
		default: php_error(E_WARNING, "Couldn't remove attribute on %s: unknown error (%d)", url, errno); break;
	}
	RETURN_FALSE;
}

PHP_FUNCTION(smbclient_option_get)
{
	zend_long option;
	const char *ret;
	zval *zstate;
	php_smbclient_state *state;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rl", &zstate, &option) == FAILURE) {
		return;
	}
	STATE_FROM_ZSTATE;

	switch (option)
	{
	case SMBCLIENT_OPT_OPEN_SHAREMODE:
		RETURN_LONG(smbc_getOptionOpenShareMode(state->ctx));

	case SMBCLIENT_OPT_ENCRYPT_LEVEL:
		RETURN_LONG(smbc_getOptionSmbEncryptionLevel(state->ctx));

	case SMBCLIENT_OPT_CASE_SENSITIVE:
		RETURN_BOOL(smbc_getOptionCaseSensitive(state->ctx));

	case SMBCLIENT_OPT_BROWSE_MAX_LMB_COUNT:
		RETURN_LONG(smbc_getOptionBrowseMaxLmbCount(state->ctx));

	case SMBCLIENT_OPT_URLENCODE_READDIR_ENTRIES:
		RETURN_BOOL(smbc_getOptionUrlEncodeReaddirEntries(state->ctx));

	case SMBCLIENT_OPT_USE_KERBEROS:
		RETURN_BOOL(smbc_getOptionUseKerberos(state->ctx));

	case SMBCLIENT_OPT_FALLBACK_AFTER_KERBEROS:
		RETURN_BOOL(smbc_getOptionFallbackAfterKerberos(state->ctx));

	/* Reverse the sense of this option, the original is confusing: */
	case SMBCLIENT_OPT_AUTO_ANONYMOUS_LOGIN:
		RETURN_BOOL(!(smbc_getOptionNoAutoAnonymousLogin(state->ctx)));

	case SMBCLIENT_OPT_USE_CCACHE:
		RETURN_BOOL(smbc_getOptionUseCCache(state->ctx));

	#ifdef HAVE_SMBC_SETOPTIONUSENTHASH
	case SMBCLIENT_OPT_USE_NT_HASH:
		RETURN_BOOL(smbc_getOptionUseNTHash(state->ctx));
	#endif

	#ifdef HAVE_SMBC_SETPORT
	case SMBCLIENT_OPT_PORT:
		RETURN_LONG(smbc_getPort(state->ctx));
	#endif

	case SMBCLIENT_OPT_TIMEOUT:
		RETURN_LONG(smbc_getTimeout(state->ctx));

	case SMBCLIENT_OPT_NETBIOS_NAME:
		if ((ret = smbc_getNetbiosName(state->ctx)) == NULL) {
			RETURN_EMPTY_STRING();
		}
		if (strlen(ret) == 0) {
			RETURN_EMPTY_STRING();
		}
#if PHP_MAJOR_VERSION >= 7
		RETURN_STRING(ret);
#else
		RETURN_STRING(ret, 1);
#endif

	case SMBCLIENT_OPT_WORKGROUP:
		if ((ret = smbc_getWorkgroup(state->ctx)) == NULL) {
			RETURN_EMPTY_STRING();
		}
		if (strlen(ret) == 0) {
			RETURN_EMPTY_STRING();
		}
#if PHP_MAJOR_VERSION >= 7
		RETURN_STRING(ret);
#else
		RETURN_STRING(ret, 1);
#endif
	case SMBCLIENT_OPT_USER:
		if ((ret = smbc_getUser(state->ctx)) == NULL) {
			RETURN_EMPTY_STRING();
		}
		if (strlen(ret) == 0) {
			RETURN_EMPTY_STRING();
		}
#if PHP_MAJOR_VERSION >= 7
		RETURN_STRING(ret);
#else
		RETURN_STRING(ret, 1);
#endif

	}
	RETURN_NULL();
}

PHP_FUNCTION(smbclient_option_set)
{
	zend_long option, vbool = 0;
	zval *zstate;
	zval *zvalue;
	php_smbclient_state *state;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rlz", &zstate, &option, &zvalue) == FAILURE) {
		return;
	}
	STATE_FROM_ZSTATE;

	switch (Z_TYPE_P(zvalue))
	{
#if PHP_MAJOR_VERSION >= 7
	case IS_TRUE:
		vbool = 1;
		/* no break, fallthrough */
	case IS_FALSE:
#else
	case IS_BOOL:
		vbool = Z_BVAL_P(zvalue);
#endif
		switch (option)
		{
		case SMBCLIENT_OPT_CASE_SENSITIVE:
			smbc_setOptionCaseSensitive(state->ctx, vbool);
			RETURN_TRUE;

		case SMBCLIENT_OPT_URLENCODE_READDIR_ENTRIES:
			smbc_setOptionUrlEncodeReaddirEntries(state->ctx, vbool);
			RETURN_TRUE;

		case SMBCLIENT_OPT_USE_KERBEROS:
			smbc_setOptionUseKerberos(state->ctx, vbool);
			RETURN_TRUE;

		case SMBCLIENT_OPT_FALLBACK_AFTER_KERBEROS:
			smbc_setOptionFallbackAfterKerberos(state->ctx, vbool);
			RETURN_TRUE;

		/* Reverse the sense of this option: */
		case SMBCLIENT_OPT_AUTO_ANONYMOUS_LOGIN:
			smbc_setOptionNoAutoAnonymousLogin(state->ctx, !(vbool));
			RETURN_TRUE;

		case SMBCLIENT_OPT_USE_CCACHE:
			smbc_setOptionUseCCache(state->ctx, vbool);
			RETURN_TRUE;

		#ifdef HAVE_SMBC_SETOPTIONUSENTHASH
		case SMBCLIENT_OPT_USE_NT_HASH:
			smbc_setOptionUseNTHash(state->ctx, vbool);
			RETURN_TRUE;
		#endif
		}
		break;

	case IS_LONG:

		switch (option)
		{
		case SMBCLIENT_OPT_OPEN_SHAREMODE:
			smbc_setOptionOpenShareMode(state->ctx, Z_LVAL_P(zvalue));
			RETURN_TRUE;

		case SMBCLIENT_OPT_ENCRYPT_LEVEL:
			smbc_setOptionSmbEncryptionLevel(state->ctx, Z_LVAL_P(zvalue));
			RETURN_TRUE;

		case SMBCLIENT_OPT_BROWSE_MAX_LMB_COUNT:
			smbc_setOptionBrowseMaxLmbCount(state->ctx, Z_LVAL_P(zvalue));
			RETURN_TRUE;

		#ifdef HAVE_SMBC_SETPORT
		case SMBCLIENT_OPT_PORT:
			smbc_setPort(state->ctx, Z_LVAL_P(zvalue));
			RETURN_TRUE;
		#endif

		case SMBCLIENT_OPT_TIMEOUT:
			smbc_setTimeout(state->ctx, Z_LVAL_P(zvalue));
			RETURN_TRUE;
		}
		break;

	case IS_STRING:

		switch (option)
		{
		case SMBCLIENT_OPT_NETBIOS_NAME:
			smbc_setNetbiosName(state->ctx, Z_STRVAL_P(zvalue));
			RETURN_TRUE;

		/* For the next two options, update our state object as well: */
		case SMBCLIENT_OPT_WORKGROUP:
			if (ctx_init_getauth(zvalue, &state->wrkg, &state->wrkglen, "workgroup") == 0) {
				RETURN_FALSE;
			}
			smbc_setWorkgroup(state->ctx, Z_STRVAL_P(zvalue));
			RETURN_TRUE;

		case SMBCLIENT_OPT_USER:
			if (ctx_init_getauth(zvalue, &state->user, &state->userlen, "username") == 0) {
				RETURN_FALSE;
			}
			smbc_setUser(state->ctx, Z_STRVAL_P(zvalue));
			RETURN_TRUE;
		}
		break;
	}
	RETURN_FALSE;
}

#if HAVE_SMBC_SETOPTIONPROTOCOLS
PHP_FUNCTION(smbclient_client_protocols)
{
	zval *zstate;
	char *minproto = NULL, *maxproto = NULL;
	strsize_t minproto_len, maxproto_len;
	php_smbclient_state *state;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r|s!s!", &zstate, &minproto, &minproto_len, &maxproto, &maxproto_len) == FAILURE) {
		return;
	}
	STATE_FROM_ZSTATE;

	RETURN_BOOL(smbc_setOptionProtocols(state->ctx, minproto, maxproto));
}
#endif

PHP_FUNCTION(smbclient_statvfs)
{
	char *url;
	strsize_t url_len;
	zval *zstate;
	struct statvfs st;
	smbc_statvfs_fn smbc_statvfs;
	php_smbclient_state *state;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rs", &zstate, &url, &url_len) == FAILURE) {
		return;
	}
	STATE_FROM_ZSTATE;

	if ((smbc_statvfs = smbc_getFunctionStatVFS(state->ctx)) == NULL) {
		RETURN_FALSE;
	}
	if (smbc_statvfs(state->ctx, url, &st) != 0) {
		hide_password(url, url_len);
		switch (state->err = errno) {
			case EBADF: php_error(E_WARNING, "Couldn't statvfs %s: bad file descriptor", url); break;
			case EACCES: php_error(E_WARNING, "Couldn't statvfs %s: permission denied", url); break;
			case EINVAL: php_error(E_WARNING, "Couldn't statvfs %s: library not initalized or otherwise invalid", url); break;
			case ENOMEM: php_error(E_WARNING, "Couldn't statvfs %s: out of memory", url); break;
			default: php_error(E_WARNING, "Couldn't statvfs %s: unknown error (%d)", url, errno); break;
		}
		RETURN_FALSE;
	}
	array_init(return_value);
	add_assoc_long(return_value, "bsize",   st.f_bsize);
	add_assoc_long(return_value, "frsize",  st.f_frsize);
	add_assoc_long(return_value, "blocks",  st.f_blocks);
	add_assoc_long(return_value, "bfree",   st.f_bfree);
	add_assoc_long(return_value, "bavail",  st.f_bavail);
	add_assoc_long(return_value, "files",   st.f_files);
	add_assoc_long(return_value, "ffree",   st.f_ffree);
	add_assoc_long(return_value, "favail",  st.f_favail);
	add_assoc_long(return_value, "fsid",    st.f_fsid);
	add_assoc_long(return_value, "flag",    st.f_flag);
	add_assoc_long(return_value, "namemax", st.f_namemax);
}

PHP_FUNCTION(smbclient_fstatvfs)
{
	zval *zstate;
	zval *zfile;
	SMBCFILE *file;
	struct statvfs st;
	smbc_fstatvfs_fn smbc_fstatvfs;
	php_smbclient_state *state;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rr", &zstate, &zfile) == FAILURE) {
		return;
	}
	STATE_FROM_ZSTATE;
	FILE_FROM_ZFILE;

	if ((smbc_fstatvfs = smbc_getFunctionFstatVFS(state->ctx)) == NULL) {
		RETURN_FALSE;
	}
	if (smbc_fstatvfs(state->ctx, file, &st) != 0) {
		switch (state->err = errno) {
			case EBADF: php_error(E_WARNING, "Couldn't fstatvfs: bad file descriptor"); break;
			case EACCES: php_error(E_WARNING, "Couldn't fstatvfs: permission denied"); break;
			case EINVAL: php_error(E_WARNING, "Couldn't fstatvfs: library not initalized or otherwise invalid"); break;
			case ENOMEM: php_error(E_WARNING, "Couldn't fstatvfs: out of memory"); break;
			default: php_error(E_WARNING, "Couldn't fstatvfs: unknown error (%d)", errno); break;
		}
		RETURN_FALSE;
	}
	array_init(return_value);
	add_assoc_long(return_value, "bsize",   st.f_bsize);
	add_assoc_long(return_value, "frsize",  st.f_frsize);
	add_assoc_long(return_value, "blocks",  st.f_blocks);
	add_assoc_long(return_value, "bfree",   st.f_bfree);
	add_assoc_long(return_value, "bavail",  st.f_bavail);
	add_assoc_long(return_value, "files",   st.f_files);
	add_assoc_long(return_value, "ffree",   st.f_ffree);
	add_assoc_long(return_value, "favail",  st.f_favail);
	add_assoc_long(return_value, "fsid",    st.f_fsid);
	add_assoc_long(return_value, "flag",    st.f_flag);
	add_assoc_long(return_value, "namemax", st.f_namemax);
}
