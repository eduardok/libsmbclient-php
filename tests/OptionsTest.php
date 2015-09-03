<?php

class OptionsTest extends PHPUnit_Framework_TestCase
{
	public function
	testOptionConstantsExist ()
	{
		$this->assertTrue(is_long(SMBCLIENT_OPT_OPEN_SHAREMODE));
		$this->assertTrue(is_long(SMBCLIENT_OPT_ENCRYPT_LEVEL));
		$this->assertTrue(is_long(SMBCLIENT_OPT_CASE_SENSITIVE));
		$this->assertTrue(is_long(SMBCLIENT_OPT_BROWSE_MAX_LMB_COUNT));
		$this->assertTrue(is_long(SMBCLIENT_OPT_URLENCODE_READDIR_ENTRIES));
		$this->assertTrue(is_long(SMBCLIENT_OPT_USE_KERBEROS));
		$this->assertTrue(is_long(SMBCLIENT_OPT_FALLBACK_AFTER_KERBEROS));
		$this->assertTrue(is_long(SMBCLIENT_OPT_AUTO_ANONYMOUS_LOGIN));
		$this->assertTrue(is_long(SMBCLIENT_OPT_USE_CCACHE));
		$this->assertTrue(is_long(SMBCLIENT_OPT_USE_NT_HASH));
		$this->assertTrue(is_long(SMBCLIENT_OPT_NETBIOS_NAME));
		$this->assertTrue(is_long(SMBCLIENT_OPT_WORKGROUP));
		$this->assertTrue(is_long(SMBCLIENT_OPT_USER));
		$this->assertTrue(is_long(SMBCLIENT_OPT_PORT));
		$this->assertTrue(is_long(SMBCLIENT_OPT_TIMEOUT));
	}

	public function
	testOptionNotExists ()
	{
		$state = smbclient_state_new();
		$this->assertNull(smbclient_option_get($state, 9999));
		smbclient_state_free($state);
	}

	public function
	testOptionSetGetCaseSensitive ()
	{
		$state = smbclient_state_new();
		smbclient_state_init($state, null, SMB_USER, SMB_PASS);
		$this->assertTrue(smbclient_option_set($state, SMBCLIENT_OPT_CASE_SENSITIVE, true));
		$this->assertTrue(smbclient_option_get($state, SMBCLIENT_OPT_CASE_SENSITIVE));

		$this->assertTrue(smbclient_option_set($state, SMBCLIENT_OPT_CASE_SENSITIVE, false));
		$this->assertFalse(smbclient_option_get($state, SMBCLIENT_OPT_CASE_SENSITIVE));
		smbclient_state_free($state);
	}

	public function
	testOptionSetGetUser ()
	{
		$state = smbclient_state_new();
		smbclient_state_init($state, null, SMB_USER, SMB_PASS);
		$this->assertTrue(smbclient_option_set($state, SMBCLIENT_OPT_USER, SMB_USER));
		$this->assertEquals(SMB_USER, smbclient_option_get($state, SMBCLIENT_OPT_USER));
		smbclient_state_free($state);
	}

	public function
	testOptionSetGetUserWithOpen ()
	{
		$state = smbclient_state_new();
		smbclient_state_init($state, null, SMB_USER, SMB_PASS);
		$this->assertTrue(smbclient_option_set($state, SMBCLIENT_OPT_USER, SMB_USER));
		$this->assertEquals(SMB_USER, smbclient_option_get($state, SMBCLIENT_OPT_USER));
		$dir = smbclient_opendir($state, 'smb://'.SMB_HOST.'/'.SMB_SHARE);
		while (($out = smbclient_readdir($state, $dir)) !== false);
		smbclient_closedir($state, $dir);
		smbclient_state_free($state);
	}

	public function
	testOptionUrlencodeReaddir ()
	{
		$state = smbclient_state_new();
		smbclient_state_init($state, null, SMB_USER, SMB_PASS);

		// Create a file via the regular filesystem:
		touch(SMB_LOCAL.'/readdir%option.txt');

		// Technically options can only be set between new and init...
		smbclient_option_set($state, SMBCLIENT_OPT_URLENCODE_READDIR_ENTRIES, false);
		$dir = smbclient_opendir($state, 'smb://'.SMB_HOST.'/'.SMB_SHARE);
		$found = false;
		while (($out = smbclient_readdir($state, $dir)) !== false) {
			if ($out['name'] === 'readdir%option.txt') {
				$found = true;
				break;
			}
		}
		smbclient_closedir($state, $dir);
		$this->assertTrue($found);

		smbclient_option_set($state, SMBCLIENT_OPT_URLENCODE_READDIR_ENTRIES, true);
		$dir = smbclient_opendir($state, 'smb://'.SMB_HOST.'/'.SMB_SHARE);
		$found = false;
		while (($out = smbclient_readdir($state, $dir)) !== false) {
			if ($out['name'] === 'readdir%25option.txt') {
				$found = true;
				break;
			}
		}
		smbclient_closedir($state, $dir);
		smbclient_state_free($state);
		$this->assertTrue($found);

		unlink(SMB_LOCAL.'/readdir%option.txt');
	}

	public function
	testOptionNTHash ()
	{
		$state = smbclient_state_new();

		if (smbclient_option_get($state, SMBCLIENT_OPT_USE_NT_HASH) === null) {
			smbclient_state_free($state);
			return;
		}
		smbclient_option_set($state, SMBCLIENT_OPT_USE_NT_HASH, true);

		// NTLM hash of 'password' generated at http://www.tobtu.com/lmntlm.php
		smbclient_state_init($state, null, SMB_USER, SMB_HASH);

		$dir = smbclient_opendir($state, 'smb://'.SMB_HOST.'/'.SMB_SHARE);
		while (($out = smbclient_readdir($state, $dir)) !== false);
		$this->assertTrue(is_resource($dir));
		smbclient_closedir($state, $dir);
		smbclient_state_free($state);
	}
}
