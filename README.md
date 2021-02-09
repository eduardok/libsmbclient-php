libsmbclient-php: a PHP wrapper for libsmbclient
================================================

libsmbclient-php is a PHP extension that uses Samba's libsmbclient library
to provide Samba related functions to PHP programs.

[![Build Status](https://travis-ci.org/eduardok/libsmbclient-php.svg)](https://travis-ci.org/eduardok/libsmbclient-php)

Getting started
---------------

### Installation from PECL

```sh
pecl install smbclient
```


### Binary package installation

Some distributions provide binary packages:

* RPM for Fedora / RHEL / CentOS: [php-smbclient](https://apps.fedoraproject.org/packages/php-smbclient)
* DEB for Debian: [php-smbclient](https://packages.debian.org/search?keywords=php-smbclient)
* DEB for Ubuntu: [php-smbclient](http://packages.ubuntu.com/search?keywords=php-smbclient)


### Installation from sources

- Install the required libsmbclient header files, via package libsmbclient-dev (Debian/Ubuntu) or libsmbclient-devel (CentOS/Fedora/Red Hat).

- Download a [release tarball](https://github.com/eduardok/libsmbclient-php/releases) or check out the source code using git:

```sh
git clone git://github.com/eduardok/libsmbclient-php.git
```

- phpize it:

```sh
cd libsmbclient-php ; phpize
```

- Build the module

```sh
./configure
make
```

- As root, install the module into the extensions directory:

```sh
sudo make install
```

- Or for packaging purposes, install to a specific root directory:

```sh
make install INSTALL_ROOT=/tmp/smbc
```

- Activate libsmbclient-php in php.ini:

```sh
extension="smbclient.so"
```

Contributions and bug reports
-----------------------------

If you encounter a bug or want to contribute, please file an [issue](https://github.com/eduardok/libsmbclient-php/issues) on GitHub.
Sending pull requests on GitHub is the preferred method of contributing code, because Travis CI will automatically build and test your pull request.

## License

Since version 0.7.0, libsmbclient-php is licensed under the [BSD 2-clause](http://opensource.org/licenses/BSD-2-Clause) license.
See [Issue #15](https://github.com/eduardok/libsmbclient-php/issues/15) for background.
The full license text can be found in the `LICENSE` file.
Before that, libsmbclient-php was licensed under the [PHP license, version 2.02](http://www.php.net/license/2_02.txt).

## PHP interface

### URI's

URI's have the format `smb://[[[workgroup;]user[:password@]]server[/share[/path[/file]]]]`.
They should be urlencoded to escape special characters.
Use PHP's [`rawurlencode`](http://php.net/manual/en/function.rawurlencode.php) function to encode an URI.
If you need to specify a workgroup, username or password, you can either include them in the URI, or specify them when you create a state resource.
Examples of valid URI's:

```
smb://server
smb://server/share
smb://user:password@server/share/path/to/file.txt
smb://server/share/Moscow%20is%20written%20%D0%9C%D0%BE%D1%81%D0%BA%D0%B2%D0%B0.txt
```

### Error handling

As a low-level extension, libsmbclient-php does not throw exceptions.
Success or failure is communicated the old-fashioned way, by the function's return value.
You should always check if a function returns `false` for failure.
If you really want exceptions, you can build your own high-level layer by translating return values and error codes to appropriate exceptions.

For errors that occur in the Samba layer, the `smbclient_` functions will generally print a human-readable PHP warning with an interpretation of what went wrong.
The interpretations come from Samba's `libsmbclient.h` header file.
You can suppress the warnings by prefixing the function call with an `@`.
Please don't attempt to parse the warning messages, their wording is not very consistent and likely to change in future versions.

For some unlikely errors encountered by the extension itself, no warning is printed and the function just returns `false`.

When an error occurs, you can get the error number with `smbclient_state_errno`.
For errors outside of Samba (such as wrong arguments to the function), its value will be 0, but for errors originating within Samba, it will be a Unix `errno` value straight from the underlying library.
For example, `smbclient_open` may set the error code to `13`, which corresponds with `EACCES`, which means that permission was denied.
Please refer to Samba's `libsmbclient.h` for documentation on which error codes you can expect to see; each function has its own list of things that can go wrong.

For convenience, here's a non-exhaustive list of popular error codes:

name | value | description
---- | ----- | -----------
`EPERM` | 1 | Operation not permitted
`ENOENT` | 2 | No such file or directory
`EBADF` | 9 | Bad file or directory resource
`ENOMEM` | 12 | Out of memory
`EACCES` | 13 | Permission denied
`EBUSY` | 16 | Device or resource busy
`EEXIST` | 17 | Resource exists
`ENOTDIR` | 20 | Not a directory
`EISDIR` | 21 | Is a directory
`EINVAL` | 22 | Invalid argument
`ENOSPC` | 28 | No space left on device
`ENOTEMPTY` | 39 | Directory not empty
`ECONNREFUSED` | 111 | Connection refused (Samba not running?)

### smbclient_version

```php
string smbclient_version ( )
```

Returns libsmbclient-php's own version string.

### smbclient_library_version

```php
string smbclient_library_version ( )
```

Returns libsmbclient's version string, which is the same as the Samba version string.

### smbclient_state_new

```php
resource smbclient_state_new ( )
```

Acquire a new smbclient state.
Returns a state resource on success, or `false` on failure.
The state resource holds persistent data about the current server connection, so that the backend can reuse the existing channel instead of reconnecting for every operation.
The state resource must be passed on to most of the other functions in this extension.
Before using the state resource in other functions, it must be initialized by calling `smbclient_state_init`.
Between creating and initializing the resource, you can set certain options for the connection with `smbclient_option_set`.
The state resource should be released when you're done with it by passing it to `smbclient_state_free` (although PHP will auto-destroy it when it goes out of scope).

### smbclient_client_protocols

```php
bool smbclient_client_protocols ( resource $state, string $min_protocol = null, string $max_protocol = null )
```

Sets the minimum and maximum protocols (client min protocol and client max protocol) for negotiation.
Either can be set to null.
Returns `true` on success, `false` on failure.

### smbclient_option_set

```php
bool smbclient_option_set ( resource $state, int option, mixed value )
```

Sets the value of an option to `libsmbclient`.
Returns `true` if setting the option succeeded, `false` on failure.
This function should be called before calling `smbclient_state_init` on your context.
The second argument should be one of the constants below:

* `SMBCLIENT_OPT_OPEN_SHAREMODE`

  The share mode to use when opening files.
  The value can be one of these constants:
  * `SMBCLIENT_SHAREMODE_DENY_DOS`
  * `SMBCLIENT_SHAREMODE_DENY_ALL`
  * `SMBCLIENT_SHAREMODE_DENY_WRITE`
  * `SMBCLIENT_SHAREMODE_DENY_READ`
  * `SMBCLIENT_SHAREMODE_DENY_NONE`
  * `SMBCLIENT_SHAREMODE_DENY_FCB`

  The default is `SMBCLIENT_SHAREMODE_DENY_NONE`.

* `SMBCLIENT_OPT_ENCRYPT_LEVEL`

  The encryption level to adopt for the connection.
  The value can be one of these constants:
  * `SMBCLIENT_ENCRYPTLEVEL_NONE`
  * `SMBCLIENT_ENCRYPTLEVEL_REQUEST`
  * `SMBCLIENT_ENCRYPTLEVEL_REQUIRE`

* `SMBCLIENT_OPT_CASE_SENSITIVE`

  Boolean.
  What to do when we can't determine from the file system attributes whether the file system is case sensitive.
  Assume that the filesystem is case sensitive (`true`), or that it isn't (`false`).
  Defaults to `false`, because only really old file systems aren't autodetected, and most of those are case insensitive.

* `SMBCLIENT_OPT_BROWSE_MAX_LMB_COUNT`

  From how many servers to retrieve the list of workgroups, if you're doing that.
  See Samba's `libsmbclient.h` for details.

* `SMBCLIENT_OPT_URLENCODE_READDIR_ENTRIES`

  Boolean.
  Whether the entries returned by `smbclient_readdir` are urlencoded.
  Defaults to `false`, the entries are returned "raw".

* `SMBCLIENT_OPT_USE_KERBEROS`

  Boolean.
  Whether to use Kerberos authentication.

* `SMBCLIENT_OPT_FALLBACK_AFTER_KERBEROS`

  Boolean.
  Whether to fall back on regular authentication if Kerberos didn't work out.
  The regular username and password given in `smbclient_state_init` will be queried.

* `SMBCLIENT_OPT_AUTO_ANONYMOUS_LOGIN`

  Boolean.
  Whether to automatically select anonymous login.

* `SMBCLIENT_OPT_USE_CCACHE`

  Boolean.
  Whether to use the Winbind cache.

* `SMBCLIENT_OPT_USE_NT_HASH`

  Boolean.
  Whether the password supplied in `smbclient_state_init` is actually an NT hash.
  If you set this to `true` and work with NT hashes, you can avoid passing around plaintext passwords.

  The `smbc_getOptionUseNTHash` function is relatively new to libsmbclient (June 2012), so the configure script tests whether your libsmbclient has that symbol, and conditionally activates this option.
  If the option is not available, trying to set it will return `false`.

* `SMBCLIENT_OPT_NETBIOS_NAME`

  String.
  The NetBIOS (host) name used for making connections.

* `SMBCLIENT_OPT_WORKGROUP`

  String.
  The workgroup used for making connections.

* `SMBCLIENT_OPT_USER`

  String.
  The username used to make connections.
  This appears to be something different from the username given in `smbclient_state_init`, and appears to correspond to the system user running PHP.

* `SMBCLIENT_OPT_PORT`

  Int.
  The TCP port to connect to.
  `0` means "use the default".

  The `smbc_setPort` function is relatively new to libsmbclient (April 2013), so the configure script tests whether your libsmbclient has that symbol, and conditionally activates this option.
  If the option is not available, trying to set it will return `false`.

* `SMBCLIENT_OPT_TIMEOUT`

  Int.
  The timeout value for connections and responses in milliseconds.

### smbclient_option_get

```php
mixed smbclient_option_get ( resource $state, int option )
```

This is a mirror function of `smbclient_option_set`.
Everything settable is also gettable.
See that function for the description of the available options and their return types/values.
If a given option is not available, this function will return `null` and not `false`, to distinguish it from an option's legitimate `false` value.

### smbclient_state_init

```php
int smbclient_state_init ( resource $state [, string $workgroup = null [, string $username = null [, string $password = null ] ] ] )
```

Initialize the smbclient state resource.
Returns `0` on success, `1` on failure.
Before using the state resource in other functions, it must be initialized.
Workgroup, username and password are optional parameters.
You can specify any of them as `null` or `false` to indicate that the credential is not available.
Such might be the case for anonymous or guest access.

### smbclient_state_free

```php
bool smbclient_state_free ( resource $state )
```

Release the state resource passed to it.
Returns `true` on success, `false` on failure.

### smbclient_state_errno

```php
int smbclient_state_errno ( resource $state )
```

Returns the error number of the last error encountered by libsmbclient.
Returns 0 on failure (invalid resource) or if no error has yet occurred for this resource.
The numbers returned are the standard Posix constants as returned by libsmbclient itself, so check your system's `errno.h` or `man errno` for documentation.

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
By default, the name is *not* returned in urlencoded format, it's been decoded for convenience.
You can toggle that by setting the `SMBCLIENT_OPT_URLENCODE_READDIR_ENTRIES` option to `true`.
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

### smbclient_fstat

```php
array smbclient_fstat ( resource $state, resource $file )
```

Returns information about the given file or directory.
Returns an array with information on success, `false` on failure.
The structure of the return array is the same as [PHP's native `stat`](http://php.net/manual/en/function.stat.php).
See that manual for a complete description.

### smbclient_open

```php
resource smbclient_open ( resource $state, string $uri, string $mode [, int $mask = 0666 ] )
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

Returns a file resource on success, or `false` on failure.

### smbclient_creat

```php
resource smbclient_creat ( resource $state, string $uri [, int $mask = 0666 ] )
```

Almost the same as calling `smbclient_open` with mode `'c'`, but will truncate the file to 0 bytes if it already exists.
Opens the file write-only and creates it if it doesn't already exist.

Returns a file resource on success, or `false` on failure.

### smbclient_read

```php
string smbclient_read ( resource $state, resource $file, int $bytes )
```

Reads data from a file resource obtained through `smbclient_open` or `smbclient_creat`.
Tries to read the amount of bytes given in `$bytes`, but may return less.
Returns a string longer than 0 bytes on success, a string of 0 bytes on end-of-file, or `false` on failure.

Checking the string length to figure out EOF is primitive, but libsmbclient does not expose an `feof` equivalent.
`strlen` in PHP is relatively efficient because PHP tracks string lengths internally.

### smbclient_write

```php
int smbclient_write ( resource $state, resource $file, string $data [, int $length ] )
```

Writes data to a file resource obtained through `smbclient_open` or `smbclient_creat`.
If `$length` is not specified, write the whole contents of `$data`.
If `$length` is specified, write either the whole content of `$data` or `$length` bytes, whichever is less.
`$length`, if specified, must be larger than 0.
If you want to write zero bytes for some reason, write the empty string and omit `$length`.

Returns the number of bytes written on success, or `false` on failure.

### smbclient_lseek

```php
int smbclient_lseek ( resource $state, resource $file, int offset, int whence )
```

Places the internal file pointer at the given byte offset.
The `whence` parameter indicates from where to count.
It can take three possible constants, which are the same as for [PHP's native `fseek`](http://php.net/manual/en/function.fseek.php):

* `SEEK_SET`: set position equal to offset bytes;
* `SEEK_CUR`: set position to current location plus offset;
* `SEEK_END`: set position to end of file plus offset.

Returns the new file offset as measured from the start of the file on success, `false` on failure.

### smbclient_ftruncate

```php
bool smbclient_ftruncate ( resource $state, resource $file, int size )
```

Truncates the given file to the given file size.
Returns `true` on success, `false` on failure.

### smbclient_close

```php
bool smbclient_close ( resource $state, resource $file )
```

Close a file resource obtained with `smbclient_open` or `smbclient_creat`.
Returns `true` on success, `false` on failure.

### smbclient_chmod

```php
bool smbclient_chmod ( resource $state, string $uri, int mode )
```

Set the DOS attributes for a file.
According to the libsmbclient header file, this function is not implemented.
However, the Samba sources do seem to implement it, and use the following mapping:

Permission | description
---------- | -----------
Not u+w, g+w or o+w | File is read-only
u+x | File is archived
g+x | File is system
o+x | File is hidden

So to set a file to readable and hidden, you would use o+wx, or mode `003`.
This function is a Posix compatibility shim; if you want better control over file attributes, use the more powerful `xattr` functions.

### smbclient_utimes

```php
bool smbclient_utimes ( resource $state, string $uri [, int $mtime = time() [, int $atime = $mtime ] ] )
```

Set the write time and access time for the given file or directory.
These correspond to Unix mtime and atime.
Timestamps are in Unix timestamp format.
Returns `true` on success, `false` on failure.

Beware of inconsistencies in how Samba stores and retrieves timestamps.
When you change the mtime and atime for a file, then stat the file with `smbclient_stat`, the stat output will indicate that you changed ctime and mtime, in that order, instead.
(This is likely a bug somewhere, but it's hard to pinpoint the cause.)
When you use mount.cifs to mount the share and check the results of this function with the `stat` commandline tool, the `mtime` argument will set both the mtime and ctime, and the `atime` argument will set the atime.
This is a Posix compatibility shim.
Use the more powerful `xattr` functions if you need more control, such as setting the ctime.

### smbclient_listxattr

```php
array smbclient_listxattr ( resource $state, string $uri )
```

This function should, according to Samba documentation, return a list of all names of extended attributes applicable to the given file or directory.
Instead, the function returns an array of the names of all extended attributes known to Samba, regardless of what the filesystem actually supports or which attributes are actually available on the resource.
Since the underlying function always returns a static string without looking, don't take the output as gospel.
It does provide you with a list of attribute names that you can use to fetch individually.
Returns `false` on failure.

### smbclient_getxattr

```php
string smbclient_getxattr ( resource $state, string $uri, string $key )
```

Returns the value of the given extended attribute with name `$key`, or `false` on failure.
The value returned is always a string.

For example, to get a file's [extended attributes](http://msdn.microsoft.com/en-us/library/cc246322.aspx), query the `system.dos_attr.mode` key.

### smbclient_setxattr

```php
bool smbclient_setxattr ( resource $state, string $uri, string $key, string $value [, int flags = 0 ] )
```

Sets the extended attribute with name `$key` to value `$value`.
For now, see `libsmbclient.h`, the section on `smbc_setxattr`, for details on how to specify keys and values.
`flags` defaults to zero, meaning that the attribute will be created if it does not exist, and replaced if it already exists.
You can also set `flags` to one of these values:

Constant | description
-------- | -----------
`SMBCLIENT_XATTR_CREATE` | Only create the attribute: fail with `EEXIST` if it already exists
`SMBCLIENT_XATTR_REPLACE` | Only replace the attribute: fail with `ENOATTR` if it does not exist

Returns `true` on success, `false` on failure.

### smbclient_removexattr

```php
bool smbclient_removexattr ( resource $state, string $uri, string $key )
```

Removes the extended attribute with name `$key` from the file or directory pointed to by the URI.
Returns `true` on success, `false` on failure.

### smbclient_statvfs

```php
array smbclient_statvfs ( resource $state, string $uri )
```

Returns an array with file system statistics for the given URI, or `false` on failure.
The array contains the keys listed below, each with an integer value.
See the manpage for the Unix `statvfs` function for more information on how to interpret the values.

* `bsize`: file system block size;
* `frsize`: fragment size;
* `blocks`: size of filesystem in `frsize` units;
* `bfree`: number of free blocks;
* `bavail`: number of free blocks for unprivileged users;
* `files`: number of inodes;
* `ffree`: number of free inodes;
* `favail`: number of free inodes for unprivileged users;
* `fsid`: file system ID;
* `flag`: mount flags;
* `namemax`: maximum filename length.

The `flag` value can contain a boolean `OR` of the following constants:

* `SMBCLIENT_VFS_RDONLY`;
* `SMBCLIENT_VFS_DFS`;
* `SMBCLIENT_VFS_CASE_INSENSITIVE`;
* `SMBCLIENT_VFS_NO_UNIXCIFS`;

### smbclient_fstatvfs

```php
array smbclient_fstatvfs ( resource $state, resource $file_or_dir )
```

Returns an array with file system statistics for the given file or directory resource, or `false` on failure.
See `smbclient_statvfs` for a description of the returned array.

## streams support

Starting with version 0.8.0, streams support is enabled.
Most of standard functions work transparently with 'smb' URIs.

```php
  readfile('smb://user:password@smbserver/share/file.txt');
  scandir('smb://user:password@smbserver/share/somedir/');
```

Which include: copy, file_get_contents, file_put_contents, fileperms, fopen, mkdir, opendir, rmdir, rename, stat, unlink...
Notice: touch and chmod functions require PHP >= 5.4

## Examples

Some bare-bones examples of how to use libsmbclient-php.
These have deliberately been kept simple.
In production, you should at least check whether the extension has been loaded.
Also, you should urlencode your URI's, check the return value of each function, and handle errors appropriately.

List the contents of a directory:

```php
<?php

// Create new state:
$state = smbclient_state_new();

// Initialize the state with workgroup, username and password:
smbclient_state_init($state, null, 'testuser', 'password');

// Open a directory:
$dir = smbclient_opendir($state, 'smb://localhost/testshare');

// Loop over the directory contents, print each node:
while (($entry = smbclient_readdir($state, $dir)) !== false) {
	echo "{$entry['name']} : {$entry['type']}\n";
}
// Close the directory handle:
smbclient_closedir($state, $dir);

// Free the state:
smbclient_state_free($state);
```

Dump a file to standard output:

```php
<?php

// Create new state:
$state = smbclient_state_new();

// Initialize the state with workgroup, username and password:
smbclient_state_init($state, null, 'testuser', 'password');

// Open a file for reading:
$file = smbclient_open($state, 'smb://localhost/testshare/testdir/testfile.txt', 'r');

// Read the file incrementally, dump contents to output:
while (true) {
	$data = smbclient_read($state, $file, 100000);
	if ($data === false || strlen($data) === 0) {
		break;
	}
	echo $data;
}
// Close the file handle:
smbclient_close($state, $file);

// Free the state:
smbclient_state_free($state);
```

