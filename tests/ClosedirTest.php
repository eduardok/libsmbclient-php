<?php

class ClosedirTest extends PHPUnit_Framework_TestCase
{
	public function
	testClosedirSuccess ()
	{
		$state = smbclient_state_new();
		smbclient_state_init($state, null, SMB_USER, SMB_PASS);
		$dir = smbclient_opendir($state, 'smb://'.SMB_HOST.'/'.SMB_SHARE);
		$this->assertTrue(smbclient_closedir($state, $dir));
	}

	/**
	 * @expectedException PHPUnit_Framework_Error_Warning
	 */
	public function
	testClosedirEmpty ()
	{
		$state = smbclient_state_new();
		smbclient_state_init($state, null, SMB_USER, SMB_PASS);
		$this->assertFalse(smbclient_closedir($state));
	}

	/**
	 * @expectedException PHPUnit_Framework_Error_Warning
	 */
	public function
	testClosedirNull ()
	{
		$state = smbclient_state_new();
		smbclient_state_init($state, null, SMB_USER, SMB_PASS);
		$this->assertTrue(smbclient_closedir($state, null));
	}

	/**
	 * @expectedException PHPUnit_Framework_Error_Warning
	 */
	public function
	testClosedirDouble ()
	{
		$state = smbclient_state_new();
		smbclient_state_init($state, null, SMB_USER, SMB_PASS);
		$dir = smbclient_opendir($state, 'smb://'.SMB_HOST.'/'.SMB_SHARE);
		$this->assertTrue(smbclient_closedir($state, $dir));
		$this->assertFalse(smbclient_closedir($state, $dir));
	}
}
