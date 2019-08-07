#!/bin/sh
if [ -f gcc111libbid.a ]; then exit 0; fi

if [ -z $MK ]; then
  which gmake >/dev/null
  if [ $? -eq 0 ]; then
    MK=gmake
  else
    MK=make
  fi
fi

which gcc >/dev/null
if [ $? -eq 0 ]; then
  CC=gcc
else
  CC=cc
fi

if [ `uname -s` == "FreeBSD" ]; then
  OS_ARG="CFLAGS_OPT=-DLINUX"
else
  OS_ARG=
fi

if [ `echo ab | od -x | awk '{print $2}'` == "6162" ]; then
  ENDIAN_ARG="-e BID_BIG_ENDIAN=1"
else
  ENDIAN_ARG=
fi

tar xvfz ../inteldecimal/IntelRDFPMathLib20U1.tar.gz
cd IntelRDFPMathLib20U1
# When building for architectures other than x86 or x86_64, I comment out the
# section titled "Determine host architecture" in
# IntelRDFPMathLib20U1/LIBRARY/makefile.iml_head, and replace it with a simple
# "_HOST_ARCH := x86" or "_HOST_ARCH := x86_64", depending on whether I'm
# building for a 32-bit or 64-bit platform, respectively. The actual CPU you
# specify seems to matter less than its word size. Thus, setting _HOST_ARCH
# to x86 works when targeting armv7 and ppc, both 32-bit platforms, and setting
# it to x86_64 works when targeting arm64, a 64-bit platform.
# Of course, proceed with caution. Your mileage may vary.
patch -p0 <../intel-lib-linux.patch
cd LIBRARY
# When building for big-endian targets, add "-e BID_BIG_ENDIAN=1" to the "make"
# command line. The library will work even if you don't, but the state files
# written by Free42 will only be compatible with those written on little-endian
# platforms if you do.
$MK $OS_ARG CC=$CC CALL_BY_REF=1 GLOBAL_RND=1 GLOBAL_FLAGS=1 UNCHANGED_BINARY_FLAGS=0 $ENDIAN_ARG
mv libbid.a ../../gcc111libbid.a
cd ../..
( echo '#ifdef FREE42_FPTEST'; echo 'const char *readtest_lines[] = {'; tr -d '\r' < IntelRDFPMathLib20U1/TESTS/readtest.in | sed 's/^\(.*\)$/"\1",/'; echo '0 };'; echo '#endif' ) > readtest_lines.cc
