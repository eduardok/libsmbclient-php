<?php declare(strict_types=1);
use PHPUnit\Framework\TestCase;

final class StateFreeTest extends TestCase
{
	public function
	testStateFreeValid ()
	{
		$state = smbclient_state_new();
		$this->assertTrue(smbclient_state_free($state));
	}

	/**
	 * @expectedException PHPUnit_Framework_Error_Warning
	 */
	public function
	testStateFreeEmpty ()
	{
                error_reporting(0);
		$this->assertFalse(@smbclient_state_free());
	}

	/**
	 * @expectedException PHPUnit_Framework_Error_Warning
	 */
	public function
	testStateFreeNull ()
	{
                error_reporting(0);
		$this->assertFalse(@smbclient_state_free(null));
	}

	/**
	 * @expectedException PHPUnit_Framework_Error_Warning
	 */
	public function
	testStateFreeDouble ()
	{
		$state = smbclient_state_new();
		$this->assertTrue(smbclient_state_free($state));
                error_reporting(0);
		$this->assertFalse(@smbclient_state_free($state));
	}
}
