# Peal postlinker makefile

RM = rm -f

OPTIMIZATION = -g -O0
WARNINGS = -W -Wall -Wmissing-prototypes -Wpointer-arith

POSTLINK_SRCS = image.cc postlinker.cc relocation.cc section.cc symbol.cc symboltable.cc complain.cc
POSTLINK_HDRS = $(POSTLINK_SRCS:.cc=.h) got.h stringtable.h elf.h elf32.h elf_common.h swap.h 

INCDIRS = -I.

all: peal-postlink

peal-postlink: Makefile $(POSTLINK_HDRS) $(POSTLINK_SRCS)
	$(CXX) $(OPTIMIZATION) $(WARNINGS) $(POSTLINK_SRCS) -o peal-postlink

clean: 
	$(RM) *~ *.o peal-postlink

.PHONY=all clean
.SUFFIXES=
