<?php

class ReaddirTest extends PHPUnit_Framework_TestCase
{
	public function
	testReaddirSuccess ()
	{
		$state = smbclient_state_new();
		smbclient_state_init($state, null, SMB_USER, SMB_PASS);
		$dir = smbclient_opendir($state, 'smb://'.SMB_HOST.'/'.SMB_SHARE);

		$smb = array();
		while (($out = smbclient_readdir($state, $dir)) !== false) {
			$smb[] = $out['name'];
		}
		// Compare this with a copy we get through the filesystem:
		$local = scandir(SMB_LOCAL);

		// Sort both arrays to ensure equality:
		sort($smb);
		sort($local);

		$this->assertEquals($smb, $local);
	}
}
