#!/bin/sh

NDK="$HOME/Library/Android/sdk/ndk-bundle"
export PATH="`/bin/pwd`/bin:$NDK/prebuilt/darwin-x86_64/bin:$NDK/toolchains/llvm/prebuilt/darwin-x86_64/bin:$PATH"
BUILT=0

build_arch () {
    ARCH=$1
    ARCH_CC=$2
    ARCH_AR=$3
    ARCH_LIB=libgcc111libbid-$ARCH.a
    if [ -f $ARCH_LIB ]; then
        return
    fi
    rm -rf bin IntelRDFPMathLib20U1
    mkdir bin
    echo "$ARCH_CC"' "$@"' > bin/gcc
    echo "$ARCH_AR"' "$@"' > bin/ar
    chmod +x bin/*
    if [ -f ../../../../../inteldecimal/IntelRDFPMathLib20U1.tar.gz ]
    then
        tar xfz ../../../../../inteldecimal/IntelRDFPMathLib20U1.tar.gz
    else
        tar xfz $HOME/free42/inteldecimal/IntelRDFPMathLib20U1.tar.gz
    fi

    cd IntelRDFPMathLib20U1
    patch -p0 <../intel-lib-android-$ARCH.patch
    cd LIBRARY
    make CC=gcc CALL_BY_REF=1 GLOBAL_RND=1 GLOBAL_FLAGS=1 UNCHANGED_BINARY_FLAGS=0 _HOST_OS=Linux
    mv libbid.a ../../$ARCH_LIB
    cd ../..
    BUILT=1
}

build_arch armv7 armv7a-linux-androideabi28-clang arm-linux-androideabi-ar
build_arch arm64 aarch64-linux-android28-clang aarch64-linux-android-ar
build_arch x86 i686-linux-android28-clang i686-linux-android-ar
build_arch x86_64 x86_64-linux-android28-clang x86_64-linux-android-ar

if [ $BUILT -ne 0 ]; then
    cd IntelRDFPMathLib20U1/TESTS
    cp readtest.c readtest.h test_bid_conf.h test_bid_functions.h ../..
    cd ../..
    ( echo '#ifdef FREE42_FPTEST'; echo 'const char *readtest_lines[] = {'; tr -d '\r' < IntelRDFPMathLib20U1/TESTS/readtest.in | sed 's/^\(.*\)$/"\1",/'; echo '0 };'; echo '#endif' ) > readtest_lines.cc
    rm -rf bin IntelRDFPMathLib20U1
fi
