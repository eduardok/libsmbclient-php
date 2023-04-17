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

#ifndef PHP_SMBCLIENT_H
#define PHP_SMBCLIENT_H

#include <libsmbclient.h>

#define PHP_SMBCLIENT_VERSION "1.1.1"

extern zend_module_entry smbclient_module_entry;
#define phpext_smbclient_ptr &smbclient_module_entry

typedef struct _php_smbclient_state
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
php_smbclient_state;

struct _php_smb_pool {
	unsigned char          hash[20];
	php_smbclient_state    *state;
	struct _php_smb_pool   *next;
	int                    nb;
};

ZEND_BEGIN_MODULE_GLOBALS(smbclient)
	struct _php_smb_pool *pool_first;
ZEND_END_MODULE_GLOBALS(smbclient)

extern ZEND_DECLARE_MODULE_GLOBALS(smbclient)

PHP_MINIT_FUNCTION(smbclient);
PHP_MSHUTDOWN_FUNCTION(smbclient);
PHP_RSHUTDOWN_FUNCTION(smbclient);
PHP_RINIT_FUNCTION(smbclient);
PHP_MINFO_FUNCTION(smbclient);
PHP_GINIT_FUNCTION(smbclient);
PHP_FUNCTION(smbclient_version);
PHP_FUNCTION(smbclient_library_version);
PHP_FUNCTION(smbclient_state_new);
PHP_FUNCTION(smbclient_state_init);
PHP_FUNCTION(smbclient_state_errno);
PHP_FUNCTION(smbclient_state_free);
PHP_FUNCTION(smbclient_option_get);
PHP_FUNCTION(smbclient_option_set);
PHP_FUNCTION(smbclient_client_protocols);
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
 * php_smbclient_globals structure, the elements of which it can access
 * through the SMBCLIENT() macro. Without ZTS, there is just one master
 * structure in which we access the members directly: */

#if PHP_MAJOR_VERSION >= 7
#define SMBCLIENT_G(v) ZEND_MODULE_GLOBALS_ACCESSOR(smbclient, v)
#else
#ifdef ZTS
#define SMBCLIENT_G(v) TSRMG(smbclient_globals_id, zend_smbclient_globals *, v)
#else
#define SMBCLIENT_G(v) (smbclient_globals.v)
#endif
#endif

#if PHP_MAJOR_VERSION >= 8
#define TSRMLS_D void
#define TSRMLS_DC
#define TSRMLS_C
#define TSRMLS_CC
#define TSRMLS_FETCH()
#endif

extern php_stream_wrapper php_stream_smb_wrapper;
php_smbclient_state * php_smbclient_state_new  (php_stream_context *context, int init TSRMLS_DC);
void                  php_smbclient_state_free (php_smbclient_state *state TSRMLS_DC);
int                   php_smbclient_state_init (php_smbclient_state *state TSRMLS_DC);
int                   flagstring_to_smbflags (const char *flags, int flags_len, int *retval TSRMLS_DC);
void                  php_smb_pool_cleanup(TSRMLS_D);

#endif /* PHP_SMBCLIENT_H */
