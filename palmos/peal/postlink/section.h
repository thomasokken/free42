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

#ifndef SECTION_H
#define SECTION_H

#include <string>
#include <vector>
#include <stdint.h>

using namespace std;

#include "elf.h"
#include "swap.h"
#include "relocation.h"

class Image;
class Symbol;

class Thunk {
 public:
    Thunk(const Symbol *newSymbol, int32_t newAddend)
        : symbol(newSymbol), addend(newAddend) { }
    bool operator==(const Thunk &other) const {
        return (symbol == other.symbol  &&  addend == other.addend);
    }
 protected:
    const Symbol *symbol;
    int32_t addend;
};

class Section {
 protected:
    string mName;
    uint32_t mType;
    uint32_t mFlags;
    uint32_t mVMaddr;
    uint32_t mAlign;
    uint32_t mSize;
    uint8_t *mContents;  // section's bytes, or NULL (NEEDS SWAP)

    uint32_t mLink;
    uint32_t mInfo;

    uint32_t mEntrySize;
    
    Image& mImage;
    vector<Relocation> mRelocations;  // to apply to this section
    mutable Symbol *mBaseSymbol;  // symbol pointing to this section, offset 0
    vector<Thunk> mARMThunks;     // thunks for branches out of this section
    vector<Thunk> mThumbThunks;   // thunks for branches out of this section
    size_t mThunkSize;

    void reserveARMThunk(const Relocation &r);
    void reserveThumbThunk(const Relocation &r);
    void reserveThunkFromARM(const Relocation &r, uint32_t insn);
    void reserveThunkFromThumb(const Relocation &r, uint16_t insn1, uint16_t insn2);

public:
    Section(Image& newImage);
    Section(Image &newImage, string newName, uint32_t newType, uint32_t newFlags, uint32_t newVMAddr, uint8_t *newContents, uint32_t newSize);
    virtual ~Section(void) { }

    virtual void read(Elf32_Shdr *shdr);

    void applyRelocations(Elf32_Shdr *rhdr);

    bool isReadOnly(void) const;
    const string& name(void) const;
    uint32_t vmaddr(void) const;
    uint32_t type(void) const;
    uint32_t flags(void) const { return mFlags; }
    uint32_t size(void) const { return mSize; }
    uint32_t thunkSize(void) const { return mThunkSize; }
    uint32_t alignment(void) const { return mAlign; }

    Symbol *baseSymbol(void) const;
    vector<Relocation>& relocations(void) { return mRelocations; }

    void setLink(uint32_t newLink);
    void setInfo(uint32_t newInfo);
    void setEntrySize(uint32_t newEntrySize);

    virtual void emit(Elf32_Shdr *shdr, uint8_t *&buf, uint32_t& bufLen);


    uint32_t peek32(uint32_t offset) { 
        uint32_t *src = (uint32_t *)(mContents+offset);
        return swap32(*src);
    }

    uint16_t peek16(uint32_t offset) { 
        uint16_t *src = (uint16_t *)(mContents+offset);
        return swap16(*src);
    }

    uint8_t peek8(uint32_t offset) { 
        uint8_t *src = (uint8_t *)(mContents+offset);
        return *src;
    }

    void poke32(uint32_t offset, uint32_t value) { 
        uint32_t *dst = (uint32_t *)(mContents+offset);
        *dst = swap32(value); 
    }
};

#endif
