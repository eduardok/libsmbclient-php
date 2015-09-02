<?php

require_once dirname(__FILE__) . '/config.php.dist';

class StateNewTest extends PHPUnit_Framework_TestCase
{
	public function
	testStateNew ()
	{
		$state = smbclient_state_new();
		$this->assertTrue(is_resource($state));
	}
}
