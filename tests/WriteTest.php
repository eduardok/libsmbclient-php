<?php

require_once dirname(__FILE__) . '/config.php.dist';

class WriteTest extends PHPUnit_Framework_TestCase
{
	private $testdata =
		"Lorem ipsum dolor sit amet, consectetur adipisicing elit,
		sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.\n";

	// The URI of the test file seen through Samba:
	private $testuri = 'smb://'.SMB_HOST.'/'.SMB_SHARE.'/writetest.txt';

	// The "real" file on the filesystem:
	private $realfile = SMB_LOCAL.'/writetest.txt';

	public function
	testWriteSuccess ()
	{
		$state = smbclient_state_new();
		smbclient_state_init($state, null, SMB_USER, SMB_PASS);
		$file = smbclient_creat($state, $this->testuri);
		$this->assertTrue(is_resource($file));

		$ret = smbclient_write($state, $file, $this->testdata);
		$this->assertTrue(is_integer($ret));

		$this->assertEquals(strlen($this->testdata), $ret);

		// Use the regular filesystem to check the file that was created:
		$this->assertFileExists($this->realfile);
		$this->assertEquals(filesize($this->realfile), $ret);

		$this->assertStringEqualsFile($this->realfile, $this->testdata);
	}

	public function
	testWritePartial ()
	{
		$state = smbclient_state_new();
		smbclient_state_init($state, null, SMB_USER, SMB_PASS);
		$file = smbclient_creat($state, $this->testuri);
		$this->assertTrue(is_resource($file));

		// Write just 30 bytes:
		$ret = smbclient_write($state, $file, $this->testdata, 30);
		$this->assertEquals(30, $ret);

		// Use the regular filesystem to check the file that was created:
		$this->assertFileExists($this->realfile);
		$this->assertEquals(filesize($this->realfile), $ret);

		$this->assertStringEqualsFile($this->realfile, substr($this->testdata, 0, 30));
	}

	public function
	testWriteOversized ()
	{
		$state = smbclient_state_new();
		smbclient_state_init($state, null, SMB_USER, SMB_PASS);
		$file = smbclient_creat($state, $this->testuri);
		$this->assertTrue(is_resource($file));

		// Write way more bytes than data:
		$ret = smbclient_write($state, $file, $this->testdata, 1000);
		$this->assertTrue(is_integer($ret));

		$this->assertEquals(strlen($this->testdata), $ret);

		// Use the regular filesystem to check the file that was created:
		$this->assertFileExists($this->realfile);
		$this->assertEquals(filesize($this->realfile), $ret);

		$this->assertStringEqualsFile($this->realfile, $this->testdata);
	}
}
