<html>
<head><title>libsmbclient PHP test</title></head>
<body>

<?
	$url = "smb://guest:guest@minusone/";
	if(isset($_REQUEST["url"])) $url = $_REQUEST["url"];
?>

<h1>libsmbclient PHP test</h1>
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
		printf("%s: %s (%s)<br>\n", $de["type"], $de["name"], $de["comment"]);
	}
	smbclient_closedir($dh);
}
?>
</tt>
</p>
</body></html>
