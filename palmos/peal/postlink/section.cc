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
#include <vector>
#include <stdint.h>

#include "elf.h"
#include "got.h"
#include "swap.h"
#include "image.h"
#include "symbol.h"
#include "complain.h"
#include "relocation.h"
#include "stringtable.h"

#include "section.h"


Section::Section(Image& newImage)
    : mImage(newImage)
{ }

Section::Section(Image &newImage, string newName, uint32_t newType, uint32_t newFlags, uint32_t newVMAddr, uint8_t *newContents, uint32_t newSize)
    : mName(newName), 
      mType(newType), 
      mFlags(newFlags), 
      mVMaddr(newVMAddr), 
      mAlign(4), 
      mSize(newSize), 
      mContents(newContents), 
      mLink(0), 
      mInfo(0), 
      mEntrySize(0), 
      mImage(newImage), 
      mBaseSymbol(NULL)
{ }

void Section::read(Elf32_Shdr *shdr)
{
    if (mImage.strtab()) {
        mName = (*mImage.strtab())[swap32(shdr->sh_name)];
    } else {
        mName = ".shstrtab";  // this is the section name strtab itself
    }
    mType = swap32(shdr->sh_type);
    mFlags = swap32(shdr->sh_flags);
    mVMaddr = swap32(shdr->sh_addr);
    mAlign = swap32(shdr->sh_addralign);
    mSize = swap32(shdr->sh_size);
    mContents = mImage.contents() + swap32(shdr->sh_offset);
    // relocations will be read later
    mLink = 0;
    mInfo = 0;
    mEntrySize = swap32(shdr->sh_entsize);
    mBaseSymbol = NULL;
}

void Section::reserveThumbThunk(const Relocation &r)
{
    Thunk query(r.symbol(), r.addend());
    vector<Thunk>::iterator t;
    // fixme slow, should use hash_map
    t = find(mThumbThunks.begin(), mThumbThunks.end(), query);
    if (t == mThumbThunks.end()) mThumbThunks.push_back(query);
}

void Section::reserveARMThunk(const Relocation &r)
{
    Thunk query(r.symbol(), r.addend());
    vector<Thunk>::iterator t;
    // fixme slow, should use hash_map
    t = find(mARMThunks.begin(), mARMThunks.end(), query);
    if (t == mARMThunks.end()) mARMThunks.push_back(query);
}

void Section::reserveThunkFromThumb(const Relocation &r, uint16_t insn1, 
                                    uint16_t insn2)
{
    uint16_t opcode1 = insn1 >> 11;
    uint16_t opcode2 = insn2 >> 11;
    if (opcode1 == 0x001e  &&  opcode2 == 0x001f) {
        // BL: Thumb->Thumb branch, use Thumb thunk
        reserveThumbThunk(r);
    } else if (opcode1 == 0x001e  &&  opcode2 == 0x001d) {
        // BLX: Thumb->ARM branch, use ARM thunk
        reserveARMThunk(r);
    } else {
        unimplemented("unexpected R_ARM_THM_PC22 relocation for Thumb insns "
                      "0x%x, 0x%x", insn1, insn2);
    }
}

void Section::reserveThunkFromARM(const Relocation &r, uint32_t insn)
{
    uint32_t cond = (insn & 0xf0000000) >> 28;
    uint32_t opcode = (insn & 0x0e000000) >> 25;
    if (opcode == 0x05) {
        if (cond == 0x0f) {
            // BLX: ARM->Thumb branch, use Thumb thunk
            reserveThumbThunk(r);
        } else {
            // Bcc, BLcc: ARM->ARM branch, use ARM thunk
            reserveARMThunk(r);
        }
    } else {
        // unknown instruction
        unimplemented("unexpected R_ARM_PC24 relocation for ARM insn 0x%x", 
                      insn);
    }
}

void Section::applyRelocations(Elf32_Shdr *rhdr)
{
    Elf32_Rel *rel = (Elf32_Rel *)
        (mImage.contents() + swap32(rhdr->sh_offset));
    Elf32_Rel *relEnd = (Elf32_Rel *)
        ((uint8_t *)rel + swap32(rhdr->sh_size));
    
    for ( ; rel < relEnd; ++rel) {
        Relocation r(rel, mImage.symtab(), *this);
        const Section *symSection = r.symbol()->section();
        if (!symSection) unimplemented("relocation relative to absolute or common symbol");
        
        switch (r.type()) {
        case R_ARM_PC24:
            // Linker stored the offset from here to a symbol in a branch insn.
            // This needs no additional relocation if here and the symbol 
            // are in the same section.
            if (this == symSection) {
                // Source and dest are in the same section - nothing to do
            } else {
                // Source and dest are in different sections, which may slide
                // relative to each other - create a relocation and a thunk
                int32_t offset = peek32(r.offset()) & 0x00ffffff; // SIGNED!!
                // sign-extend and convert to byte offset from start of insn
                offset = ((offset << 8) >> 8) * 4 + 8;
                Relocation r2 = Relocation(R_ARM_PC24, r.offset(), 
                                           symSection->baseSymbol(), 
                                           vmaddr() + r.offset() + 
                                           offset - symSection->vmaddr());
                mRelocations.push_back(r2);
                reserveThunkFromARM(r2, peek32(r.offset()));
            }
            break;

        case R_ARM_THM_PC22:
            // Like R_ARM_PC24, except each of the next two Thumb 
            // instructions contains half of a 22-bit offset.
            if (this == symSection) {
                // Source and dest are in the same section - nothing to do
            } else {
                // Source and dest are in different sections, which may slide
                // relative to each other - create a relocation and a thunk
                int32_t offset_hi = peek16(r.offset()) & 0x07ff;
                int32_t offset_lo = peek16(r.offset()+2) & 0x07ff;
                int32_t offset = (offset_hi << 11) | offset_lo;
                // sign-extend and convert to byte offset from start of insn
                offset = ((offset << 10) >> 10) * 2 + 4;
                Relocation r2 = Relocation(R_ARM_THM_PC22, r.offset(), 
                                           symSection->baseSymbol(), 
                                           vmaddr() + r.offset() + 
                                           offset - symSection->vmaddr());
                
                mRelocations.push_back(r2);
                reserveThunkFromThumb(r2, peek16(r.offset()),
                                      peek16(r.offset()+2));
            }
            break;

        case R_ARM_GOTOFF:
            // Linker stored the offset from the GOT to a target symbol.
            // This needs no additional relocation if the symbol is in r/w mem.
            if (!symSection->isReadOnly()) {
                // Target symbol is in a r/w section - nothing to do
                // At runtime, the GOT and all r/w sections will be 
                // in the same places relative to each other, so 
                // linker-generated GOT offsets will be correct.
            } else {
                // Target symbol is in a r/o section - create relocation
                // At runtime, the GOT and the r/o sections will move 
                // relative to each other. 
                mRelocations.push_back
                    (Relocation(R_ARM_GOTOFF, r.offset(),
                                symSection->baseSymbol(), 
                                peek32(r.offset()) - (symSection->vmaddr() - mImage.got().vmaddr())));
            }
            break;

        case R_ARM_GOT32: {
            // Linker stored the absolute address of a symbol in the GOT. 
            // Find the GOT entry containing the symbol, and create a 
            // relocation that will change the *GOT entry* at runtime.
            // These new relocations aren't actually created until 
            // GOT::buildRelocations() is called.
            uint32_t sym_vmaddr = r.symbol()->vmaddr(thumb);
            GOT& got = mImage.got();
            unsigned int g;
            for (g = 0; g < got.entries().size(); g++) {
                if (got.entries()[g] == sym_vmaddr) {
                    if (got.symbols()[g] == NULL) {
                        got.symbols()[g] = r.symbol();
                    } else if (got.symbols()[g] != r.symbol()) {
                        error("single GOT entry has multiple R_ARM_GOT32 uses");
                    }
                    break;
                }
            }
            if (g == got.entries().size()) {
                error("R_ARM_GOT32 relocation has no associated GOT entry (symbol %s, symbol vmaddr 0x%x, section %s", r.symbol()->name().c_str(), sym_vmaddr, symSection->name().c_str());
            }
            break;
        }
        case R_ARM_ABS32:
            // Linker stored an absolute address at r.offset() in this section
            // Subtract symbol section's linker-determined vmaddr and relocate 
            // at runtime using symbol section's base symbol.
            mRelocations.push_back
                (Relocation(R_ARM_ABS32, r.offset(), symSection->baseSymbol(), 
                            peek32(r.offset()) - symSection->vmaddr()));
            break;

        case R_ARM_REL32:
            // Linker stored the offset between here and the symbol.
            // If here and the symbol are in different sections, they 
            // may move relative to each other at runtime. 
            if (this == symSection) {
                // Source and dest are in the same section - nothing to do
            } else {
                // Source and dest are in different sections, which may slide
                // relative to each other - create a relocation
                mRelocations.push_back
                    (Relocation(R_ARM_REL32, r.offset(), 
                                symSection->baseSymbol(), 
                                vmaddr() + r.offset() + peek32(r.offset()) - symSection->vmaddr()));
            }
            break;

        case R_ARM_GOTPC:
            // Like R_ARM_REL32, but the symbol is the start of the GOT.
            // Replace with an ordinary R_ARM_REL32 relocation that uses 
            // the GOT's section symbol instead of _GLOBAL_OFFSET_TABLE_.
            if (isReadOnly()) {
                mRelocations.push_back
                    (Relocation(R_ARM_REL32, r.offset(),
                                mImage.got().baseSymbol(), 
                                vmaddr() + r.offset() + peek32(r.offset()) - mImage.got().vmaddr()));
            } else {
                // read/write sections do not move relative to the GOT.
            }
            break;

        case R_ARM_PLT32:
            warning("ignoring R_ARM_PLT32 relocation (symbol '%s')", 
                    r.symbol()->name().c_str());
            break;
            
        default:
            unimplemented("relocation type %d", r.type());
            break;
        }
    }
}

bool Section::isReadOnly(void) const { return ! (mFlags & SHF_WRITE); }
const string& Section::name(void) const { return mName; }
uint32_t Section::vmaddr(void) const { return mVMaddr; }
uint32_t Section::type(void) const { return mType; }

Symbol *Section::baseSymbol(void) const 
{
    if (!mBaseSymbol) {
        mBaseSymbol = new Symbol(mName, this, 0, STB_GLOBAL, STT_SECTION);
    }
    return mBaseSymbol;
}

void Section::setLink(uint32_t newLink) { mLink = newLink; }
void Section::setInfo(uint32_t newInfo) { mInfo = newInfo; }
void Section::setEntrySize(uint32_t newEntrySize) { mEntrySize = newEntrySize; }


void Section::emit(Elf32_Shdr *shdr, uint8_t *&buf, uint32_t& bufLen)
{
    uint32_t offset = bufLen;


    // add blank space for relocation thunks
    mThunkSize = 0;
    mThunkSize += mThumbThunks.size() * 16;       // Thumb thunks 16 bytes each
    mThunkSize += mARMThunks.size() * 8;          // ARM thunks 8 bytes each
    if (mThunkSize && (mSize % 4)) mThunkSize += 4 - (mSize%4);  // thunks are 4-byte aligned
    mSize += mThunkSize;

    // write shdr before buf gets reallocated (shdr points into buf)
    shdr->sh_name = swap32(mImage.strtab()->indexOf(mName));
    shdr->sh_type = swap32(mType);
    shdr->sh_flags = swap32(mFlags);
    shdr->sh_addr = swap32(mVMaddr);
    shdr->sh_offset = swap32(offset);
    shdr->sh_size = swap32(mSize);
    shdr->sh_link = swap32(mLink);
    shdr->sh_info = swap32(mInfo);
    shdr->sh_addralign = swap32(mAlign);
    shdr->sh_entsize = swap32(mEntrySize);

    if (mType != SHT_NOBITS) {
        bufLen += mSize;
        buf = (uint8_t *)realloc(buf, bufLen);
        memcpy(buf+offset, mContents, mSize - mThunkSize);
        memset(buf+offset+mSize-mThunkSize, 0, mThunkSize);
    }
}
