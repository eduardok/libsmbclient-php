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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "ext/standard/url.h"
#include "ext/standard/info.h"
#include "ext/standard/php_filestat.h"
#include "ext/standard/sha1.h"
#include "php_smbclient.h"

#include <libsmbclient.h>

#define STREAM_DATA_FROM_STREAM() \
	php_smb_stream_data *self = (php_smb_stream_data *) stream->abstract;

typedef struct _php_smb_stream_data {
	php_smbclient_state *state;
	SMBCFILE *handle;
	/* pointers cache for multiple call */
	smbc_read_fn smbc_read;
	smbc_readdir_fn smbc_readdir;
	smbc_write_fn smbc_write;
	smbc_lseek_fn smbc_lseek;
	smbc_ftruncate_fn smbc_ftruncate;
} php_smb_stream_data;


static php_smbclient_state *php_smb_pool_get(php_stream_context *context, const char *url TSRMLS_DC)
{
	PHP_SHA1_CTX sha1;
	unsigned char hash[20];
	struct _php_smb_pool *pool;

	/* Create a hash for connection parameter */
	PHP_SHA1Init(&sha1);
	if (!memcmp(url, "smb://", 6)) {
		char *p;
		p = strchr(url+6, '/'); // we only want smb://workgroup;user:pass@server/
		PHP_SHA1Update(&sha1, (const unsigned char *)url+6, p ? p - url - 6 : strlen(url+6));
	}
	if (context) {
#if PHP_MAJOR_VERSION >= 7
		zval *tmpzval;

		if (NULL != (tmpzval = php_stream_context_get_option(context, "smb", "workgroup"))) {
			if (Z_TYPE_P(tmpzval) == IS_STRING) {
				PHP_SHA1Update(&sha1, (const unsigned char *)Z_STRVAL_P(tmpzval), Z_STRLEN_P(tmpzval)+1);
			}
		}
		if (NULL != (tmpzval = php_stream_context_get_option(context, "smb", "username"))) {
			if (Z_TYPE_P(tmpzval) == IS_STRING) {
				PHP_SHA1Update(&sha1, (const unsigned char *)Z_STRVAL_P(tmpzval), Z_STRLEN_P(tmpzval)+1);
			}
		}
		if (NULL != (tmpzval = php_stream_context_get_option(context, "smb", "password"))) {
			if (Z_TYPE_P(tmpzval) == IS_STRING) {
				PHP_SHA1Update(&sha1, (const unsigned char *)Z_STRVAL_P(tmpzval), Z_STRLEN_P(tmpzval)+1);
			}
		}
#else
		zval **tmpzval;

		if (php_stream_context_get_option(context, "smb", "workgroup", &tmpzval) == SUCCESS) {
			if (Z_TYPE_PP(tmpzval) == IS_STRING) {
				PHP_SHA1Update(&sha1, (const unsigned char *)Z_STRVAL_PP(tmpzval), Z_STRLEN_PP(tmpzval)+1);
			}
		}
		if (php_stream_context_get_option(context, "smb", "username", &tmpzval) == SUCCESS) {
			if (Z_TYPE_PP(tmpzval) == IS_STRING) {
				PHP_SHA1Update(&sha1, (const unsigned char *)Z_STRVAL_PP(tmpzval), Z_STRLEN_PP(tmpzval)+1);
			}
		}
		if (php_stream_context_get_option(context, "smb", "password", &tmpzval) == SUCCESS) {
			if (Z_TYPE_PP(tmpzval) == IS_STRING) {
				PHP_SHA1Update(&sha1, (const unsigned char *)Z_STRVAL_PP(tmpzval), Z_STRLEN_PP(tmpzval)+1);
			}
		}
#endif
	}
	PHP_SHA1Final(hash, &sha1);

	/* Reuse state from pool if exists */
	for (pool = SMBCLIENT_G(pool_first); pool; pool = pool->next) {
		if (!memcmp(hash, pool->hash, 20)) {
			pool->nb++;
			return pool->state;
		}
	}

	/* Create a new state and save it in the pool */
	pool = emalloc(sizeof(*pool));
	memcpy(pool->hash, hash, 20);
	pool->nb = 1;
	pool->next = SMBCLIENT_G(pool_first);
	pool->state = php_smbclient_state_new(context, 1 TSRMLS_CC);
	SMBCLIENT_G(pool_first) = pool;

	return pool->state;
}

static void php_smb_pool_drop(php_smbclient_state *state TSRMLS_DC)
{
	struct _php_smb_pool *pool;

	for (pool = SMBCLIENT_G(pool_first); pool; pool = pool->next) {
		if (pool->state == state) {
			pool->nb--;
			return;
		}
	}

	/* Not found (after php_smb_pool_cleanup) so close it */
	php_smbclient_state_free(state TSRMLS_CC);
}

void php_smb_pool_cleanup(TSRMLS_D) {
	struct _php_smb_pool *pool;

	pool = SMBCLIENT_G(pool_first);
	while (pool) {
		if (!pool->nb) { /* Keep it when still used */
			php_smbclient_state_free(pool->state TSRMLS_CC);
		}
		pool = pool->next;
		efree(pool);
	}
	SMBCLIENT_G(pool_first) = NULL;
}

static int php_smb_ops_close(php_stream *stream, int close_handle TSRMLS_DC)
{
	smbc_close_fn smbc_close;
	STREAM_DATA_FROM_STREAM();

	if (!self) {
		return EOF;
	}

	if (close_handle) {
		if (self->handle) {
			smbc_close = smbc_getFunctionClose(self->state->ctx);
			if (smbc_close) {
				smbc_close(self->state->ctx, self->handle);
			}
			self->handle = NULL;
		}
	}

	php_smb_pool_drop(self->state TSRMLS_CC);
	efree(self);
	stream->abstract = NULL;
	return EOF;
}

static int php_smb_ops_flush(php_stream *stream TSRMLS_DC)
{
	return 0;
}

#if PHP_VERSION_ID < 70400
static size_t php_smb_ops_read(php_stream *stream, char *buf, size_t count TSRMLS_DC)
#else
static ssize_t php_smb_ops_read(php_stream *stream, char *buf, size_t count TSRMLS_DC)
#endif
{
	ssize_t n = 0;
	STREAM_DATA_FROM_STREAM();

	if (!self || !self->handle) {
		return 0;
	}
	if (!self->smbc_read) {
		self->smbc_read = smbc_getFunctionRead(self->state->ctx);
	}
	if (self->smbc_read) {
		n = self->smbc_read(self->state->ctx, self->handle, buf, count);
	}
	/* cast count to signed value to avoid possibly negative n being cast to unsigned value */
	if (n == 0 || n < (ssize_t)count) {
		stream->eof = 1;
	}
#if PHP_VERSION_ID < 70400
	return (n < 1 ? 0 : (size_t)n);
#else
	return n;
#endif
}

#if PHP_VERSION_ID < 70400
static size_t php_smb_ops_write(php_stream *stream, const char *buf, size_t count TSRMLS_DC)
#else
static ssize_t php_smb_ops_write(php_stream *stream, const char *buf, size_t count TSRMLS_DC)
#endif
{
	ssize_t len = 0;
	STREAM_DATA_FROM_STREAM();

	if (!self || !self->handle) {
		return 0;
	}
	if (!self->smbc_write) {
		self->smbc_write = smbc_getFunctionWrite(self->state->ctx);
	}
	if (self->smbc_write) {
		len = self->smbc_write(self->state->ctx, self->handle, buf, count);
	}

#if PHP_VERSION_ID < 70400
	return (len < 0 ? 0 : (size_t)len);
#else
	return len;
#endif
}

static int php_smb_ops_stat(php_stream *stream, php_stream_statbuf *ssb TSRMLS_DC) /* {{{ */
{
	smbc_fstat_fn smbc_fstat;
	STREAM_DATA_FROM_STREAM();

	if (!self || !self->handle) {
		return -1;
	}
	if ((smbc_fstat = smbc_getFunctionFstat(self->state->ctx)) == NULL) {
		return -1;
	}
	if (smbc_fstat(self->state->ctx, self->handle, &ssb->sb) < 0) {
		return -1;
	}
	return 0;
}

static int php_smb_ops_seek(php_stream *stream, off_t offset, int whence, off_t *newoffset TSRMLS_DC)
{
	STREAM_DATA_FROM_STREAM();

	if (!self || !self->handle) {
		return -1;
	}
	if (!self->smbc_lseek) {
		self->smbc_lseek = smbc_getFunctionLseek(self->state->ctx);
	}
	if (self->smbc_lseek) {
		*newoffset = self->smbc_lseek(self->state->ctx, self->handle, offset, whence);
		return 0;
	}
	return -1;
}

static int php_smb_ops_set_option(php_stream *stream, int option, int value, void *ptrparam TSRMLS_DC)
{
	size_t newsize;
	STREAM_DATA_FROM_STREAM();

	if (!self || !self->handle) {
		return PHP_STREAM_OPTION_RETURN_ERR;
	}
	if (!self->smbc_ftruncate) {
		self->smbc_ftruncate = smbc_getFunctionFtruncate(self->state->ctx);
	}
	if (!self->smbc_ftruncate) {
		return PHP_STREAM_OPTION_RETURN_ERR;
	}

	switch(option) {
		case PHP_STREAM_OPTION_TRUNCATE_API:
			switch (value) {
				case PHP_STREAM_TRUNCATE_SUPPORTED:
					return PHP_STREAM_OPTION_RETURN_OK;

				case PHP_STREAM_TRUNCATE_SET_SIZE:
					newsize = *(size_t*)ptrparam;
					if (self->smbc_ftruncate(self->state->ctx, self->handle, newsize) == 0) {
						return PHP_STREAM_OPTION_RETURN_OK;
					}
					return PHP_STREAM_OPTION_RETURN_ERR;
			}
	}
	return PHP_STREAM_OPTION_RETURN_NOTIMPL;
}

static php_stream_ops php_stream_smbio_ops = {
	php_smb_ops_write,
	php_smb_ops_read,
	php_smb_ops_close,
	php_smb_ops_flush,
	"smb",
	php_smb_ops_seek,
	NULL, /* cast */
	php_smb_ops_stat,
	php_smb_ops_set_option
};

static php_stream *
php_stream_smb_opener(
	php_stream_wrapper *wrapper,
#if PHP_VERSION_ID < 50600
	char *path,
	char *mode,
#else
	const char *path,
	const char *mode,
#endif
	int options,
#if PHP_MAJOR_VERSION < 7
	char **opened_path,
#else
	zend_string **opened_path,
#endif
	php_stream_context *context
	STREAMS_DC TSRMLS_DC)
{
	php_smbclient_state *state;
	int smbflags;
	long smbmode = 0666;
	smbc_open_fn smbc_open;
	SMBCFILE *handle;
	php_smb_stream_data *self;

	/* Context */
	state = php_smb_pool_get(context, path TSRMLS_CC);
	if (!state) {
		return NULL;
	}
	/* File */
	if (!strcmp(mode, "wb")) {
		mode = "w";
	} else if (!strcmp(mode, "rb")) {
		mode = "r";
	}
	if (flagstring_to_smbflags(mode, strlen(mode), &smbflags TSRMLS_CC) == 0) {
		php_smb_pool_drop(state TSRMLS_CC);
		return NULL;
	}
	if ((smbc_open = smbc_getFunctionOpen(state->ctx)) == NULL) {
		php_smb_pool_drop(state TSRMLS_CC);
		return NULL;
	}
	if ((handle = smbc_open(state->ctx, path, smbflags, smbmode)) == NULL) {
		php_smb_pool_drop(state TSRMLS_CC);
		return NULL;
	}
	self = ecalloc(sizeof(*self), 1);
	self->state = state;
	self->handle = handle;

	return php_stream_alloc(&php_stream_smbio_ops, self, NULL, mode);
}

static int
php_stream_smb_unlink(
	php_stream_wrapper *wrapper,
#if PHP_VERSION_ID < 50600
	char *url,
#else
	const char *url,
#endif
	int options,
	php_stream_context *context
	TSRMLS_DC)
{
	php_smbclient_state *state;
	smbc_unlink_fn smbc_unlink;

	/* Context */
	state = php_smb_pool_get(context, url TSRMLS_CC);
	if (!state) {
		return 0;
	}
	/* Unlink */
	if ((smbc_unlink = smbc_getFunctionUnlink(state->ctx)) == NULL) {
		if (options & REPORT_ERRORS) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unlink not supported");
		}
		php_smb_pool_drop(state TSRMLS_CC);
		return 0;
	}
	if (smbc_unlink(state->ctx, url) == 0) {
		php_smb_pool_drop(state TSRMLS_CC);
		return 1;
	}
	if (options & REPORT_ERRORS) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unlink fails: %s", strerror(errno));
	}
	php_smb_pool_drop(state TSRMLS_CC);
	return 0;
}

static int
php_stream_smb_mkdir(
	php_stream_wrapper *wrapper,
#if PHP_VERSION_ID < 50600
	char *url,
#else
	const char *url,
#endif
	int mode,
	int options,
	php_stream_context *context
	TSRMLS_DC)
{
	php_smbclient_state *state;
	smbc_mkdir_fn smbc_mkdir;

	if (options & PHP_STREAM_MKDIR_RECURSIVE) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Recursive mkdir not supported");
		return 0;
	}
	/* Context */
	state = php_smb_pool_get(context, url TSRMLS_CC);
	if (!state) {
		return 0;
	}
	/* Mkdir */
	if ((smbc_mkdir = smbc_getFunctionMkdir(state->ctx)) == NULL) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Mkdir not supported");
		php_smb_pool_drop(state TSRMLS_CC);
		return 0;
	}
	if (smbc_mkdir(state->ctx, url, (mode_t)mode) == 0) {
		php_smb_pool_drop(state TSRMLS_CC);
		return 1;
	}
	php_error_docref(NULL TSRMLS_CC, E_WARNING, "Mkdir fails: %s", strerror(errno));
	php_smb_pool_drop(state TSRMLS_CC);
	return 0;
}

static int
php_stream_smb_rmdir(
	php_stream_wrapper *wrapper,
#if PHP_VERSION_ID < 50600
	char *url,
#else
	const char *url,
#endif
	int options,
	php_stream_context *context
	TSRMLS_DC)
{
	php_smbclient_state *state;
	smbc_rmdir_fn smbc_rmdir;

	/* Context */
	state = php_smb_pool_get(context, url TSRMLS_CC);
	if (!state) {
		return 0;
	}
	/* Rmdir */
	if ((smbc_rmdir = smbc_getFunctionRmdir(state->ctx)) == NULL) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Rmdir not supported");
		php_smb_pool_drop(state TSRMLS_CC);
		return 0;
	}
	if (smbc_rmdir(state->ctx, url) == 0) {
		php_smb_pool_drop(state TSRMLS_CC);
		return 1;
	}
	php_error_docref(NULL TSRMLS_CC, E_WARNING, "Rmdir fails: %s", strerror(errno));
	php_smb_pool_drop(state TSRMLS_CC);
	return 0;
}

static int
php_stream_smb_rename(
	php_stream_wrapper *wrapper,
#if PHP_VERSION_ID < 50600
	char *url_from,
	char *url_to,
#else
	const char *url_from,
	const char *url_to,
#endif
	int options,
	php_stream_context *context
	TSRMLS_DC)
{
	php_smbclient_state *state;
	smbc_rename_fn smbc_rename;

	/* Context */
	state = php_smb_pool_get(context, url_from TSRMLS_CC);
	if (!state) {
		return 0;
	}
	if ((smbc_rename = smbc_getFunctionRename(state->ctx)) == NULL) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Rename not supported");
		php_smb_pool_drop(state TSRMLS_CC);
		return 0;
	}
	if (smbc_rename(state->ctx, url_from, state->ctx, url_to) == 0) {
		php_smb_pool_drop(state TSRMLS_CC);
		return 1;
	}
	php_error_docref(NULL TSRMLS_CC, E_WARNING, "Rename fails: %s", strerror(errno));
	php_smb_pool_drop(state TSRMLS_CC);
	return 0;
}

static int php_smbdir_ops_close(php_stream *stream, int close_handle TSRMLS_DC)
{
	smbc_closedir_fn smbc_closedir;
	STREAM_DATA_FROM_STREAM();

	if (close_handle) {
		if (self->handle) {
			smbc_closedir = smbc_getFunctionClosedir(self->state->ctx);
			if (smbc_closedir) {
				smbc_closedir(self->state->ctx, self->handle);
			}
			self->handle = NULL;
		}
	}
	php_smb_pool_drop(self->state TSRMLS_CC);
	efree(self);
	stream->abstract = NULL;
	return EOF;
}

#if PHP_VERSION_ID < 70400
static size_t php_smbdir_ops_read(php_stream *stream, char *buf, size_t count TSRMLS_DC)
#else
static ssize_t php_smbdir_ops_read(php_stream *stream, char *buf, size_t count TSRMLS_DC)
#endif
{
	struct smbc_dirent *dirent;
	php_stream_dirent *ent = (php_stream_dirent*)buf;
	STREAM_DATA_FROM_STREAM();

	if (!self || !self->handle) {
		return 0;
	}
	/* avoid problems if someone mis-uses the stream */
	if (count != sizeof(php_stream_dirent)) {
		return 0;
	}
	if (!self->smbc_readdir) {
		self->smbc_readdir = smbc_getFunctionReaddir(self->state->ctx);
	}
	if (self->smbc_readdir) {
		if ((dirent = self->smbc_readdir(self->state->ctx, self->handle)) != NULL) {
			PHP_STRLCPY(ent->d_name, dirent->name, sizeof(ent->d_name), dirent->namelen);
			return sizeof(php_stream_dirent);
		}
	}
	stream->eof = 1;
	return 0;
}

static php_stream_ops	php_stream_smbdir_ops = {
	NULL,
	php_smbdir_ops_read,
	php_smbdir_ops_close,
	NULL,
	"smbdir",
	NULL, /* rewind */
	NULL, /* cast */
	NULL, /* stat */
	NULL  /* set_option */
};

static php_stream *
php_stream_smbdir_opener(
	php_stream_wrapper *wrapper,
#if PHP_VERSION_ID < 50600
	char *path,
	char *mode,
#else
	const char *path,
	const char *mode,
#endif
	int options,
#if PHP_MAJOR_VERSION < 7
	char **opened_path,
#else
	zend_string **opened_path,
#endif
	php_stream_context *context
	STREAMS_DC TSRMLS_DC)
{
	php_smbclient_state *state;
	smbc_opendir_fn smbc_opendir;
	SMBCFILE *handle;
	php_smb_stream_data *self;

	/* Context */
	state = php_smb_pool_get(context, path TSRMLS_CC);
	if (!state) {
		return NULL;
	}
	/* Directory */
	if ((smbc_opendir = smbc_getFunctionOpendir(state->ctx)) == NULL) {
		php_smb_pool_drop(state TSRMLS_CC);
		return NULL;
	}
	if ((handle = smbc_opendir(state->ctx, path)) == NULL) {
		php_smb_pool_drop(state TSRMLS_CC);
		return NULL;
	}
	self = ecalloc(sizeof(*self), 1);
	self->state  = state;
	self->handle = handle;

	return php_stream_alloc(&php_stream_smbdir_ops, self, NULL, mode);
}

static int
php_stream_smb_stat(
	php_stream_wrapper *wrapper,
#if PHP_VERSION_ID < 50600
	char *url,
#else
	const char *url,
#endif
	int flags,
	php_stream_statbuf *ssb,
	php_stream_context *context
	TSRMLS_DC)
{
	php_smbclient_state *state;
	smbc_stat_fn smbc_stat;

	/* Context */
	state = php_smb_pool_get(context, url TSRMLS_CC);
	if (!state) {
		return 0;
	}
	/* Stat */
	if ((smbc_stat = smbc_getFunctionStat(state->ctx)) == NULL) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Stat not supported");
		php_smb_pool_drop(state TSRMLS_CC);
		return -1;
	}
	if (smbc_stat(state->ctx, url, &ssb->sb) >= 0) {
		php_smb_pool_drop(state TSRMLS_CC);
		return 0;
	}
	/* dont display error as PHP use this method internally to check if file exists */
	php_smb_pool_drop(state TSRMLS_CC);
	return -1;
}

#if PHP_VERSION_ID >= 50400
static int
php_stream_smb_metadata(
	php_stream_wrapper *wrapper,
#if PHP_VERSION_ID < 50600
	char *url,
#else
	const char *url,
#endif
	int option,
	void *value,
	php_stream_context *context
	TSRMLS_DC)
{
	php_smbclient_state *state;
	smbc_chmod_fn smbc_chmod;
	smbc_open_fn smbc_open;
	smbc_utimes_fn smbc_utimes;
	smbc_close_fn smbc_close;
	mode_t mode;
	struct utimbuf *newtime;
	struct timeval times[2];
	SMBCFILE *handle;
	int ret = 0;

	switch(option) {
		case PHP_STREAM_META_TOUCH:
			newtime = (struct utimbuf *)value;

			/* Context */
			state = php_smb_pool_get(context, url TSRMLS_CC);
			if (!state) {
				return 0;
			}
			/* Create + Utimes */
			if ((smbc_open = smbc_getFunctionOpen(state->ctx)) == NULL
				|| (smbc_close = smbc_getFunctionClose(state->ctx)) == NULL
				|| (smbc_utimes = smbc_getFunctionUtimes(state->ctx)) == NULL) {
				ret = -1;
				break;
			}
			/* Create can fail if file exists, ignore result */
			handle = smbc_open(state->ctx, url, O_EXCL|O_CREAT, 0666);
			if (handle) {
				smbc_close(state->ctx, handle);
			}
			if (newtime) {
				times[0].tv_usec = 0;
				times[0].tv_sec = newtime->actime;
				times[1].tv_usec = 0;
				times[1].tv_sec = newtime->modtime;

				ret = smbc_utimes(state->ctx, url, times);
			}
			break;

		case PHP_STREAM_META_ACCESS:
			mode = (mode_t)*(long *)value;
			/* Context */
			state = php_smb_pool_get(context, url TSRMLS_CC);
			if (!state) {
				return 0;
			}
			/* Chmod */
			if ((smbc_chmod = smbc_getFunctionChmod(state->ctx)) == NULL) {
				ret = -1;
				break;
			}
			ret = smbc_chmod(state->ctx, url, (mode_t)mode);
			break;

		default:
			php_error_docref1(NULL TSRMLS_CC, url, E_WARNING, "Unknown option %d for stream_metadata", option);
			return 0;
	}
	php_smb_pool_drop(state TSRMLS_CC);
	if (ret == -1) {
		php_error_docref1(NULL TSRMLS_CC, url, E_WARNING, "Operation failed: %s", strerror(errno));
		return 0;
	}
	php_clear_stat_cache(0, NULL, 0 TSRMLS_CC);
	return 1;
}
#endif

static php_stream_wrapper_ops smb_stream_wops = {
	php_stream_smb_opener,
	NULL,	/* close */
	NULL,	/* fstat */
	php_stream_smb_stat,
	php_stream_smbdir_opener,
	"smb",
	php_stream_smb_unlink,
	php_stream_smb_rename,
	php_stream_smb_mkdir,
	php_stream_smb_rmdir
#if PHP_VERSION_ID >= 50400
	, php_stream_smb_metadata
#endif
};

php_stream_wrapper php_stream_smb_wrapper = {
	&smb_stream_wops,
	NULL,
	1 /* is_url */
};
