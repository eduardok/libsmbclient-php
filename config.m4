dnl If your extension references something external, use with:

PHP_ARG_ENABLE(libsmbclient, whether to enable smbclient support,
[  --enable-smbclient      Enable libsmbclient support])

PHP_ARG_WITH(libsmbclient, for libsmbclient support,
[  --with-libsmbclient=DIR Installation prefix of libsmbclient.
                          If DIR is not specified, use the system library.])
if test "$PHP_SMBCLIENT" != "no"; then

  dnl If a path was given, expect to find the library in $PHP_LIBDIR
  dnl and the include file in include/
  if test "$PHP_LIBSMBCLIENT" != "yes"; then

    LIBSMBCLIENT_DIR=$PHP_LIBSMBCLIENT/$PHP_LIBDIR
    LIBSMBCLIENT_INCDIR=$PHP_LIBSMBCLIENT/include

    AC_DEFINE(HAVE_LIBSMBCLIENT, 1, [ ])
    PHP_ADD_INCLUDE($LIBSMBCLIENT_INCDIR)
    PHP_ADD_LIBRARY_WITH_PATH(smbclient, $LIBSMBCLIENT_DIR, SMBCLIENT_SHARED_LIBADD)

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
    PHP_CHECK_LIBRARY(smbclient, smbc_getOptionUserData,
    [
      AC_DEFINE(HAVE_LIBSMBCLIENT, 1, [ ])
      PHP_ADD_INCLUDE($LIBSMBCLIENT_INCDIR)
      PHP_ADD_LIBRARY(smbclient, 1, SMBCLIENT_SHARED_LIBADD)
    ],[
      AC_MSG_ERROR([Could not find libsmbclient.so or symbol smbc_getOptionUserData. Check version and config.log for more information.])
    ],[
      -lsmbclient
    ])

  fi

  dnl Check for smbc_setOptionUseNTHash;
  dnl a relatively recent addition that is not available in our Travis test environment:
  PHP_CHECK_LIBRARY(smbclient, smbc_setOptionUseNTHash,
  [
    AC_DEFINE(HAVE_SMBC_SETOPTIONUSENTHASH, 1, [Whether smbc_setOptionUseNTHash is available])
  ], [
  ], [
    -lsmbclient
  ])

  dnl Check for smbc_setPort;
  dnl a relatively recent addition that is not available in our Travis test environment:
  PHP_CHECK_LIBRARY(smbclient, smbc_setPort,
  [
    AC_DEFINE(HAVE_SMBC_SETPORT, 1, [Whether smbc_setPort is available])
  ], [
  ], [
    -lsmbclient
  ])

  dnl Check for smbc_setOptionProtocols;
  dnl seems missing on Alpine
  PHP_CHECK_LIBRARY(smbclient, smbc_setOptionProtocols,
  [
    AC_DEFINE(HAVE_SMBC_SETOPTIONPROTOCOLS, 1, [Whether smbc_setOptionProtocols is available])
  ], [
  ], [
    -lsmbclient
  ])

  PHP_SUBST(SMBCLIENT_SHARED_LIBADD)
  PHP_NEW_EXTENSION(smbclient, smbclient.c smb_streams.c, $ext_shared)

fi
