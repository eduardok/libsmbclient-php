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
	smbc_read_fn            smbc_read;
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

php_stream *php_stream_smb_opener(
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
	state = php_libsmbclient_state_new(TSRMLS_C);
	if (!state) {
		return NULL;
	}
	if (php_libsmbclient_state_init(state TSRMLS_CC)) {
		php_libsmbclient_state_free(state TSRMLS_CC);
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

static int php_stream_smb_unlink(
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
	state = php_libsmbclient_state_new(TSRMLS_C);
	if (!state) {
		return NULL;
	}
	if (php_libsmbclient_state_init(state TSRMLS_CC)) {
		php_libsmbclient_state_free(state TSRMLS_CC);
		return NULL;
	}
	/* Unlink */
	if ((smbc_unlink = smbc_getFunctionUnlink(state->ctx)) == NULL) {
		if (options & REPORT_ERRORS) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unlink not supported");
		}
		return 0;
	}
	if (smbc_unlink(state->ctx, url) == 0) {
		return 1;
	}
	if (options & REPORT_ERRORS) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unlink fails: ", strerror(errno));
	}
	return 0;
}

static php_stream_wrapper_ops smb_stream_wops = {
	php_stream_smb_opener,
	NULL,	/* close */
	NULL,	/* fstat */
	NULL,	/* stat */
	NULL,	/* opendir */
	"smb wrapper",
	php_stream_smb_unlink,
	NULL,	/* rename */
	NULL,	/* mkdir */
	NULL	/* rmdir */
};

php_stream_wrapper php_stream_smb_wrapper = {
	&smb_stream_wops,
	NULL,
	1 /* is_url */
};
