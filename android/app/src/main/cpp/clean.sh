#!/bin/sh
find . -type l -exec rm {} \;
rm -f readtest.c readtest.h readtest_lines.cc
rm -f test_bid_conf.h test_bid_functions.h
rm -f *.a
rm -rf bin IntelRDFPMathLib20U1
