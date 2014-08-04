<?php

class OpendirTest extends PHPUnit_Framework_TestCase
{
	public function
	testOpendirSuccess ()
	{
		$state = smbclient_state_new();
		smbclient_state_init($state, null, 'testuser', 'password');
		$dir = smbclient_opendir($state, 'smb://localhost/testshare/testdir');
		$this->assertTrue(is_resource($dir));
	}

	/**
	 * @expectedException PHPUnit_Framework_Error_Warning
	 */
	public function
	testOpendirInvalidState ()
	{
		$dir = smbclient_opendir(null, 'smb://localhost/testshare/does-not-exist');
		$this->assertFalse($dir);
	}

	/**
	 * @expectedException PHPUnit_Framework_Error_Warning
	 */
	public function
	testOpendirNotFound ()
	{
		$state = smbclient_state_new();
		smbclient_state_init($state, null, 'testuser', 'password');
		$dir = smbclient_opendir($state, 'smb://localhost/testshare/does-not-exist');
		$this->assertFalse($dir);
		$errno = smbclient_state_errno($state);
		$this->assertEquals(2, $errno); // ENOENT
	}

	/**
	 * @expectedException PHPUnit_Framework_Error_Warning
	 */
	public function
	testOpendirPermissionDenied ()
	{
		$state = smbclient_state_new();
		smbclient_state_init($state, null, 'testuser', 'password');
		$dir = smbclient_opendir($state, 'smb://localhost/testshare/noaccess');
		$this->assertFalse($dir);
		$errno = smbclient_state_errno($state);
		$this->assertEquals(13, $errno); // EACCES
	}

	/**
	 * @expectedException PHPUnit_Framework_Error_Warning
	 */
	public function
	testOpendirNotDir ()
	{
		$state = smbclient_state_new();
		smbclient_state_init($state, null, 'testuser', 'password');
		$dir = smbclient_opendir($state, 'smb://localhost/testshare/testfile.txt');
		$this->assertFalse($dir);
		$errno = smbclient_state_errno($state);
		$this->assertEquals(20, $errno); // ENOTDIR
	}
}
