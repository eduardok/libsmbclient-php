/* ------------------------------------------------------------------
 * This file is part of libsmbclient-php: Samba bindings for PHP.
 * Libsmbclient-php is licensed under the BSD 2-clause license:
 * ------------------------------------------------------------------
 *
 * Copyright (c) 2003,        Matthew Sachs
 *               2009 - 2014, Eduardo Bacchi Kienetz
 *               2013 - 2015, Alfred Klomp
 *               2015,        Remi Collet
 *
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
#include "php_libsmbclient.h"

#include <libsmbclient.h>

#define STREAM_DATA_FROM_STREAM() \
	php_smb_stream_data *self = (php_smb_stream_data *) stream->abstract;

typedef struct _php_smb_stream_data {
	php_libsmbclient_state *state;
	SMBCFILE               *handle;
	/* pointers cache for multiple call */
	smbc_read_fn            smbc_read;
	smbc_readdir_fn         smbc_readdir;
	smbc_write_fn           smbc_write;
	smbc_lseek_fn           smbc_lseek;
} php_smb_stream_data;

static int php_smb_ops_close(php_stream *stream, int close_handle TSRMLS_DC)
{
	smbc_close_fn smbc_close;
	STREAM_DATA_FROM_STREAM();

	if (close_handle) {
		if (self->handle) {
			smbc_close = smbc_getFunctionClose(self->state->ctx);
			if (smbc_close) {
				smbc_close(self->state->ctx, self->handle);
			}
			self->handle = NULL;
		}
	}

	php_libsmbclient_state_free(self->state TSRMLS_CC);
	efree(self);
	stream->abstract = NULL;
	return EOF;
}

static int php_smb_ops_flush(php_stream *stream TSRMLS_DC)
{
	return 0;
}

static size_t php_smb_ops_read(php_stream *stream, char *buf, size_t count TSRMLS_DC)
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
	return (n < 1 ? 0 : (size_t)n);
}

static size_t php_smb_ops_write(php_stream *stream, const char *buf, size_t count TSRMLS_DC)
{
	size_t len = 0;
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
	return len;
}

static int php_smb_ops_stat(php_stream *stream, php_stream_statbuf *ssb TSRMLS_DC) /* {{{ */
{
	smbc_fstat_fn smbc_fstat;
	STREAM_DATA_FROM_STREAM();

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

	if (!self->smbc_lseek) {
		self->smbc_lseek = smbc_getFunctionLseek(self->state->ctx);
	}
	if (self->smbc_lseek) {
		*newoffset = self->smbc_lseek(self->state->ctx, self->handle, offset, whence);
		return 0;
	}
	return -1;
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
	NULL  /* set_option */
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
	char **opened_path,
	php_stream_context *context
	STREAMS_DC TSRMLS_DC)
{
	php_libsmbclient_state *state;
	int                     smbflags;
	long                    smbmode = 0666;
	smbc_open_fn            smbc_open;
	SMBCFILE               *handle;
	php_smb_stream_data    *self;

	/* Context */
	state = php_libsmbclient_state_new(context, 1 TSRMLS_CC);
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
		php_libsmbclient_state_free(state TSRMLS_CC);
		return NULL;
	}
	if ((smbc_open = smbc_getFunctionOpen(state->ctx)) == NULL) {
		php_libsmbclient_state_free(state TSRMLS_CC);
		return NULL;
	}
	if ((handle = smbc_open(state->ctx, path, smbflags, smbmode)) == NULL) {
		php_libsmbclient_state_free(state TSRMLS_CC);
		return NULL;
	}
	self = ecalloc(sizeof(*self), 1);
	self->state  = state;
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
	php_libsmbclient_state *state;
	smbc_unlink_fn smbc_unlink;

	/* Context */
	state = php_libsmbclient_state_new(context, 1 TSRMLS_CC);
	if (!state) {
		return 0;
	}
	/* Unlink */
	if ((smbc_unlink = smbc_getFunctionUnlink(state->ctx)) == NULL) {
		if (options & REPORT_ERRORS) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unlink not supported");
		}
		php_libsmbclient_state_free(state TSRMLS_CC);
		return 0;
	}
	if (smbc_unlink(state->ctx, url) == 0) {
		php_libsmbclient_state_free(state TSRMLS_CC);
		return 1;
	}
	if (options & REPORT_ERRORS) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unlink fails: %s", strerror(errno));
	}
	php_libsmbclient_state_free(state TSRMLS_CC);
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
	php_libsmbclient_state *state;
	smbc_mkdir_fn smbc_mkdir;

	if (options & PHP_STREAM_MKDIR_RECURSIVE) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Recursive mkdir not supported");
		return 0;
	}
	/* Context */
	state = php_libsmbclient_state_new(context, 1 TSRMLS_CC);
	if (!state) {
		return 0;
	}
	/* Mkdir */
	if ((smbc_mkdir = smbc_getFunctionMkdir(state->ctx)) == NULL) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Mkdir not supported");
		php_libsmbclient_state_free(state TSRMLS_CC);
		return 0;
	}
	if (smbc_mkdir(state->ctx, url, (mode_t)mode) == 0) {
		php_libsmbclient_state_free(state TSRMLS_CC);
		return 1;
	}
	php_error_docref(NULL TSRMLS_CC, E_WARNING, "Mkdir fails: %s", strerror(errno));
	php_libsmbclient_state_free(state TSRMLS_CC);
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
	php_libsmbclient_state *state;
	smbc_rmdir_fn smbc_rmdir;

	/* Context */
	state = php_libsmbclient_state_new(context, 1 TSRMLS_CC);
	if (!state) {
		return 0;
	}
	/* Rmdir */
	if ((smbc_rmdir = smbc_getFunctionRmdir(state->ctx)) == NULL) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Rmdir not supported");
		php_libsmbclient_state_free(state TSRMLS_CC);
		return 0;
	}
	if (smbc_rmdir(state->ctx, url) == 0) {
		php_libsmbclient_state_free(state TSRMLS_CC);
		return 1;
	}
	php_error_docref(NULL TSRMLS_CC, E_WARNING, "Rmdir fails: %s", strerror(errno));
	php_libsmbclient_state_free(state TSRMLS_CC);
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
	php_libsmbclient_state *state;
	smbc_rename_fn smbc_rename;

	/* Context */
	state = php_libsmbclient_state_new(context, 1 TSRMLS_CC);
	if (!state) {
		return 0;
	}
	if ((smbc_rename = smbc_getFunctionRename(state->ctx)) == NULL) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Rename not supported");
		php_libsmbclient_state_free(state TSRMLS_CC);
		return 0;
	}
	if (smbc_rename(state->ctx, url_from, state->ctx, url_to) == 0) {
		php_libsmbclient_state_free(state TSRMLS_CC);
		return 1;
	}
	php_error_docref(NULL TSRMLS_CC, E_WARNING, "Rename fails: %s", strerror(errno));
	php_libsmbclient_state_free(state TSRMLS_CC);
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
	php_libsmbclient_state_free(self->state TSRMLS_CC);
	efree(self);
	stream->abstract = NULL;
	return EOF;
}

static size_t php_smbdir_ops_read(php_stream *stream, char *buf, size_t count TSRMLS_DC)
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
	char **opened_path,
	php_stream_context *context
	STREAMS_DC TSRMLS_DC)
{
	php_libsmbclient_state *state;
	smbc_opendir_fn         smbc_opendir;
	SMBCFILE               *handle;
	php_smb_stream_data    *self;

	/* Context */
	state = php_libsmbclient_state_new(context, 1 TSRMLS_CC);
	if (!state) {
		return NULL;
	}
	/* Directory */
	if ((smbc_opendir = smbc_getFunctionOpendir(state->ctx)) == NULL) {
		php_libsmbclient_state_free(state TSRMLS_CC);
		return NULL;
	}
	if ((handle = smbc_opendir(state->ctx, path)) == NULL) {
		php_libsmbclient_state_free(state TSRMLS_CC);
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
	php_libsmbclient_state *state;
	smbc_stat_fn smbc_stat;

	/* Context */
	state = php_libsmbclient_state_new(context, 1 TSRMLS_CC);
	if (!state) {
		return 0;
	}
	/* Stat */
	if ((smbc_stat = smbc_getFunctionStat(state->ctx)) == NULL) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Stat not supported");
		php_libsmbclient_state_free(state TSRMLS_CC);
		return -1;
	}
	if (smbc_stat(state->ctx, url, &ssb->sb) >= 0) {
		php_libsmbclient_state_free(state TSRMLS_CC);
		return 0;
	}
	/* dont display error as PHP use this method internally to check if file exists */
	php_libsmbclient_state_free(state TSRMLS_CC);
	return -1;
}

static php_stream_wrapper_ops smb_stream_wops = {
	php_stream_smb_opener,
	NULL,	/* close */
	NULL,	/* fstat */
	php_stream_smb_stat,
	php_stream_smbdir_opener,
	"smb wrapper",
	php_stream_smb_unlink,
	php_stream_smb_rename,
	php_stream_smb_mkdir,
	php_stream_smb_rmdir,
	NULL     /* metadata */
};

php_stream_wrapper php_stream_smb_wrapper = {
	&smb_stream_wops,
	NULL,
	1 /* is_url */
};
