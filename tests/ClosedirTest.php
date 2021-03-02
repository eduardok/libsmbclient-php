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
		$this->assertFalse(smbclient_closedir($state));
	}

	/**
	 * @expectedException PHPUnit_Framework_Error_Warning
	 */
	public function
	testClosedirNull ()
	{
		$this->assertTrue(smbclient_closedir($state, null));
	}

	/**
	 * @expectedException PHPUnit_Framework_Error_Warning
	 */
	public function
	testClosedirDouble ()
	{
		$dir = smbclient_opendir($state, 'smb://'.SMB_HOST.'/'.SMB_SHARE);
		$this->assertTrue(smbclient_closedir($state, $dir));
		$this->assertFalse(smbclient_closedir($state, $dir));
	}
}
