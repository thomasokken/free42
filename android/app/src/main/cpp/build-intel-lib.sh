#!/bin/sh

if [ -f libgcc111libbid-armv7.a -a -f libgcc111libbid-arm64.a ]; then exit 0; fi

NDK="$HOME/Library/Android/sdk/ndk-bundle"
ORIGPATH="$PATH"

rm -rf bin
mkdir bin
echo 'armv7a-linux-androideabi28-clang "$@"' > bin/gcc
echo 'arm-linux-androideabi-ar "$@"' > bin/ar
chmod +x bin/*
rm -rf IntelRDFPMathLib20U1

export PATH="`/bin/pwd`/bin:$NDK/prebuilt/darwin-x86_64/bin:$NDK/toolchains/llvm/prebuilt/darwin-x86_64/bin:$ORIGPATH"

if [ -f ../../../../../inteldecimal/IntelRDFPMathLib20U1.tar.gz ]
then
    tar xvfz ../../../../../inteldecimal/IntelRDFPMathLib20U1.tar.gz
else
    tar xvfz $HOME/free42/inteldecimal/IntelRDFPMathLib20U1.tar.gz
fi
cd IntelRDFPMathLib20U1
patch -p0 <../intel-lib-android-armv7.patch
cd LIBRARY
make CC=gcc CALL_BY_REF=1 GLOBAL_RND=1 GLOBAL_FLAGS=1 UNCHANGED_BINARY_FLAGS=0 _HOST_OS=Linux
mv libbid.a ../../libgcc111libbid-armv7.a

cd ../..
rm -rf IntelRDFPMathLib20U1
echo 'aarch64-linux-android28-clang "$@"' > bin/gcc
echo 'aarch64-linux-android-ar "$@"' > bin/ar

if [ -f ../../../../../inteldecimal/IntelRDFPMathLib20U1.tar.gz ]
then
    tar xvfz ../../../../../inteldecimal/IntelRDFPMathLib20U1.tar.gz
else
    tar xvfz $HOME/free42/inteldecimal/IntelRDFPMathLib20U1.tar.gz
fi
cd IntelRDFPMathLib20U1
patch -p0 <../intel-lib-android-arm64.patch
cd LIBRARY
make CC=gcc CALL_BY_REF=1 GLOBAL_RND=1 GLOBAL_FLAGS=1 UNCHANGED_BINARY_FLAGS=0 _HOST_OS=Linux
mv libbid.a ../../libgcc111libbid-arm64.a

cd ../TESTS
cp readtest.c readtest.h test_bid_conf.h test_bid_functions.h ../..
cd ../..

( echo '#ifdef FREE42_FPTEST'; echo 'const char *readtest_lines[] = {'; tr -d '\r' < IntelRDFPMathLib20U1/TESTS/readtest.in | sed 's/^\(.*\)$/"\1",/'; echo '0 };'; echo '#endif' ) > readtest_lines.cc

#rm -rf bin IntelRDFPMathLib20U1
