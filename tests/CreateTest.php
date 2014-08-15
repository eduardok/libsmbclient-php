<?php

class CreateTest extends PHPUnit_Framework_TestCase
{
	public function
	testCreateSuccess ()
	{
		$testuri = 'smb://localhost/testshare/writetest.txt';
		$realfile = '/home/testuser/testshare/writetest.txt';

		$state = smbclient_state_new();
		smbclient_state_init($state, null, 'testuser', 'password');
		$file = smbclient_creat($state, $testuri);
		$this->assertTrue(is_resource($file));
		$this->assertFileExists($realfile);
		unlink($realfile);
	}

	public function
	testCreateUrlencodedSuccess ()
	{
		$testuri = 'smb://localhost/testshare/ex%25%25%25ample.txt';
		$realfile = '/home/testuser/testshare/ex%%%ample.txt';

		$state = smbclient_state_new();
		smbclient_state_init($state, null, 'testuser', 'password');
		$file = smbclient_creat($state, $testuri);
		$this->assertTrue(is_resource($file));
		$this->assertFileExists($realfile);
		unlink($realfile);
	}
}
