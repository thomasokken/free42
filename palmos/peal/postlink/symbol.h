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

#ifndef SYMBOL_H
#define SYMBOL_H

#include <string>
#include <stdint.h>

using namespace std;

#include "elf.h"

class Image;
class Section;
class StringTable;

// used for vmaddr() and offset() to get Thumb interworkable address
#define thumb true

class Symbol {
    string mName; // of symbol
    const Section *mSection;  // containing symbol
    uint32_t mOffset;  // of symbol's data in containing section
    uint8_t mBind;
    uint8_t mType;
    uint16_t mSpecialSection; // if mSection == 0, this is st_shndx
    
public:
    Symbol(const string &newName, const Section *newSection, 
           uint32_t newOffset, uint8_t newBind, uint8_t newType);
    Symbol(Image& image, const StringTable &strtab, Elf32_Sym *sym);

    const string& name(void) const;
    const Section *section(void) const;
    uint32_t offset(bool interwork = false) const;
    bool isGlobal(void) const; 
    char type(void) const { return mType; }

    uint32_t vmaddr(bool interwork = false) const;

    Elf32_Sym asElf(const Image& image) const;
};

#endif
