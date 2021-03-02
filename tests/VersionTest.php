<?php declare(strict_types=1);
use PHPUnit\Framework\TestCase;

final class VersionTest extends TestCase
{
	public function
	testVersion ()
	{
		$this->assertTrue(is_string(smbclient_version()));
	}

	public function
	testLibraryVersion ()
	{
		$this->assertTrue(is_string(smbclient_library_version()));
	}
}
