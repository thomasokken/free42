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

#ifndef GOT_H
#define GOT_H

#include <stdint.h>
#include <vector>

using namespace std;

#include "swap.h"
#include "section.h"
#include "relocation.h"
#include "symbol.h"
#include "complain.h"

class GOT : public Section {
    vector<uint32_t> mEntries;
    vector<const Symbol *> mSymbols; 

public:
    GOT(Image& newImage)
        : Section(newImage)
    { }
    
    void read(Elf32_Shdr *shdr)
    {
        Section::read(shdr);

        uint32_t *entryArray = (uint32_t *)mContents;
        mEntries.resize(mSize / 4);
        mSymbols.resize(mSize / 4);
        for (unsigned int i = 0; i < mSize / 4; i++) {
            mEntries[i] = swap32(entryArray[i]);
            mSymbols[i] = NULL;
        }
    }

    const vector<uint32_t>& entries(void) const { return mEntries; }
    vector<const Symbol *>& symbols(void) { return mSymbols; }

    void buildRelocations(void) 
    {
        // GOT entries may refer to local symbols, which will be stripped. 
        // Instead, use section symbol + offset in section.

        mContents = (uint8_t *)calloc(mEntries.size(), 4);
        mSize = mEntries.size() * 4;

        for (unsigned int g = 0; g < mSymbols.size(); g++) {
            if (mSymbols[g]) {
                if (!mSymbols[g]->section()) {
                    unimplemented("GOT entry is an absolute or other section-less symbol");
                }
                Symbol *sectionSymbol = mSymbols[g]->section()->baseSymbol();
                mRelocations.push_back
                    (Relocation(R_ARM_ABS32, g*4, sectionSymbol, 
                                mSymbols[g]->offset(thumb)));
            }
        }
    }

    void emit(Elf32_Shdr *shdr, uint8_t *&buf, uint32_t& bufLen)
    {
        // Emitted GOT is all zero. Use NOBITS rather than PROGBITS.
        mType = SHT_NOBITS;
        Section::emit(shdr, buf, bufLen);
    }
};

#endif
