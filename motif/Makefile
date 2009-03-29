###############################################################################
# Free42 -- an HP-42S calculator simulator
# Copyright (C) 2004-2009  Thomas Okken
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

VERSION = "\"$(shell cat ../VERSION)\""
CXXFLAGS = -MMD -Wall -g -I/usr/X11R6/include -fno-exceptions -fno-rtti -DVERSION=$(VERSION)
LDFLAGS = -L/usr/X11R6/lib
LIBS = -lm -lXm -lXt -lX11 -lXmu -lXpm

ifeq "$(shell uname -s)" "Darwin"
CXXFLAGS += -I/usr/OpenMotif/include -DNO_SINCOS=1
LDFLAGS += -L/usr/OpenMotif/lib
endif

ifeq "$(shell uname -s)" "Linux"
LDFLAGS += -Wl,--hash-style=both
endif

#CXXFLAGS = -MMD -Wall -g -I/usr/local/lesstif/include -I/usr/X11R6/include -fno-exceptions -fno-rtti -DVERSION=$(VERSION)
#LDFLAGS = -L/usr/local/lesstif/lib -L/usr/X11R6/lib
#LIBS = -lm -lXm -lXpm -lXmu

SRCS = shell_main.cc shell_skin.cc skins.cc keymap.cc shell_loadimage.cc \
	shell_spool.cc core_main.cc core_commands1.cc core_commands2.cc \
	core_commands3.cc core_commands4.cc core_commands5.cc \
	core_commands6.cc core_display.cc core_globals.cc core_helpers.cc \
	core_keydown.cc core_linalg1.cc core_linalg2.cc core_math1.cc \
	core_math2.cc core_phloat.cc core_sto_rcl.cc core_tables.cc \
	core_variables.cc
OBJS = shell_main.o shell_skin.o skins.o keymap.o shell_loadimage.o \
	shell_spool.o core_main.o core_commands1.o core_commands2.o \
	core_commands3.o core_commands4.o core_commands5.o \
	core_commands6.o core_display.o core_globals.o core_helpers.o \
	core_keydown.o core_linalg1.o core_linalg2.o core_math1.o \
	core_math2.o core_phloat.o core_sto_rcl.o core_tables.o \
	core_variables.o

ifdef BCD_MATH
CXXFLAGS += -DBCD_MATH
SRCS += bcdfloat.cc bcd.cc bcdmath.cc
OBJS += bcdfloat.o bcd.o bcdmath.o
EXE = free42dec
else
EXE = free42bin
endif

$(EXE): $(OBJS)
	$(CXX) -o $(EXE) $(LDFLAGS) $(OBJS) $(LIBS)

$(SRCS): symlinks

.cc.o:
	$(CXX) $(CXXFLAGS) -c -o $@ $<

shell_main.cc: icon.xpm

skin2cc: skin2cc.cc
	$(CXX) -o skin2cc skin2cc.cc

skins.cc: skin2cc skin2cc.conf
	./skin2cc

keymap2cc: keymap2cc.cc
	$(CXX) -o keymap2cc keymap2cc.cc

keymap.cc: keymap2cc keymap.txt
	./keymap2cc
	
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
	ln -s ../common/bcd.cc
	ln -s ../common/bcd.h
	ln -s ../common/bcdfloat.cc
	ln -s ../common/bcdfloat.h
	ln -s ../common/bcdmath.cc
	ln -s ../common/bcdmath.h
	touch symlinks

clean: FORCE
	rm -f `find . -type l` \
		skin2cc skin2cc.exe skins.cc \
		keymap2cc keymap2cc.exe keymap.cc \
		*.o *.d *.i *.ii *.s symlinks core.*

cleaner: FORCE
	rm -f `find . -type l` \
		free42bin free42bin.exe free42dec free42dec.exe \
		skin2cc skin2cc.exe skins.cc \
		keymap2cc keymap2cc.exe keymap.cc \
		*.o *.d *.i *.ii *.s symlinks core.*

FORCE:

-include $(SRCS:.cc=.d)
