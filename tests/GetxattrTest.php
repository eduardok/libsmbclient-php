<?php

class GetxattrTest extends PHPUnit_Framework_TestCase
{
	public function
	testGetxattrFileSuccess ()
	{
		$state = smbclient_state_new();
		smbclient_state_init($state, null, SMB_USER, SMB_PASS);
		$attr = smbclient_getxattr($state, 'smb://'.SMB_HOST.'/'.SMB_SHARE.'/testdir/testfile.txt', 'system.*');
		$this->assertTrue(is_string($attr) && strlen($attr));
	}

	public function
	testGetxattrDirSuccess ()
	{
		$state = smbclient_state_new();
		smbclient_state_init($state, null, SMB_USER, SMB_PASS);
		$attr = smbclient_getxattr($state, 'smb://'.SMB_HOST.'/'.SMB_SHARE.'/testdir', 'system.*');
		$this->assertTrue(is_string($attr) && strlen($attr));
	}

	public function
	testGetxattrShareSuccess ()
	{
		$state = smbclient_state_new();
		smbclient_state_init($state, null, SMB_USER, SMB_PASS);
		$attr = smbclient_getxattr($state, 'smb://'.SMB_HOST.'/'.SMB_SHARE, 'system.*');
		$this->assertTrue(is_string($attr) && strlen($attr));
	}

	/**
	 * @expectedException PHPUnit_Framework_Error_Warning
	 */
	public function
	testGetxattrServer ()
	{
		$state = smbclient_state_new();
		smbclient_state_init($state, null, SMB_USER, SMB_PASS);
		$attr = smbclient_getxattr($state, 'smb://'.SMB_HOST, 'system.*');
		$this->assertFalse($attr);
	}

	/**
	 * @expectedException PHPUnit_Framework_Error_Warning
	 */
	public function
	testGetxattrFileNotFound ()
	{
		$state = smbclient_state_new();
		smbclient_state_init($state, null, SMB_USER, SMB_PASS);
		$attr = smbclient_getxattr($state, 'smb://'.SMB_HOST.'/'.SMB_SHARE.'/testdir/does-not-exist', 'system.dos_attr.mode');
		$this->assertFalse($attr);
		$this->assertEquals(smbclient_state_errno($state), 2);	// ENOENT
	}
}
