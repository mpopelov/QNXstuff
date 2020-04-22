dnl $Id$
dnl config.m4 for extension qbase

dnl Comments in this file start with the string 'dnl'.
dnl Remove where necessary. This file will not work
dnl without editing.

dnl If your extension references something external, use with:

dnl PHP_ARG_WITH(qbase, for qbase support,
dnl Make sure that the comment is aligned:
dnl [  --with-qbase             Include qbase support])

dnl Otherwise use enable:

PHP_ARG_ENABLE(qbase, whether to enable qbase support,
dnl Make sure that the comment is aligned:
[  --enable-qbase           Enable qbase support])

if test "$PHP_QBASE" != "no"; then
  PHP_NEW_EXTENSION(qbase, qbase.c, $ext_shared)
  PHP_SUBST(QBASE_SHARED_LIBADD)
  AC_DEFINE(HAVE_QBASELIB,1,[ ])
fi
