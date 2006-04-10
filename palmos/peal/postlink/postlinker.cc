/**********
 * Copyright (c) 2004-2005 Greg Parker.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY GREG PARKER ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 **********/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdarg.h>
#include <string.h>

#include "elf.h"

#include <map>
#include <string>
#include <vector>
#include <algorithm>

using namespace std;

#include "got.h"
#include "swap.h"
#include "image.h"
#include "symbol.h"
#include "section.h"
#include "postlinker.h"
#include "symboltable.h"
#include "stringtable.h"

#ifndef O_BINARY
#define O_BINARY 0
#endif

#define Version "2005-4-14"
#define Copyright  \
"Copyright (c) 2004-2005 Greg Parker\n" \
"Copyright (c) 2001 David E. O'Brien\n" \
"Copyright (c) 1996-1998 John D. Polstra\n" \
"All rights reserved.\n" \
"\n" \
"Redistribution and use in source and binary forms, with or without\n" \
"modification, are permitted provided that the following conditions\n" \
"are met:\n" \
"1. Redistributions of source code must retain the above copyright\n" \
"   notice, this list of conditions and the following disclaimer.\n" \
"2. Redistributions in binary form must reproduce the above copyright\n" \
"   notice, this list of conditions and the following disclaimer in the\n" \
"   documentation and/or other materials provided with the distribution.\n" \
"\n" \
"THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND\n" \
"ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE\n" \
"IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE\n"\
"ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE\n" \
"FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL\n"\
"DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS\n" \
"OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)\n" \
"HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT\n"\
"LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY\n" \
"OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF\n" \
"SUCH DAMAGE.\n"

int Verbose = 0;


static void version(void)
{
    fprintf(stderr, "peal-postlink version %s\n\n", Version);
    fprintf(stderr, "%s\n", Copyright);
}


static void usage(const char *name)
{
    fprintf(stderr, 
            "Usage: %s [-vV] [-K filename] [-t resType] [-s resID] [-o output] filename\n"
            "   -V: print version info\n"
            "   -v: verbose\n"
            "   -K <filename>: only keep global symbols listed in file 'filename'\n"
            "   -t <resType>: set resource type for -s (default is 'armc')\n"
            "   -o <output>: write result to file 'output'\n"
            "   -s <resID>: write result in .ro format with one resource per ELF section\n"
            "            (default is .bin format with everything in one resource)\n"
            , name);
    exit(1);
}

int main(int argc, char **argv)
{
    int fd;
    int err;
    struct stat sb;
    int ch;
    const char *infilename;
    const char *outfilename = NULL;
    const char *keepfilename = NULL;
    const char *selfname;
    vector<string> *keepSymbols = NULL;
    int printversion = 0;
    int splitindex = -1;
    const char *resType = "armc";

    // Parse options 
    Verbose = 0;
    selfname = strrchr(argv[0], '/');
    if (!selfname) selfname = argv[0];

    while ((ch = getopt(argc, argv, "vVK:o:s:t:")) != -1) {
        switch (ch) {
        case 'V': 
            printversion = 1;
            break;

        case 'v':
            Verbose = 1;
            break;

        case 'K':
            if (keepfilename) {
                fprintf(stderr, "%s: -K may be used only once\n", selfname);
                usage(selfname);
            } else {
                keepfilename = optarg;
            }
            break;

        case 'o':
            if (outfilename) {
                fprintf(stderr, "%s: -o may be used only once\n", selfname);
                usage(selfname);
            } else {
                outfilename = optarg;
            }
            break;

        case 's':
            if (splitindex != -1) {
                fprintf(stderr, "%s: -s may be used only once\n", selfname);
                usage(selfname);
            } else {
                splitindex = atoi(optarg);
            }
            break;

        case 't':
            resType = optarg;
            if (strlen(resType) != 4) {
                fprintf(stderr, "%s: -t resource type must be exactly four characters long\n", selfname);
                usage(selfname);
            }
            break;

        case '?':
        default:
            usage(selfname);
            break;
        }
    }
    
    argc -= optind;
    argv += optind;

    if (printversion) {
        version();
    }

    if (argc == 0) {
        fprintf(stderr, "%s: no file specified\n", selfname);
        usage(selfname);
    }

    infilename = argv[argc-1];
    if (!outfilename) outfilename = infilename;


    // Read keepfile, if any
    if (keepfilename) {
        FILE *keepfile = fopen(keepfilename, "r");
        if (!keepfile) { perror(keepfilename); return 1; }
        keepSymbols = new vector<string>(0);
        char buf[1024];
        char *keep;
        size_t len;
        while ((keep = fgets(buf, sizeof(buf), keepfile))) {
            char *end = strstr(keep, "\n");
            if (end) len = end - keep;  // skip newline, if any
            else len = strlen(keep);
            keepSymbols->push_back(string(keep, len));
        }
    }


    // Read file into memory

    fd = open(infilename, O_RDONLY | O_BINARY, 0);
    if (fd < 0) { perror(infilename); return 1; }

    err = fstat(fd, &sb);
    if (err) { perror(infilename); return 1; }

    uint8_t *buf = (uint8_t *)malloc(sb.st_size);
    if (sb.st_size != read(fd, buf, sb.st_size)) { 
        perror(infilename); return 1;
    }
    close(fd);


    // Read the ELF image, undo relocations, and record new relocations.

    Image image(buf);


    // Scrub the symbol table. 
    // - keep all global symbols
    // - keep a symbol for data and text sections
    // - fixme apply export list here
    
    image.symtab().strip(keepSymbols);
    image.trimSections();  // removes all string tables
    image.addSectionGlobals();
    image.buildSymbolStringTable();
    image.buildRelocations();
    image.buildSectionStringTable();

    image.write(resType, splitindex, outfilename);

    // fixme paranoia checks:
    // all ro+alloc+contents sections contiguous (in file and vm)
    // no ro+alloc-contents sections
    // all rw+alloc sections coniguous (in file and vm)

    return 0;
}
