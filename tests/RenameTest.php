<?php

class RenameTest extends PHPUnit_Framework_TestCase
{
	public function
	testRenameFileSuccess ()
	{
		$state = smbclient_state_new();
		smbclient_state_init($state, null, 'testuser', 'password');

		// Create mock file "foo" via regular filesystem:
		file_put_contents('/home/testuser/testshare/foo', 'this is a test');

		$this->assertTrue(smbclient_rename($state, 'smb://localhost/testshare/foo', $state, 'smb://localhost/testshare/bar'));

		$moved = is_file('/home/testuser/testshare/bar');
		$this->assertTrue($moved);

		$gone = is_file('/home/testuser/testshare/foo');
		$this->assertFalse($gone);

		if ($moved) {
			$this->assertEquals('this is a test', file_get_contents('/home/testuser/testshare/bar'));
		}
		@unlink('/home/testuser/testshare/foo');
		@unlink('/home/testuser/testshare/bar');
	}

	/**
	 * @expectedException PHPUnit_Framework_Error_Warning
	 */
	public function
	testRenameFileNotExists ()
	{
		$state = smbclient_state_new();
		smbclient_state_init($state, null, 'testuser', 'password');
		$this->assertFalse(smbclient_rename($state, 'smb://localhost/testshare/does-not-exist', $state, 'smb://localhost/testshare/somewhere-else'));
		$this->assertEquals(2, smbclient_state_errno($state));	// ENOENT
	}

	public function
	testRenameDirSuccess ()
	{
		$state = smbclient_state_new();
		smbclient_state_init($state, null, 'testuser', 'password');

		// Create mock dir "foo" via regular filesystem:
		mkdir('/home/testuser/testshare/foo');

		$this->assertTrue(smbclient_rename($state, 'smb://localhost/testshare/foo', $state, 'smb://localhost/testshare/bar'));

		$moved = is_dir('/home/testuser/testshare/bar');
		$this->assertTrue($moved);

		$gone = is_dir('/home/testuser/testshare/foo');
		$this->assertFalse($gone);

		@rmdir('/home/testuser/testshare/foo');
		@rmdir('/home/testuser/testshare/bar');
	}
}
