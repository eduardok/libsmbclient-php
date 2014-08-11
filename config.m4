dnl If your extension references something external, use with:

PHP_ARG_WITH(libsmbclient, for libsmbclient support,
[  --with-libsmbclient=DIR Include libsmbclient support.
                          If DIR is not specified, use the system library.])

dnl Otherwise use enable:

dnl PHP_ARG_ENABLE(libsmbclient, whether to enable libsmbclient support,
dnl Make sure that the comment is aligned:
dnl [  --enable-libsmbclient           Enable libsmbclient support])

if test "$PHP_LIBSMBCLIENT" != "no"; then

  dnl If a path was given, expect to find the library in $PHP_LIBDIR
  dnl and the include file in include/
  if test "$PHP_LIBSMBCLIENT" != "yes"; then

    LIBSMBCLIENT_DIR=$PHP_LIBSMBCLIENT/$PHP_LIBDIR
    LIBSMBCLIENT_INCDIR=$PHP_LIBSMBCLIENT/include

    AC_DEFINE(HAVE_LIBSMBCLIENT, 1, [ ])
    PHP_ADD_INCLUDE($LIBSMBCLIENT_INCDIR)
    PHP_ADD_LIBRARY_WITH_PATH(smbclient, $LIBSMBCLIENT_DIR, LIBSMBCLIENT_SHARED_LIBADD)

  dnl Otherwise find the header and shared library by the normal means:
  else

    dnl Find the header file:
    AC_MSG_CHECKING([for libsmbclient.h in default paths])
    for i in /usr/local/include /usr/local/include/samba-* /usr/include /usr/include/samba-* ; do
      if test -r $i/libsmbclient.h; then
        LIBSMBCLIENT_INCDIR="$i";
        AC_MSG_RESULT([found in $i])
        break;
      fi
    done
    if test -z "$LIBSMBCLIENT_INCDIR"; then
      AC_MSG_RESULT([not found])
      AC_MSG_ERROR([Could not find libsmbclient.h])
    fi

    dnl Find the shared library:
    PHP_CHECK_LIBRARY(smbclient, smbc_open,
    [
      AC_DEFINE(HAVE_LIBSMBCLIENT, 1, [ ])
      PHP_ADD_INCLUDE($LIBSMBCLIENT_INCDIR)
      PHP_ADD_LIBRARY(smbclient, 1, LIBSMBCLIENT_SHARED_LIBADD)
    ],[
      AC_MSG_ERROR([Could not find libsmbclient.so or symbol smbc_open. Check config.log for more information.])
    ],[
      -lsmbclient -lm -ldl
    ])

  fi

  PHP_SUBST(LIBSMBCLIENT_SHARED_LIBADD)
  PHP_NEW_EXTENSION(libsmbclient, libsmbclient.c, $ext_shared)

fi
