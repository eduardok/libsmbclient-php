dnl
dnl $Id$
dnl

PHP_ARG_WITH(libsmbclient,forlibsmbclient support,
[  --with-libsmbclient[=DIR]       Include libsmbclient support])

PHP_ARG_WITH(libsmbclient-dir,if the location of libsmbclient install directory is defined,
[  --with-libsmbclient-dir=<DIR>   Define the location of the libsmbclient install directory], no, no)

if test "$PHP_LIBSMBCLIENT" != "no" -o "$PHP_LIBSMBCLIENT_DIR" != "no"; then
  PHP_NEW_EXTENSION(libsmbclient, libsmbclient.c, $ext_shared)
  PHP_SUBST(LIBSMBCLIENT_SHARED_LIBADD)
  
  if test "$PHP_LIBSMBCLIENT" != "yes" -a "$PHP_LIBSMBCLIENT" != "no"; then 
    if test -f $PHP_LIBSMBCLIENT/include/libsmbclient.h; then
      LIBSMBCLIENT_DIR=$PHP_LIBSMBCLIENT
      LIBSMBCLIENT_INCDIR=$LIBSMBCLIENT_DIR/include
    fi
  else 
    for i in /usr/local /usr $PHP_LIBSMBCLIENT_DIR; do
      if test -f $i/include/libsmbclient.h; then
        LIBSMBCLIENT_DIR=$i
        LIBSMBCLIENT_INCDIR=$i/include
      fi
    done
  fi
  
  if test -z "$LIBSMBCLIENT_DIR"; then
    AC_MSG_ERROR(Cannot find libsmbclient)
  fi

  PHP_CHECK_LIBRARY(smbclient, smbc_new_context, [
    AC_DEFINE(HAVE_LIBSMBCLIENT,1,[ ]) 
  ],[
    AC_MSG_ERROR(libsmbclient extension requires libsmbclient)
  ],[
    -L$LIBSMBCLIENT_DIR/lib
  ])

  PHP_ADD_LIBPATH($LIBSMBCLIENT_DIR/lib, LIBSMBCLIENT_SHARED_LIBADD)

  PHP_LIBSMBCLIENT_DIR=$LIBSMBCLIENT_DIR
  PHP_ADD_LIBRARY(smbclient,, LIBSMBCLIENT_SHARED_LIBADD)
  PHP_ADD_INCLUDE($LIBSMBCLIENT_INCDIR)

fi
