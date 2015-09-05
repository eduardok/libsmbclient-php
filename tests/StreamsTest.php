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
	testRegistered ()
	{
		$this->assertContains('smb', stream_get_wrappers());
	}

	public function
	testReadWrite ()
	{
		$fic = fopen($this->writeuri, "w+");
		$this->assertTrue(is_resource($fic), "Get a resource");

		$len = fwrite($fic, $this->testdata);
		$this->assertEquals(strlen($this->testdata), $len);
		$this->assertEquals($len, ftell($fic));

		$this->assertEquals(3, fwrite($fic, "foo"));
		$this->assertEquals($len, fwrite($fic, $this->testdata));
		$this->assertEquals(2*$len+3, ftell($fic));

		$this->assertEquals(0, fseek($fic, $len, SEEK_SET));
		$this->assertEquals($len, ftell($fic));
		$this->assertEquals('foo', fread($fic, 3));

		$stat = fstat($fic);
		$this->assertCount(26, $stat);
		$this->assertArrayHasKey('atime', $stat);
		$this->assertArrayHasKey('mtime', $stat);
		$this->assertArrayHasKey('ctime', $stat);
		$this->assertArrayHasKey('size',  $stat);
		$this->assertEquals(2*$len+3, $stat['size']);

		$this->assertTrue(fclose($fic));
		unlink($this->realfile);
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
