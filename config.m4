PHP_ARG_WITH(grib2c, for grib2c support,
[  --with-grib2c[=DIR]        Include grib2c support. DIR is the optional path to the grib2c directory.], yes)
PHP_ARG_ENABLE(grib2c-debug, whether to enable build debug output,
[  --enable-grib2c-debug        grib2c: Enable debugging during build], no, no)

if test "$PHP_GRIB2C" != "no"; then
  withgrib2c="$PHP_GRIB2C"
  
  grib2c_enabledebug="$PHP_GRIB2C_DEBUG"
  dnl Pass in the library directory name specified by PHP - defaults to lib
  grib2c_libdirname="$PHP_LIBDIR"

  dnl Include common grib2c availability test function



      PHP_NEW_EXTENSION(grib2, src/php_grib2.c, $ext_shared)

else
  AC_MSG_RESULT([grib2 was not enabled])
  AC_MSG_ERROR([Enable grib2 to build this extension.])
fi
