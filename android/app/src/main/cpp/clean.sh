#!/bin/sh
find . -type l -exec rm {} \;
rm -f readtest.c readtest.h test_bid_conf.h test_bid_functions.h readtest_lines.cc
rm -f *.a
rm -rf bin
