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

#include "section.h"
#include "symboltable.h"
#include "stringtable.h"

SymbolTable::SymbolTable(Image& newImage)
    : Section(newImage)
{ 
    // symbol table with single empty symbol, unless replaced by read().
    mSymbols.push_back(new Symbol(string(""), mImage.sections()[0], 0, 0, 0));
}


void SymbolTable::read(Elf32_Shdr *shdr)
{ 
    Section::read(shdr);

    vector<Section *>& sections = mImage.sections();
    Section *section = sections[swap32(shdr->sh_link)];
    StringTable *strtab = dynamic_cast<StringTable *>(section);
    Elf32_Sym *sym = (Elf32_Sym *)mContents;
    Elf32_Sym *end = (Elf32_Sym *)(mContents + mSize);

    mSymbols.erase(mSymbols.begin(), mSymbols.end());
    for ( ; sym < end; ++sym) {
        mSymbols.push_back(new Symbol(mImage, *strtab, sym));
    }
}


// strip all local symbols
// strip all globals symbols not mentioned in keep[] (if it exists)
void SymbolTable::strip(vector<string> *keep)
{
    vector<Symbol *> newSymbols;
    newSymbols.push_back(new Symbol(string(""), mImage.sections()[0], 0, 0, 0));
    vector<Symbol *>::iterator iter;
    for (iter = mSymbols.begin(); iter != mSymbols.end(); ++iter)
    {
        Symbol *sym = *iter;
        const string &name = sym->name();

        if (name == "PealArmStub"  ||  name == "_PealArmStub") {
            // PealArmStub is special - never strip it
            newSymbols.push_back(sym);
        }
        else if (!sym->isGlobal()) {
            // local symbol - strip it
        } 
        else if (keep  &&  
                 keep->end() == find(keep->begin(), keep->end(), name))
        {
            // keep list exists, but doesn't have this symbol - strip it
        } 
        else {
            newSymbols.push_back(sym);
        }
    }

    mSymbols = newSymbols;
}


void SymbolTable::addSymbol(Symbol *sym)
{
    mSymbols.push_back(sym);
}


void SymbolTable::emit(Elf32_Shdr *shdr, uint8_t *&buf, uint32_t& bufLen)
{
    // construct mContents and mSize
    mSize = mSymbols.size() * sizeof(Elf32_Sym);
    Elf32_Sym *newContents = (Elf32_Sym *)malloc(mSize);

    for (unsigned int i = 0; i < mSymbols.size(); i++) {
        newContents[i] = mSymbols[i]->asElf(mImage);
    }

    mContents = (uint8_t *)newContents;

    // set mLink to strtab
    mLink = find(mImage.sections().begin(), mImage.sections().end(), mImage.strtab()) - mImage.sections().begin();

    Section::emit(shdr, buf, bufLen);
}
