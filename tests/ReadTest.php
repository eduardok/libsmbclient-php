<?php

class ReadTest extends PHPUnit_Framework_TestCase
{
	private $testdata =
		"Lorem ipsum dolor sit amet, consectetur adipisicing elit,
		sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.\n";

	// The URI of the test file seen through Samba:
	private $testuri;

	// The "real" file on the filesystem:
	private $realfile;

	public function
	setup()
	{
		$this->testuri = 'smb://'.SMB_HOST.'/'.SMB_SHARE.'/readtest.txt';
		$this->realfile = SMB_LOCAL.'/readtest.txt';

		file_put_contents($this->realfile, $this->testdata);
	}

	public function
	tearDown()
	{
		@unlink($this->realfile);
	}

	public function
	testReadSuccess()
	{
		$state = smbclient_state_new();
		smbclient_state_init($state, null, SMB_USER, SMB_PASS);
		$file = smbclient_open($state, $this->testuri, 'r');
		$this->assertTrue(is_resource($file));

		for ($data = ''; $tmp = smbclient_read($state, $file, 42) ; $data .= $tmp) {
			$this->assertTrue(\strlen($tmp) > 0 && \strlen($tmp) <= 42);
		}
		$this->assertEmpty($tmp);
		$this->assertEquals($this->testdata, $data);
	}
}
