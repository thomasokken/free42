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

#ifndef SYMBOLTABLE_H
#define SYMBOLTABLE_H

#include <vector>
#include <algorithm>

using namespace std;

#include "symbol.h"
#include "section.h"

class SymbolTable : public Section {
    vector<Symbol *> mSymbols;

public:
    SymbolTable(Image& newImage);
    void read(Elf32_Shdr *shdr);
    void strip(vector<string> *keep);
    void addSymbol(Symbol *sym);

    vector<Symbol *>::reference operator[](int index) {
        return mSymbols[index];
    }

    Symbol *get(int index) {
        return mSymbols[index];
    }

    vector<Symbol *>::const_reference operator[](int index) const {
        return mSymbols[index];
    }

    size_t indexOf(const Symbol *sym) const {
        return find(mSymbols.begin(), mSymbols.end(), sym) - mSymbols.begin();
    }

    size_t size(void) const { return mSymbols.size(); }

    void emit(Elf32_Shdr *shdr, uint8_t *&buf, uint32_t& bufLen);
};

#endif
