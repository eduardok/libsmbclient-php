<?php

class GetxattrTest extends PHPUnit_Framework_TestCase
{
	public function
	testGetxattrFileSuccess ()
	{
		$state = smbclient_state_new();
		smbclient_state_init($state, null, 'testuser', 'password');
		$attr = smbclient_getxattr($state, 'smb://localhost/testshare/testdir/testfile.txt', 'system.*');
		$this->assertTrue(is_string($attr));
	}

	public function
	testGetxattrDirSuccess ()
	{
		$state = smbclient_state_new();
		smbclient_state_init($state, null, 'testuser', 'password');
		$attr = smbclient_getxattr($state, 'smb://localhost/testshare/testdir', 'system.*');
		$this->assertTrue(is_string($attr));
	}

	public function
	testGetxattrShareSuccess ()
	{
		$state = smbclient_state_new();
		smbclient_state_init($state, null, 'testuser', 'password');
		$attr = smbclient_getxattr($state, 'smb://localhost/testshare', 'system.*');
		$this->assertTrue(is_string($attr));
	}

	/**
	 * @expectedException PHPUnit_Framework_Error_Warning
	 */
	public function
	testGetxattrServer ()
	{
		$state = smbclient_state_new();
		smbclient_state_init($state, null, 'testuser', 'password');
		$attr = smbclient_getxattr($state, 'smb://localhost', 'system.*');
		$this->assertFalse($attr);
	}

	/**
	 * @expectedException PHPUnit_Framework_Error_Warning
	 */
	public function
	testGetxattrFileNotFound ()
	{
		$state = smbclient_state_new();
		smbclient_state_init($state, null, 'testuser', 'password');
		$attr = smbclient_getxattr($state, 'smb://localhost/testshare/testdir/does-not-exist', 'system.dos_attr.mode');
		$this->assertFalse($attr);
		$this->assertEquals(smbclient_state_errno($state), 2);	// ENOENT
	}
}
