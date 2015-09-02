<?php

class RenameTest extends PHPUnit_Framework_TestCase
{
	public function
	testRenameFileSuccess ()
	{
		$state = smbclient_state_new();
		smbclient_state_init($state, null, SMB_USER, SMB_PASS);

		// Create mock file "foo" via regular filesystem:
		file_put_contents(SMB_LOCAL.'/foo', 'this is a test');

		$this->assertTrue(smbclient_rename($state, 'smb://'.SMB_HOST.'/'.SMB_SHARE.'/foo', $state, 'smb://'.SMB_HOST.'/'.SMB_SHARE.'/bar'));

		$moved = is_file(SMB_LOCAL.'/bar');
		$this->assertTrue($moved);

		$gone = is_file(SMB_LOCAL.'/foo');
		$this->assertFalse($gone);

		if ($moved) {
			$this->assertEquals('this is a test', file_get_contents(SMB_LOCAL.'/bar'));
		}
		@unlink(SMB_LOCAL.'/foo');
		@unlink(SMB_LOCAL.'/bar');
	}

	/**
	 * @expectedException PHPUnit_Framework_Error_Warning
	 */
	public function
	testRenameFileNotExists ()
	{
		$state = smbclient_state_new();
		smbclient_state_init($state, null, SMB_USER, SMB_PASS);
		$this->assertFalse(smbclient_rename($state, 'smb://'.SMB_HOST.'/'.SMB_SHARE.'/does-not-exist', $state, 'smb://'.SMB_HOST.'/'.SMB_SHARE.'/somewhere-else'));
		$this->assertEquals(2, smbclient_state_errno($state));	// ENOENT
	}

	public function
	testRenameDirSuccess ()
	{
		$state = smbclient_state_new();
		smbclient_state_init($state, null, SMB_USER, SMB_PASS);

		// Create mock dir "foo" via regular filesystem:
		mkdir(SMB_LOCAL.'/foo');

		$this->assertTrue(smbclient_rename($state, 'smb://'.SMB_HOST.'/'.SMB_SHARE.'/foo', $state, 'smb://'.SMB_HOST.'/'.SMB_SHARE.'/bar'));

		$moved = is_dir(SMB_LOCAL.'/bar');
		$this->assertTrue($moved);

		$gone = is_dir(SMB_LOCAL.'/foo');
		$this->assertFalse($gone);

		@rmdir(SMB_LOCAL.'/foo');
		@rmdir(SMB_LOCAL.'/bar');
	}
}
