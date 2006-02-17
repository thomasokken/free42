# Makefile - Free42.dsp

ifndef CFG
CFG=Release
endif
CC=gcc
CFLAGS=-DNO_SINCOS
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
TARGET=Free42.exe
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
	core_commands1.cc \
	core_commands2.cc \
	core_commands3.cc \
	core_commands4.cc \
	core_commands5.cc \
	core_commands6.cc \
	core_decimal.cc \
	core_display.cc \
	core_globals.cc \
	core_helpers.cc \
	core_keydown.cc \
	core_linalg1.cc \
	core_linalg2.cc \
	core_main.cc \
	core_math1.cc \
	core_math2.cc \
	core_phloat.cc \
	core_sto_rcl.cc \
	core_tables.cc \
	core_variables.cc \
	Free42.rc \
	keymap.cc \
	msg2string.cc \
	shell.cc \
	shell_loadimage.cc \
	shell_skin.cc \
	shell_spool.cc \
	skins.cc

HEADER_FILES= \
	core_commands1.h \
	core_commands2.h \
	core_commands3.h \
	core_commands4.h \
	core_commands5.h \
	core_commands6.h \
	core_decimal.h \
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

$(TARGET): $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

skin2cc: skin2cc.cc
	$(CXX) -o skin2cc skin2cc.cc

skins.cc: skin2cc skin2cc.conf
	./skin2cc

keymap2cc: keymap2cc.cc
	$(CXX) -o keymap2cc keymap2cc.cc

keymap.cc: keymap2cc keymap.txt
	./keymap2cc

rom2raw: rom2raw.c
	$(CC) -o rom2raw rom2raw.c
	
.PHONY: clean
clean:
	-rm -f $(OBJS) $(TARGET) Free42.dep keymap2cc.exe keymap.cc skin2cc.exe skins.cc rom2raw.exe

.PHONY: depends
depends:
	-$(CXX) $(CXXFLAGS) $(CPPFLAGS) -MM $(filter %.c %.cc %.cpp %.cxx,$(SRCS)) > Free42.dep

-include Free42.dep

