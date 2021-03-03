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
		try {
			@smbclient_state_free();
			$this->assertFalse(true);
		} catch (\ArgumentCountError $ae) {
			$this->assertFalse(false);
		}
	}

	/**
	 * @expectedException PHPUnit_Framework_Error_Warning
	 */
	public function
	testStateFreeNull ()
	{
		try {
			@smbclient_state_free(null);
			$this->assertFalse(true);
		} catch (\TypeError $ae) {
			$this->assertFalse(false);
		}
	}

	/**
	 * @expectedException PHPUnit_Framework_Error_Warning
	 */
	public function
	testStateFreeDouble ()
	{
		$state = smbclient_state_new();
		$this->assertTrue(smbclient_state_free($state));
		try {
			@smbclient_state_free($state);
			$this->assertFalse(true);
		} catch (\TypeError $ae) {
			$this->assertFalse(false);
		}
	}
}
