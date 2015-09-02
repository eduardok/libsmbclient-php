<?php

class CreateTest extends PHPUnit_Framework_TestCase
{
	public function
	testCreateSuccess ()
	{
		$testuri = 'smb://'.SMB_HOST.'/'.SMB_SHARE.'/writetest.txt';
		$realfile = SMB_LOCAL . '/writetest.txt';

		$state = smbclient_state_new();
		smbclient_state_init($state, null, SMB_USER, SMB_PASS);
		$file = smbclient_creat($state, $testuri);
		$this->assertTrue(is_resource($file));
		$this->assertFileExists($realfile);
		unlink($realfile);
	}

	public function
	testCreateUrlencodedSuccess ()
	{
		$testuri = 'smb://'.SMB_HOST.'/'.SMB_SHARE.'/ex%25%25%25ample.txt';
		$realfile = SMB_LOCAL . '/ex%%%ample.txt';

		$state = smbclient_state_new();
		smbclient_state_init($state, null, SMB_USER, SMB_PASS);
		$file = smbclient_creat($state, $testuri);
		$this->assertTrue(is_resource($file));
		$this->assertFileExists($realfile);
		#unlink($realfile);
	}
}
