###############################################################################
# Free42 -- a free HP-42S calculator clone
# Copyright (C) 2004-2006  Thomas Okken
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

CFLAGS = -MMD -Wall -g -DPALMOS=1
#CFLAGS = -MMD -Wall -g -DPALMOS=1 -save-temps
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
LDFLAGS = -g

SRCS = shell1.cc shell2.cc shell_spool.cc myglue.cc filesys.cc core_main.cc \
	core_commands1.cc core_commands2.cc core_commands3.cc \
	core_commands4.cc core_commands5.cc core_commands6.cc core_display.cc \
	core_globals.cc core_helpers.cc core_keydown.cc core_linalg1.cc \
	core_linalg2.cc core_math1.cc core_math2.cc core_phloat.cc \
	core_sto_rcl.cc core_tables.cc core_variables.cc
OBJS = shell1.o shell2.o shell_spool.o myglue.o filesys.o core_main.o \
	core_commands1.o core_commands2.o core_commands3.o \
	core_commands4.o core_commands5.o core_commands6.o core_display.o \
	core_globals.o core_helpers.o core_keydown.o core_linalg1.o \
	core_linalg2.o core_math1.o core_math2.o core_phloat.o \
	core_sto_rcl.o core_tables.o core_variables.o

free42.prc: shell shell.ro Original.prc
	build-prc -o free42.prc shell.def shell shell.ro Original.prc

$(SRCS): symlinks

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
	ln -s ../common/core_math1.cc
	ln -s ../common/core_math1.h
	ln -s ../common/core_math2.cc
	ln -s ../common/core_math2.h
	ln -s ../common/core_main.cc
	ln -s ../common/core_main.h
	ln -s ../common/core_linalg1.cc
	ln -s ../common/core_linalg1.h
	ln -s ../common/core_linalg2.cc
	ln -s ../common/core_linalg2.h
	ln -s ../common/core_phloat.cc
	ln -s ../common/core_phloat.h
	ln -s ../common/core_sto_rcl.cc
	ln -s ../common/core_sto_rcl.h
	ln -s ../common/core_tables.cc
	ln -s ../common/core_tables.h
	ln -s ../common/core_variables.cc
	ln -s ../common/core_variables.h
	ln -s ../common/shell.h
	ln -s ../common/shell_spool.cc
	ln -s ../common/shell_spool.h
	touch symlinks

shell-sections.s shell-sections.ld: shell.def
	m68k-palmos-multigen shell.def

.c.o:
	m68k-palmos-gcc $(CFLAGS) -c -o $@ $<

.cc.o:
	m68k-palmos-g++ $(CXXFLAGS) -c -o $@ $<

shell: $(OBJS) MathLib.o shell-sections.s shell-sections.ld
	m68k-palmos-g++ $(LDFLAGS) -o shell $(OBJS) MathLib.o \
		-lnoexcept -lPalmOSGlue \
		shell-sections.s shell-sections.ld
	
shell.ro: shell.rcp
	pilrc -MD -q -allowEditID -ro -o shell.ro shell.rcp

Original.prc: skin2prc
	./make-skin ../palmskins/Original.layout \
		../palmskins/Original.gif 100 500 1 72 Original off 1

Reverse.prc: skin2prc
	./make-skin ../palmskins/Reverse.layout \
		../palmskins/Reverse.gif 1000 2000 1 72 Reverse off 1

StandardHD.prc: skin2prc
	./make-skin ../palmskins/Standard.layout \
		../palmskins/Standard.gif 1000 2000 1,4,8 144 StandardHD off 2

SemiReal42.prc: skin2prc
	./make-skin ../palmskins/SemiReal42.layout \
		../palmskins/SemiReal42.gif 1000 2000 1,4,8 72 SemiReal42 off 1

SemiReal42HD.prc: skin2prc
	./make-skin ../palmskins/SemiReal42HD.layout \
		../palmskins/SemiReal42HD.gif 1000 2000 1,4,8 144 SemiReal42HD off 2

SemiAuto42HD.prc: skin2prc
	./make-skin ../palmskins/SemiAuto42HD.layout \
		../palmskins/SemiAuto42HD.gif 1000 2000 1,4,8 144 SemiAuto42HD off 2

Free42PalmOSSkins.zip: Reverse.prc StandardHD.prc SemiReal42.prc SemiReal42HD.prc SemiAuto42HD.prc
	rm -f Free42PalmOSSkins.zip
	zip Free42PalmOSSkins.zip Reverse.prc StandardHD.prc \
		SemiReal42.prc SemiReal42HD.prc SemiAuto42HD.prc

skin2prc: skin2prc.cc skin2prc.h
	c++ -o skin2prc skin2prc.cc

clean:
	rm -f *.prc shell shell.ro *.o *.i *.ii *.s *.d \
		*.out *.grc shell.gdb `find . -type l` \
		shell-sections.s shell-sections.ld symlinks \
		skin2prc skin2prc.exe tmp_skin.* Free42PalmOSSkins.zip

-include $(SRCS:.cc=.d)
-include MathLib.d
-include shell.d
