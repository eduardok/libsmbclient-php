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
#include "ext/standard/info.h"

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
	PHP_FE(smbclient_stat, NULL)
	PHP_FE(smbclient_open, NULL)
	PHP_FE(smbclient_creat, NULL)
	PHP_FE(smbclient_read, NULL)
	PHP_FE(smbclient_close, NULL)
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
			case ENOMEM: php_error(E_ERROR, "Couldn't initialize libsmbclient: Out of memory."); break;
			case ENOENT: php_error(E_ERROR, "Couldn't initialize libsmbclient: The smb.conf file would not load.  It must be located in ~/.smb/ (probably /root/.smb) ."); break;
			case EINVAL: php_error(E_ERROR, "Couldn't initialize libsmbclient: Invalid parameter."); break;
			default: php_error(E_ERROR, "Couldn't initialize libsmbclient: Unknown error (%d).", errno); break;
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
			case EACCES: php_error(E_ERROR, "Couldn't open SMB directory %s: Permission denied", Z_STRVAL_PP(path)); break;
			case EINVAL: php_error(E_ERROR, "Couldn't open SMB directory %s: Invalid URL", Z_STRVAL_PP(path)); break;
			case ENOENT: php_error(E_ERROR, "Couldn't open SMB directory %s: Path does not exist", Z_STRVAL_PP(path)); break;
			case ENOMEM: php_error(E_ERROR, "Couldn't open SMB directory %s: Insufficient memory", Z_STRVAL_PP(path)); break;
			case ENOTDIR: php_error(E_ERROR, "Couldn't open SMB directory %s: Not a directory", Z_STRVAL_PP(path)); break;
			case EPERM: php_error(E_ERROR, "Couldn't open SMB directory %s: Workgroup not found", Z_STRVAL_PP(path)); break;
			case ENODEV: php_error(E_ERROR, "Couldn't open SMB directory %s: Workgroup or server not found", Z_STRVAL_PP(path)); break;
			default: php_error(E_ERROR, "Couldn't open SMB directory %s: Unknown error (%d)", Z_STRVAL_PP(path), errno); break;
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
			case EBADF: php_error(E_ERROR, "Couldn't close SMB directory handle %d: Not a directory handle", Z_LVAL_PP(dirhandle)); break;
			default: php_error(E_ERROR, "Couldn't close SMB directory handle %d: Unknown error (%d)", Z_LVAL_PP(dirhandle), errno); break;
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
			case EBADF: php_error(E_ERROR, "Couldn't read SMB directory handle %d: Not a directory handle", Z_LVAL_PP(dirhandle)); break;
			case EINVAL: php_error(E_ERROR, "Couldn't read SMB directory handle %d: smbc_init not called", Z_LVAL_PP(dirhandle)); break;
			default: php_error(E_ERROR, "Couldn't read SMB directory handle %d: Unknown error (%d)", Z_LVAL_PP(dirhandle), errno); break;
		}

		RETURN_FALSE;
	}

	if(array_init(return_value) != SUCCESS) {
		php_error(E_ERROR, "Couldn't initialize array");
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

PHP_FUNCTION(smbclient_stat)
{
	zval **file;
	struct stat statbuf;
	int retval;

	if((ZEND_NUM_ARGS() != 1) || (zend_get_parameters_ex(1, &file) != SUCCESS))
	{
		WRONG_PARAM_COUNT;
	}

	convert_to_string_ex(file);
	retval = smbc_stat(Z_STRVAL_PP(file), &statbuf);
	if(retval < 0) {
		switch(errno) {
			case ENOENT: php_error(E_ERROR, "Couldn't stat %s: Does not exist", Z_STRVAL_PP(file)); break;
			case EINVAL: php_error(E_ERROR, "Couldn't stat: null URL or smbc_init failed"); break;
			case EACCES: php_error(E_ERROR, "Couldn't stat %s: Permission denied", Z_STRVAL_PP(file)); break;
			case ENOMEM: php_error(E_ERROR, "Couldn't stat %s: Out of memory", Z_STRVAL_PP(file)); break;
			case ENOTDIR: php_error(E_ERROR, "Couldn't stat %s: Not a directory", Z_STRVAL_PP(file)); break;
			default: php_error(E_ERROR, "Couldn't stat %s: Unknown error (%d)", Z_STRVAL_PP(file), errno); break;
		}

		RETURN_FALSE;
	}

	if(array_init(return_value) != SUCCESS) {
		php_error(E_ERROR, "Couldn't initialize array");
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


PHP_FUNCTION(smbclient_open)
{
	zval **file, **flags, **mode;
	int retval;

	if((ZEND_NUM_ARGS() != 1) || (zend_get_parameters_ex(1, &file) != SUCCESS) /*|| (zend_get_parameters_ex(2, &flags)) || (zend_get_parameters_ex(3, &mode))*/)
	{
		WRONG_PARAM_COUNT;
	}

	convert_to_string_ex(file);
	//convert_to_long_ex(flags);
	//convert_to_long_ex(mode);

	retval = smbc_open(Z_STRVAL_PP(file), 0, 0); //Z_LVAL_PP(flags), Z_LVAL_PP(mode));

	if(retval < 0) {
		switch(errno) {
			case ENOMEM: php_error(E_ERROR, "Couldn't open %s: Out of memory", Z_STRVAL_PP(file)); break;
			case EINVAL: php_error(E_ERROR, "Couldn't open %s: No file?", Z_STRVAL_PP(file)); break;
			case EEXIST: php_error(E_ERROR, "Couldn't open %s: Pathname already exists and O_CREAT and O_EXECL were specified", Z_STRVAL_PP(file)); break;
			case EISDIR: php_error(E_ERROR, "Couldn't open %s: Can't write to a directory", Z_STRVAL_PP(file)); break;
			case EACCES: php_error(E_ERROR, "Couldn't open %s: Access denied", Z_STRVAL_PP(file)); break;
			case ENODEV: php_error(E_ERROR, "Couldn't open %s: Requested share does not exist", Z_STRVAL_PP(file)); break;
			case ENOTDIR: php_error(E_ERROR, "Couldn't open %s: Path component isn't a directory", Z_STRVAL_PP(file)); break;
			case ENOENT: php_error(E_ERROR, "Couldn't open %s: Directory in path doesn't exist", Z_STRVAL_PP(file)); break;
			default: php_error(E_ERROR, "Couldn't open %s: Unknown error (%d)", Z_STRVAL_PP(file), errno); break;
		}

		RETURN_FALSE;
	}

	
	RETURN_LONG(retval);
}

PHP_FUNCTION(smbclient_creat)
{
	zval **file, **mode;
	int retval;

	if((ZEND_NUM_ARGS() != 2) || (zend_get_parameters_ex(1, &file) != SUCCESS) || (zend_get_parameters_ex(2, &mode)))
	{
		WRONG_PARAM_COUNT;
	}

	convert_to_string_ex(file);
	convert_to_long_ex(mode);

	retval = smbc_creat(Z_STRVAL_PP(file), Z_LVAL_PP(mode));
	if(retval < 0) {
		switch(errno) {
			case ENOMEM: php_error(E_ERROR, "Couldn't create %s: Out of memory", Z_STRVAL_PP(file)); break;
			case EINVAL: php_error(E_ERROR, "Couldn't create %s: No file?", Z_STRVAL_PP(file)); break;
			case EEXIST: php_error(E_ERROR, "Couldn't create %s: Pathname already exists and O_CREAT and O_EXECL were specified", Z_STRVAL_PP(file)); break;
			case EISDIR: php_error(E_ERROR, "Couldn't create %s: Can't write to a directory", Z_STRVAL_PP(file)); break;
			case EACCES: php_error(E_ERROR, "Couldn't create %s: Access denied", Z_STRVAL_PP(file)); break;
			case ENODEV: php_error(E_ERROR, "Couldn't create %s: Requested share does not exist", Z_STRVAL_PP(file)); break;
			case ENOENT: php_error(E_ERROR, "Couldn't create %s: Directory in path doesn't exist", Z_STRVAL_PP(file)); break;
			default: php_error(E_ERROR, "Couldn't create %s: Unknown error (%d)", Z_STRVAL_PP(file), errno); break;
		}

		RETURN_FALSE;
	}

	
	RETURN_LONG(retval);
}

PHP_FUNCTION(smbclient_read)
{
	zval **file, **count;
	long retval;

	if((ZEND_NUM_ARGS() != 2) || (zend_get_parameters_ex(1, &file) != SUCCESS) || (zend_get_parameters_ex(2, &count)))
	{
		WRONG_PARAM_COUNT;
	}

	convert_to_long_ex(file);
	convert_to_long_ex(count);

        Z_STRVAL_P(return_value) = emalloc(Z_LVAL_PP(count) + 1);
	retval = smbc_read(Z_LVAL_PP(file), Z_STRVAL_P(return_value), Z_LVAL_PP(count));
        Z_STRLEN_P(return_value) = retval;

        /* needed because recv/read/gzread doesnt put a null at the end*/
        Z_STRVAL_P(return_value)[Z_STRLEN_P(return_value)] = 0;

        Z_TYPE_P(return_value) = IS_STRING;

	if(retval < 0) {
		switch(errno) {
			case EISDIR: php_error(E_ERROR, "Couldn't read from %d: Is a directory", Z_LVAL_PP(file)); break;
			case EBADF: php_error(E_ERROR, "Couldn't read from %d: Not a valid file descriptor or not open for reading", Z_LVAL_PP(file)); break;
			case EINVAL: php_error(E_ERROR, "Couldn't read from %d: Object not suitable for reading or bad buffer", Z_LVAL_PP(file)); break;
			default: php_error(E_ERROR, "Couldn't read from %d: Unknown error (%d)", Z_LVAL_PP(file), errno); break;
		}

		RETURN_FALSE;
	}
}

PHP_FUNCTION(smbclient_close)
{
	zval **file;
	int retval;

	if((ZEND_NUM_ARGS() != 1) || (zend_get_parameters_ex(1, &file) != SUCCESS))
	{
		WRONG_PARAM_COUNT;
	}

	convert_to_long_ex(file);

	retval = smbc_close(Z_LVAL_PP(file));
	if(retval < 0) {
		switch(errno) {
			case EBADF: php_error(E_ERROR, "Couldn't close %d: Not a valid file descriptor or not open for reading", Z_LVAL_PP(file)); break;
			case EINVAL: php_error(E_ERROR, "Couldn't close %d: smbc_init not called", Z_LVAL_PP(file)); break;
			default: php_error(E_ERROR, "Couldn't close %d: Unknown error (%d)", Z_LVAL_PP(file), errno); break;
		}

		RETURN_FALSE;
	}

	
	RETURN_LONG(retval);
}

