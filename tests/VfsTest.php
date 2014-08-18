<?php

class VfsTest extends PHPUnit_Framework_TestCase
{
	private $testuri = 'smb://localhost/testshare/testdir/testfile.txt';

	public function
	testHaveConstants ()
	{
		$this->assertTrue(defined('SMBCLIENT_VFS_RDONLY'));
		$this->assertTrue(defined('SMBCLIENT_VFS_DFS'));
		$this->assertTrue(defined('SMBCLIENT_VFS_CASE_INSENSITIVE'));
		$this->assertTrue(defined('SMBCLIENT_VFS_NO_UNIXCIFS'));
	}

	private function
	hasParts ($vfs)
	{
		$parts = array('bsize','frsize','blocks','bfree','bavail','files','ffree','favail','fsid','flag','namemax');
		foreach ($parts as $part) {
			$this->assertTrue(isset($vfs[$part]));
			$this->assertTrue(is_int($vfs[$part]));
		}
	}

	public function
	testStatVFS ()
	{
		$state = smbclient_state_new();
		smbclient_state_init($state, null, 'testuser', 'password');

		$vfs = smbclient_statvfs($state, $this->testuri);

		smbclient_state_free($state);

		$this->assertTrue(is_array($vfs));
		$this->hasParts($vfs);
	}

	public function
	testFstatVFS ()
	{
		$state = smbclient_state_new();
		smbclient_state_init($state, null, 'testuser', 'password');
		$file = smbclient_open($state, $this->testuri, 'r');

		$vfs = smbclient_fstatvfs($state, $file);

		smbclient_close($state, $file);
		smbclient_state_free($state);

		$this->assertTrue(is_array($vfs));
		$this->hasParts($vfs);
	}
}
