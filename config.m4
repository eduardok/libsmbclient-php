dnl $Id$
dnl config.m4 for extension libsmbclient

dnl Comments in this file start with the string 'dnl'.
dnl Remove where necessary. This file will not work
dnl without editing.

dnl If your extension references something external, use with:

PHP_ARG_WITH(libsmbclient, for libsmbclient support,
Make sure that the comment is aligned:
[  --with-libsmbclient             Include libsmbclient support])

dnl Otherwise use enable:

dnl PHP_ARG_ENABLE(libsmbclient, whether to enable libsmbclient support,
dnl Make sure that the comment is aligned:
dnl [  --enable-libsmbclient           Enable libsmbclient support])

if test "$PHP_LIBSMBCLIENT" != "no"; then
  dnl Write more examples of tests here...

  dnl # --with-libsmbclient -> check with-path
  SEARCH_PATH="/usr/local /usr"     # you might want to change this
  SEARCH_FOR="/include/libsmbclient.h"  # you most likely want to change this
  if test -r $PHP_LIBSMBCLIENT/; then # path given as parameter
    LIBSMBCLIENT_DIR=$PHP_LIBSMBCLIENT
  else # search default path list
    AC_MSG_CHECKING([for libsmbclient files in default path])
    for i in $SEARCH_PATH ; do
      if test -r $i/$SEARCH_FOR; then
        LIBSMBCLIENT_DIR=$i
        AC_MSG_RESULT(found in $i)
      fi
    done
  fi

  if test -z "$LIBSMBCLIENT_DIR"; then
    AC_MSG_RESULT([not found])
    AC_MSG_ERROR([Please reinstall the libsmbclient distribution])
  fi

  dnl # --with-libsmbclient -> add include path
  PHP_ADD_INCLUDE($LIBSMBCLIENT_DIR/include)

  dnl # --with-libsmbclient -> chech for lib and symbol presence
  LIBNAME=smbclient # you may want to change this
  LIBSYMBOL=smbc_init # you most likely want to change this 

  PHP_CHECK_LIBRARY($LIBNAME,$LIBSYMBOL,
  [
    PHP_ADD_LIBRARY_WITH_PATH($LIBNAME, $LIBSMBCLIENT_DIR/lib, LIBSMBCLIENT_SHARED_LIBADD)
    AC_DEFINE(HAVE_LIBSMBCLIENTLIB,1,[ ])
  ],[
    AC_MSG_ERROR([wrong libsmbclient lib version or lib not found])
  ],[
    -L$LIBSMBCLIENT_DIR/lib -lm -ldl
  ])
  
  PHP_SUBST(LIBSMBCLIENT_SHARED_LIBADD)

  PHP_EXTENSION(libsmbclient, $ext_shared)
fi
