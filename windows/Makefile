# Makefile - Free42.dsp

ifndef CFG
CFG=Release
endif
CC=gcc
CFLAGS=-DNO_SINCOS
ifdef BCD_MATH
CFLAGS += -DBCD_MATH
endif
CXX=g++
CXXFLAGS=$(CFLAGS)
RC=windres -O COFF
ifeq "$(CFG)"  "Release"
CFLAGS+=-fno-exceptions -fno-rtti -O2 -D_WINDOWS -DWINDOWS -DWIN32 -DNDEBUG -D_MBCS -W -Wno-uninitialized
LD=$(CXX) $(CXXFLAGS)
LDFLAGS=
LDFLAGS+=-Wl,--subsystem,windows
#LIBS+=-lkernel32 -luser32 -lgdi32 -lwinspool -lcomdlg32 -ladvapi32 -lshell32 -lole32 -loleaut32 -luuid -lodbc32 -lodbccp32
LIBS+=-lgdi32 -lcomdlg32
else
ifeq "$(CFG)"  "Debug"
CFLAGS+=-fno-exceptions -fno-rtti -g -O0 -D_WINDOWS -DWINDOWS -DWIN32 -D_DEBUG -D_MBCS -W
LD=$(CXX) $(CXXFLAGS)
LDFLAGS=
LDFLAGS+=-Wl,--subsystem,windows
#LIBS+=-lkernel32 -luser32 -lgdi32 -lwinspool -lcomdlg32 -ladvapi32 -lshell32 -lole32 -loleaut32 -luuid -lodbc32 -lodbccp32
LIBS+=-lgdi32 -lcomdlg32
endif
endif

ifndef TARGET
ifdef BCD_MATH
TARGET=Free42Decimal.exe
else
TARGET=Free42Binary.exe
endif
endif

.PHONY: all
all: $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ -c $<

%.o: %.cc
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -o $@ -c $<

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -o $@ -c $<

%.o: %.cxx
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -o $@ -c $<

%.res: %.rc
	$(RC) $(CPPFLAGS) -o $@ -i $<

CFLAGS+=
SOURCE_FILES= \
	core_commands1.cpp \
	core_commands2.cpp \
	core_commands3.cpp \
	core_commands4.cpp \
	core_commands5.cpp \
	core_commands6.cpp \
	core_display.cpp \
	core_globals.cpp \
	core_helpers.cpp \
	core_keydown.cpp \
	core_linalg1.cpp \
	core_linalg2.cpp \
	core_main.cpp \
	core_math1.cpp \
	core_math2.cpp \
	core_phloat.cpp \
	core_sto_rcl.cpp \
	core_tables.cpp \
	core_variables.cpp \
	Free42.rc \
	keymap.cpp \
	msg2string.cpp \
	shell.cpp \
	shell_loadimage.cpp \
	shell_skin.cpp \
	shell_spool.cpp \
	skins.cpp

HEADER_FILES= \
	core_commands1.h \
	core_commands2.h \
	core_commands3.h \
	core_commands4.h \
	core_commands5.h \
	core_commands6.h \
	core_display.h \
	core_globals.h \
	core_helpers.h \
	core_keydown.h \
	core_linalg1.h \
	core_linalg2.h \
	core_main.h \
	core_math1.h \
	core_math2.h \
	core_phloat.h \
	core_sto_rcl.h \
	core_tables.h \
	core_variables.h \
	free42.h \
	msg2string.h \
	resource.h \
	shell.h \
	shell_loadimage.h \
	shell_skin.h \
	shell_spool.h

RESOURCE_FILES= \
	Free42.ico \
	small.ico

SRCS=$(SOURCE_FILES) $(HEADER_FILES) $(RESOURCE_FILES)  \
	ReadMe.txt

OBJS=$(patsubst %.rc,%.res,$(patsubst %.cxx,%.o,$(patsubst %.cpp,%.o,$(patsubst %.cc,%.o,$(patsubst %.c,%.o,$(filter %.c %.cc %.cpp %.cxx %.rc,$(SRCS)))))))

ifdef BCD_MATH
OBJS += bcdfloat.o bcd.o bcdmath.o
endif

$(TARGET): $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

skin2cpp: skin2cpp.cpp
	$(CXX) -DWINDOWS -o skin2cpp skin2cpp.cpp

skins.cpp: skin2cpp skin2cpp.conf
	./skin2cpp

keymap2cpp: keymap2cpp.cpp
	$(CXX) -DWINDOWS -o keymap2cpp keymap2cpp.cpp

keymap.cpp: keymap2cpp keymap.txt
	./keymap2cpp

rom2raw: rom2raw.c
	$(CC) -o rom2raw rom2raw.c
	
.PHONY: clean
clean:
	-rm -f $(OBJS) Free42.dep keymap2cpp.exe keymap.cpp skin2cpp.exe skins.cpp rom2raw.exe

.PHONY: cleaner
cleaner:
	-rm -f $(OBJS) Free42Binary.exe Free42Decimal.exe Free42.dep keymap2cpp.exe keymap.cpp skin2cpp.exe skins.cpp rom2raw.exe

.PHONY: depends
depends:
	-$(CXX) $(CXXFLAGS) $(CPPFLAGS) -MM $(filter %.c %.cc %.cpp %.cxx,$(SRCS)) > Free42.dep

-include Free42.dep

