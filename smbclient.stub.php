<?php

/**
 * @generate-function-entries
 * @generate-legacy-arginfo
 */

function smbclient_version(): string {}

function smbclient_library_version(): string {}

/**
 * @return false|resource
 */
function smbclient_state_new() {}

/**
 * @param resource $state
 */
function smbclient_state_init($state, string $workgroup=null, string $user=null, string $password=null): bool {}

/**
 * @param resource $state
 */
function smbclient_state_errno($state): int {}

/**
 * @param resource $state
 */
function smbclient_state_free($state): bool {}

/**
 * @param resource $state
 * @return mixed
 */
function smbclient_option_get($state, int $option) {}

/**
 * @param resource $state
 * @param mixed $value
 * @return mixed
 */
function smbclient_option_set($state, int $option, $value) {}


#if HAVE_SMBC_SETOPTIONPROTOCOLS
/**
 * @param resource $state
 * @param mixed $value
 * @return mixed
 */
function smbclient_client_protocols($state, string $minproto=null, string $maxproto=null): bool {}
#endif

/**
 * @param resource $state
 * @return false|resource
 */
function smbclient_opendir($state, string $path) {}

/**
 * @param resource $state
 * @param resource $dir
 */
function smbclient_readdir($state, $dir): false|array {}

/**
 * @param resource $state
 * @param resource $dir
 */
function smbclient_closedir($state, $dir): bool {}

/**
 * @param resource $state
 * @return false|resource
 */
function smbclient_stat($state, string $path): false|array {}

/**
 * @param resource $state
 * @param resource $file
 */
function smbclient_fstat($state, $file): false|array {}

/**
 * @param resource $state
 * @return false|resource
 */
function smbclient_open($state, string $path, string $flags, int $mode=0666) {}

/**
 * @param resource $state
 * @return false|resource
 */
function smbclient_creat($state, string $path, int $mode=0666) {}

/**
 * @param resource $state
 * @param resource $file
 */
function smbclient_read($state, $file, int $count): false|string {}

/**
 * @param resource $state
 * @param resource $file
 */
function smbclient_close($state, $file): bool {}

/**
 * @param resource $state
 */
function smbclient_mkdir($state, string $path, int $mode=0666): bool {}

/**
 * @param resource $state
 */
function smbclient_rmdir($state, string $path): bool {}

/**
 * @param resource $oldstate
 * @param resource $newstate
 */
function smbclient_rename($oldstate, string $oldpath, $newstate, string $newpath): bool {}

/**
 * @param resource $state
 * @param resource $file
 */
function smbclient_write($state, $file, string $buffer, int $count=0): false|int {}

/**
 * @param resource $state
 */
function smbclient_unlink($state, string $path): bool {}

/**
 * @param resource $state
 * @param resource $file
 */
function smbclient_lseek($state, $file, int $offset, int $whence): false|int {}

/**
 * @param resource $state
 * @param resource $file
 */
function smbclient_ftruncate($state, $file, int $offset): bool {}

/**
 * @param resource $state
 */
function smbclient_chmod($state, string $path, int $mode): bool {}

/**
 * @param resource $state
 */
function smbclient_utimes($state, string $path, int $mtime=-1, int $atime=-1): bool {}

/**
 * @param resource $state
 */
function smbclient_listxattr($state, string $path): false|array {}

/**
 * @param resource $state
 */
function smbclient_getxattr($state, string $path, string $name): false|string {}

/**
 * @param resource $state
 */
function smbclient_setxattr($state, string $path, string $name, string $value, int $flags=0): false|string {}

/**
 * @param resource $state
 */
function smbclient_removexattr($state, string $path, string $name): bool {}

/**
 * @param resource $state
 * @return false|resource
 */
function smbclient_statvfs($state, string $path): false|array {}

/**
 * @param resource $state
 * @param resource $file
 */
function smbclient_fstatvfs($state, $file): false|array {}

