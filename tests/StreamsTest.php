<?php

class StreamsTest extends PHPUnit_Framework_TestCase
{
	private $readuri;
	private $writeuri;
	private $realfile;
	private $testdata =
		"Lorem ipsum dolor sit amet, consectetur adipisicing elit,
		sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.\n";

	public function setup() {
		$this->readuri  = 'smb://'.SMB_USER.':'.SMB_PASS.'@'.SMB_HOST.'/'.SMB_SHARE.'/testdir/testfile.txt';
		$this->writeuri = 'smb://'.SMB_USER.':'.SMB_PASS.'@'.SMB_HOST.'/'.SMB_SHARE.'/streamfile.txt';
		$this->realfile = SMB_LOCAL . '/streamfile.txt';
	}

	public function
	testPutContents ()
	{
		$len = file_put_contents($this->writeuri, $this->testdata);
		$this->assertEquals(strlen($this->testdata), $len);
		$this->assertFileExists($this->realfile);
		unlink($this->realfile);
	}

	public function
	testGetContents ()
	{
		$data = file_get_contents($this->readuri);
		$this->assertTrue(strlen($data) > 0);
	}
}
