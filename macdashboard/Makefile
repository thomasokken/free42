###############################################################################
# Free42 -- an HP-42S calculator simulator
# Copyright (C) 2004-2019  Thomas Okken
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 2,
# as published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, see http://www.gnu.org/licenses/.
###############################################################################

CFLAGS = -DNO_SINCOS \
	 -DOLD_PSE \
	 -arch x86_64 \
	 -mmacosx-version-min=10.8 \
	 -Wall \
	 -Wno-sometimes-uninitialized \
	 -Wno-constant-conversion \
	 -Wno-parentheses \
	 -g \
	 -DDECIMAL_CALL_BY_REFERENCE=1 \
	 -DDECIMAL_GLOBAL_ROUNDING=1 \
	 -DDECIMAL_GLOBAL_ROUNDING_ACCESS_FUNCTIONS=1 \
	 -DDECIMAL_GLOBAL_EXCEPTION_FLAGS=1 \
	 -DDECIMAL_GLOBAL_EXCEPTION_FLAGS_ACCESS_FUNCTIONS=1 \
	 -D_WCHAR_T_DEFINED

CXXFLAGS = $(CFLAGS) \
	   -mmacosx-version-min=10.8 \
	   -stdlib=libc++ \
	   -fno-exceptions \
	   -fno-rtti

LDFLAGS = -Wl,-syslibroot,/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.14.sdk \
	  -mmacosx-version-min=10.8 -stdlib=libc++
LIBS = -arch x86_64 -lm gcc111libbid.a

SRCS = shell.cc shell_spool.cc \
	core_main.cc core_commands1.cc core_commands2.cc \
	core_commands3.cc core_commands4.cc core_commands5.cc \
	core_commands6.cc core_commands7.cc core_display.cc \
	core_globals.cc core_helpers.cc core_keydown.cc core_linalg1.cc \
	core_linalg2.cc core_math1.cc core_math2.cc core_phloat.cc \
	core_sto_rcl.cc core_tables.cc core_variables.cc
OBJS = shell.o shell_spool.o \
	core_main.o core_commands1.o core_commands2.o \
	core_commands3.o core_commands4.o core_commands5.o \
	core_commands6.o core_commands7.o core_display.o \
	core_globals.o core_helpers.o core_keydown.o core_linalg1.o \
	core_linalg2.o core_math1.o core_math2.o core_phloat.o \
	core_sto_rcl.o core_tables.o core_variables.o

ifdef BCD_MATH
CFLAGS += -DBCD_MATH
endif

ifdef FREE42_FPTEST
CFLAGS += -DFREE42_FPTEST
SRCS += readtest.c readtest_lines.cc
OBJS += readtest.o readtest_lines.o
endif

Free42.wdgt: free42 keymap.txt
	cp -rv Widget.Template Free42.wdgt
	find Free42.wdgt -type d -name .svn -prune -exec rm -rf {} \;
	find Free42.wdgt -type d -name CVS -prune -exec rm -rf {} \;
	find Free42.wdgt -type f -name .cvsignore -exec rm -f {} \;
	sed -e "s/%VERSION%/$(shell cat ../VERSION)/g" < Widget.Template/Free42.html> Free42.wdgt/Free42.html
	cp keymap.txt Free42.wdgt
	cp ../util/set-version-in-info-plist.c .
	cc -o set-version-in-info-plist set-version-in-info-plist.c
	./set-version-in-info-plist
	- mkdir Free42.wdgt/bin
	cp free42 Free42.wdgt/bin
	cp Info.plist Free42.wdgt
	codesign -f -s "Developer ID Application: Thomas Okken (F4D88K2P5W)" Free42.wdgt

free42: $(OBJS)
	$(CXX) -o free42 $(LDFLAGS) $(OBJS) $(LIBS)

$(SRCS): symlinks

.cc.o:
	$(CXX) $(CXXFLAGS) -c -o $@ $<

readtest.o: readtest.c
	$(CC) $(CFLAGS) -I IntelRDFPMathLib20U1/TESTS -D__intptr_t_defined -DLINUX -c -o $@ $<

symlinks:
	ln -s ../common/free42.h
	ln -s ../common/core_commands1.cc
	ln -s ../common/core_commands1.h
	ln -s ../common/core_commands2.cc
	ln -s ../common/core_commands2.h
	ln -s ../common/core_commands3.cc
	ln -s ../common/core_commands3.h
	ln -s ../common/core_commands4.cc
	ln -s ../common/core_commands4.h
	ln -s ../common/core_commands5.cc
	ln -s ../common/core_commands5.h
	ln -s ../common/core_commands6.cc
	ln -s ../common/core_commands6.h
	ln -s ../common/core_commands7.cc
	ln -s ../common/core_commands7.h
	ln -s ../common/core_display.cc
	ln -s ../common/core_display.h
	ln -s ../common/core_globals.cc
	ln -s ../common/core_globals.h
	ln -s ../common/core_helpers.cc
	ln -s ../common/core_helpers.h
	ln -s ../common/core_keydown.cc
	ln -s ../common/core_keydown.h
	ln -s ../common/core_linalg1.cc
	ln -s ../common/core_linalg1.h
	ln -s ../common/core_linalg2.cc
	ln -s ../common/core_linalg2.h
	ln -s ../common/core_math1.cc
	ln -s ../common/core_math1.h
	ln -s ../common/core_math2.cc
	ln -s ../common/core_math2.h
	ln -s ../common/core_main.cc
	ln -s ../common/core_main.h
	ln -s ../common/core_phloat.cc
	ln -s ../common/core_phloat.h
	ln -s ../common/core_sto_rcl.cc
	ln -s ../common/core_sto_rcl.h
	ln -s ../common/core_tables.cc
	ln -s ../common/core_tables.h
	ln -s ../common/core_variables.cc
	ln -s ../common/core_variables.h
	ln -s ../common/shell.h
	ln -s ../common/shell_loadimage.cc
	ln -s ../common/shell_loadimage.h
	ln -s ../common/shell_spool.cc
	ln -s ../common/shell_spool.h
	ln -s ../common/skin2cc.cc
	ln -s ../common/skin2cc.conf
	ln -s ../common/keymap2cc.cc
	ln -s ../common/bid_conf.h
	ln -s ../common/bid_functions.h
	sh ./build-intel-lib.sh
	ln -s IntelRDFPMathLib20U1/TESTS/readtest.c
	touch symlinks

clean: FORCE
	rm -f `find . -type l` free42 \
		keymap2cc keymap.cc readtest_lines.cc \
		set-version-in-info-plist set-version-in-info-plist.c \
		*.o *.d *.i *.ii *.s *.a symlinks core.*
	rm -rf Free42.wdgt
	rm -rf IntelRDFPMathLib20U1

FORCE:
