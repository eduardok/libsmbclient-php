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
<p>
<tt>
<?php
//dl("./.libs/libsmbclient.so");

if(isset($_REQUEST["submit"])) {
	$dh = smbclient_opendir($url);
	while($de = smbclient_readdir($dh)) {
		printf("%s: %s (%s)", $de["type"], $de["name"], $de["comment"]);
		if($de["type"] == "file") {
			$statbuf = smbclient_stat($url . "/" . $de["name"]);
			printf(" [%lu bytes]", $statbuf["size"]);
		}
		echo "<br>\n";
	}
	smbclient_closedir($dh);
}
?>
</tt>
</p>
</body></html>
