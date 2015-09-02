/* ------------------------------------------------------------------
 * This file is part of libsmbclient-php: Samba bindings for PHP.
 * Libsmbclient-php is licensed under the BSD 2-clause license:
 * ------------------------------------------------------------------
 *
 * Copyright (c) 2003,        Matthew Sachs
 *               2009 - 2014, Eduardo Bacchi Kienetz
 *               2013 - 2015, Alfred Klomp
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
#include "php_libsmbclient.h"

#include <libsmbclient.h>

#define LIBSMBCLIENT_VERSION	"0.6.1"

/* If Zend Thread Safety (ZTS) is defined, each thread gets its own copy of
 * the php_libsmbclient_globals structure. Else we use a single global copy: */
#ifdef ZTS
static int libsmbclient_globals_id;
#else
static php_libsmbclient_globals libsmbclient_globals;
#endif

static void php_libsmbclient_init_globals(php_libsmbclient_globals *libsmbclient_globals_p TSRMLS_DC)
{
	/* This function initializes the thread-local storage.
	 * We currently don't use this. */
}

typedef struct _php_libsmbclient_state
{
	SMBCCTX *ctx;
	char *wrkg;
	char *user;
	char *pass;
	int wrkglen;
	int userlen;
	int passlen;
	int err;
}
php_libsmbclient_state;

#define PHP_LIBSMBCLIENT_STATE_NAME "smbclient state"
#define PHP_LIBSMBCLIENT_FILE_NAME "smbclient file"

static int le_libsmbclient_state;
static int le_libsmbclient_file;

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
php_libsmbclient_options;

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

static zend_function_entry libsmbclient_functions[] =
{
	PHP_FE(smbclient_version, NULL)
	PHP_FE(smbclient_library_version, NULL)
	PHP_FE(smbclient_state_new, NULL)
	PHP_FE(smbclient_state_init, NULL)
	PHP_FE(smbclient_state_errno, NULL)
	PHP_FE(smbclient_state_free, NULL)
	PHP_FE(smbclient_option_get, NULL)
	PHP_FE(smbclient_option_set, NULL)
	PHP_FE(smbclient_opendir, NULL)
	PHP_FE(smbclient_readdir, NULL)
	PHP_FE(smbclient_closedir, NULL)
	PHP_FE(smbclient_stat, NULL)
	PHP_FE(smbclient_fstat, NULL)
	PHP_FE(smbclient_open, NULL)
	PHP_FE(smbclient_creat, NULL)
	PHP_FE(smbclient_read, NULL)
	PHP_FE(smbclient_close, NULL)
	PHP_FE(smbclient_mkdir, NULL)
	PHP_FE(smbclient_rmdir, NULL)
	PHP_FE(smbclient_rename, NULL)
	PHP_FE(smbclient_write, NULL)
	PHP_FE(smbclient_unlink, NULL)
	PHP_FE(smbclient_lseek, NULL)
	PHP_FE(smbclient_ftruncate, NULL)
	PHP_FE(smbclient_chmod, NULL)
	PHP_FE(smbclient_utimes, NULL)
	PHP_FE(smbclient_listxattr, NULL)
	PHP_FE(smbclient_getxattr, NULL)
	PHP_FE(smbclient_setxattr, NULL)
	PHP_FE(smbclient_removexattr, NULL)
	PHP_FE(smbclient_statvfs, NULL)
	PHP_FE(smbclient_fstatvfs, NULL)
	{NULL, NULL, NULL}
};

zend_module_entry libsmbclient_module_entry =
	{ STANDARD_MODULE_HEADER
	, "libsmbclient"		/* name                  */
	, libsmbclient_functions	/* functions             */
	, PHP_MINIT(smbclient)		/* module_startup_func   */
	, PHP_MSHUTDOWN(smbclient)	/* module_shutdown_func  */
	, PHP_RINIT(smbclient)		/* request_startup_func  */
	, NULL				/* request_shutdown_func */
	, PHP_MINFO(smbclient)		/* info_func             */
	, LIBSMBCLIENT_VERSION		/* version               */
	, STANDARD_MODULE_PROPERTIES
	} ;

#ifdef COMPILE_DL_LIBSMBCLIENT
ZEND_GET_MODULE(libsmbclient)
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

	php_libsmbclient_state *state;

	if (ctx == NULL || (state = smbc_getOptionUserData(ctx)) == NULL) {
		return;
	}
	auth_copy(wrkg, state->wrkg, (size_t)state->wrkglen, (size_t)wrkglen);
	auth_copy(user, state->user, (size_t)state->userlen, (size_t)userlen);
	auth_copy(pass, state->pass, (size_t)state->passlen, (size_t)passlen);
}

static void
smbclient_state_dtor (zend_rsrc_list_entry *rsrc TSRMLS_DC)
{
	php_libsmbclient_state *state = (php_libsmbclient_state *)rsrc->ptr;

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

static void
smbclient_file_dtor (zend_rsrc_list_entry *rsrc TSRMLS_DC)
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

PHP_MINIT_FUNCTION(smbclient)
{
	/* If ZTS is defined, allocate and init a copy of the global
	 * datastructure for each thread: */
	#ifdef ZTS
	ts_allocate_id(&libsmbclient_globals_id, sizeof(php_libsmbclient_globals), (ts_allocate_ctor) php_libsmbclient_init_globals, NULL);
	#else
	php_libsmbclient_init_globals(&libsmbclient_globals);
	#endif

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

	le_libsmbclient_state = zend_register_list_destructors_ex(smbclient_state_dtor, NULL, PHP_LIBSMBCLIENT_STATE_NAME, module_number);
	le_libsmbclient_file = zend_register_list_destructors_ex(smbclient_file_dtor, NULL, PHP_LIBSMBCLIENT_FILE_NAME, module_number);

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

PHP_MINFO_FUNCTION(smbclient)
{
	php_info_print_table_start();
	php_info_print_table_row(2, "libsmbclient Support", "enabled");
	php_info_print_table_row(2, "libsmbclient Version", LIBSMBCLIENT_VERSION);
	php_info_print_table_end();
}

PHP_FUNCTION(smbclient_state_new)
{
	SMBCCTX *ctx;
	php_libsmbclient_state *state;

	if ((ctx = smbc_new_context()) == NULL) {
		switch (errno) {
			case ENOMEM: php_error(E_WARNING, "Couldn't create smbclient state: insufficient memory"); break;
			default: php_error(E_WARNING, "Couldn't create smbclient state: unknown error (%d)", errno); break;
		};
		RETURN_FALSE;
	}
	state = emalloc(sizeof(php_libsmbclient_state));
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

	ZEND_REGISTER_RESOURCE(return_value, state, le_libsmbclient_state);
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

		case IS_BOOL:
			if (Z_LVAL_P(z) == 1) {
				php_error(E_WARNING, "invalid value for %s", varname);
				return 0;
			}
			return 1;

		case IS_STRING:
			*destlen = Z_STRLEN_P(z);
			*dest = estrndup(Z_STRVAL_P(z), *destlen);
			return 1;

		default:
			php_error(E_WARNING, "invalid datatype for %s", varname);
			return 0;
	}
}

PHP_FUNCTION(smbclient_version)
{
	RETURN_STRING(LIBSMBCLIENT_VERSION, 1);
}

PHP_FUNCTION(smbclient_library_version)
{
	RETURN_STRING(smbc_version(), 1);
}

PHP_FUNCTION(smbclient_state_init)
{
	SMBCCTX *ctx;
	zval *zstate;
	zval *zwrkg = NULL;
	zval *zuser = NULL;
	zval *zpass = NULL;
	php_libsmbclient_state *state;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r|zzz", &zstate, &zwrkg, &zuser, &zpass) != SUCCESS) {
		RETURN_FALSE;
	}
	ZEND_FETCH_RESOURCE(state, php_libsmbclient_state *, &zstate, -1, PHP_LIBSMBCLIENT_STATE_NAME, le_libsmbclient_state);

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
	if ((ctx = smbc_init_context(state->ctx)) != NULL) {
		state->ctx = ctx;
		RETURN_TRUE;
	}
	switch (state->err = errno) {
		case EBADF: php_error(E_WARNING, "Couldn't init SMB context: null context given"); break;
		case ENOMEM: php_error(E_WARNING, "Couldn't init SMB context: insufficient memory"); break;
		case ENOENT: php_error(E_WARNING, "Couldn't init SMB context: cannot load smb.conf"); break;
		default: php_error(E_WARNING, "Couldn't init SMB context: unknown error (%d)", errno); break;
	}
	RETURN_FALSE;
}

PHP_FUNCTION(smbclient_state_errno)
{
	zval *zstate;
	php_libsmbclient_state *state;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r", &zstate) != SUCCESS) {
		RETURN_LONG(0);
	}
	ZEND_FETCH_RESOURCE(state, php_libsmbclient_state *, &zstate, -1, PHP_LIBSMBCLIENT_STATE_NAME, le_libsmbclient_state);
	RETURN_LONG(state->err);
}

PHP_FUNCTION(smbclient_state_free)
{
	zval *zstate;
	php_libsmbclient_state *state;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r", &zstate) != SUCCESS) {
		RETURN_FALSE;
	}
	ZEND_FETCH_RESOURCE(state, php_libsmbclient_state *, &zstate, -1, PHP_LIBSMBCLIENT_STATE_NAME, le_libsmbclient_state);

	if (state->ctx == NULL) {
		zend_list_delete(Z_LVAL_P(zstate));
		RETURN_TRUE;
	}
	if (smbc_free_context(state->ctx, 1) == 0) {
		state->ctx = NULL;
		zend_list_delete(Z_LVAL_P(zstate));
		RETURN_TRUE;
	}
	switch (state->err = errno) {
		case EBUSY: php_error(E_WARNING, "Couldn't destroy smbclient state: connection in use"); break;
		case EBADF: php_error(E_WARNING, "Couldn't destroy smbclient state: invalid handle"); break;
		default: php_error(E_WARNING, "Couldn't destroy smbclient state: unknown error (%d)", errno); break;
	}
	RETURN_FALSE;
}

#define STATE_FROM_ZSTATE \
	ZEND_FETCH_RESOURCE(state, php_libsmbclient_state *, &zstate, -1, PHP_LIBSMBCLIENT_STATE_NAME, le_libsmbclient_state); \
	if (state == NULL || state->ctx == NULL) { \
		php_error(E_WARNING, PHP_LIBSMBCLIENT_STATE_NAME " not found"); \
		RETURN_FALSE; \
	}

#define FILE_FROM_ZFILE \
	ZEND_FETCH_RESOURCE(file, SMBCFILE *, &zfile, -1, PHP_LIBSMBCLIENT_FILE_NAME, le_libsmbclient_file); \
	if (file == NULL) { \
		php_error(E_WARNING, PHP_LIBSMBCLIENT_FILE_NAME " not found"); \
		RETURN_FALSE; \
	}

PHP_FUNCTION(smbclient_opendir)
{
	char *path;
	int path_len;
	zval *zstate;
	SMBCFILE *dir;
	smbc_opendir_fn smbc_opendir;
	php_libsmbclient_state *state;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rs", &zstate, &path, &path_len) == FAILURE) {
		return;
	}
	STATE_FROM_ZSTATE;

	if ((smbc_opendir = smbc_getFunctionOpendir(state->ctx)) == NULL) {
		RETURN_FALSE;
	}
	if ((dir = smbc_opendir(state->ctx, path)) != NULL) {
		ZEND_REGISTER_RESOURCE(return_value, dir, le_libsmbclient_file);
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

PHP_FUNCTION(smbclient_readdir)
{
	struct smbc_dirent *dirent;
	char *type;
	zval *zstate;
	zval *zfile;
	SMBCFILE *file;
	smbc_readdir_fn smbc_readdir;
	php_libsmbclient_state *state;

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
			case EBADF: php_error(E_WARNING, "Couldn't read " PHP_LIBSMBCLIENT_FILE_NAME ": Not a directory resource"); break;
			case EINVAL: php_error(E_WARNING, "Couldn't read " PHP_LIBSMBCLIENT_FILE_NAME ": State resource not initialized"); break;
			default: php_error(E_WARNING, "Couldn't read " PHP_LIBSMBCLIENT_FILE_NAME ": unknown error (%d)", errno); break;
		}
		RETURN_FALSE;
	}
	if (array_init(return_value) != SUCCESS) {
		php_error(E_WARNING, "Couldn't initialize array");
		RETURN_FALSE;
	}
	switch (dirent->smbc_type) {
		case SMBC_WORKGROUP: type = "workgroup"; break;
		case SMBC_SERVER: type = "server"; break;
		case SMBC_FILE_SHARE: type = "file share"; break;
		case SMBC_PRINTER_SHARE: type = "printer share"; break;
		case SMBC_COMMS_SHARE: type = "communication share"; break;
		case SMBC_IPC_SHARE: type = "IPC share"; break;
		case SMBC_DIR: type = "directory"; break;
		case SMBC_FILE: type = "file"; break;
		case SMBC_LINK: type = "link"; break;
		default: type = "unknown"; break;
	}
	add_assoc_string(return_value, "type", type, 1);
	add_assoc_stringl(return_value, "comment", dirent->comment, dirent->commentlen, 1);
	add_assoc_stringl(return_value, "name", dirent->name, dirent->namelen, 1);
}

PHP_FUNCTION(smbclient_closedir)
{
	zval *zstate;
	zval *zfile;
	SMBCFILE *file;
	smbc_closedir_fn smbc_closedir;
	php_libsmbclient_state *state;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rr", &zstate, &zfile) == FAILURE) {
		return;
	}
	STATE_FROM_ZSTATE;
	FILE_FROM_ZFILE;

	if ((smbc_closedir = smbc_getFunctionClosedir(state->ctx)) == NULL) {
		RETURN_FALSE;
	}
	if (smbc_closedir(state->ctx, file) == 0) {
		zend_list_delete(Z_LVAL_P(zfile));
		RETURN_TRUE;
	}
	switch (state->err = errno) {
		case EBADF: php_error(E_WARNING, "Couldn't close " PHP_LIBSMBCLIENT_FILE_NAME ": Not a directory resource"); break;
		default: php_error(E_WARNING, "Couldn't close " PHP_LIBSMBCLIENT_FILE_NAME ": unknown error (%d)", errno); break;
	}
	RETURN_FALSE;
}

PHP_FUNCTION(smbclient_rename)
{
	char *ourl, *nurl;
	int ourl_len, nurl_len;
	zval *zstate_old;
	zval *zstate_new;
	smbc_rename_fn smbc_rename;
	php_libsmbclient_state *state_old;
	php_libsmbclient_state *state_new;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rsrs", &zstate_old, &ourl, &ourl_len, &zstate_new, &nurl, &nurl_len) == FAILURE) {
		return;
	}
	ZEND_FETCH_RESOURCE(state_old, php_libsmbclient_state *, &zstate_old, -1, PHP_LIBSMBCLIENT_STATE_NAME, le_libsmbclient_state);
	ZEND_FETCH_RESOURCE(state_new, php_libsmbclient_state *, &zstate_new, -1, PHP_LIBSMBCLIENT_STATE_NAME, le_libsmbclient_state);

	if (state_old == NULL || state_old->ctx == NULL) {
		php_error(E_WARNING, "old " PHP_LIBSMBCLIENT_STATE_NAME " is null");
		RETURN_FALSE;
	}
	if (state_new == NULL || state_new->ctx == NULL) {
		php_error(E_WARNING, "new " PHP_LIBSMBCLIENT_STATE_NAME " is null");
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
	int url_len;
	zval *zstate;
	smbc_unlink_fn smbc_unlink;
	php_libsmbclient_state *state;

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
	int path_len;
	long mode = 0777;	/* Same as PHP's native mkdir() */
	zval *zstate;
	smbc_mkdir_fn smbc_mkdir;
	php_libsmbclient_state *state;

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
	int url_len;
	zval *zstate;
	smbc_rmdir_fn smbc_rmdir;
	php_libsmbclient_state *state;

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
	int file_len;
	zval *zstate;
	smbc_stat_fn smbc_stat;
	php_libsmbclient_state *state;

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
	if (array_init(return_value) != SUCCESS) {
		php_error(E_WARNING, "Couldn't initialize array");
		RETURN_FALSE;
	}
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
	php_libsmbclient_state *state;

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
			case ENOENT: php_error(E_WARNING, "Couldn't fstat " PHP_LIBSMBCLIENT_FILE_NAME ": Does not exist"); break;
			case EINVAL: php_error(E_WARNING, "Couldn't fstat: null resource or smbc_init failed"); break;
			case EACCES: php_error(E_WARNING, "Couldn't fstat " PHP_LIBSMBCLIENT_FILE_NAME ": Permission denied"); break;
			case ENOMEM: php_error(E_WARNING, "Couldn't fstat " PHP_LIBSMBCLIENT_FILE_NAME ": Out of memory"); break;
			case ENOTDIR: php_error(E_WARNING, "Couldn't fstat " PHP_LIBSMBCLIENT_FILE_NAME ": Not a directory"); break;
			default: php_error(E_WARNING, "Couldn't fstat " PHP_LIBSMBCLIENT_FILE_NAME ": unknown error (%d)", errno); break;
		}
		RETURN_FALSE;
	}
	if (array_init(return_value) != SUCCESS) {
		php_error(E_WARNING, "Couldn't initialize array");
		RETURN_FALSE;
	}
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

static int
flagstring_to_smbflags (char *flags, int flags_len, int *retval)
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

err:	php_error(E_WARNING, "Invalid flag string");
	return 0;
}

PHP_FUNCTION(smbclient_open)
{
	char *file, *flags;
	int file_len, flags_len;
	int smbflags;
	long mode = 0666;
	zval *zstate;
	SMBCFILE *handle;
	smbc_open_fn smbc_open;
	php_libsmbclient_state *state;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rss|l", &zstate, &file, &file_len, &flags, &flags_len, &mode) == FAILURE) {
		return;
	}
	STATE_FROM_ZSTATE;

	/* The flagstring is in the same format as the native fopen() uses, so
	 * one of the characters r, w, a, x, c, optionally followed by a plus.
	 * Need to translate this to an integer value for smbc_open: */
	if (flagstring_to_smbflags(flags, flags_len, &smbflags) == 0) {
		RETURN_FALSE;
	}
	if ((smbc_open = smbc_getFunctionOpen(state->ctx)) == NULL) {
		RETURN_FALSE;
	}
	if ((handle = smbc_open(state->ctx, file, smbflags, mode)) != NULL) {
		ZEND_REGISTER_RESOURCE(return_value, handle, le_libsmbclient_file);
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
	int file_len;
	long mode = 0666;
	zval *zstate;
	SMBCFILE *handle;
	smbc_creat_fn smbc_creat;
	php_libsmbclient_state *state;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rs|l", &zstate, &file, &file_len, &mode) == FAILURE) {
		return;
	}
	STATE_FROM_ZSTATE;

	if ((smbc_creat = smbc_getFunctionCreat(state->ctx)) == NULL) {
		RETURN_FALSE;
	}
	if ((handle = smbc_creat(state->ctx, file, (mode_t)mode)) != NULL) {
		ZEND_REGISTER_RESOURCE(return_value, handle, le_libsmbclient_file);
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
	long count;
	ssize_t nbytes;
	zval *zstate;
	zval *zfile;
	SMBCFILE *file;
	smbc_read_fn smbc_read;
	php_libsmbclient_state *state;

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
	void *buf = emalloc(count);

	if ((nbytes = smbc_read(state->ctx, file, buf, count)) >= 0) {
		RETURN_STRINGL(buf, nbytes, 0);
	}
	efree(buf);
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
	long count = 0;
	int str_len;
	char * str;
	size_t nwrite;
	ssize_t nbytes;
	zval *zstate;
	zval *zfile;
	SMBCFILE *file;
	smbc_write_fn smbc_write;
	php_libsmbclient_state *state;

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
	long offset, whence, ret;
	zval *zstate;
	zval *zfile;
	SMBCFILE *file;
	smbc_lseek_fn smbc_lseek;
	php_libsmbclient_state *state;

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
	long offset;
	zval *zstate;
	zval *zfile;
	SMBCFILE *file;
	smbc_ftruncate_fn smbc_ftruncate;
	php_libsmbclient_state *state;

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
	php_libsmbclient_state *state;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rr", &zstate, &zfile) == FAILURE) {
		return;
	}
	STATE_FROM_ZSTATE;
	FILE_FROM_ZFILE;

	if ((smbc_close = smbc_getFunctionClose(state->ctx)) == NULL) {
		RETURN_FALSE;
	}
	if (smbc_close(state->ctx, file) == 0) {
		zend_list_delete(Z_LVAL_P(zfile));
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
	int file_len;
	long mode;
	zval *zstate;
	smbc_chmod_fn smbc_chmod;
	php_libsmbclient_state *state;

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
	int file_len;
	long mtime = -1, atime = -1;
	zval *zstate;
	struct timeval times[2];
	smbc_utimes_fn smbc_utimes;
	php_libsmbclient_state *state;

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
	int url_len;
	char values[1000];
	zval *zstate;
	smbc_listxattr_fn smbc_listxattr;
	php_libsmbclient_state *state;

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
		if (array_init(return_value) != SUCCESS) {
			php_error(E_WARNING, "Couldn't initialize array");
			RETURN_FALSE;
		}
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
			add_next_index_stringl(return_value, s, c - s, 1);
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

PHP_FUNCTION(smbclient_getxattr)
{
	char *url, *name;
	int url_len, name_len, retsize;
	char values[1000];
	zval *zstate;
	smbc_getxattr_fn smbc_getxattr;
	php_libsmbclient_state *state;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rss", &zstate, &url, &url_len, &name, &name_len) == FAILURE) {
		return;
	}
	STATE_FROM_ZSTATE;

	if ((smbc_getxattr = smbc_getFunctionGetxattr(state->ctx)) == NULL) {
		RETURN_FALSE;
	}
	/* TODO: 1000 chars should be enough for everyone...?
	 * However, doing an initial blank call to determine the response size
	 * seems wasteful, and vulnerable to a time-of-check, time-of-use
	 * error. */
	if ((retsize = smbc_getxattr(state->ctx, url, name, values, sizeof(values))) >= 0) {
		if (retsize > sizeof(values)) {
			retsize = sizeof(values);
		}
		RETURN_STRINGL(values, retsize, 1);
	}
	hide_password(url, url_len);
	switch (state->err = errno) {
		case EINVAL: php_error(E_WARNING, "Couldn't get xattr for %s: library not initialized or incorrect parameter", url); break;
		case ENOMEM: php_error(E_WARNING, "Couldn't get xattr for %s: out of memory", url); break;
		case EPERM: php_error(E_WARNING, "Couldn't get xattr for %s: permission denied", url); break;
		case ENOTSUP: php_error(E_WARNING, "Couldn't get xattr for %s: file system does not support extended attributes", url); break;
		default: php_error(E_WARNING, "Couldn't get xattr for %s: unknown error (%d)", url, errno); break;
	}
	RETURN_FALSE;
}

PHP_FUNCTION(smbclient_setxattr)
{
	char *url, *name, *val;
	int url_len, name_len, val_len;
	long flags = 0;
	zval *zstate;
	smbc_setxattr_fn smbc_setxattr;
	php_libsmbclient_state *state;

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
	int url_len, name_len;
	zval *zstate;
	smbc_removexattr_fn smbc_removexattr;
	php_libsmbclient_state *state;

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
	long option;
	char *ret;
	zval *zstate;
	php_libsmbclient_state *state;

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
		RETURN_STRING(ret, 1);

	case SMBCLIENT_OPT_WORKGROUP:
		if ((ret = smbc_getWorkgroup(state->ctx)) == NULL) {
			RETURN_EMPTY_STRING();
		}
		if (strlen(ret) == 0) {
			RETURN_EMPTY_STRING();
		}
		RETURN_STRING(ret, 1);

	case SMBCLIENT_OPT_USER:
		if ((ret = smbc_getUser(state->ctx)) == NULL) {
			RETURN_EMPTY_STRING();
		}
		if (strlen(ret) == 0) {
			RETURN_EMPTY_STRING();
		}
		RETURN_STRING(ret, 1);
	}
	RETURN_NULL();
}

PHP_FUNCTION(smbclient_option_set)
{
	long option;
	zval *zstate;
	zval *zvalue;
	php_libsmbclient_state *state;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rlz", &zstate, &option, &zvalue) == FAILURE) {
		return;
	}
	STATE_FROM_ZSTATE;

	switch (Z_TYPE_P(zvalue))
	{
	case IS_BOOL:

		switch (option)
		{
		case SMBCLIENT_OPT_CASE_SENSITIVE:
			smbc_setOptionCaseSensitive(state->ctx, Z_BVAL_P(zvalue));
			RETURN_TRUE;

		case SMBCLIENT_OPT_URLENCODE_READDIR_ENTRIES:
			smbc_setOptionUrlEncodeReaddirEntries(state->ctx, Z_BVAL_P(zvalue));
			RETURN_TRUE;

		case SMBCLIENT_OPT_USE_KERBEROS:
			smbc_setOptionUseKerberos(state->ctx, Z_BVAL_P(zvalue));
			RETURN_TRUE;

		case SMBCLIENT_OPT_FALLBACK_AFTER_KERBEROS:
			smbc_setOptionFallbackAfterKerberos(state->ctx, Z_BVAL_P(zvalue));
			RETURN_TRUE;

		/* Reverse the sense of this option: */
		case SMBCLIENT_OPT_AUTO_ANONYMOUS_LOGIN:
			smbc_setOptionNoAutoAnonymousLogin(state->ctx, !(Z_BVAL_P(zvalue)));
			RETURN_TRUE;

		case SMBCLIENT_OPT_USE_CCACHE:
			smbc_setOptionUseCCache(state->ctx, Z_BVAL_P(zvalue));
			RETURN_TRUE;

		#ifdef HAVE_SMBC_SETOPTIONUSENTHASH
		case SMBCLIENT_OPT_USE_NT_HASH:
			smbc_setOptionUseNTHash(state->ctx, Z_BVAL_P(zvalue));
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

PHP_FUNCTION(smbclient_statvfs)
{
	char *url;
	int url_len;
	zval *zstate;
	struct statvfs st;
	smbc_statvfs_fn smbc_statvfs;
	php_libsmbclient_state *state;

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
	if (array_init(return_value) != SUCCESS) {
		php_error(E_WARNING, "Couldn't initialize array");
		RETURN_FALSE;
	}
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
	php_libsmbclient_state *state;

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
	if (array_init(return_value) != SUCCESS) {
		php_error(E_WARNING, "Couldn't initialize array");
		RETURN_FALSE;
	}
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
