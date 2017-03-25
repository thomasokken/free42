#!/bin/sh

if [ -f libgcc111libbid.a ]; then exit 0; fi

mkdir bin
export PATH="`/bin/pwd`/bin:$PATH"
if [ `hostname` == "air" ]
then
    TOOLCHAIN=arm-linux-androideabi-4.8
else
    TOOLCHAIN=arm-linux-androideabi-4.9
fi
ln -s "/opt/android-ndk/toolchains/$TOOLCHAIN/prebuilt/darwin-x86_64/bin/arm-linux-androideabi-gcc" bin/gcc
ln -s "/opt/android-ndk/toolchains/$TOOLCHAIN/prebuilt/darwin-x86_64/bin/arm-linux-androideabi-ar" bin/ar
export MAKE="/opt/android-ndk/prebuilt/darwin-x86_64/bin/make"

if [ -f ../../inteldecimal/IntelRDFPMathLib20U1.tar.gz ]
then
    tar xvfz ../../inteldecimal/IntelRDFPMathLib20U1.tar.gz
else
    tar xvfz $HOME/free42/inteldecimal/IntelRDFPMathLib20U1.tar.gz
fi
cd IntelRDFPMathLib20U1
patch -p0 <../intel-lib-android-armv7.patch
cd LIBRARY
$MAKE CC=gcc CALL_BY_REF=1 GLOBAL_RND=1 GLOBAL_FLAGS=1 UNCHANGED_BINARY_FLAGS=0 _HOST_OS=Linux
mv libbid.a ../../libgcc111libbid.a
cd ../TESTS
cp readtest.c readtest.h test_bid_conf.h test_bid_functions.h ../..
cd ../..

( echo '#ifdef FREE42_FPTEST'; echo 'const char *readtest_lines[] = {'; tr -d '\r' < IntelRDFPMathLib20U1/TESTS/readtest.in | sed 's/^\(.*\)$/"\1",/'; echo '0 };'; echo '#endif' ) > readtest_lines.cc

rm -rf bin IntelRDFPMathLib20U1
