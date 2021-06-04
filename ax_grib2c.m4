#####
#
# SYNOPSIS
#
#   AX_GRIB2C()
#
# DESCRIPTION
#
#   This macro provides tests of availability for the GRIB2C library and headers.
#
#   This macro calls:
#
#     AC_SUBST(GRIB2C_LIBDIR)
#     AC_SUBST(GRIB2C_LIBS)
#     AC_SUBST(GRIB2C_INCLUDEDIR)
#
#####
AC_DEFUN([AX_GRIB2C], [
  AC_MSG_CHECKING(for grib2c)

  AC_CANONICAL_HOST
  LIB_EXTENSION="so"
  case $host_os in
    darwin*)
      LIB_EXTENSION="so"
      ;;
  esac

  GRIB2C_LIB_NAME="grib2"
  GRIB2C_LIB_FILENAME="lib$GRIB2C_LIB_NAME.$LIB_EXTENSION"
  GRIB2C_INCLUDE_FILENAME="$GRIB2C_LIB_NAME.h"

  if test -z "$withgrib2c" -o "$withgrib2c" = "yes"; then
    for i in "/usr/$grib2c_libdirname" "/usr/local/$grib2c_libdirname"  "$withgrib2c/grib2c"; do
      if test -f "$i/$GRIB2C_LIB_FILENAME"; then
        GRIB2C_LIB_PATH="$i"
      fi
    done
  elif test "$withgrib2c" != "no"; then  
    for i in "$withgrib2c" "$withgrib2c/$grib2c_libdirname" "$withgrib2c/.libs" "$withgrib2c/grib2c"; do
      if test -f "$i/$GRIB2C_LIB_FILENAME"; then
        GRIB2C_LIB_PATH="$i"
      fi
    done
  else
    AC_MSG_RESULT([ php_grib2c $withgrib2c])
    AC_MSG_ERROR(["Cannot build whilst grib2c is disabled."])
  fi

  if test "$GRIB2C_LIB_PATH" = ""; then
    AC_MSG_ERROR(["Could not find '$GRIB2C_LIB_FILENAME'. Try specifying the path to the grib2c build directory."])
  fi

  GRIB2C_LIBDIR="-L$GRIB2C_LIB_PATH"
  GRIB2C_LIBS="-l$GRIB2C_LIB_NAME"
  GRIB2C_LIBDIR_NO_FLAG="$GRIB2C_LIB_PATH"

  if test -z "$withgrib2c" -o "$withgrib2c" = "yes"; then
    for i in /usr/include /usr/local/include; do
      if test -f "$i/$GRIB2C_INCLUDE_FILENAME"; then
        GRIB2C_INCLUDEDIR="$i"
      fi
    done
  elif test "$withgrib2c" != "no"; then
    for i in "$withgrib2c" "$withgrib2c/.." "$withgrib2c/include"; do
      if test -f "$i/$GRIB2C_INCLUDE_FILENAME"; then
        GRIB2C_INCLUDEDIR="$i"
      fi
    done
  else
    AC_MSG_ERROR(["Cannot build whilst grib2c is disabled."])
  fi

  if test "$GRIB2C_INCLUDEDIR" = ""; then
    AC_MSG_ERROR(["Could not find grib2c '$GRIB2C_INCLUDE_FILENAME' header file. Try specifying the path to the grib2c build directory."])
  fi
  
  GRIB2C_INCLUDEDIR_NO_FLAG="$GRIB2C_INCLUDEDIR"
  GRIB2C_INCLUDEDIR="-I$GRIB2C_INCLUDEDIR"
    
  AC_DEFINE([GRIB2C_ENABLED], [1], [Enables grib2c])
  
  GRIB2C_FOUND="yes"
  
  if test "$grib2c_enabledebug" = "yes"; then
    echo " "
    echo " "
    echo " "
    echo "======================== Debug =============================="
    echo " "
    echo "\$host_os                    :  $host_os"
    echo "\$GRIB2C_LIB_NAME            :  $GRIB2C_LIB_NAME"
    echo "\$GRIB2C_LIB_FILENAME        :  $GRIB2C_LIB_FILENAME"
    echo "\$GRIB2C_INCLUDE_FILENAME    :  $GRIB2C_INCLUDE_FILENAME"
    echo "\$GRIB2C_INC_DIR             :  $GRIB2C_INCLUDEDIR"
    echo "\$GRIB2C_INCLUDEDIR_NO_FLAG  :  $GRIB2C_INCLUDEDIR_NO_FLAG"
    echo "\$GRIB2C_LIBDIR              :  $GRIB2C_LIBDIR"
    echo "\$GRIB2C_LIBDIR_NO_FLAG      :  $GRIB2C_LIBDIR_NO_FLAG"
    echo "\$GRIB2C_FOUND               :  $GRIB2C_FOUND"
    echo "\$withgrib2c                 :  $withgrib2c"
    echo "\$grib2c_enabledebug         :  $grib2c_enabledebug"
    echo " "
    echo "============================================================="
    echo " "
    echo " "
    echo " "
  fi
  
  AC_SUBST(GRIB2C_LIBDIR)
  AC_SUBST(GRIB2C_LIBS)
  AC_SUBST(GRIB2C_INCLUDEDIR)

])
