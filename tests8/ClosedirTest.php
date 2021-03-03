<?php declare(strict_types=1);
use PHPUnit\Framework\TestCase;

final class ClosedirTest extends TestCase
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
		try {
			@smbclient_closedir($state);
			$this->assertTrue(false);
		} catch (\ArgumentCountError $ae) {
			$this->assertTrue(true);
		}
	}

	/**
	 * @expectedException PHPUnit_Framework_Error_Warning
	 */
	public function
	testClosedirNull ()
	{
		$state = smbclient_state_new();
		smbclient_state_init($state, null, SMB_USER, SMB_PASS);
		try {
			@smbclient_closedir($state, null);
			$this->assertTrue(false);
		} catch (\TypeError $te) {
			$this->assertTrue(true);
		}
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
		try {
			@smbclient_closedir($state, $dir);
			$this->assertTrue(false);
		} catch (\TypeError $te) {
			$this->assertTrue(true);
		}
	}
}
