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

#include "elf.h"
#include "swap.h"
#include "symboltable.h"
#include "complain.h"

#include "relocation.h"


Relocation::Relocation(uint8_t newType, uint32_t newOffset, const Symbol *newSymbol, int32_t newAddend) 
    : mType(newType), 
      mOffset(newOffset), 
      mAddend(newAddend), 
      mSymbol(newSymbol) 
{ 
    // no local-symbol-relative relocations allowed
    if (!mSymbol->isGlobal()) 
        error("attempted relocation relative to non-global symbol '%s'", 
              mSymbol->name().c_str());
}

Relocation::Relocation(const Elf32_Rel *rel, const SymbolTable& symtab, const Section &section)
    : mType(ELF32_R_TYPE(swap32(rel->r_info))), 
      mOffset(swap32(rel->r_offset)), 
      mAddend(0), 
      mSymbol(symtab[ELF32_R_SYM(swap32(rel->r_info))])
{ 
    // use offset in section, not vm address
    mOffset -= section.vmaddr();
}

uint8_t Relocation::type(void) const { return mType; }
uint32_t Relocation::offset(void) const { return mOffset; }
int32_t Relocation::addend(void) const { return mAddend; }
const Symbol *Relocation::symbol(void) const { return mSymbol; }

Elf32_Rela Relocation::asElf(const SymbolTable& symtab) const 
{
    Elf32_Rela result;
    result.r_offset = swap32(mOffset);
    result.r_info = swap32(ELF32_R_INFO(symtab.indexOf(mSymbol), mType));
    result.r_addend = swap32(mAddend);
    return result;
}
