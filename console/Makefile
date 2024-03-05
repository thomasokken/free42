###############################################################################
# Free42 -- an HP-42S calculator simulator
# Copyright (C) 2004-2014  Thomas Okken
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

VPATH += ../common
CXXFLAGS += -I../common

ifdef RELEASE
CXXFLAGS += -DRELEASE
endif

CXXFLAGS += -MMD \
	    -Wall \
	    -Wno-misleading-indentation \
	    -Wno-parentheses \
	    -Wno-write-strings \
	    -Wno-maybe-uninitialized \
	    -fno-exceptions \
	    -fno-rtti \
	    -DVERSION="\"$(shell cat ../gtk/VERSION)\"" \
	    -DDECIMAL_CALL_BY_REFERENCE=1 \
	    -DDECIMAL_GLOBAL_ROUNDING=1 \
	    -DDECIMAL_GLOBAL_ROUNDING_ACCESS_FUNCTIONS=1 \
	    -DDECIMAL_GLOBAL_EXCEPTION_FLAGS=1 \
	    -DDECIMAL_GLOBAL_EXCEPTION_FLAGS_ACCESS_FUNCTIONS=1 \
	    -D_WCHAR_T_DEFINED -O2 -fdata-sections -ffunction-sections -Wl,--gc-sections -ggdb



FREE42LIB = libfree42.a

FREE42SRCS = keymap.cc \
	core_main.cc core_commands1.cc core_commands2.cc \
	core_commands3.cc core_commands4.cc core_commands5.cc \
	core_commands6.cc core_commands7.cc core_display.cc core_globals.cc \
	core_helpers.cc core_keydown.cc core_linalg1.cc core_linalg2.cc \
	core_math1.cc core_math2.cc core_phloat.cc core_sto_rcl.cc \
	core_tables.cc core_variables.cc core_aux.cc

SRCS = main_con.cc 


FREE42OBJS = $(FREE42SRCS:.cc=.o)
OBJS = $(SRCS:.cc=.o)



#LIBS := gcc111libbid-$(shell uname -m).a

ifdef ARM_LIB
CXXFLAGS += -mthumb -march=armv7e-m -mfloat-abi=hard -mfpu=fpv4-sp-d16 -DARM
#CXXFLAGS += -mthumb -march=armv7e-m -mfloat-abi=softfp -mfpu=fpv4-sp-d16
#CXXFLAGS += -mthumb -march=armv7e-m
#CXXFLAGS += -DANDROID

CROSS_COMPILE = arm-none-eabi-

else

LIBS := $(FREE42LIB) gcc111libbid-$(shell uname -m).a
ifeq "$(shell uname -s)" "Linux"
LDFLAGS += -Wl,--hash-style=both
endif

endif

AS = $(CROSS_COMPILE)gcc
CC = $(CROSS_COMPILE)gcc
CXX = $(CROSS_COMPILE)g++
LD = $(CROSS_COMPILE)gcc
SIZE = $(CROSS_COMPILE)size
OBJCOPY = $(CROSS_COMPILE)objcopy
OBJDUMP = $(CROSS_COMPILE)objdump
RANLIB = $(CROSS_COMPILE)ranlib

HOSTCXX=g++


# -ffunction-sections -fdata-sections
ifdef BCD_MATH
CXXFLAGS += -DBCD_MATH
LDFLAGS += -Wl,--gc-sections  -Wl,-Map=abcd.map
EXE = free42dec
else
LDFLAGS += -Wl,--gc-sections  -Wl,-Map=abin.map
EXE = free42bin
endif

ifdef USE_CURSES
CXXFLAGS += -DUSE_CURSES
LIBS += -lcurses
endif

ifdef UTF8_DISP
CXXFLAGS += -DUTF8_DISP
endif



$(EXE): $(OBJS) $(FREE42LIB)
	$(CXX) -o $(EXE) $(LDFLAGS) $(OBJS) $(LIBS)

$(FREE42LIB): $(FREE42OBJS)
	-rm -f $@
	$(AR) -r $@ $(FREE42OBJS)
	$(RANLIB) $@


#$(SRCS) keymap2cc.cc : #symlinks

.cc.o:
	$(CXX) $(CXXFLAGS) -c -o $@ $<


keymap2cc: keymap2cc.cc
	$(HOSTCXX) -O2 -o $@ $<

keymap.cc: keymap2cc keymap.txt
	./keymap2cc

#symlinks:
#	for fn in `cd ../common; /bin/ls`; do ln -s ../common/$$fn; done
#	touch symlinks

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
