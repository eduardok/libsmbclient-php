<?php

class StateInitTest extends PHPUnit_Framework_TestCase
{
	public function
	testStateInitValid ()
	{
		$state = smbclient_state_new();
		$this->assertTrue(smbclient_state_init($state, null, SMB_USER, SMB_PASS));
	}

	/**
	 * @expectedException PHPUnit_Framework_Error_Warning
	 */
	public function
	testStateInitInvalidState ()
	{
		$this->assertFalse(smbclient_state_init(null));
	}
}
