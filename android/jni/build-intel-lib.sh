#!/bin/sh

if [ -f libgcc111libbid.a ]; then exit 0; fi

mkdir bin
export PATH="`/bin/pwd`/bin:$PATH"
ln -s "/opt/android-ndk-r10e/toolchains/arm-linux-androideabi-4.9/prebuilt/darwin-x86_64/bin/arm-linux-androideabi-gcc" bin/gcc
export MAKE="/opt/android-ndk-r10e/prebuilt/darwin-x86_64/bin/make"

tar xvfz ../../inteldecimal/IntelRDFPMathLib20U1.tar.gz
cd IntelRDFPMathLib20U1
patch -p0 <../intel-lib-android-armv7.patch
cd LIBRARY
$MAKE CC=gcc CALL_BY_REF=1 GLOBAL_RND=1 GLOBAL_FLAGS=1 UNCHANGED_BINARY_FLAGS=0 _HOST_OS=Linux
mv libbid.a ../../libgcc111libbid.a
cd ../..

( echo '#ifdef FREE42_FPTEST'; echo 'const char *readtest_lines[] = {'; tr -d '\r' < IntelRDFPMathLib20U1/TESTS/readtest.in | sed 's/^\(.*\)$/"\1",/'; echo '0 };'; echo '#endif' ) > readtest_lines.cc
