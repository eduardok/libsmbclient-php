<?php
//dl("./.libs/libsmbclient.so");

$param = 2;
$return = smbclient_test($param);

print("We sent \"$param\" and got \"$return\"");

?>
