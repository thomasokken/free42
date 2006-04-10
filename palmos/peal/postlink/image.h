/**********
 * Copyright (c) 2004 Greg Parker.  All rights reserved.
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

#ifndef IMAGE_H
#define IMAGE_H

#include <vector>

using namespace std;

class Section;
class SymbolTable;
class StringTable;
class GOT;

class Image {
    uint8_t *mContents;
    Elf32_Ehdr mEhdr;
    vector<Elf32_Shdr *>sectionPointers;
    vector<Section *> mSections;
    SymbolTable *mSymtab;
    StringTable *sectionNames;
    GOT *mGOT;

    void allocate_sections(void);
    void read_sections(void);

public:
    Image(uint8_t *buf);
    uint8_t *contents(void) { return mContents; }
    vector<Section *>& sections(void) { return mSections; }
    const vector<Section *>& sections(void) const { return mSections; }
    StringTable *strtab(void) { return sectionNames; }
    const StringTable *strtab(void) const { return sectionNames; }
    SymbolTable& symtab(void) { return *mSymtab; }
    GOT& got(void) { return *mGOT; }

    void addSectionGlobals(void);
    void trimSections(void);
    void buildSymbolStringTable(void);
    void buildRelocations(void);
    void buildSectionStringTable(void);

    void write(const char *type, int id, const char *name);
};
#endif
