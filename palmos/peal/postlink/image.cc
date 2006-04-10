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

#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <fcntl.h>

#include "elf.h"
#include "got.h"
#include "swap.h"
#include "image.h"
#include "section.h"
#include "complain.h"
#include "symboltable.h"
#include "stringtable.h"

#ifndef O_BINARY
#define O_BINARY 0
#endif

// Max Palm resource size (actually 65505 for 3.0+)
// Note: this number must be in sync with peal.c.
#define RESOURCE_MAX 65400

// .prc header (68K-swapped!)
typedef struct {
    char name[32];
    uint16_t attr;
    uint16_t version;
    uint32_t created;
    uint32_t modified;
    uint32_t backup;
    uint32_t modnum;
    uint32_t appinfo;
    uint32_t sortinfo;
    char type[4];
    char creator[4];
    uint32_t uidseed;
    uint32_t nextlist;
    uint16_t count;
} __attribute__((packed,aligned(1))) prc_header;


// .prc resource header (68K-swapped!)
typedef struct {
    char type[4];
    uint16_t id;
    uint32_t offset;
} __attribute__((packed,aligned(1))) res_header;

void Image::allocate_sections()
{
    Elf32_Shdr *shdr = (Elf32_Shdr *)(mContents + mEhdr.e_shoff);
    mSections.resize(mEhdr.e_shnum);
    for (int i = 0; i < mEhdr.e_shnum; i++) {
        Elf32_Shdr *s = (Elf32_Shdr *)((uint8_t *)shdr+i*mEhdr.e_shentsize);
        Elf32_Word type = swap32(s->sh_type);
        sectionPointers.push_back(s);

        switch (type) {
        case SHT_NULL: 
        case SHT_PROGBITS:
        case SHT_NOBITS:
            mSections[i] = new Section(*this);
            break;
        case SHT_SYMTAB:
            mSections[i] = new SymbolTable(*this);
            break;
        case SHT_STRTAB:
            mSections[i] = new StringTable(*this);
            break;
        case SHT_REL:
            // nothing to do
            break;
        default:
            unimplemented("unrecognized section type %d\n", type);
            break;
        }
    }
}
    
void Image::read_sections()
{
    // strtabs
    for (unsigned int i = 0; i < sectionPointers.size(); i++) {
        Elf32_Shdr *s = sectionPointers[i];
        Elf32_Word type = swap32(s->sh_type);
        if (type == SHT_STRTAB) {
            mSections[i]->read(s);
            if (i == mEhdr.e_shstrndx) sectionNames = (StringTable *)mSections[i];
        }
    }
    

    // null
    for (unsigned int i = 0; i < sectionPointers.size(); i++) {
        Elf32_Shdr *s = sectionPointers[i];
        Elf32_Word type = swap32(s->sh_type);
        if (type == SHT_NULL) {
            mSections[i]->read(s);
        }
    }


    // allocated
    for (unsigned int i = 0; i < sectionPointers.size(); i++) {
        Elf32_Shdr *s = sectionPointers[i];
        Elf32_Word type = swap32(s->sh_type);
        if (type == SHT_PROGBITS  ||  type == SHT_NOBITS) {
            mSections[i]->read(s);
            if (!mGOT  &&  mSections[i]->name() == ".got") {
                delete mSections[i];
                mGOT = new GOT(*this);
                mSections[i] = mGOT;
                mGOT->read(s);
            }
        }
    }
    
    
    // symtab
    for (unsigned int i = 0; i < sectionPointers.size(); i++) {
        Elf32_Shdr *s = sectionPointers[i];
        Elf32_Word type = swap32(s->sh_type);
        if (type == SHT_SYMTAB) {
            mSections[i]->read(s);
            mSymtab = (SymbolTable *)mSections[i];
        }
    }
    
    
    // relocations
    for (unsigned int i = 0; i < sectionPointers.size(); i++) {
        Elf32_Shdr *s = sectionPointers[i];
        Elf32_Word type = swap32(s->sh_type);
        if (type == SHT_REL) {
            mSections[swap32(s->sh_info)]->applyRelocations(s);
        }
    }
}


Image::Image(uint8_t *buf)
{
    mContents = buf;
    memcpy(&mEhdr, mContents, sizeof(mEhdr));
    swap_ehdr(&mEhdr);

    sectionNames = NULL;
    mGOT = NULL;
    mSymtab = NULL;

    // Perform some sanity checks against the ELF header
    
    if (!IS_ELF(mEhdr)) {
        error("not an ELF file (bad magic number)");
    }

    if (mEhdr.e_ident[EI_CLASS] != ELFCLASS32) {
        error("not a 32-bit ELF file");
    }

    if (mEhdr.e_ident[EI_DATA] != ELFDATA2LSB) {
        error("not a little-endian ELF file");
    }

    if (mEhdr.e_ident[EI_VERSION] != EV_CURRENT) {
        error("not a version %d ELF file", EV_CURRENT);
    }

    if (mEhdr.e_ident[EI_OSABI] != ELFOSABI_ARM) {
        error("not an ARM ABI ELF file");
    }

    if (mEhdr.e_type != ET_EXEC) {
        error("not an executable ELF file");
    }

    if (mEhdr.e_machine != EM_ARM) {
        error("not an ARM machine ELF file");
    }

    if (mEhdr.e_version != EV_CURRENT) {
        error("not a version %d ELF file", EV_CURRENT);
    }


    // Allocate all sections. They must all be allocated before any are 
    // read because of cyclic references. 

    allocate_sections();


    // Read all sections.

    read_sections();
}


void Image::addSectionGlobals(void)
{
    vector<Section *>::iterator iter;
    for (iter = mSections.begin(); iter != mSections.end(); ++iter) 
    {
        Section *section = *iter;
        if (section  &&  (section->type() == SHT_NOBITS  ||  
                          section->type() == SHT_PROGBITS)) 
        {
            Symbol *sym = section->baseSymbol();
            mSymtab->addSymbol(sym);
        }
    }
}


void Image::trimSections(void)
{
    vector<Section *> newSections;

    for (unsigned int i = 0; i < mSections.size(); i++) {
        Section *s = mSections[i];
        if (!s) continue;
        uint32_t type = s->type();

        // elided: relocs, strtabs other than main strtab
        if (!(type == SHT_NULL  ||  type == SHT_SYMTAB  ||  
              type == SHT_NOBITS  ||  type == SHT_PROGBITS))
            continue;
        // elided: useless named sections
        if (s->name() == ".disposn"  ||  s->name() == ".got.plt"  ||  
            s->name() == ".comment"  ||  
            0 == strncmp(s->name().c_str(), ".debug", 6)  ||
            0 == strncmp(s->name().c_str(), ".debu.", 6))
            continue;

        newSections.push_back(s);
    }

    mSections = newSections;
}


void Image::buildSymbolStringTable(void)
{
    sectionNames = new StringTable(*this);
    mSections.push_back(sectionNames);

    for (unsigned int i = 0; i < mSymtab->size(); i++) {
        sectionNames->addString(mSymtab->get(i)->name());
    }
}


void Image::buildSectionStringTable(void)
{
    for (unsigned int i = 0; i < mSections.size(); i++) {
        if (!mSections[i]) continue;
        sectionNames->addString(mSections[i]->name());
    }
}


void Image::buildRelocations(void)
{
    vector<Section *> newSections;

    if (mGOT) mGOT->buildRelocations();

    for (unsigned int i = 0; i < mSections.size(); i++) {
        Section *s = mSections[i];
        vector<Relocation>& rels = s->relocations();
        if (rels.size() == 0) continue;

        Elf32_Rela *relBytes = new Elf32_Rela[rels.size()];
        for (unsigned int r = 0; r < rels.size(); r++) {
            relBytes[r] = rels[r].asElf(*mSymtab);
        }

        string name = string(".rela") + s->name();
        Section *relSection = new Section(*this, name, SHT_RELA, 0, 0, (uint8_t *)relBytes, rels.size() * sizeof(Elf32_Rela));
        rels.erase(rels.begin(), rels.end());
        
        relSection->setLink(find(mSections.begin(), mSections.end(), mSymtab) - mSections.begin());
        relSection->setInfo(i);
        relSection->setEntrySize(sizeof(Elf32_Rela));
        newSections.push_back(relSection);
    }

    mSections.insert(mSections.end(), newSections.begin(), newSections.end());
}


// if id == -1:
//     write 'name' file in .ro format with a resource for each section, starting with resource ID baseID
// else
//     write 'name' file in .bin format, ignoring resType and baseID
void Image::write(const char *resType, int baseID, const char *name)
{
    uint32_t bufLen = sizeof(Elf32_Ehdr) + mSections.size()*sizeof(Elf32_Shdr);
    uint8_t *buf = (uint8_t *)calloc(bufLen, 1);
    int lastID;

    // fill out section data and section headers
    bool toobig = false;
    for (unsigned int i = 0; i < mSections.size(); i++) {
        Elf32_Shdr *shdr = (Elf32_Shdr *)(buf + sizeof(Elf32_Ehdr));
        Section *s = mSections[i];
        uint32_t offset = bufLen;
        s->emit(shdr+i, buf, bufLen);

        if (s->type() == SHT_NOBITS) {
            // .bss et al take no space in resources - no size check
            inform("emitting section '%s' (type %d flags %d) at 0x%x (%d bytes)", s->name().c_str(), s->type(), s->flags(), offset, s->size());
        } else {
            if (s->thunkSize()) {
                inform("emitting section '%s' (type %d flags %d) at 0x%x (%d bytes [%d for thunks])", s->name().c_str(), s->type(), s->flags(), offset, s->size(), s->thunkSize());
            } else {
                inform("emitting section '%s' (type %d flags %d) at 0x%x (%d bytes)", s->name().c_str(), s->type(), s->flags(), offset, s->size());
            }
            if (baseID != -1  &&  s->size() > RESOURCE_MAX) {
                toobig = true;
            }
        }

        if (s->isReadOnly()  &&  s->alignment() > 4) {
            warning("Read-only section '%s' is %d-byte aligned. "
                    "Peal only guarantees 4-byte alignment at runtime.", 
                    s->name().c_str(), s->alignment());
        }
    }
    if (toobig) {
        warning("Some sections are over %d bytes. "
                "Consider using ld's --split-by-file to reduce section size "
                "and save dynamic heap memory at runtime.", 
                RESOURCE_MAX);
    }

    // fill out ELF header
    Elf32_Ehdr *ehdr = (Elf32_Ehdr *)buf;
    memcpy(ehdr, &mEhdr, sizeof(mEhdr));
    ehdr->e_entry = 0;
    ehdr->e_phoff = 0;
    ehdr->e_shoff = sizeof(Elf32_Ehdr);
    ehdr->e_phentsize = 0;
    ehdr->e_phnum = 0;
    ehdr->e_shentsize = sizeof(Elf32_Shdr);
    ehdr->e_shnum = mSections.size();
    ehdr->e_shstrndx = find(mSections.begin(), mSections.end(), sectionNames) - mSections.begin();

    ehdr->e_type = ET_REL;

    int fd = open(name, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, 0666);
    if (fd < 0) { perror(name); exit(1); }

    if (baseID == -1) {
        // write everything together
        swap_ehdr(ehdr);
        ::write(fd, buf, bufLen);
        swap_ehdr(ehdr);
    } else {
        // count non-empty resources
        // resource baseID+0 is ehdr+shdrs
        // additional sections are in consecutive resources
        // sections bigger than 64 KB are split into multiple resources
        // section 0 (SHT_NULL) is skipped
        int res_count = 0;
        res_count++; // ehdr+shdrs
        for (unsigned i = 1; i < ehdr->e_shnum; i++) {
            Elf32_Shdr *shdr = i+(Elf32_Shdr *)(ehdr+1);
            uint32_t type = swap32(shdr->sh_type);
            size_t size = swap32(shdr->sh_size);
            if (size == 0  ||  type == SHT_NULL  ||  type == SHT_NOBITS) {
                continue; // no resource
            }
            res_count += (size + RESOURCE_MAX - 1) / RESOURCE_MAX;
        }

        // gather .prc header (68K-swapped!)
        // Most fields are blank because this is only enough of a .prc 
        // to work with build-prc.
        prc_header prc;
        memset(&prc, 0, sizeof(prc));        
        strcpy(prc.name, "foo");
        prc.attr = swap16_68K(1); // dmHdrAttrResDB
        prc.version = swap16_68K(1);
        strncpy(prc.type, "RESO", 4);
        strncpy(prc.creator, "pRES", 4);
        prc.count = swap16_68K(res_count);

        // gather resource headers
        // resource baseID+0 is ehdr+shdrs
        // additional sections are in consecutive resources
        // sections bigger than 64 KB are split into multiple resources
        // section 0 (SHT_NULL) is skipped
        int r;
        res_header res[res_count];
        ptrdiff_t offset;

        r = 0;
        lastID = baseID;
        offset = sizeof(prc) + sizeof(res) + 2;

        strncpy(res[r].type, resType, 4);
        res[r].id = swap16_68K(lastID);
        res[r].offset = swap32_68K(offset);

        r++;
        lastID++;
        offset += sizeof(*ehdr) + ehdr->e_shnum*ehdr->e_shentsize;

        for (unsigned int i = 1; i < ehdr->e_shnum; i++) {
            Elf32_Shdr *shdr = i+(Elf32_Shdr *)(ehdr+1);
            uint32_t type = swap32(shdr->sh_type);
            size_t size = swap32(shdr->sh_size);
            if (size == 0  ||  type == SHT_NULL  ||  type == SHT_NOBITS) {
                continue; // no resource
            }

            while (1) {
                strncpy(res[r].type, resType, 4);
                res[r].id = swap16_68K(lastID);
                res[r].offset = swap32_68K(offset);

                r++;
                lastID++;
                if (size > RESOURCE_MAX) {
                    // section too big - do another resource
                    offset += RESOURCE_MAX;
                    size -= RESOURCE_MAX;
                } else {
                    offset += size;
                    break;
                }
            }
        }

        // write prc header and resource headers
        uint16_t gap = 0;
        ::write(fd, &prc, sizeof(prc));
        ::write(fd, res, sizeof(res));
        ::write(fd, &gap, 2);
        
        // write resource data
        swap_ehdr(ehdr);
        ::write(fd, buf, bufLen);
        swap_ehdr(ehdr);
    }

    close(fd);
    if (baseID == -1) {
        inform("wrote file %s", name);
    } else {
        inform("wrote file %s (resource type '%s', id %d..%d)", name, resType, baseID, lastID-1);
    }
}

