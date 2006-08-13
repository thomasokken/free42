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

#ifndef RELOCATION_H
#define RELOCATION_H

#include "elf.h"

class Symbol;
class SymbolTable;
class Section;

class Relocation {
    // R_ARM_RELATIVE at runtime: 
    // *(section_base + mOffset) = mSymbol.vmaddr + mAddend
    // fixme this might not really be R_ARM_RELATIVE
    uint8_t mType;
    uint32_t mOffset;       // of address to change, in containing section
    int32_t mAddend;        // offset from symbol
    const Symbol *mSymbol;  // symbol whose vmaddr should be added

public:
    Relocation(uint8_t newType, uint32_t newOffset, const Symbol *newSymbol, int32_t newAddend);
    Relocation(const Elf32_Rel *rel, const SymbolTable& symtab, const Section& section);

    uint8_t type(void) const;
    uint32_t offset(void) const;
    int32_t addend(void) const;
    const Symbol *symbol(void) const;

    Elf32_Rela asElf(const SymbolTable& symtab) const;
};

#endif
