/*****************************************************************************
 * Free42 -- a free HP-42S calculator clone
 * Copyright (C) 2004-2005  Thomas Okken
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *****************************************************************************/

#ifndef FILESYS_H
#define FILESYS_H 1


#include "free42.h"

#define FILENAMELEN 256


/* 'type' values */
#define FSA_TYPE_PALM_VOLUME 1
#define FSA_TYPE_VFS_VOLUME 2
#define FSA_TYPE_DIRECTORY 3
#define FSA_TYPE_FILE 4
    
#define FSA_ERR_NONE 0
#define FSA_ERR_NOT_SUPPORTED 1
#define FSA_ERR_NOT_FOUND 2 
#define FSA_ERR_NOT_YET_IMPLEMENTED 3
#define FSA_ERR_UNKNOWN_TYPE 4
#define FSA_ERR_OUT_OF_MEMORY 5 
#define FSA_ERR_ENUMERATION_END 6 
#define FSA_ERR_PERMISSION_DENIED 7
#define FSA_ERR_FILE_NOT_OPEN 8
#define FSA_ERR_SEEK_FAILED 9
#define FSA_ERR_READ_FAILED 10
#define FSA_ERR_WRITE_FAILED 11
#define FSA_ERR_INVALID_ARGUMENT 12
#define FSA_ERR_VOLUME_NOT_FOUND 13
#define FSA_ERR_CANT_CREATE 14
#define FSA_ERR_FILE_EXISTS 15

#define FSA_MODE_READONLY 0
#define FSA_MODE_WRITEONLY 1
#define FSA_MODE_READWRITE 2

#define FSA_SEEK_START 0
#define FSA_SEEK_CURRENT 1
#define FSA_SEEK_END 2

typedef struct {
    int type;
    char name[FILENAMELEN];
    char path[FILENAMELEN];
} fsa_obj;

int fsa_resolve(const char *vol_and_path, fsa_obj **obj,
			    int resolve_parent, char *basename) FILESYS_SECT;
int fsa_create(fsa_obj *parent, const char *filename, fsa_obj **file)
								FILESYS_SECT;
int fsa_open(fsa_obj *obj, int mode) FILESYS_SECT;
int fsa_seek(fsa_obj *obj, int mode, int4 offset) FILESYS_SECT;
int fsa_read(fsa_obj *obj, void *buf, uint4 *nbytes) FILESYS_SECT;
int fsa_write(fsa_obj *obj, const void *buf, uint4 *nbytes) FILESYS_SECT;
void fsa_delete(fsa_obj *obj) FILESYS_SECT;
void fsa_release(fsa_obj *obj) FILESYS_SECT;

void dbfs_init() FILESYS_SECT;
void dbfs_finish() FILESYS_SECT;
void dbfs_erase() FILESYS_SECT;
void dbfs_makedirty() FILESYS_SECT;

int select_file(const char *title, const char *ok_label,
			const char *types, char *buf, int buflen) FILESYS_SECT;
int select_dir(const char *title, const char *ok_label,
			const char *types, char *buf, int buflen) FILESYS_SECT;


#endif
