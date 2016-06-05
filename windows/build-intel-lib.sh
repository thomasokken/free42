#!/bin/sh -e
if [ -f cl111libbid.lib ]; then exit 0; fi
tar xvfz ../inteldecimal/IntelRDFPMathLib20U1.tar.gz
cd IntelRDFPMathLib20U1
patch -p0 <../intel-lib-windows-32bit.patch
cd LIBRARY
cmd /c ..\\..\\build-intel-lib.bat
mv libbid.lib ../../cl111libbid.lib
cd ../..
( echo '#ifdef FREE42_FPTEST'; echo 'const char *readtest_lines[] = {'; tr -d '\r' < IntelRDFPMathLib20U1/TESTS/readtest.in | sed 's/^\(.*\)$/"\1",/'; echo '0 };'; echo '#endif' ) > readtest_lines.cpp
cp IntelRDFPMathLib20U1/TESTS/readtest.c .
