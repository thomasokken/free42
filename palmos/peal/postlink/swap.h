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

#ifndef SWAP_H
#define SWAP_H

#include "elf.h"


static inline uint32_t swap32_68K(uint32_t v)
{
#if !defined(__i386__)
    return v;
#else
    return (
            ((v & 0x000000ff) << 24) |
            ((v & 0x0000ff00) << 8) |
            ((v & 0x00ff0000) >> 8) |
            ((v & 0xff000000) >> 24)
            );
#endif
}


static inline uint32_t swap32(uint32_t v)
{
#if defined(__i386__)
    return v;
#else
    return (
            ((v & 0x000000ff) << 24) |
            ((v & 0x0000ff00) << 8) |
            ((v & 0x00ff0000) >> 8) |
            ((v & 0xff000000) >> 24)
            );
#endif
}


static inline uint16_t swap16_68K(uint16_t v)
{
#if !defined(__i386__)
    return v;
#else
    return (
            ((v & 0x00ff) << 8) |
            ((v & 0xff00) >> 8)
            );
#endif
}


static inline uint16_t swap16(uint16_t v)
{
#if defined(__i386__)
    return v;
#else
    return (
            ((v & 0x00ff) << 8) |
            ((v & 0xff00) >> 8)
            );
#endif
}


static inline void swap_ehdr(Elf32_Ehdr *ehdr)
{
    ehdr->e_type      = swap16(ehdr->e_type);
    ehdr->e_machine   = swap16(ehdr->e_machine);
    ehdr->e_version   = swap32(ehdr->e_version);
    ehdr->e_entry     = swap32(ehdr->e_entry);
    ehdr->e_phoff     = swap32(ehdr->e_phoff);
    ehdr->e_shoff     = swap32(ehdr->e_shoff);
    ehdr->e_flags     = swap32(ehdr->e_flags);
    ehdr->e_ehsize    = swap16(ehdr->e_ehsize);
    ehdr->e_phentsize = swap16(ehdr->e_phentsize);
    ehdr->e_phnum     = swap16(ehdr->e_phnum);
    ehdr->e_shentsize = swap16(ehdr->e_shentsize);
    ehdr->e_shnum     = swap16(ehdr->e_shnum);
    ehdr->e_shstrndx  = swap16(ehdr->e_shstrndx);
}

#endif
