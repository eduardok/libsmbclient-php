/*
   +----------------------------------------------------------------------+
   | PHP Version 4                                                        |
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
   +----------------------------------------------------------------------+
 */
/* $Id$ */
#define IS_EXT_MODULE

#include "php.h"
#include "php_libsmbclient.h"

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

function_entry libsmbclient_functions[] =
{
	PHP_FE(smbclient_opendir, NULL)
	PHP_FE(smbclient_readdir, NULL)
	PHP_FE(smbclient_closedir, NULL)
	{NULL, NULL, NULL}
};

zend_module_entry libsmbclient_module_entry =
{
	STANDARD_MODULE_HEADER,
	"libsmbclient",
	libsmbclient_functions,
	PHP_MINIT(smbclient),
	PHP_MSHUTDOWN(smbclient),
	PHP_RINIT(smbclient),
	NULL,
	PHP_MINFO(smbclient),
	"0.1",
	STANDARD_MODULE_PROPERTIES
};

#if COMPILE_DL_LIBSMBCLIENT
ZEND_GET_MODULE(libsmbclient)
#endif


void smbclient_auth_func(const char *server, const char *share, char *workgroup, int wglen, char *username, int userlen, char *password, int passlen) {
}


PHP_MINIT_FUNCTION(smbclient) {
	int retval;

	#ifdef ZTS
	ts_allocate_id(&libsmbclient_globals_id, sizeof(php_libsmbclient_globals), (ts_allocate_ctor) php_libsmbclient_init_globals, NULL);
	#endif

	retval = smbc_init(smbclient_auth_func, 0);
	if(retval == 0) {
		return SUCCESS;
	} else {
		switch(errno) {
			case ENOMEM: php_error_docref(NULL TSRMLS_CC, E_ERROR, "Couldn't initialize libsmbclient: Out of memory."); break;
			case ENOENT: php_error_docref(NULL TSRMLS_CC, E_ERROR, "Couldn't initialize libsmbclient: The smb.conf file would not load.  It must be located in ~/.smb/ (probably /root/.smb) ."); break;
			case EINVAL: php_error_docref(NULL TSRMLS_CC, E_ERROR, "Couldn't initialize libsmbclient: Invalid parameter."); break;
			default: php_error_docref(NULL TSRMLS_CC, E_ERROR, "Couldn't initialize libsmbclient: Unknown error (%d).", errno); break;
		}

		return FAILURE;
	}
}

PHP_RINIT_FUNCTION(smbclient) { return SUCCESS; }
PHP_MSHUTDOWN_FUNCTION(smbclient) { return SUCCESS; }

PHP_MINFO_FUNCTION(smbclient) {
	php_info_print_table_start();
	php_info_print_table_row(2, "libsmbclient Support", "enabled");
	php_info_print_table_end();
}

PHP_FUNCTION(smbclient_opendir)
{
	zval **path;
	int dirhandle;

	if((ZEND_NUM_ARGS() != 1) || (zend_get_parameters_ex(1, &path) != SUCCESS))
	{
		WRONG_PARAM_COUNT;
	}

	convert_to_string_ex(path);
	dirhandle = smbc_opendir(Z_STRVAL_PP(path));
	if(dirhandle < 0) {
		switch(errno) {
			case EACCES: php_error_docref(NULL TSRMLS_CC, E_ERROR, "Couldn't open SMB directory %s: Permission denied", Z_STRVAL_PP(path)); break;
			case EINVAL: php_error_docref(NULL TSRMLS_CC, E_ERROR, "Couldn't open SMB directory %s: Invalid URL", Z_STRVAL_PP(path)); break;
			case ENOENT: php_error_docref(NULL TSRMLS_CC, E_ERROR, "Couldn't open SMB directory %s: Path does not exist", Z_STRVAL_PP(path)); break;
			case ENOMEM: php_error_docref(NULL TSRMLS_CC, E_ERROR, "Couldn't open SMB directory %s: Insufficient memory", Z_STRVAL_PP(path)); break;
			case ENOTDIR: php_error_docref(NULL TSRMLS_CC, E_ERROR, "Couldn't open SMB directory %s: Not a directory", Z_STRVAL_PP(path)); break;
			case EPERM: php_error_docref(NULL TSRMLS_CC, E_ERROR, "Couldn't open SMB directory %s: Workgroup not found", Z_STRVAL_PP(path)); break;
			case ENODEV: php_error_docref(NULL TSRMLS_CC, E_ERROR, "Couldn't open SMB directory %s: Workgroup or server not found", Z_STRVAL_PP(path)); break;
			default: php_error_docref(NULL TSRMLS_CC, E_ERROR, "Couldn't open SMB directory %s: Unknown error (%d)", Z_STRVAL_PP(path), errno); break;
		}

		RETURN_FALSE;
	}

	RETURN_LONG(dirhandle);
}

PHP_FUNCTION(smbclient_closedir)
{
	zval **dirhandle;
	int retval;

	if((ZEND_NUM_ARGS() != 1) || (zend_get_parameters_ex(1, &dirhandle) != SUCCESS))
	{
		WRONG_PARAM_COUNT;
	}

	
	convert_to_long_ex(dirhandle);
	retval = smbc_closedir(Z_LVAL_PP(dirhandle));
	if(retval < 0) {
		switch(errno) {
			case EBADF: php_error_docref(NULL TSRMLS_CC, E_ERROR, "Couldn't close SMB directory handle %d: Not a directory handle", Z_LVAL_PP(dirhandle)); break;
			default: php_error_docref(NULL TSRMLS_CC, E_ERROR, "Couldn't close SMB directory handle %d: Unknown error (%d)", Z_LVAL_PP(dirhandle), errno); break;
		}

		RETURN_FALSE;
	}

	RETURN_TRUE;
}


PHP_FUNCTION(smbclient_readdir)
{
	zval **dirhandle;
	struct smbc_dirent *dirent;
	char *type;

	if((ZEND_NUM_ARGS() != 1) || (zend_get_parameters_ex(1, &dirhandle) != SUCCESS))
	{
		WRONG_PARAM_COUNT;
	}

	
	convert_to_long_ex(dirhandle);
	errno = 0;
	dirent = smbc_readdir(Z_LVAL_PP(dirhandle));
	if(dirent == NULL) {
		switch(errno) {
			case 0: RETURN_FALSE;
			case EBADF: php_error_docref(NULL TSRMLS_CC, E_ERROR, "Couldn't read SMB directory handle %d: Not a directory handle", Z_LVAL_PP(dirhandle)); break;
			case EINVAL: php_error_docref(NULL TSRMLS_CC, E_ERROR, "Couldn't read SMB directory handle %d: smbc_init not called", Z_LVAL_PP(dirhandle)); break;
			default: php_error_docref(NULL TSRMLS_CC, E_ERROR, "Couldn't read SMB directory handle %d: Unknown error (%d)", Z_LVAL_PP(dirhandle), errno); break;
		}

		RETURN_FALSE;
	}

	if(array_init(return_value) != SUCCESS) {
		php_error_docref(NULL TSRMLS_CC, E_ERROR, "Couldn't initialize array");
		RETURN_FALSE;
	}

	switch(dirent->smbc_type) {
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

