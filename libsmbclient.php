<?php
dl("smbclient_module.so");

$param = 2;
$return = smbclient_module($param);

print("We sent \"$param\" and got \"$return\"");

?>
