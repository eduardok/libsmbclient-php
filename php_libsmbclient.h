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

#ifndef PHP_LIBSMBCLIENT_H
#define PHP_LIBSMBCLIENT_H

extern zend_module_entry libsmbclient_module_entry;
#define phpext_libsmbclient_ptr &libsmbclient_module_entry

typedef struct {
} php_libsmbclient_globals;

PHP_MINIT_FUNCTION(smbclient);
PHP_MSHUTDOWN_FUNCTION(smbclient);
PHP_RINIT_FUNCTION(smbclient);
PHP_MINFO_FUNCTION(smbclient);
PHP_FUNCTION(smbclient_version);
PHP_FUNCTION(smbclient_library_version);
PHP_FUNCTION(smbclient_state_new);
PHP_FUNCTION(smbclient_state_init);
PHP_FUNCTION(smbclient_state_errno);
PHP_FUNCTION(smbclient_state_free);
PHP_FUNCTION(smbclient_option_get);
PHP_FUNCTION(smbclient_option_set);
PHP_FUNCTION(smbclient_opendir);
PHP_FUNCTION(smbclient_readdir);
PHP_FUNCTION(smbclient_closedir);
PHP_FUNCTION(smbclient_rename);
PHP_FUNCTION(smbclient_unlink);
PHP_FUNCTION(smbclient_mkdir);
PHP_FUNCTION(smbclient_rmdir);
PHP_FUNCTION(smbclient_stat);
PHP_FUNCTION(smbclient_fstat);
PHP_FUNCTION(smbclient_open);
PHP_FUNCTION(smbclient_creat);
PHP_FUNCTION(smbclient_read);
PHP_FUNCTION(smbclient_write);
PHP_FUNCTION(smbclient_lseek);
PHP_FUNCTION(smbclient_ftruncate);
PHP_FUNCTION(smbclient_close);
PHP_FUNCTION(smbclient_chmod);
PHP_FUNCTION(smbclient_utimes);
PHP_FUNCTION(smbclient_listxattr);
PHP_FUNCTION(smbclient_getxattr);
PHP_FUNCTION(smbclient_setxattr);
PHP_FUNCTION(smbclient_removexattr);
PHP_FUNCTION(smbclient_statvfs);
PHP_FUNCTION(smbclient_fstatvfs);

/* If Zend Thread Safety (ZTS) is defined, each thread gets its own private
 * php_libsmbclient_globals structure, the elements of which it can access
 * through the LIBSMBCLIENT() macro. Without ZTS, there is just one master
 * structure in which we access the members directly: */
#ifdef ZTS
#define LIBSMBCLIENT(v) TSRMG(libsmbclient_globals_id, php_libsmbclient_globals *, v)
#else
#define LIBSMBCLIENT(v) (libsmbclient_globals.v)
#endif

#endif /* PHP_LIBSMBCLIENT_H */
