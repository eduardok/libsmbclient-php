<?php declare(strict_types=1);
use PHPUnit\Framework\TestCase;

final class StateNewTest extends TestCase
{
	public function
	testStateNew ()
	{
		$state = smbclient_state_new();
		$this->assertTrue(is_resource($state));
	}
}
