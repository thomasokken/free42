###############################################################################
# Free42 -- a free HP-42S calculator clone
# Copyright (C) 2004-2005  Thomas Okken
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
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
###############################################################################

CXXFLAGS = -MMD -DNO_SINCOS -Wall -g -fno-exceptions -fno-rtti
LDFLAGS =
LIBS = -lm

SRCS = shell.cc shell_spool.cc \
	core_main.cc core_commands1.cc core_commands2.cc \
	core_commands3.cc core_commands4.cc core_decimal.cc core_display.cc \
	core_globals.cc core_helpers.cc core_keydown.cc core_linalg.cc \
	core_math.cc core_tables.cc core_variables.cc
OBJS = shell.o shell_spool.o \
	core_main.o core_commands1.o core_commands2.o \
	core_commands3.o core_commands4.o core_decimal.o core_display.o \
	core_globals.o core_helpers.o core_keydown.o core_linalg.o \
	core_math.o core_tables.o core_variables.o

ifdef BCD_MATH
CXXFLAGS += -DBCD_MATH
SRCS += bcdfloat.cc bcd.cc bcdmath.cc
OBJS += bcdfloat.o bcd.o bcdmath.o
endif

Free42.wdgt: free42 keymap.txt
	cp -rv Widget.Template Free42.wdgt
	cp keymap.txt Free42.wdgt
	cp free42 Free42.wdgt/bin

free42: $(OBJS)
	$(CXX) -o free42 $(LDFLAGS) $(OBJS) $(LIBS)

$(SRCS): symlinks

.cc.o:
	$(CXX) $(CXXFLAGS) -c -o $@ $<
	
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
	ln -s ../common/core_decimal.cc
	ln -s ../common/core_decimal.h
	ln -s ../common/core_display.cc
	ln -s ../common/core_display.h
	ln -s ../common/core_globals.cc
	ln -s ../common/core_globals.h
	ln -s ../common/core_helpers.cc
	ln -s ../common/core_helpers.h
	ln -s ../common/core_keydown.cc
	ln -s ../common/core_keydown.h
	ln -s ../common/core_linalg.cc
	ln -s ../common/core_linalg.h
	ln -s ../common/core_math.cc
	ln -s ../common/core_math.h
	ln -s ../common/core_main.cc
	ln -s ../common/core_main.h
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
	ln -s ../common/bcd.cc
	ln -s ../common/bcd.h
	ln -s ../common/bcdfloat.cc
	ln -s ../common/bcdfloat.h
	ln -s ../common/bcdmath.cc
	ln -s ../common/bcdmath.h
	touch symlinks

clean: FORCE
	rm -f `find . -type l` free42 \
		keymap2cc keymap.cc \
		*.o *.d *.i *.ii *.s symlinks core.*
	rm -rf Free42.wdgt

FORCE:
