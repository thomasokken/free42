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

CXXFLAGS := -MMD -Wall -g -fno-exceptions -fno-rtti $(shell pkg-config --cflags gtk+-2.0)
LDFLAGS = 
LIBS := $(shell pkg-config --libs gtk+-2.0)

SRCS = shell_main.cc shell_skin.cc skins.cc shell_loadimage.cc
OBJS = shell_main.o shell_skin.o skins.o shell_loadimage.o

free42: $(OBJS)
	$(CXX) -o free42 $(LDFLAGS) $(OBJS) $(LIBS)

$(SRCS): symlinks

.cc.o:
	$(CXX) $(CXXFLAGS) -c -o $@ $<

skin2cc: skin2cc.cc
	$(CXX) -o skin2cc skin2cc.cc

skins.cc: skin2cc skin2cc.conf
	./skin2cc

symlinks:
	ln -s ../common/shell_loadimage.cc
	ln -s ../common/shell_loadimage.h
	ln -s ../common/skin2cc.cc
	ln -s ../common/skin2cc.conf
	touch symlinks

clean: FORCE
	rm -f `find . -type l` \
	free42 free42.exe \
	skin2cc skin2cc.exe skins.cc \
	keymap2cc keymap2cc.exe keymap.cc \
	*.o *.d *.i *.ii *.s symlinks core.*

FORCE:

-include $(SRCS:.cc=.d)
