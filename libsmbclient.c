/*
   +----------------------------------------------------------------------+
   | PHP Version 5                                                        |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997-2002 The PHP Group                                |
   +----------------------------------------------------------------------+
   | This source file is subject to version 2.02 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available at through the world-wide-web at                           |
   | http://www.php.net/license/2_02.txt.                                 |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Authors: Matthew Sachs <matthewg@zevils.com>                         |
   |          Eduardo Bacchi Kienetz <eduardo@kienetz.com>                |
   |          Alfred Klomp <git@alfredklomp.com>                          |
   +----------------------------------------------------------------------+
 */

#define IS_EXT_MODULE

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "ext/standard/info.h"
#include "php_libsmbclient.h"

#include <libsmbclient.h>

#define LIBSMBCLIENT_VERSION	"0.3"

#ifdef ZTS
int libsmbclient_globals_id;
#else
#define php_libsmbclient_globals libsmbclient_globals;
#endif

#ifdef ZTS
static void php_libsmbclient_init_globals(php_libsmbclient_globals *libsmbclient_globals_p TSRMLS_DC)
{
}
#endif

typedef struct _php_libsmbclient_state
{
	SMBCCTX *ctx;
	char *wrkg;
	char *user;
	char *pass;
	int wrkglen;
	int userlen;
	int passlen;
}
php_libsmbclient_state;

#define PHP_LIBSMBCLIENT_STATE_NAME "smbclient state"

int le_libsmbclient_state;

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
	PHP_FE(smbclient_state_new, NULL)
	PHP_FE(smbclient_state_init, NULL)
	PHP_FE(smbclient_state_free, NULL)
	PHP_FE(smbclient_opendir, NULL)
	PHP_FE(smbclient_readdir, NULL)
	PHP_FE(smbclient_closedir, NULL)
	PHP_FE(smbclient_stat, NULL)
	PHP_FE(smbclient_open, NULL)
	PHP_FE(smbclient_creat, NULL)
	PHP_FE(smbclient_read, NULL)
	PHP_FE(smbclient_close, NULL)
	PHP_FE(smbclient_mkdir, NULL)
	PHP_FE(smbclient_rmdir, NULL)
	PHP_FE(smbclient_rename, NULL)
	PHP_FE(smbclient_write, NULL)
	PHP_FE(smbclient_unlink, NULL)
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
			default: php_error(E_WARNING, "Couldn't destroy SMB context: unknown error"); break;
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

PHP_MINIT_FUNCTION(smbclient)
{
	#ifdef ZTS
	ts_allocate_id(&libsmbclient_globals_id, sizeof(php_libsmbclient_globals), (ts_allocate_ctor) php_libsmbclient_init_globals, NULL);
	#endif

	le_libsmbclient_state = zend_register_list_destructors_ex(smbclient_state_dtor, NULL, PHP_LIBSMBCLIENT_STATE_NAME, module_number);

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
			default: php_error(E_WARNING, "Couldn't create smbclient state: unknown error %d", errno); break;
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

	smbc_setFunctionAuthDataWithContext(state->ctx, smbclient_auth_func);

	/* Must also save a pointer to the state object inside the context, to
	 * find the state from the context in the auth function: */
	smbc_setOptionUserData(ctx, (void *)state);

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
	switch (errno) {
		case EBADF: php_error(E_WARNING, "Couldn't init SMB context: null context given"); break;
		case ENOMEM: php_error(E_WARNING, "Couldn't init SMB context: insufficient memory"); break;
		case ENOENT: php_error(E_WARNING, "Couldn't init SMB context: cannot load smb.conf"); break;
		default: php_error(E_WARNING, "Couldn't init SMB context: unknown error %d", errno); break;
	}
	RETURN_FALSE;
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
	switch (errno) {
		case EBUSY: php_error(E_WARNING, "Couldn't destroy smbclient state: connection in use"); break;
		case EBADF: php_error(E_WARNING, "Couldn't destroy smbclient state: invalid handle"); break;
		default: php_error(E_WARNING, "Couldn't destroy smbclient state: unknown error"); break;
	}
	RETURN_FALSE;
}

#define STATE_FROM_ZSTATE \
	ZEND_FETCH_RESOURCE(state, php_libsmbclient_state *, &zstate, -1, PHP_LIBSMBCLIENT_STATE_NAME, le_libsmbclient_state); \
	if (state == NULL || state->ctx == NULL) { \
		php_error(E_WARNING, PHP_LIBSMBCLIENT_STATE_NAME " not found"); \
		RETURN_FALSE; \
	}

PHP_FUNCTION(smbclient_opendir)
{
	char *path;
	int path_len, dirhandle;
	zval *zstate;
	smbc_opendir_fn smbc_opendir;
	php_libsmbclient_state *state;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rs", &zstate, &path, &path_len) == FAILURE) {
		return;
	}
	STATE_FROM_ZSTATE;

	if ((smbc_opendir = smbc_getFunctionOpendir(state->ctx)) == NULL) {
		RETURN_FALSE;
	}
	if ((dirhandle = smbc_opendir(state->ctx, path)) >= 0) {
		RETURN_LONG(dirhandle);
	}
	hide_password(path, path_len);
	switch (errno) {
		case EACCES: php_error(E_WARNING, "Couldn't open SMB directory %s: Permission denied", path); break;
		case EINVAL: php_error(E_WARNING, "Couldn't open SMB directory %s: Invalid URL", path); break;
		case ENOENT: php_error(E_WARNING, "Couldn't open SMB directory %s: Path does not exist", path); break;
		case ENOMEM: php_error(E_WARNING, "Couldn't open SMB directory %s: Insufficient memory", path); break;
		case ENOTDIR: php_error(E_WARNING, "Couldn't open SMB directory %s: Not a directory", path); break;
		case EPERM: php_error(E_WARNING, "Couldn't open SMB directory %s: Workgroup not found", path); break;
		case ENODEV: php_error(E_WARNING, "Couldn't open SMB directory %s: Workgroup or server not found", path); break;
		default: php_error(E_WARNING, "Couldn't open SMB directory %s: Unknown error (%d)", path, errno); break;
	}
	RETURN_FALSE;
}

PHP_FUNCTION(smbclient_readdir)
{
	long dirhandle;
	struct smbc_dirent *dirent;
	char *type;
	zval *zstate;
	smbc_readdir_fn smbc_readdir;
	php_libsmbclient_state *state;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rl", &zstate, &dirhandle) == FAILURE) {
		return;
	}
	STATE_FROM_ZSTATE;

	if ((smbc_readdir = smbc_getFunctionReaddir(state->ctx)) == NULL) {
		RETURN_FALSE;
	}
	errno = 0;
	dirent = smbc_readdir(state->ctx, dirhandle);
	if (dirent == NULL) {
		switch (errno) {
			case 0: RETURN_FALSE;
			case EBADF: php_error(E_WARNING, "Couldn't read SMB directory handle %ld: Not a directory handle", dirhandle); break;
			case EINVAL: php_error(E_WARNING, "Couldn't read SMB directory handle %ld: smbc_init not called", dirhandle); break;
			default: php_error(E_WARNING, "Couldn't read SMB directory handle %ld: Unknown error (%d)", dirhandle, errno); break;
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
	long dirhandle;
	zval *zstate;
	smbc_closedir_fn smbc_closedir;
	php_libsmbclient_state *state;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rl", &zstate, &dirhandle) == FAILURE) {
		return;
	}
	STATE_FROM_ZSTATE;

	if ((smbc_closedir = smbc_getFunctionClosedir(state->ctx)) == NULL) {
		RETURN_FALSE;
	}
	if (smbc_closedir(state->ctx, dirhandle) == 0) {
		RETURN_TRUE;
	}
	switch (errno) {
		case EBADF: php_error(E_WARNING, "Couldn't close SMB directory handle %ld: Not a directory handle", dirhandle); break;
		default: php_error(E_WARNING, "Couldn't close SMB directory handle %ld: Unknown error (%d)", dirhandle, errno); break;
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
	switch (errno) {
		case EISDIR: php_error(E_WARNING, "Couldn't rename SMB directory %s: existing url is not a directory", ourl); break;
		case EACCES: php_error(E_WARNING, "Couldn't open SMB directory %s: Permission denied", ourl); break;
		case EINVAL: php_error(E_WARNING, "Couldn't open SMB directory %s: Invalid URL", ourl); break;
		case ENOENT: php_error(E_WARNING, "Couldn't open SMB directory %s: Path does not exist", ourl); break;
		case ENOMEM: php_error(E_WARNING, "Couldn't open SMB directory %s: Insufficient memory", ourl); break;
		case ENOTDIR: php_error(E_WARNING, "Couldn't open SMB directory %s: Not a directory", ourl); break;
		case EPERM: php_error(E_WARNING, "Couldn't open SMB directory %s: Workgroup not found", ourl); break;
		case EXDEV: php_error(E_WARNING, "Couldn't open SMB directory %s: Workgroup or server not found", ourl); break;
		case EEXIST: php_error(E_WARNING, "Couldn't rename SMB directory %s: new name already exists", ourl); break;
		default: php_error(E_WARNING, "Couldn't open SMB directory %s: Unknown error (%d)", ourl, errno); break;
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
	switch (errno) {
		case EACCES: php_error(E_WARNING, "Couldn't delete %s: Permission denied", url); break;
		case EINVAL: php_error(E_WARNING, "Couldn't delete %s: Invalid URL", url); break;
		case ENOENT: php_error(E_WARNING, "Couldn't delete %s: Path does not exist", url); break;
		case ENOMEM: php_error(E_WARNING, "Couldn't delete %s: Insufficient memory", url); break;
		case EPERM: php_error(E_WARNING, "Couldn't delete %s: Workgroup not found", url); break;
		case EISDIR: php_error(E_WARNING, "Couldn't delete %s: It is a Directory (use rmdir instead)", url); break;
		case EBUSY: php_error(E_WARNING, "Couldn't delete %s: Device or resource busy", url); break;
		default: php_error(E_WARNING, "Couldn't delete %s: Unknown error (%d)", url, errno); break;
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
	switch (errno) {
		case EACCES: php_error(E_WARNING, "Couldn't create SMB directory %s: Permission denied", path); break;
		case EINVAL: php_error(E_WARNING, "Couldn't create SMB directory %s: Invalid URL", path); break;
		case ENOENT: php_error(E_WARNING, "Couldn't create SMB directory %s: Path does not exist", path); break;
		case ENOMEM: php_error(E_WARNING, "Couldn't create SMB directory %s: Insufficient memory", path); break;
		case EEXIST: php_error(E_WARNING, "Couldn't create SMB directory %s: Directory already exists", path); break;
		default: php_error(E_WARNING, "Couldn't create SMB directory %s: Unknown error (%d)", path, errno); break;
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
	switch (errno) {
		case EACCES: php_error(E_WARNING, "Couldn't delete %s: Permission denied", url); break;
		case EINVAL: php_error(E_WARNING, "Couldn't delete %s: Invalid URL", url); break;
		case ENOENT: php_error(E_WARNING, "Couldn't delete %s: Path does not exist", url); break;
		case ENOMEM: php_error(E_WARNING, "Couldn't delete %s: Insufficient memory", url); break;
		case EPERM: php_error(E_WARNING, "Couldn't delete %s: Workgroup not found", url); break;
		case ENOTEMPTY: php_error(E_WARNING, "Couldn't delete %s: It is not empty", url); break;
		default: php_error(E_WARNING, "Couldn't delete %s: Unknown error (%d)", url, errno); break;
	}
	RETURN_FALSE;
}

PHP_FUNCTION(smbclient_stat)
{
	char *file;
	struct stat statbuf;
	int retval, file_len;
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
	retval = smbc_stat(state->ctx, file, &statbuf);
	if (retval < 0) {
		hide_password(file, file_len);
		switch (errno) {
			case ENOENT: php_error(E_WARNING, "Couldn't stat %s: Does not exist", file); break;
			case EINVAL: php_error(E_WARNING, "Couldn't stat: null URL or smbc_init failed"); break;
			case EACCES: php_error(E_WARNING, "Couldn't stat %s: Permission denied", file); break;
			case ENOMEM: php_error(E_WARNING, "Couldn't stat %s: Out of memory", file); break;
			case ENOTDIR: php_error(E_WARNING, "Couldn't stat %s: Not a directory", file); break;
			default: php_error(E_WARNING, "Couldn't stat %s: Unknown error (%d)", file, errno); break;
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
	int handle;
	int smbflags;
	long mode = 0666;
	zval *zstate;
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
	if ((handle = smbc_open(state->ctx, file, smbflags, mode)) >= 0) {
		RETURN_LONG(handle);
	}
	hide_password(file, file_len);
	switch (errno) {
		case ENOMEM: php_error(E_WARNING, "Couldn't open %s: Out of memory", file); break;
		case EINVAL: php_error(E_WARNING, "Couldn't open %s: No file?", file); break;
		case EEXIST: php_error(E_WARNING, "Couldn't open %s: Pathname already exists and O_CREAT and O_EXECL were specified", file); break;
		case EISDIR: php_error(E_WARNING, "Couldn't open %s: Can't write to a directory", file); break;
		case EACCES: php_error(E_WARNING, "Couldn't open %s: Access denied", file); break;
		case ENODEV: php_error(E_WARNING, "Couldn't open %s: Requested share does not exist", file); break;
		case ENOTDIR: php_error(E_WARNING, "Couldn't open %s: Path component isn't a directory", file); break;
		case ENOENT: php_error(E_WARNING, "Couldn't open %s: Directory in path doesn't exist", file); break;
		default: php_error(E_WARNING, "Couldn't open %s: Unknown error (%d)", file, errno); break;
	}
	RETURN_FALSE;
}

PHP_FUNCTION(smbclient_creat)
{
	char *file;
	int file_len;
	int handle;
	long mode = 0666;
	zval *zstate;
	smbc_creat_fn smbc_creat;
	php_libsmbclient_state *state;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rs|l", &zstate, &file, &file_len, &mode) == FAILURE) {
		return;
	}
	STATE_FROM_ZSTATE;

	if ((smbc_creat = smbc_getFunctionCreat(state->ctx)) == NULL) {
		RETURN_FALSE;
	}
	if ((handle = smbc_creat(state->ctx, file, (mode_t)mode)) >= 0) {
		RETURN_LONG(handle);
	}
	hide_password(file, file_len);
	switch (errno) {
		case ENOMEM: php_error(E_WARNING, "Couldn't create %s: Out of memory", file); break;
		case EINVAL: php_error(E_WARNING, "Couldn't create %s: No file?", file); break;
		case EEXIST: php_error(E_WARNING, "Couldn't create %s: Pathname already exists and O_CREAT and O_EXECL were specified", file); break;
		case EISDIR: php_error(E_WARNING, "Couldn't create %s: Can't write to a directory", file); break;
		case EACCES: php_error(E_WARNING, "Couldn't create %s: Access denied", file); break;
		case ENODEV: php_error(E_WARNING, "Couldn't create %s: Requested share does not exist", file); break;
		case ENOENT: php_error(E_WARNING, "Couldn't create %s: Directory in path doesn't exist", file); break;
		default: php_error(E_WARNING, "Couldn't create %s: Unknown error (%d)", file, errno); break;
	}
	RETURN_FALSE;
}

PHP_FUNCTION(smbclient_read)
{
	long file, count;
	ssize_t nbytes;
	zval *zstate;
	smbc_read_fn smbc_read;
	php_libsmbclient_state *state;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rll", &zstate, &file, &count) == FAILURE) {
		return;
	}
	if (count < 0) {
		php_error(E_WARNING, "Negative byte count: %ld", count);
		RETURN_FALSE;
	}
	STATE_FROM_ZSTATE;

	if ((smbc_read = smbc_getFunctionRead(state->ctx)) == NULL) {
		RETURN_FALSE;
	}
	void *buf = emalloc(count);

	if ((nbytes = smbc_read(state->ctx, file, buf, count)) >= 0) {
		RETURN_STRINGL(buf, nbytes, 0);
	}
	efree(buf);
	switch (errno) {
		case EISDIR: php_error(E_WARNING, "Couldn't read from %ld: Is a directory", file); break;
		case EBADF: php_error(E_WARNING, "Couldn't read from %ld: Not a valid file descriptor or not open for reading", file); break;
		case EINVAL: php_error(E_WARNING, "Couldn't read from %ld: Object not suitable for reading or bad buffer", file); break;
		default: php_error(E_WARNING, "Couldn't read from %ld: Unknown error (%d)", file, errno); break;
	}
	RETURN_FALSE;
}

PHP_FUNCTION(smbclient_write)
{
	long file, count = 0;
	int str_len;
	char * str;
	size_t nwrite;
	ssize_t nbytes;
	zval *zstate;
	smbc_write_fn smbc_write;
	php_libsmbclient_state *state;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rls|l", &zstate, &file, &str, &str_len, &count) == FAILURE) {
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

	if ((smbc_write = smbc_getFunctionWrite(state->ctx)) == NULL) {
		RETURN_FALSE;
	}
	if ((nbytes = smbc_write(state->ctx, file, str, nwrite)) >= 0) {
		RETURN_LONG(nbytes);
	}
	switch (errno) {
		case EISDIR: php_error(E_WARNING, "Couldn't write to %ld: Is a directory", file); break;
		case EBADF: php_error(E_WARNING, "Couldn't write to %ld: Not a valid file descriptor or not open for reading", file); break;
		case EINVAL: php_error(E_WARNING, "Couldn't write to %ld: Object not suitable for reading or bad buffer", file); break;
		case EACCES: php_error(E_WARNING, "Couldn't write to %ld: Permission denied", file); break;
		default: php_error(E_WARNING, "Couldn't write to %ld: Unknown error (%d)", file, errno); break;
	}
	RETURN_FALSE;
}

PHP_FUNCTION(smbclient_close)
{
	long file;
	zval *zstate;
	smbc_close_fn smbc_close;
	php_libsmbclient_state *state;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rl", &zstate, &file) == FAILURE) {
		return;
	}
	STATE_FROM_ZSTATE;

	if ((smbc_close = smbc_getFunctionClose(state->ctx)) == NULL) {
		RETURN_FALSE;
	}
	if (smbc_close(state->ctx, file) == 0) {
		RETURN_TRUE;
	}
	switch (errno) {
		case EBADF: php_error(E_WARNING, "Couldn't close %ld: Not a valid file descriptor or not open for reading", file); break;
		case EINVAL: php_error(E_WARNING, "Couldn't close %ld: smbc_init not called", file); break;
		default: php_error(E_WARNING, "Couldn't close %ld: Unknown error (%d)", file, errno); break;
	}
	RETURN_FALSE;
}
