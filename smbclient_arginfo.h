/* This is a generated file, edit the .stub.php file instead.
 * Stub hash: 6abe27ac2cfb259cc835af1bafe32bb639c422b1 */

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_smbclient_version, 0, 0, IS_STRING, 0)
ZEND_END_ARG_INFO()

#define arginfo_smbclient_library_version arginfo_smbclient_version

ZEND_BEGIN_ARG_INFO_EX(arginfo_smbclient_state_new, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_smbclient_state_init, 0, 1, _IS_BOOL, 0)
	ZEND_ARG_INFO(0, state)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, workgroup, IS_STRING, 0, "null")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, user, IS_STRING, 0, "null")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, password, IS_STRING, 0, "null")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_smbclient_state_errno, 0, 1, IS_LONG, 0)
	ZEND_ARG_INFO(0, state)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_smbclient_state_free, 0, 1, _IS_BOOL, 0)
	ZEND_ARG_INFO(0, state)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_smbclient_option_get, 0, 0, 2)
	ZEND_ARG_INFO(0, state)
	ZEND_ARG_TYPE_INFO(0, option, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_smbclient_option_set, 0, 0, 3)
	ZEND_ARG_INFO(0, state)
	ZEND_ARG_TYPE_INFO(0, option, IS_LONG, 0)
	ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

#if HAVE_SMBC_SETOPTIONPROTOCOLS
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_smbclient_client_protocols, 0, 1, _IS_BOOL, 0)
	ZEND_ARG_INFO(0, state)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, minproto, IS_STRING, 0, "null")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, maxproto, IS_STRING, 0, "null")
ZEND_END_ARG_INFO()
#endif

ZEND_BEGIN_ARG_INFO_EX(arginfo_smbclient_opendir, 0, 0, 2)
	ZEND_ARG_INFO(0, state)
	ZEND_ARG_TYPE_INFO(0, path, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_MASK_EX(arginfo_smbclient_readdir, 0, 2, MAY_BE_FALSE|MAY_BE_ARRAY)
	ZEND_ARG_INFO(0, state)
	ZEND_ARG_INFO(0, dir)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_smbclient_closedir, 0, 2, _IS_BOOL, 0)
	ZEND_ARG_INFO(0, state)
	ZEND_ARG_INFO(0, dir)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_MASK_EX(arginfo_smbclient_stat, 0, 2, MAY_BE_FALSE|MAY_BE_ARRAY)
	ZEND_ARG_INFO(0, state)
	ZEND_ARG_TYPE_INFO(0, path, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_MASK_EX(arginfo_smbclient_fstat, 0, 2, MAY_BE_FALSE|MAY_BE_ARRAY)
	ZEND_ARG_INFO(0, state)
	ZEND_ARG_INFO(0, file)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_smbclient_open, 0, 0, 3)
	ZEND_ARG_INFO(0, state)
	ZEND_ARG_TYPE_INFO(0, path, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, flags, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, mode, IS_LONG, 0, "0666")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_smbclient_creat, 0, 0, 2)
	ZEND_ARG_INFO(0, state)
	ZEND_ARG_TYPE_INFO(0, path, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, mode, IS_LONG, 0, "0666")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_MASK_EX(arginfo_smbclient_read, 0, 3, MAY_BE_FALSE|MAY_BE_STRING)
	ZEND_ARG_INFO(0, state)
	ZEND_ARG_INFO(0, file)
	ZEND_ARG_TYPE_INFO(0, count, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_smbclient_close, 0, 2, _IS_BOOL, 0)
	ZEND_ARG_INFO(0, state)
	ZEND_ARG_INFO(0, file)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_smbclient_mkdir, 0, 2, _IS_BOOL, 0)
	ZEND_ARG_INFO(0, state)
	ZEND_ARG_TYPE_INFO(0, path, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, mode, IS_LONG, 0, "0666")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_smbclient_rmdir, 0, 2, _IS_BOOL, 0)
	ZEND_ARG_INFO(0, state)
	ZEND_ARG_TYPE_INFO(0, path, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_smbclient_rename, 0, 4, _IS_BOOL, 0)
	ZEND_ARG_INFO(0, oldstate)
	ZEND_ARG_TYPE_INFO(0, oldpath, IS_STRING, 0)
	ZEND_ARG_INFO(0, newstate)
	ZEND_ARG_TYPE_INFO(0, newpath, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_MASK_EX(arginfo_smbclient_write, 0, 3, MAY_BE_FALSE|MAY_BE_LONG)
	ZEND_ARG_INFO(0, state)
	ZEND_ARG_INFO(0, file)
	ZEND_ARG_TYPE_INFO(0, buffer, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, count, IS_LONG, 0, "0")
ZEND_END_ARG_INFO()

#define arginfo_smbclient_unlink arginfo_smbclient_rmdir

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_MASK_EX(arginfo_smbclient_lseek, 0, 4, MAY_BE_FALSE|MAY_BE_LONG)
	ZEND_ARG_INFO(0, state)
	ZEND_ARG_INFO(0, file)
	ZEND_ARG_TYPE_INFO(0, offset, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, whence, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_smbclient_ftruncate, 0, 3, _IS_BOOL, 0)
	ZEND_ARG_INFO(0, state)
	ZEND_ARG_INFO(0, file)
	ZEND_ARG_TYPE_INFO(0, offset, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_smbclient_chmod, 0, 3, _IS_BOOL, 0)
	ZEND_ARG_INFO(0, state)
	ZEND_ARG_TYPE_INFO(0, path, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, mode, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_smbclient_utimes, 0, 2, _IS_BOOL, 0)
	ZEND_ARG_INFO(0, state)
	ZEND_ARG_TYPE_INFO(0, path, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, mtime, IS_LONG, 0, "-1")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, atime, IS_LONG, 0, "-1")
ZEND_END_ARG_INFO()

#define arginfo_smbclient_listxattr arginfo_smbclient_stat

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_MASK_EX(arginfo_smbclient_getxattr, 0, 3, MAY_BE_FALSE|MAY_BE_STRING)
	ZEND_ARG_INFO(0, state)
	ZEND_ARG_TYPE_INFO(0, path, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, name, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_MASK_EX(arginfo_smbclient_setxattr, 0, 4, MAY_BE_FALSE|MAY_BE_STRING)
	ZEND_ARG_INFO(0, state)
	ZEND_ARG_TYPE_INFO(0, path, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, name, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, value, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, flags, IS_LONG, 0, "0")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_smbclient_removexattr, 0, 3, _IS_BOOL, 0)
	ZEND_ARG_INFO(0, state)
	ZEND_ARG_TYPE_INFO(0, path, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, name, IS_STRING, 0)
ZEND_END_ARG_INFO()

#define arginfo_smbclient_statvfs arginfo_smbclient_stat

#define arginfo_smbclient_fstatvfs arginfo_smbclient_fstat


ZEND_FUNCTION(smbclient_version);
ZEND_FUNCTION(smbclient_library_version);
ZEND_FUNCTION(smbclient_state_new);
ZEND_FUNCTION(smbclient_state_init);
ZEND_FUNCTION(smbclient_state_errno);
ZEND_FUNCTION(smbclient_state_free);
ZEND_FUNCTION(smbclient_option_get);
ZEND_FUNCTION(smbclient_option_set);
#if HAVE_SMBC_SETOPTIONPROTOCOLS
ZEND_FUNCTION(smbclient_client_protocols);
#endif
ZEND_FUNCTION(smbclient_opendir);
ZEND_FUNCTION(smbclient_readdir);
ZEND_FUNCTION(smbclient_closedir);
ZEND_FUNCTION(smbclient_stat);
ZEND_FUNCTION(smbclient_fstat);
ZEND_FUNCTION(smbclient_open);
ZEND_FUNCTION(smbclient_creat);
ZEND_FUNCTION(smbclient_read);
ZEND_FUNCTION(smbclient_close);
ZEND_FUNCTION(smbclient_mkdir);
ZEND_FUNCTION(smbclient_rmdir);
ZEND_FUNCTION(smbclient_rename);
ZEND_FUNCTION(smbclient_write);
ZEND_FUNCTION(smbclient_unlink);
ZEND_FUNCTION(smbclient_lseek);
ZEND_FUNCTION(smbclient_ftruncate);
ZEND_FUNCTION(smbclient_chmod);
ZEND_FUNCTION(smbclient_utimes);
ZEND_FUNCTION(smbclient_listxattr);
ZEND_FUNCTION(smbclient_getxattr);
ZEND_FUNCTION(smbclient_setxattr);
ZEND_FUNCTION(smbclient_removexattr);
ZEND_FUNCTION(smbclient_statvfs);
ZEND_FUNCTION(smbclient_fstatvfs);


static const zend_function_entry ext_functions[] = {
	ZEND_FE(smbclient_version, arginfo_smbclient_version)
	ZEND_FE(smbclient_library_version, arginfo_smbclient_library_version)
	ZEND_FE(smbclient_state_new, arginfo_smbclient_state_new)
	ZEND_FE(smbclient_state_init, arginfo_smbclient_state_init)
	ZEND_FE(smbclient_state_errno, arginfo_smbclient_state_errno)
	ZEND_FE(smbclient_state_free, arginfo_smbclient_state_free)
	ZEND_FE(smbclient_option_get, arginfo_smbclient_option_get)
	ZEND_FE(smbclient_option_set, arginfo_smbclient_option_set)
#if HAVE_SMBC_SETOPTIONPROTOCOLS
	ZEND_FE(smbclient_client_protocols, arginfo_smbclient_client_protocols)
#endif
	ZEND_FE(smbclient_opendir, arginfo_smbclient_opendir)
	ZEND_FE(smbclient_readdir, arginfo_smbclient_readdir)
	ZEND_FE(smbclient_closedir, arginfo_smbclient_closedir)
	ZEND_FE(smbclient_stat, arginfo_smbclient_stat)
	ZEND_FE(smbclient_fstat, arginfo_smbclient_fstat)
	ZEND_FE(smbclient_open, arginfo_smbclient_open)
	ZEND_FE(smbclient_creat, arginfo_smbclient_creat)
	ZEND_FE(smbclient_read, arginfo_smbclient_read)
	ZEND_FE(smbclient_close, arginfo_smbclient_close)
	ZEND_FE(smbclient_mkdir, arginfo_smbclient_mkdir)
	ZEND_FE(smbclient_rmdir, arginfo_smbclient_rmdir)
	ZEND_FE(smbclient_rename, arginfo_smbclient_rename)
	ZEND_FE(smbclient_write, arginfo_smbclient_write)
	ZEND_FE(smbclient_unlink, arginfo_smbclient_unlink)
	ZEND_FE(smbclient_lseek, arginfo_smbclient_lseek)
	ZEND_FE(smbclient_ftruncate, arginfo_smbclient_ftruncate)
	ZEND_FE(smbclient_chmod, arginfo_smbclient_chmod)
	ZEND_FE(smbclient_utimes, arginfo_smbclient_utimes)
	ZEND_FE(smbclient_listxattr, arginfo_smbclient_listxattr)
	ZEND_FE(smbclient_getxattr, arginfo_smbclient_getxattr)
	ZEND_FE(smbclient_setxattr, arginfo_smbclient_setxattr)
	ZEND_FE(smbclient_removexattr, arginfo_smbclient_removexattr)
	ZEND_FE(smbclient_statvfs, arginfo_smbclient_statvfs)
	ZEND_FE(smbclient_fstatvfs, arginfo_smbclient_fstatvfs)
	ZEND_FE_END
};
