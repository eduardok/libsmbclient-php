gcc -fpic -DCOMPILE_DL_SMBCLIENT_MODULE=1 -I/usr/include/php4 -I. -I/usr/include/php4/Zend -I/usr/include/php4/main -I/usr/include/php4/TSRM -c -o smbclient_module.o smbclient_module.c
gcc -shared -rdynamic -lsmbclient -o smbclient_module.so smbclient_module.o
