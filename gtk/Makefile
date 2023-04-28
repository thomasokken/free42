###############################################################################
# Free42 -- an HP-42S calculator simulator
# Copyright (C) 2004-2023  Thomas Okken
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

PKG_CONFIG ?= pkg-config
CXX_FOR_BUILD ?= $(CXX)

CFLAGS += -MMD \
	 -Wall \
	 -Wno-parentheses \
	 -Wno-write-strings \
	 -Wno-sign-compare \
	 -Wno-narrowing \
	 -Wno-constant-conversion \
	 -Wno-sometimes-uninitialized \
	 -Wno-format-truncation \
	 -Wno-class-memaccess \
	 -Wno-unknown-warning-option \
	 -g \
	 $(shell $(PKG_CONFIG) --cflags gtk+-3.0) \
	 -DGDK_DISABLE_DEPRECATION_WARNINGS -DGTK_DISABLE_SINGLE_INCLUDES -DGSEAL_ENABLE \
	 -DVERSION="\"$(shell cat VERSION)\"" \
	 -DVERSION_PLATFORM="\"$(shell uname -s)\"" \
	 -DDECIMAL_CALL_BY_REFERENCE=1 \
	 -DDECIMAL_GLOBAL_ROUNDING=1 \
	 -DDECIMAL_GLOBAL_ROUNDING_ACCESS_FUNCTIONS=1 \
	 -DDECIMAL_GLOBAL_EXCEPTION_FLAGS=1 \
	 -DDECIMAL_GLOBAL_EXCEPTION_FLAGS_ACCESS_FUNCTIONS=1

CXXFLAGS = $(CFLAGS) \
	 -fno-exceptions \
	 -fno-rtti \
	 -D_WCHAR_T_DEFINED

LIBS = gcc111libbid.a $(shell $(PKG_CONFIG) --libs gtk+-3.0)

ifdef AUDIO_ALSA
LIBS += -lpthread -ldl
endif

ifneq "$(findstring 6162,$(shell echo ab | od -x))" ""
CFLAGS += -DF42_BIG_ENDIAN -DBID_BIG_ENDIAN
endif

ifeq "$(shell uname -m)" "arm64"
CFLAGS += -fsigned-char -DBID_SIZE_LONG=8
else ifeq "$(shell uname -m)" "aarch64"
CFLAGS += -fsigned-char -DBID_SIZE_LONG=8
else ifeq "$(shell uname -m | sed 's/^\(...\).*$$/\1/')" "arm"
CFLAGS += -fsigned-char -DBID_SIZE_LONG=4
endif

SRCS = shell_main.cc shell_skin.cc skins.cc keymap.cc shell_loadimage.cc \
	shell_spool.cc core_main.cc core_commands1.cc core_commands2.cc \
	core_commands3.cc core_commands4.cc core_commands5.cc \
	core_commands6.cc core_commands7.cc core_display.cc core_globals.cc \
	core_helpers.cc core_keydown.cc core_linalg1.cc core_linalg2.cc \
	core_math1.cc core_math2.cc core_phloat.cc core_sto_rcl.cc \
	core_tables.cc core_variables.cc
OBJS = shell_main.o shell_skin.o skins.o keymap.o shell_loadimage.o \
	shell_spool.o core_main.o core_commands1.o core_commands2.o \
	core_commands3.o core_commands4.o core_commands5.o \
	core_commands6.o core_commands7.o core_display.o core_globals.o \
	core_helpers.o core_keydown.o core_linalg1.o core_linalg2.o \
	core_math1.o core_math2.o core_phloat.o core_sto_rcl.o \
	core_tables.o core_variables.o

ifdef BCD_MATH
CXXFLAGS += -DBCD_MATH
EXE = free42dec
else
EXE = free42bin
endif

ifdef FREE42_FPTEST
CFLAGS += -DFREE42_FPTEST
SRCS += readtest.c readtest_lines.cc
OBJS += readtest.o readtest_lines.o
endif

ifdef AUDIO_ALSA
# Note: the name of the libasound shared library that is usually compiled into
# the executable is defined in the corresponding *.la file, in the 'dlname'
# property. I suppose it would be better to grab it from there at build time,
# rather than hard-coding it into the Makefile like this, but as long as the
# build environment doesn't change (currently Ubuntu 12.04), this is fine.
CXXFLAGS += -DAUDIO_ALSA -DALSALIB="\"libasound.so.2\""
SRCS += audio_alsa.cc
OBJS += audio_alsa.o
endif

$(EXE): $(OBJS) gcc111libbid.a
	$(CXX) -o $(EXE) $(LDFLAGS) $(OBJS) $(LIBS)

$(SRCS) skin2cc.cc keymap2cc.cc skin2cc.conf: symlinks

.cc.o:
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

readtest.o: readtest.c
	$(CC) $(CFLAGS) -I IntelRDFPMathLib20U1/TESTS -D__intptr_t_defined -DLINUX -c -o $@ $<

skin2cc: skin2cc.cc
	$(CXX_FOR_BUILD) -o skin2cc skin2cc.cc

skins.cc: skin2cc skin2cc.conf
	./skin2cc

keymap2cc: keymap2cc.cc
	$(CXX_FOR_BUILD) -o keymap2cc keymap2cc.cc

keymap.cc: keymap2cc keymap.txt
	./keymap2cc

gcc111libbid.a:
	sh ./build-intel-lib.sh
	ln -s IntelRDFPMathLib20U1/TESTS/readtest.c

symlinks:
	for fn in `cd ../common; /bin/ls`; do ln -s ../common/$$fn; done
	touch symlinks

clean: FORCE
	rm -f `find . -type l ! -name readtest.c` \
		skin2cc skin2cc.exe skins.cc \
		keymap2cc keymap2cc.exe keymap.cc \
		*.o *.d *.i *.ii *.s symlinks core.*

cleaner: FORCE
	rm -f `find . -type l` \
		free42bin free42bin.exe free42dec free42dec.exe \
		skin2cc skin2cc.exe skins.cc \
		keymap2cc keymap2cc.exe keymap.cc \
		readtest_lines.cc \
		gcc111libbid.a \
		*.o *.d *.i *.ii *.s symlinks core.*
	rm -rf IntelRDFPMathLib20U1

FORCE:

-include $(OBJS:.o=.d)
