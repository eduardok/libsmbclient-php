<html>
<head><title>libsmbclient PHP test</title></head>
<body>

<?
	$url = "smb://guest:guest@minusone/";
	if(isset($_REQUEST["url"])) $url = $_REQUEST["url"];
?>

<h1>libsmbclient PHP test</h1>
<p>
<tt>
We accept smb://[[[domain;]user[:password@]]server[/share[/path[/file]]]]<br>
<br>
smb:// &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;means show all the workgroups<br>
smb://name/ &nbsp;means, if name&lt;1D&gt; exists, list servers in workgroup,
              else, if name&lt;20&gt; exists, list all shares for server ...
</p>
<form method="post" action="libsmbclient.php">
URL: <input name="url" type="text" value="<?echo htmlentities($url)?>" size="100"><br>
<input type="submit" name="submit" value="Browse">
</form>
<?php
//dl("./.libs/libsmbclient.so");

function dodir($url) {
	echo "<ul>\n";
	$dh = smbclient_opendir($url);
	if(!$dh) return;
	while($de = smbclient_readdir($dh)) {
		printf("<li>%s: %s (%s)", $de["type"], $de["name"], $de["comment"]);
		if($de["type"] == "file") {
			$statbuf = smbclient_stat($url . "/" . $de["name"]);
			printf(" [%lu bytes] {<a href=\"libsmbclient.php?read=1&amp;url=%s\">READ</a>}", $statbuf["size"], urlencode($url . "/" . $de["name"]));
		} else if($de["type"] == "file share" || $de["type"] == "directory") {
			if($de["name"] != "." && $de["name"] != "..") {
				//$stat = smbclient_stat($url . "/" . $de["name"]);
				//printf("Got mode %o.", $stat["mode"]);
				if($stat["mode"] & 04) dodir($url . "/" . $de["name"]);
			}
		}
		echo "</li>\n";
	}
	smbclient_closedir($dh);
}

if(isset($_REQUEST["submit"])) {
	dodir($url);
} else if(isset($_REQUEST["read"])) {
	$fh = smbclient_open($url);
	while($str = smbclient_read($fh, 4096)) {
		echo $str;
	}
	smbclient_close($fh);
}
?>
</tt>
</p>
</body></html>
