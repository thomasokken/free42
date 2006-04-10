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

#ifndef STRINGTABLE_H
#define STRINGTABLE_H

#include <string.h>

#include <vector>
#include <string>

using namespace std;

#include "elf.h"
#include "swap.h"
#include "image.h"
#include "section.h"
#include "complain.h"


using namespace std;

class StringTable : public Section {

public:
    StringTable(Image& newImage)
        : Section(newImage, string(".strtab"), SHT_STRTAB, 0, 0, (uint8_t *)calloc(1, 1), 1)
    { 
        // strtab containing the empty string, unless read() replaces it
    }

    void read(Elf32_Shdr *shdr)
    {
        Section::read(shdr);

        char *strings = (char *)malloc(mSize);
        memcpy(strings, mContents, mSize);
        mContents = (uint8_t *)strings;
    }
    
    string operator[](int offset) const
    {
        return string((char *)mContents+offset);
    }

    void addString(const string& str) 
    {
        const char *cstr = str.c_str();
        int len = strlen(cstr) + 1;
        mContents = (uint8_t *)realloc((char *)mContents, mSize + len);
        memcpy((char *)mContents + mSize, cstr, len);
        mSize += len;
    }

    int indexOf(const string& str) const
    {
        const char *cstr = str.c_str();
        const char *strings = (const char *)mContents;
        for (const char *s = strings; 
             s < strings+mSize; 
             s++) 
        {
            if (0 == strcmp(s, cstr)) return s - strings;
        }
        error("string %s not in strtab", cstr);
        return -1;
    }
};

#endif
