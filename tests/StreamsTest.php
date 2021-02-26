<?php

class StreamsTest extends PHPUnit_Framework_TestCase
{
	private $readuri;
	private $writeuri;
	private $realfile;
	private $testdata =
		"Lorem ipsum dolor sit amet, consectetur adipisicing elit,
		sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.\n";

	public function
	setup()
	{
		$this->readuri  = 'smb://'.SMB_USER.':'.SMB_PASS.'@'.SMB_HOST.'/'.SMB_SHARE.'/testdir/testfile.txt';
		$this->realread = SMB_LOCAL . '/testdir/testfile.txt';

		$this->writeuri = 'smb://'.SMB_USER.':'.SMB_PASS.'@'.SMB_HOST.'/'.SMB_SHARE.'/streamfile.txt';
		$this->writealt = 'smb://'.SMB_HOST.'/'.SMB_SHARE.'/streamfile.txt';
		$this->realfile = SMB_LOCAL . '/streamfile.txt';

		$this->diruri   = 'smb://'.SMB_USER.':'.SMB_PASS.'@'.SMB_HOST.'/'.SMB_SHARE.'/streamdir';
		$this->realdir  = SMB_LOCAL . '/streamdir';
	}

	public function
	tearDown()
	{
		@unlink($this->realfile);
		@rmdir($this->realdir);
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

		$this->assertTrue(ftruncate($fic,42));
		$stat = fstat($fic);
		$this->assertEquals(42, $stat['size']);

		$this->assertTrue(fclose($fic));
	}

	public function
	testPutContents ()
	{
		$len = file_put_contents($this->writeuri, $this->testdata);
		$this->assertEquals(strlen($this->testdata), $len);
		$this->assertFileExists($this->realfile);
	}

	public function
	testPutContentsContext ()
	{
		$context = stream_context_create(array('smb' => array(
			'username' => SMB_USER,
			'password' => SMB_PASS
		)));
		$len = file_put_contents($this->writealt, $this->testdata, 0, $context);
		$this->assertEquals(strlen($this->testdata), $len);
		$this->assertFileExists($this->realfile);
	}

	public function
	testGetContents ()
	{
		$data = file_get_contents($this->readuri);
		$this->assertTrue(strlen($data) > 0);
	}

	public function
	testUnlink ()
	{
		$this->assertTrue(copy($this->readuri, $this->writeuri));
		$this->assertFileExists($this->realfile);
		$this->assertTrue(unlink($this->writeuri));
		$this->assertFileNotExists($this->realfile);
	}

	public function
	testMkdirRmdir ()
	{
		$this->assertTrue(mkdir($this->diruri));
		$this->assertTrue(is_dir($this->realdir), "Directory exists");
		$this->assertTrue(rmdir($this->diruri));
		clearstatcache();
		$this->assertFalse(is_dir($this->realdir), "Directory not exists");
	}

	public function
	testRenameFile ()
	{
		$this->assertTrue(copy($this->readuri, $this->writeuri));
		$this->assertTrue(rename($this->writeuri, $this->writeuri.'2'));
		$this->assertFileExists($this->realfile.'2');
		$this->assertTrue(unlink($this->writeuri.'2'));
	}

	public function
	testRenameDir ()
	{
		$this->assertTrue(mkdir($this->diruri));
		$this->assertTrue(is_dir($this->realdir), "Directory exists");
		$this->assertTrue(rename($this->diruri, $this->diruri.'2'));
		$this->assertTrue(is_dir($this->realdir.'2'), "Directory exists");
		$this->assertTrue(rmdir($this->diruri.'2'));
	}

	public function
	testReaddir ()
	{
		$local = scandir(dirname($this->realdir));
		$smb   = scandir(dirname($this->diruri));

		$this->assertEquals($local, $smb);
	}

	public function
	testStat ()
	{
		$stat = stat($this->readuri);
		$this->assertCount(26, $stat);
		$this->assertArrayHasKey('atime', $stat);
		$this->assertArrayHasKey('mtime', $stat);
		$this->assertArrayHasKey('ctime', $stat);
		$this->assertArrayHasKey('size',  $stat);

		$local = stat($this->realread);
		$this->assertEquals($local['size'], $stat['size']);
		/* can't compare other values as local/remote are different */
	}

	public function
	testChmod ()
	{
		if (version_compare(PHP_VERSION, '5.4.0', '<')) {
			$this->markTestSkipped("PHP > 5.4 needed");
		}
		$this->assertTrue(copy($this->readuri, $this->writeuri));
		$old = fileperms($this->writeuri) & 0777;

		/* noop only test */
		$this->assertTrue(chmod($this->writeuri, $old));
		$new = fileperms($this->writeuri) & 0777;
		$this->assertEquals($old, $new);
	}

	public function
	testTouch ()
	{
		if (version_compare(PHP_VERSION, '5.4.0', '<')) {
			$this->markTestSkipped("PHP > 5.4 needed");
		}
		$mt = mktime(0,0,0,9,5,2015);
		$at = mktime(0,0,0,9,6,2015);
		$this->assertTrue(touch($this->writeuri, $mt, $at));
		$this->assertFileExists($this->realfile);

		$stat = stat($this->writeuri);
		$this->assertEquals($mt, $stat['mtime'], "mtime");
		$this->assertEquals($at, $stat['atime'], "atime");
	}

	public function
	testFileinfo ()
	{
		if (!extension_loaded('fileinfo')) {
			$this->markTestSkipped('Fileinfo extension needed');
		}
		$fi = new finfo(FILEINFO_MIME_TYPE);
		$this->assertEquals('text/plain', $fi->file($this->readuri));
	}

	public function
	testListShares ()
	{
		$smb = scandir('smb://'.SMB_USER.':'.SMB_PASS.'@'.SMB_HOST.'/');

		$this->assertContains(SMB_SHARE, $smb, print_r($smb, true));
	}
}
