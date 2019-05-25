#!/bin/sh

if [ -f libgcc111libbid-armv7.a -a -f libgcc111libbid-arm64.a ]; then exit 0; fi

NDK="$HOME/Library/Android/sdk/ndk-bundle"

mkdir bin
export PATH="`/bin/pwd`/bin:$PATH"

ln -s "$NDK/toolchains/llvm/prebuilt/darwin-x86_64/bin/armv7a-linux-androideabi26-clang" bin/gcc
ln -s "$NDK/toolchains/llvm/prebuilt/darwin-x86_64/bin/arm-linux-androideabi-ar" bin/ar
export MAKE="$NDK/prebuilt/darwin-x86_64/bin/make"

if [ -f ../../../../../inteldecimal/IntelRDFPMathLib20U1.tar.gz ]
then
    tar xvfz ../../../../../inteldecimal/IntelRDFPMathLib20U1.tar.gz
else
    tar xvfz $HOME/free42/inteldecimal/IntelRDFPMathLib20U1.tar.gz
fi
cd IntelRDFPMathLib20U1
patch -p0 <../intel-lib-android-armv7.patch
cd LIBRARY
$MAKE CC=gcc CALL_BY_REF=1 GLOBAL_RND=1 GLOBAL_FLAGS=1 UNCHANGED_BINARY_FLAGS=0 _HOST_OS=Linux
mv libbid.a ../../libgcc111libbid-armv7.a

cd ../..
rm -rf IntelRDFPMathLib20U1
ln -fs "$NDK/toolchains/llvm/prebuilt/darwin-x86_64/bin/aarch64-linux-android26-clang" bin/gcc
ln -fs "$NDK/toolchains/llvm/prebuilt/darwin-x86_64/bin/aarch64-linux-android-ar" bin/ar

if [ -f ../../../../../inteldecimal/IntelRDFPMathLib20U1.tar.gz ]
then
    tar xvfz ../../../../../inteldecimal/IntelRDFPMathLib20U1.tar.gz
else
    tar xvfz $HOME/free42/inteldecimal/IntelRDFPMathLib20U1.tar.gz
fi
cd IntelRDFPMathLib20U1
patch -p0 <../intel-lib-android-arm64.patch
cd LIBRARY
$MAKE CC=gcc CALL_BY_REF=1 GLOBAL_RND=1 GLOBAL_FLAGS=1 UNCHANGED_BINARY_FLAGS=0 _HOST_OS=Linux
mv libbid.a ../../libgcc111libbid-arm64.a

cd ../TESTS
cp readtest.c readtest.h test_bid_conf.h test_bid_functions.h ../..
cd ../..

( echo '#ifdef FREE42_FPTEST'; echo 'const char *readtest_lines[] = {'; tr -d '\r' < IntelRDFPMathLib20U1/TESTS/readtest.in | sed 's/^\(.*\)$/"\1",/'; echo '0 };'; echo '#endif' ) > readtest_lines.cc

rm -rf bin IntelRDFPMathLib20U1
