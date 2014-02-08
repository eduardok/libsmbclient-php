libsmbclient-php: a PHP wrapper for libsmbclient
================================================

libsmbclient-php is a PHP extension that uses samba's libsmbclient library
to provide samba related functions to PHP programs.

Getting started
---------------

- Check out the source code using git:

	git clone git://github.com/eduardok/libsmbclient-php.git

- phpize it:

	cd libsmbclient-php ; phpize

- Build the module

	./configure
	make

- As root install the module into the extensions directory:

	make install

- Activate libsmbclient-php in php.ini:

	extension="libsmbclient.so"

If you want to contribute
-------------------------

libsmbclient-php has been improved on a "as needed" basis, so it could be
that there is a feature not yet implemented, etc.
As a way of keeping a history for other people using libsmbclient-php it
is better to get in contact through the mailing list instead of the
developer(s) directly.

	http://groups.google.com/group/libsmbclient-php

Eduardo Kienetz


## PHP interface

After loading this extension, the following functions will become available in PHP.

### URI's

URI's have the format `smb://[[[workgroup;]user[:password@]]server[/share[/path[/file]]]]`.
If you need to specify a workgroup, username or password, you must include them in the URI.
Examples of valid URI's:

```
smb://server
smb://server/share
smb://user:password@server/share/path/to/file.txt
```

### smbclient_state_new

```php
resource smbclient_state_new ( )
```

Acquire a new smbclient state.
Returns a state resource on success, or `false` on failure.
The state resource holds persistent data about the current server connection, so that the backend can reuse the existing channel instead of reconnecting for every operation.
The state resource must be passed on to most of the other functions in this extension.
Before using the state resource in other functions, it must be initialized by calling `smbclient_state_init`.
The state resource should be released when you're done with it by passing it to `smbclient_state_free` (although PHP will auto-destroy it when it goes out of scope).

### smbclient_state_init

```php
bool smbclient_state_init ( resource $state [, string $workgroup = null [, string $username = null [, string $password = null ] ] ] )
```

Initialize the smbclient state resource.
Returns `true` on success, `false` on failure.
Before using the state resource in other functions, it must be initialized.
Between creating and initializing the resource, you can set certain options for the connection.
Workgroup, username and password are optional parameters.
You can specify any of them as `null` or `false` to indicate that the credential is not available.
Such might be the case for anonymous or guest access.

### smbclient_state_free

```php
bool smbclient_state_free ( resource $state )
```

Release the state resource passed to it.
Returns `true` on success, `false` on failure.

### smbclient_opendir

```php
resource smbclient_opendir ( resource $state, string $uri )
```

Opens the given directory for reading with `smbclient_readdir`.

Returns either a directory resource, or `false` on failure.
The directory resource should be closed after use with `smbclient_closedir`.

### smbclient_readdir

```php
array smbclient_readdir ( resource $state, resource $dir )
```

Reads the next entry from the given directory resource obtained with `smbclient_opendir`.
Call this in a `while` loop to read all entries in the directory.

Returns an array with details for the directory entry on success, or `false` on
failure or end-of-file. The returned array has the following structure:

```php
array(
  'type'    => 'type string',
  'comment' => 'comment string',
  'name'    => 'name string'
)
```

Comment and name are passed through from libsmbclient.
`type` is one of the following strings:

* `'workgroup'`
* `'server'`
* `'file share'`
* `'printer share'`
* `'communication share'`
* `'IPC share'`
* `'directory'`
* `'file'`
* `'link'`
* `'unknown'`

### smbclient_closedir

```php
bool smbclient_closedir ( resource $state, resource $dir )
```

Closes a directory resource obtained with `smbclient_opendir`.
Returns `true` on success, `false` on failure.

### smbclient_rename

```php
bool smbclient_rename ( resource $state_old, string $uri_old, resource $state_new, string $uri_new )
```

Renames the old file/directory to the new file/directory.
`$state_old` and `$state_new` refer to the states belonging to the old and new URI's.
Due to a limitation of the underlying library, old and new locations must be on the same share.
Due to the same limitation, `$state_old` and `$state_new` should point to the same resource.
Returns `true` on success, `false` on failure.

### smbclient_unlink

```php
bool smbclient_unlink ( resource $state, string $uri )
```

Unlinks (deletes) the file.
Does not work on directories; to delete those, use `smbclient_rmdir`.
Returns `true` on success, `false` on failure.

### smbclient_mkdir

```php
bool smbclient_mkdir ( resource $state, string $uri [, int $mask = 0777 ] )
```

Creates the given directory.
If `$mask` is given, use that as the creation mask (after subtracting the [umask](http://php.net/manual/en/function.umask.php)).
Support for `$mask` may be absent; libsmbclient notes in its header file that umasks are not supported by SMB servers.
Returns `true` on success, `false` on failure.

### smbclient_rmdir

```php
bool smbclient_rmdir ( resource $state, string $uri )
```

Deletes the given directory if empty.
Returns `true` on success, `false` on failure.

### smbclient_stat

```php
array smbclient_stat ( resource $state, string $uri )
```

Returns information about the given file or directory.
Returns an array with information on success, `false` on failure.
The structure of the return array is the same as [PHP's native `stat`](http://php.net/manual/en/function.stat.php).
See that manual for a complete description.

### smbclient_open

```php
int smbclient_open ( resource $state, string $uri, string $mode [, int $mask = 0666 ] )
```

Opens a file for reading or writing according to the `$mode` specified.
Applies the creation mask in `$mask` (after subtracting the [umask](http://php.net/manual/en/function.umask.php)) if the file had to be created.
Support for `$mask` may be absent; libsmbclient notes in its header file that umasks are not supported by SMB servers.

`$mode` is in the same format as the `$mode` argument in [PHP's native `fopen`](http://php.net/manual/en/function.fopen.php).
See that manual for more information.
Summary:

value  | description
-----  | -----------
`'r'`  | open read-only, place file pointer at start of file.
`'r+'` | open read-write, place file pointer at start of file.
`'w'`  | open write-only, place file pointer at start of file; create file if not exists.
`'w+'` | as above, but open read-write.
`'a'`  | open write-only, place file pointer at end of file; create file if not exists.
`'a+'` | as above, but open read-write.
`'x'`  | exclusive open for write only; create file only if it doesn't already exist, else return error.
`'x+'` | as above, but open read-write.
`'c'`  | open write-only, create if not exists; if it already exists, don't return error. Do not truncate, but place file pointer at start of file.
`'c+'` | as above, but open read-write.

Returns a file handle on success, or `false` on failure.

### smbclient_creat

```php
int smbclient_creat ( resource $state, string $uri [, int $mask = 0666 ] )
```

Almost the same as calling `smbclient_open` with mode `'c'`, but will truncate the file to 0 bytes if it already exists.
Opens the file write-only and creates it if it doesn't already exist.

Returns a file handle on success, or `false` on failure.

### smbclient_read

```php
string smbclient_read ( resource $state, int $filehandle, int $bytes )
```

Reads data from a file handle obtained through `smbclient_open` or `smbclient_creat`.
Tries to read the amount of bytes given in `$bytes`, but may return less.
Returns a string longer than 0 bytes on success, a string of 0 bytes on end-of-file, or `false` on failure.

### smbclient_write

```php
int smbclient_write ( resource $state, int $filehandle, string $data [, int $length ] )
```

Writes data to a file handle obtained through `smbclient_open` or `smbclient_creat`.
If `$length` is not specified, write the whole contents of `$data`.
If `$length` is specified, write either the whole contents of `$data` or `$length` bytes, whichever is less.
`$length`, if specified, must be larger than 0.
If you want to write zero bytes for some reason, write the empty string and omit `$length`.

Returns the number of bytes written on success, or `false` on failure.

### smbclient_close

```php
bool smbclient_close ( resource $state, int $filehandle )
```

Close a file handle obtained with `smbclient_open` or `smbclient_creat`.
Returns `true` on success, `false` on failure.
