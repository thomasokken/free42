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

SRCS = shell_main.cc
OBJS = shell_main.o

free42: $(OBJS)
	$(CXX) -o free42 $(LDFLAGS) $(OBJS) $(LIBS)

.cc.o:
	$(CXX) $(CXXFLAGS) -c -o $@ $<

-include $(SRCS:.cc=.d)
