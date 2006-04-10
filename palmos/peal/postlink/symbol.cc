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

#include <string>
#include <stdint.h>
#include "section.h"

#include "symbol.h"
#include "stringtable.h"

Symbol::Symbol(const string &newName, const Section *newSection, uint32_t newOffset, 
               uint8_t newBind, uint8_t newType)
    : mName(newName), mSection(newSection), mOffset(newOffset), 
      mBind(newBind), mType(newType), mSpecialSection(0) 
{ 
}

Symbol::Symbol(Image& image, const StringTable& strtab, Elf32_Sym *sym)
{
    mName = strtab[swap32(sym->st_name)];
    uint16_t shndx = swap16(sym->st_shndx);
    mSpecialSection = shndx;
    if (shndx == SHN_ABS) {
        mSection = NULL;
    } else if (shndx == SHN_COMMON) {
        unimplemented("unresolved common symbol '%s'", mName.c_str());
    } else {
        mSection = image.sections()[swap16(sym->st_shndx)];
    }
    mOffset = swap32(sym->st_value) - (mSection ? mSection->vmaddr() : 0);
    mBind = ELF32_ST_BIND(sym->st_info);
    mType = ELF32_ST_TYPE(sym->st_info);
}


const string& Symbol::name(void) const { return mName; }
const Section *Symbol::section(void) const { return mSection; }
bool Symbol::isGlobal(void) const { return mBind == STB_GLOBAL; }
uint32_t Symbol::offset(bool interwork) const 
{ 
    if (interwork  &&  mType == STT_LOPROC) return mOffset+1; 
    else return mOffset;
}
uint32_t Symbol::vmaddr(bool interwork) const 
{ 
    return offset(interwork) + (mSection ? mSection->vmaddr() : 0); 
}

Elf32_Sym Symbol::asElf(const Image& image) const
{
    Elf32_Sym result;
    result.st_name = swap32(image.strtab()->indexOf(mName));
    result.st_value = swap32(offset());
    result.st_size = swap32(0); // fixme?
    result.st_info = ELF32_ST_INFO(mBind, mType);  // 1 byte, don't swap
    result.st_other = 0;

    if (mSection) {
        vector<Section *>::const_iterator pos = find(image.sections().begin(), image.sections().end(), mSection);
        if (pos == image.sections().end()) {
            result.st_shndx = 0;
        } else {
            result.st_shndx = swap16(pos - image.sections().begin());
        }
    } else {
        result.st_shndx = swap16(mSpecialSection);
    }
    
    return result;
}
