/*****************************************************************************
 * Free42 -- a free HP-42S calculator clone
 * Copyright (C) 2004-2006  Thomas Okken
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

#include <Extensions/ExpansionMgr/VFSMgr.h>

#include "filesys.h"
#include "shell2.h"
#include "shell.rcp.h"

static int vfs_manager_present() FILESYS_SECT;
static int vfs_manager_present() {
    UInt32 vfsMgrVersion;
    Err err;

    err = FtrGet(sysFileCVFSMgr, vfsFtrIDVersion, &vfsMgrVersion);
    return err == errNone;
}

#define offsetof(str, memb) (((char *) &((str *) 0)->memb) - ((char *) 0))


/******************************************/
/***** Filesystem in a Database stuff *****/
/******************************************/

#define DBFS_SEEK_START 0
#define DBFS_SEEK_CURRENT 1
#define DBFS_SEEK_END 2

#define DBFS_MODE_READONLY 0
#define DBFS_MODE_WRITEONLY 1
#define DBFS_MODE_READWRITE 2

#define DBFS_FLAG_FIRST 0x0001
#define DBFS_FLAG_LAST 0x0002

/* Number of data bytes per record, excluding the header */
#define DBFS_BLKSIZ 60000


typedef struct {
    char name[FILENAMELEN];
    UInt32 created;
    UInt32 modified;
    UInt16 seqno;
    UInt16 flags;
} dbfs_header;

typedef struct dbfs_file {
    char name[FILENAMELEN];
    UInt16 first_record;
    int4 ptr;
    int4 size;
    int mode;
    struct dbfs_file *next;
    struct dbfs_file *previous;
} dbfs_file;

static DmOpenRef fs_db = 0;
static dbfs_file *first_file = NULL;
static dbfs_file *last_file = NULL;

static void dbfs_resizedir(unsigned char *p, UInt32 offset, int before,
						  Int32 growth) FILESYS_SECT;
static void dbfs_adjust_indexes(unsigned char *p, UInt16 pos, Int16 growth)
								FILESYS_SECT;
static int dbfs_adddirentry(const char *path, UInt16 recnum) FILESYS_SECT;
static UInt32 dbfs_find(UInt32 dir_offset, const char *name, int *is_file)
								FILESYS_SECT;
static UInt32 dbfs_add(UInt32 dir_offset, const char *name, UInt16 recnum)
								FILESYS_SECT;
static void dbfs_list(const char *dir,
		      int (*filter)(const char *name, const char *pattern),
		      const char *pattern, char ***names, int **types, int *n)
								FILESYS_SECT;
static UInt32 dbfs_resolve(const char *path, int *is_file) FILESYS_SECT;
static UInt32 dbfs_path2offset(const char *path, int create, UInt16 recnum,
						  int *is_file) FILESYS_SECT;
static int dbfs_insrec(UInt16 *pos, const char *path, int first) FILESYS_SECT;
static int dbfs_delrecs(unsigned char *p, UInt16 pos, const char *path)
								FILESYS_SECT;
static int dbfs_create(const char *path) FILESYS_SECT;
static int dbfs_delete(const char *path) FILESYS_SECT;
static dbfs_file *dbfs_open(const char *path, int mode) FILESYS_SECT;
static void dbfs_close(dbfs_file *file) FILESYS_SECT;
static int dbfs_seek(dbfs_file *file, int mode, Int32 offset) FILESYS_SECT;
static int dbfs_read(dbfs_file *file, void *buf, UInt32 *nbytes) FILESYS_SECT;
static int dbfs_write(dbfs_file *file, const void *buf, UInt32 *nbytes)
								FILESYS_SECT;

static Int16 fs_db_record_compare(void *rec1, void *rec2, Int16 other,
				  SortRecordInfoPtr rec1SortInfo,
				  SortRecordInfoPtr rec2SortInfo,
				  MemHandle appInfoH) FILESYS_SECT;
static Int16 fs_db_record_compare(void *rec1, void *rec2, Int16 other,
				  SortRecordInfoPtr rec1SortInfo,
				  SortRecordInfoPtr rec2SortInfo,
				  MemHandle appInfoH) {
    UInt16 cat_a = rec1SortInfo->attributes & dmRecAttrCategoryMask;
    UInt16 cat_b = rec2SortInfo->attributes & dmRecAttrCategoryMask;
    dbfs_header *hdr_a, *hdr_b;

    /* The only records whose first byte is 0 are the directory-valid and
     * directory records (category 0); those sort before everything else.
     */
    if (cat_a == 0) {
	if (*(char *) rec1 == 0)
	    return -1;
	cat_a = 16;
    }
    if (cat_b == 0) {
	if (*(char *) rec2 == 0)
	    return 1;
	cat_b = 16;
    }

    /* Sort by ascending category (but category 0 comes last) */
    if (cat_a < cat_b)
	return -1;
    if (cat_a > cat_b)
	return 1;

    /* Within a category, sort by sequence number */
    hdr_a = (dbfs_header *) rec1;
    hdr_b = (dbfs_header *) rec2;
    if (hdr_a->seqno < hdr_b->seqno)
	return -1;
    if (hdr_a->seqno > hdr_b->seqno)
	return 1;

    return 0;
}


void dbfs_init() {
    LocalID id;
    Err err;
    MemHandle h;
    char *p;
    int dir_valid;
    UInt16 numrecs, cat0recs;

    id = DmFindDatabase(0, "Free42FileSys");
    if (id == 0) {
	err = DmCreateDatabase(0, "Free42FileSys", 'Fk42', 'FSys', false);
	if (err != errNone)
	    return;
	id = DmFindDatabase(0, "Free42FileSys");
    }
    fs_db = DmOpenDatabase(0, id, dmModeReadWrite);
    if (fs_db == 0)
	return;

    /* Find out if there are any records with categories other than 1.
     * Those categories are used for queued-up uploads; if any such records
     * exist, we must first move them to the front of the database, sorted
     * by category and sequence number, and then rebuild the directory.
     */
    numrecs = DmNumRecords(fs_db);
    cat0recs = DmNumRecordsInCategory(fs_db, 0);
    if (numrecs != cat0recs) {
	UInt16 i, idx;

	/* Make sure directory-valid and directory records exist; this
	 * is needed in case the Free42FileSys database was created by
	 * an upload.
	 */
	if (cat0recs < 2) {
	    UInt16 pos = cat0recs;
	    char c = 0;
	    h = DmNewRecord(fs_db, &pos, 1);
	    if (h == 0)
		goto give_up;
	    p = (char *) MemHandleLock(h);
	    DmWrite(p, 0, &c, 1);
	    MemHandleUnlock(h);
	    DmReleaseRecord(fs_db, pos, false);
	    cat0recs++;
	    numrecs++;
	}

	/* In all category 0 records, populate the seqno field in the
	 * header, so that sorting the database will not change their
	 * order. (The other categories have had their seqno's populated
	 * by the conduit when it uploaded them.)
	 */
	i = 0;
	idx = 0;
	err = DmSeekRecordInCategory(fs_db, &i, 0, dmSeekForward, 0);
	while (err == errNone) {
	    h = DmGetRecord(fs_db, i);
	    p = (char *) MemHandleLock(h);
	    if (idx == 0 || idx == 1) {
		/* In the first category 0 records, i.e., the directory-valid
		 * and directory records, set the first byte to 0. This makes
		 * it possible to distinguish them from all other records,
		 * since those have headers whose first item is a
		 * null-terminated string of nonzero length, hence their first
		 * byte is never 0.
		 */
		char c = 0;
		DmWrite(p, 0, &c, 1);
	    } else
		DmWrite(p, offsetof(dbfs_header, seqno), &idx, sizeof(UInt16));
	    MemHandleUnlock(h);
	    DmReleaseRecord(fs_db, i, false);
	    idx++;
	    err = DmSeekRecordInCategory(fs_db, &i, 1, dmSeekForward, 0);
	}

	/* All records now have a unique key for sorting, consisting of
	 * their category and seqno -- so without further ado, let's sort
	 * the database on this key.
	 */
	DmQuickSort(fs_db, fs_db_record_compare, 0);

	/* All records are in their proper places now.
	 * This means that now is a safe time to reset all categories to 0.
	 * I do this from category 15 to category 1, so that if we're
	 * interrupted somehow, the most recent downloads will not end up
	 * being wiped out by older ones.
	 */
	for (i = 15; i >= 1; i--)
	    DmMoveCategory(fs_db, 0, i, false);
    }

    /* Find out if the directory in record 1 is valid, i.e.
     * if the filesystem has been cleanly unmounted the last time.
     * (When the Free42 Conduit uploads files, it also marks the
     * directory invalid.)
     */
    if (numrecs == 0) {
	UInt16 pos = 0;
	char c = 0;
	h = DmNewRecord(fs_db, &pos, 1);
	if (h == NULL)
	    goto give_up;
	dir_valid = 0;
	p = (char *) MemHandleLock(h);
	DmWrite(p, 0, &c, 1);
	MemHandleUnlock(h);
	DmReleaseRecord(fs_db, 0, false);
	numrecs++;
    } else {
	h = DmGetRecord(fs_db, 0);
	p = (char *) MemHandleLock(h);
	dir_valid = *p != 0;
	if (dir_valid) {
	    char c = 0;
	    DmWrite(p, 0, &c, 1);
	}
	MemHandleUnlock(h);
	DmReleaseRecord(fs_db, 0, false);
    }

    /* If the directory is not valid, or if it doesn't exist yet,
     * rebuild it by scanning the headers of all blocks from 2 until
     * the end of the database.
     */
    if (!dir_valid) {
	/* Create or wipe the record; create root directory. */
	UInt16 i;
	char prevname[FILENAMELEN];
	if (numrecs == 1) {
	    UInt16 pos = 1;
	    h = DmNewRecord(fs_db, &pos, 6);
	    if (h == NULL)
		goto give_up;
	    numrecs++;
	} else {
	    DmGetRecord(fs_db, 1);
	    h = DmResizeRecord(fs_db, 1, 6);
	}
	p = (char *) MemHandleLock(h);
	DmWrite(p, 0, "\0\0\0\0\001\000", 6);
	MemHandleUnlock(h);
	DmReleaseRecord(fs_db, 1, false);

	/* Scan all the remaining blocks and add the files to the directory */
	prevname[0] = 0;
	for (i = 2; i < numrecs; i++) {
	    dbfs_header *hdr;
	    h = DmQueryRecord(fs_db, i);
	    if (h == NULL)
		/* Deleted record? (Should never happen.) */
		continue;
	    hdr = (dbfs_header *) MemHandleLock(h);
	    if (StrCaselessCompare(hdr->name, prevname) == 0) {
		if ((hdr->flags & DBFS_FLAG_LAST) != 0)
		    prevname[0] = 0;
		MemHandleUnlock(h);
	    } else {
		int dummy;
		if (dbfs_resolve(hdr->name, &dummy) == 0xFFFFFFFF) {
		    if ((hdr->flags & DBFS_FLAG_LAST) != 0)
			prevname[0] = 0;
		    else
			StrCopy(prevname, hdr->name);
		    dbfs_adddirentry(hdr->name, i);
		    MemHandleUnlock(h);
		} else {
		    prevname[0] = 0;
		    MemHandleUnlock(h);
		    DmRemoveRecord(fs_db, i);
		    i--;
		    numrecs--;
		}
	    }
	}
    }
    return;

    give_up:
    /* Handle the case where some fatal error is preventing us from
     * working with the database file system.
     */
    DmCloseDatabase(fs_db);
    fs_db = 0;
}

void dbfs_finish() {
    MemHandle h;
    unsigned char *p;
    if (fs_db == 0)
	return;

    /* Mark directory 'valid' by setting the 1-byte block at index 0 to 1.
     * In the case of a crash, this block will be left set to 0 (we set it
     * to 0 at application startup (dbfs_init()), and this will force the
     * directory to be rebuilt the next time dbfs_init() is called.
     */
    h = DmGetRecord(fs_db, 0);
    if (h != NULL) {
	unsigned char c = 1;
	p = (unsigned char *) MemHandleLock(h);
	DmWrite(p, 0, &c, 1);
	MemHandleUnlock(h);
	DmReleaseRecord(fs_db, 0, false);
    }

    DmCloseDatabase(fs_db);
}

void dbfs_erase() {
    if (first_file != NULL) {
	show_message("Can't erase the Free42 file system: there are open files on it.");
	return;
    }
    if (fs_db != 0) {
	LocalID id;
	DmCloseDatabase(fs_db);
	id = DmFindDatabase(0, "Free42FileSys");
	if (id != 0)
	    DmDeleteDatabase(0, id);
	dbfs_init();
    }
}

void dbfs_makedirty() {
    if (fs_db != NULL) {
	UInt16 numrecs = DmNumRecords(fs_db);
	UInt16 i;
	for (i = 2; i < numrecs; i++) {
	    UInt16 atts;
	    DmRecordInfo(fs_db, i, &atts, NULL, NULL);
	    if ((atts & dmRecAttrDirty) == 0) {
		atts |= dmRecAttrDirty;
		DmSetRecordInfo(fs_db, i, &atts, NULL);
	    }
	}
    }
}

static void dbfs_resizedir(unsigned char *p, UInt32 offset, int before,
								Int32 growth) {
    /* This function resizes the directory at offset, AND all its enclosing
     * directories, by the given amount (which may be positive or negative).
     * Note that, unlike most other dbfs_*() functions, this function does
     * not get and lock the directory record itself; it needs a pointer to
     * the locked record to be passed in using the 'p' parameter. This is
     * for efficiency, since this function is only meant to be called by
     * directory-manipulating functions which have the directory record open
     * already anyway.
     * Note: the 'before' parameter is a flag that signals that 'offset' does
     * not point at the directory to be resized, but at a child; so, if it set,
     * we need to stop one step earlier.
     */
    UInt32 size = ((((((p[1] << 8) | p[2]) << 8) | p[3]) << 8) | p[4]);
    UInt32 ptr = 5;
    unsigned char c;

    found_ancestor:
    /* At this point, we're in a directory that is known to be
     * an ancestor of the one we're resizing -- so we must resize
     * this one, too.
     */
    if (ptr - 5 < offset || !before) {
	size += growth;
	DmWrite(p, ptr - 4, &size, 4);
    }

    /* If this directory is the one at 'offset', we're done. */
    if (ptr - 5 >= offset)
	return;

    /* Skip directory name */
    c = p[ptr++];
    ptr += c;

    /* Look for the next ancestor of 'offset' */
    search_for_ancestor:
    c = p[ptr++];
    if (c == 0) {
	/* directory */
	size = p[ptr++];
	size = (size << 8) | p[ptr++];
	size = (size << 8) | p[ptr++];
	size = (size << 8) | p[ptr++];
	if (ptr + size <= offset) {
	    ptr += size;
	    goto search_for_ancestor;
	} else
	    goto found_ancestor;
    } else {
	/* file */
	ptr += 2;
	c = p[ptr++];
	ptr += c;
	goto search_for_ancestor;
    }
}

static void dbfs_adjust_indexes(unsigned char *p, UInt16 pos, Int16 growth) {
    /* This function adjusts the first-record indexes in all file entries
     * in the directory, and all open files.
     * Note that, unlike most other dbfs_*() functions, this function does
     * not get and lock the directory record itself; it needs a pointer to
     * the locked record to be passed in using the 'p' parameter. This is
     * for efficiency, since this function is only meant to be called by
     * directory-manipulating functions which have the directory record open
     * already anyway.
     */
    UInt32 ptr = 0;
    UInt32 end = MemPtrSize(p);
    dbfs_file *file;

    while (ptr < end) {
	unsigned char c = p[ptr++];
	if (c == 0) {
	    /* directory */
	    /* Note: we don't do the usual ptr += size thing here, since
	     * we don't want to skip the whole directory and all its
	     * subdirectories; we just skip the directory header, effectively
	     * treating the whole directory tree as a flat list (and thus
	     * traversing it depth-first, but without bothering to keep
	     * track of where in the tree we are at any point).
	     */
	    ptr += 4;
	    c = p[ptr++];
	    ptr += c;
	} else {
	    /* file */
	    UInt16 recnum = p[ptr++];
	    recnum = (recnum << 8) | p[ptr++];
	    if (recnum > pos) {
		recnum += growth;
		DmWrite(p, ptr - 2, &recnum, 2);
	    }
	    c = p[ptr++];
	    ptr += c;
	}
    }

    file = first_file;
    while (file != NULL) {
	if (file->first_record > pos)
	    file->first_record += growth;
	file = file->next;
    }
}

static int dbfs_adddirentry(const char *name, UInt16 recnum) {
    int dummy;
    return dbfs_path2offset(name, 1, recnum, &dummy) != 0xFFFFFFFF;
}

static UInt32 dbfs_find(UInt32 dir_offset, const char *name, int *is_file) {
    MemHandle h = DmQueryRecord(fs_db, 1);
    UInt32 end;
    unsigned char *p, uc;
    int len, len2;
    if (h == NULL)
	return 0xFFFFFFFF;
    p = (unsigned char *) MemHandleLock(h);
    dir_offset++;
    end = p[dir_offset++];
    end = (end << 8) | p[dir_offset++];
    end = (end << 8) | p[dir_offset++];
    end = (end << 8) | p[dir_offset++];
    end += dir_offset;
    uc = p[dir_offset++];
    dir_offset += uc;
    len = StrLen(name);
    while (dir_offset < end) {
	UInt32 off = dir_offset;
	int is_file2 = p[dir_offset++];
	if (is_file2) {
	    dir_offset += 2;
	    len2 = p[dir_offset++];
	    if (len == len2 && StrNCaselessCompare(name,
				(char *) p + dir_offset, len) == 0) {
		MemHandleUnlock(h);
		*is_file = 1;
		return off;
	    }
	    dir_offset += len2;
	} else {
	    UInt32 dirlen = p[dir_offset++];
	    dirlen = (dirlen << 8) | p[dir_offset++];
	    dirlen = (dirlen << 8) | p[dir_offset++];
	    dirlen = (dirlen << 8) | p[dir_offset++];
	    len2 = p[dir_offset];
	    if (len == len2 && StrNCaselessCompare(name,
				(char *) p + dir_offset + 1, len) == 0) {
		MemHandleUnlock(h);
		*is_file = 0;
		return off;
	    }
	    dir_offset += dirlen;
	}
    }
    MemHandleUnlock(h);
    return 0xFFFFFFFF;
}

static UInt32 dbfs_add(UInt32 dir_offset, const char *name, UInt16 recnum) {
    MemHandle h = DmGetRecord(fs_db, 1);
    UInt32 end, offset;
    UInt32 growth, hdrlen, size;
    unsigned char *p, c[6], uc;
    int len, len2;
    if (h == NULL)
	return 0xFFFFFFFF;
    p = (unsigned char *) MemHandleLock(h);
    offset = dir_offset;
    offset++;
    end = p[offset++];
    end = (end << 8) | p[offset++];
    end = (end << 8) | p[offset++];
    end = (end << 8) | p[offset++];
    end += offset;
    uc = p[offset++];
    offset += uc;
    len = StrLen(name);
    while (offset < end) {
	UInt32 off = offset;
	int is_file2 = p[offset++];
	if (is_file2) {
	    offset += 2;
	    len2 = p[offset++];
	    if (len == len2 && StrNCaselessCompare(name,
				(char *) p + offset, len) == 0) {
		MemHandleUnlock(h);
		DmReleaseRecord(fs_db, 1, false);
		return recnum == 0 ? 0xFFFFFFFF : off;
	    }
	    offset += len2;
	} else {
	    UInt32 dirlen = p[offset++];
	    dirlen = (dirlen << 8) | p[offset++];
	    dirlen = (dirlen << 8) | p[offset++];
	    dirlen = (dirlen << 8) | p[offset++];
	    len2 = p[offset];
	    if (len == len2 && StrNCaselessCompare(name,
				(char *) p + offset + 1, len) == 0) {
		MemHandleUnlock(h);
		DmReleaseRecord(fs_db, 1, false);
		return recnum == 0 ? off : 0xFFFFFFFF;
	    }
	    offset += dirlen;
	}
    }

    /* No match found; add item at end of directory. */
    MemHandleUnlock(h);
    if (offset > end)
	/* Messed-up directory structure; don't get involved! */
	return 0xFFFFFFFF;
    size = MemHandleSize(h);
    hdrlen = recnum == 0 ? 6 : 4;
    growth = len + hdrlen;
    h = DmResizeRecord(fs_db, 1, size + growth);
    if (h == NULL) {
	/* Not enough space, or maximum record size exceeded */
	DmReleaseRecord(fs_db, 1, false);
	return 0xFFFFFFFF;
    }

    p = (unsigned char *) MemHandleLock(h);
    /* TODO: is this use of DmWrite() safe? The docs don't say that it's OK
     * for source and destination to overlap. They don't say that it's not OK,
     * either. If DmWrite() uses MemMove() underneath, this should be fine,
     * but does it?
     * Update: I do believe DmWrite() uses MemMove() -- I think I saw it in a
     * stack trace at one point. Also, the code seems to work fine for both the
     * moving-up and moving-down cases. Fingers crossed...
     */
    DmWrite(p, offset + growth, p + offset, size - offset);
    if (recnum == 0) {
	int len1 = len + 1;
	c[0] = 0x00;
	c[1] = 0x00;
	c[2] = 0x00;
	c[3] = len1 >> 8;
	c[4] = len1;
	c[5] = len;
    } else {
	c[0] = 0x01;
	c[1] = recnum >> 8;
	c[2] = recnum;
	c[3] = len;
    }
    DmWrite(p, offset, c, hdrlen);
    DmWrite(p, offset + hdrlen, name, len);
    dbfs_resizedir(p, dir_offset, 0, growth);
    MemHandleUnlock(h);
    DmReleaseRecord(fs_db, 1, false);
    return offset;
}

static void dbfs_list(const char *dir,
	      int (*filter)(const char *name, const char *pattern),
	      const char *pattern, char ***names, int **types, int *n) {
    UInt32 off1, off2, end;
    int is_file, totallength, count;
    MemHandle h;
    unsigned char *p, c;
    char name[FILENAMELEN];
    char **nameptr;
    char *namebuf;
    int *typeptr;

    *n = 0;
    count = 0;
    totallength = 0;
    off1 = dbfs_path2offset(dir, 0, 0, &is_file);
    if (off1 == 0xFFFFFFFF || is_file)
	return;

    h = DmQueryRecord(fs_db, 1);
    if (h == NULL)
	return;
    p = (unsigned char *) MemHandleLock(h);

    off1++;
    end = p[off1++];
    end = (end << 8) | p[off1++];
    end = (end << 8) | p[off1++];
    end = (end << 8) | p[off1++];
    end += off1;
    c = p[off1++];
    off1 += c;

    off2 = off1;
    while (off2 < end) {
	int is_file = p[off2++];
	if (is_file) {
	    off2 += 2;
	    c = p[off2++];
	    StrNCopy(name, (char *) p + off2, c);
	    name[c] = 0;
	    if (filter(name, pattern)) {
		totallength += c;
		totallength++;
		count++;
	    }
	    off2 += c;
	} else {
	    UInt32 dirlen = p[off2++];
	    dirlen = (dirlen << 8) | p[off2++];
	    dirlen = (dirlen << 8) | p[off2++];
	    dirlen = (dirlen << 8) | p[off2++];
	    c = p[off2];
	    totallength += c;
	    totallength++;
	    count++;
	    off2 += dirlen;
	}
    }

    nameptr = (char **) MemPtrNew(count * sizeof(char *) + totallength);
    if (nameptr == NULL) {
	MemHandleUnlock(h);
	return;
    }
    typeptr = (int *) MemPtrNew(count * sizeof(int));
    if (typeptr == NULL) {
	MemPtrFree(nameptr);
	MemHandleUnlock(h);
	return;
    }
    namebuf = ((char *) nameptr) + count * sizeof(char *);

    off2 = off1;
    while (off2 < end && *n < count) {
	int is_file = p[off2++];
	if (is_file) {
	    off2 += 2;
	    c = p[off2++];
	    StrNCopy(name, (char *) p + off2, c);
	    name[c] = 0;
	    if (filter(name, pattern)) {
		nameptr[*n] = namebuf;
		StrCopy(namebuf, name);
		namebuf += c;
		namebuf++;
		typeptr[*n] = 0;
		(*n)++;
	    }
	    off2 += c;
	} else {
	    UInt32 dirlen = p[off2++];
	    dirlen = (dirlen << 8) | p[off2++];
	    dirlen = (dirlen << 8) | p[off2++];
	    dirlen = (dirlen << 8) | p[off2++];
	    c = p[off2];
	    nameptr[*n] = namebuf;
	    StrNCopy(namebuf, (char *) p + off2 + 1, c);
	    namebuf[c] = 0;
	    namebuf += c;
	    namebuf++;
	    typeptr[*n] = 1;
	    (*n)++;
	    off2 += dirlen;
	}
    }

    *names = nameptr;
    *types = typeptr;
    MemHandleUnlock(h);
}


static UInt32 dbfs_resolve(const char *path, int *is_file) {
    return dbfs_path2offset(path, 0, 0, is_file);
}

static UInt32 dbfs_path2offset(const char *path, int create, UInt16 recnum,
				int *is_file) {
    char namebuf[FILENAMELEN];
    int i, c, ncomps;
    UInt32 off1, off2;
    StrNCopy(namebuf, path, FILENAMELEN - 1);
    namebuf[FILENAMELEN - 1] = 0;
    ncomps = 0;
    for (i = 0; namebuf[i] != 0; i++) {
	if (namebuf[i] != '/' && (i == 0 || namebuf[i - 1] == 0))
	    ncomps++;
	if (namebuf[i] == '/')
	    namebuf[i] = 0;
    }

    off1 = 0;
    i = 0;
    *is_file = 0;
    for (c = 0; c < ncomps; c++) {
	while (namebuf[i] == 0)
	    i++;
	off2 = dbfs_find(off1, namebuf + i, is_file);
	if (off2 == 0xFFFFFFFF) {
	    if (create) {
		off2 = dbfs_add(off1, namebuf + i, c < ncomps - 1 ? 0 : recnum);
		if (off2 == 0xFFFFFFFF)
		    return 0xFFFFFFFF;
	    } else
		return 0xFFFFFFFF;
	} else {
	    if (*is_file && c < ncomps - 1)
		/* One of the requested path components already exists and
		 * is a file; abort.
		 */
		return 0xFFFFFFFF;
	}
	off1 = off2;
	while (namebuf[i] != 0)
	    i++;
    }
    return off1;
}

static int dbfs_insrec(UInt16 *pos, const char *path, int first) {
    MemHandle h, h2;
    unsigned char *p;
    dbfs_header hdr;
    int dir_updated;

    /* Make sure we can modify the directory before doing anything else */
    h = DmGetRecord(fs_db, 1);
    if (h == NULL)
	return 0;

    /* If the new record is not the first, clear 'last' flag on prev rec */
    if (!first) {
	UInt16 flags;
	h2 = DmGetRecord(fs_db, *pos - 1);
	if (h2 == NULL) {
	    DmReleaseRecord(fs_db, 1, false);
	    return 0;
	}
	p = (unsigned char *) MemHandleLock(h2);
	flags = ((dbfs_header *) p)->flags & ~DBFS_FLAG_LAST;
	DmWrite(p, offsetof(dbfs_header, flags), &flags, sizeof(UInt16));
	MemHandleUnlock(h2);
	DmReleaseRecord(fs_db, *pos - 1, false);
    }

    /* Create new record */
    h2 = DmNewRecord(fs_db, pos, sizeof(dbfs_header));
    if (h2 == NULL) {
	DmReleaseRecord(fs_db, 1, false);
	return 0;
    }
    StrCopy(hdr.name, path);
    hdr.created = hdr.modified = TimGetSeconds();
    hdr.flags = DBFS_FLAG_LAST;
    if (first)
	hdr.flags |= DBFS_FLAG_FIRST;
    p = (unsigned char *) MemHandleLock(h2);
    DmWrite(p, 0, &hdr, sizeof(dbfs_header));
    MemHandleUnlock(h2);
    DmReleaseRecord(fs_db, *pos, true);

    /* Update directory tree */
    dir_updated = *pos < DmNumRecords(fs_db) - 1;
    if (dir_updated) {
	p = (unsigned char *) MemHandleLock(h);
	dbfs_adjust_indexes(p, *pos, 1);
	MemHandleUnlock(h);
    }
    DmReleaseRecord(fs_db, 1, false);

    return 1;
}

static int dbfs_delrecs(unsigned char *p, UInt16 pos, const char *path) {
    /* Note that, unlike most other dbfs_*() functions, this function does
     * not get and lock the directory record itself; it needs a pointer to
     * the locked record to be passed in using the 'p' parameter. This is
     * for efficiency, since this function is only meant to be called by
     * directory-manipulating functions which have the directory record open
     * already anyway.
     */
    int i, max;

    /* Delete records */
    max = DmNumRecords(fs_db) - pos;
    for (i = 0; i < max ; i++) {
	Err err;
	MemHandle h = DmQueryRecord(fs_db, pos);
	dbfs_header *hdr = (dbfs_header *) MemHandleLock(h);
	int matches = StrCaselessCompare(path, hdr->name) == 0;
	MemHandleUnlock(h);
	if (!matches)
	    break;
	err = DmRemoveRecord(fs_db, pos);
	if (err != errNone)
	    break;
    }

    /* Update directory tree */
    if (i > 0)
	dbfs_adjust_indexes(p, pos, -i);

    return i > 0;
}

static int dbfs_create(const char *path) {
    /* Returns: 0=error, 1=success, 2=exists */

    int is_file;
    UInt32 off;
    UInt16 recnum;

    /* First, check for existence.
     * TODO: this is not efficient - the path lookup is done in dbfs_resolve(),
     * and then again in dbfs_adddirentry(). With a little API tweaking, those
     * activities could be combined. Not a biggie, though, since file creation
     * will presumably never be a performance bottleneck anyway.
     */
    off = dbfs_resolve(path, &is_file);
    if (off != 0xFFFFFFFF)
	return is_file ? 2 : 0;

    /* File does not exist yet. First, create an initial record for it. */
    recnum = dmMaxRecordIndex;
    if (!dbfs_insrec(&recnum, path, 1))
	return 0;

    /* Finally, create a directory entry. */
    if (!dbfs_adddirentry(path, recnum)) {
	/* Creating directory entry failed. That means we don't need the
	 * new record any more, either. I delete it directly, not using
	 * dbfs_delrecs(), because I added it at the end -- so removing it
	 * will not require the directory to be updated.
	 */
	DmRemoveRecord(fs_db, recnum);
	return 0;
    } else
	return 1;
}

static int dbfs_delete(const char *path) {
    UInt32 off;
    int is_file;
    MemHandle h;
    unsigned char *p;
    UInt32 entry_size;
    int res;

    /* Make sure this isn't an open file we're about to delete */
    dbfs_file *f = first_file;
    while (f != NULL) {
	if (StrCaselessCompare(path, f->name) == 0)
	    return 0;
	f = f->next;
    }

    off = dbfs_resolve(path, &is_file);
    if (off == 0xFFFFFFFF)
	return 0;

    h = DmGetRecord(fs_db, 1);
    if (h == NULL)
	return 0;
    p = (unsigned char *) MemHandleLock(h);

    if (is_file) {
	/* Find file data record number */
	UInt16 recnum = (p[off + 1] << 8) |  p[off + 2];
	/* Delete file data records & update directory record numbers */
	res = dbfs_delrecs(p, recnum, path);
	entry_size = ((int) p[off + 3]) + 4;
    } else {
	UInt32 size = (((((p[off + 1] << 8) | p[off + 2]) << 8)
				    | p[off + 3]) << 8) | p[off + 4];
	unsigned int namelen = p[off + 5];
	/* If the directory size (i.e. the number of bytes following
	 * the 'size' field) equals the number of bytes taken up by the
	 * directory name, then the directory is empty and we can delete it.
	 */
	res = size == namelen + 1;
	entry_size = size + 5;
    }

    if (res) {
	/* Delete the directory entry */
	UInt32 dir_size = MemHandleSize(h);
	DmWrite(p, off, p + off + entry_size, dir_size - off - entry_size);
	/* Shrink the containing directories. */
	dbfs_resizedir(p, off, 1, -entry_size);
	MemHandleUnlock(h);
	DmResizeRecord(fs_db, 1, dir_size - entry_size);
    } else
	MemHandleUnlock(h);

    DmReleaseRecord(fs_db, 1, false);
    return res;
}

static dbfs_file *dbfs_open(const char *path, int mode) {
    UInt32 off;
    int is_file;
    MemHandle h;
    unsigned char *p;
    UInt16 recnum;
    dbfs_file *file;
    dbfs_header *hdr;

    off = dbfs_resolve(path, &is_file);
    if (off == 0xFFFFFFFF || !is_file)
	return NULL;

    /* TODO: combine getting the record number with the name resolution */
    h = DmGetRecord(fs_db, 1);
    if (h == NULL)
	return NULL;
    p = (unsigned char *) MemHandleLock(h);
    recnum = (p[off + 1] << 8) | p[off + 2];
    MemHandleUnlock(h);
    DmReleaseRecord(fs_db, 1, false);

    if (mode == DBFS_MODE_WRITEONLY || mode == DBFS_MODE_READWRITE) {
	/* Check that this file isn't open with these modes already;
	 * we don't want to deal with the issues of multiple concurrent
	 * writers.
	 */
	dbfs_file *f = first_file;
	while (f != NULL) {
	    if ((f->mode == DBFS_MODE_WRITEONLY
			|| f->mode == DBFS_MODE_READWRITE)
		    && StrCaselessCompare(f->name, path) == 0)
		return NULL;
	    f = f->next;
	}
    }

    /* At this point, we know the file exists and that we're OK to open
     * it using the requested mode. Time to allocate the file control
     * structure.
     */
    file = (dbfs_file *) MemPtrNew(sizeof(dbfs_file));
    if (file == NULL)
	return NULL;

    StrCopy(file->name, path);
    file->mode = mode;
    file->first_record = recnum;
    file->ptr = 0;

    if (mode == DBFS_MODE_WRITEONLY) {
	/* Remove all data records after the first */
	int trimmed;
	MemHandle h2;

	h = DmGetRecord(fs_db, 1);
	if (h == NULL) {
	    MemPtrFree(file);
	    return NULL;
	}
	p = (unsigned char *) MemHandleLock(h);
	trimmed = dbfs_delrecs(p, recnum + 1, path);
	MemHandleUnlock(h);
	DmReleaseRecord(fs_db, 1, false);

	/* Trim the first data record to size 0 */
	h = DmResizeRecord(fs_db, recnum, sizeof(dbfs_header));

	/* Set 'last' flag on first record; update time stamp */
	h2 = DmGetRecord(fs_db, recnum);
	if (h2 != NULL) {
	    UInt16 flags;
	    UInt32 modified;
	    p = (unsigned char *) MemHandleLock(h2);
	    flags = ((dbfs_header *) p)->flags | DBFS_FLAG_LAST;
	    DmWrite(p, offsetof(dbfs_header, flags), &flags, sizeof(UInt16));
	    modified = TimGetSeconds();
	    DmWrite(p, offsetof(dbfs_header, modified), &modified,
							     sizeof(UInt32));
	    MemHandleUnlock(h2);
	    DmReleaseRecord(fs_db, recnum, true);
	}

	file->size = 0;
    } else {
	/* Figure out the current file size */
	int4 size = 0;
	UInt16 n = DmNumRecords(fs_db);
	while (recnum < n) {
	    int match;
	    h = DmQueryRecord(fs_db, recnum);
	    hdr = (dbfs_header *) MemHandleLock(h);
	    match = StrCaselessCompare(hdr->name, path) == 0;
	    MemHandleUnlock(h);
	    if (!match)
		break;
	    size += MemHandleSize(h) - sizeof(dbfs_header);
	    recnum++;
	}

	file->size = size;
    }

    /* Link the file into the list of open files */
    if (last_file == NULL) {
	first_file = last_file = file;
	file->next = NULL;
	file->previous = NULL;
    } else {
	last_file->next = file;
	file->previous = last_file;
	file->next = NULL;
	last_file = file;
    }
    return file;
}

static void dbfs_close(dbfs_file *file) {
    if (file->previous == NULL)
	first_file = file->next;
    else
	file->previous->next = file->next;
    if (file->next == NULL)
	last_file = file->previous;
    else
	file->next->previous = file->previous;
    MemPtrFree(file);
}

static int dbfs_seek(dbfs_file *file, int mode, Int32 offset) {
    int4 newpos;
    switch (mode) {
	case DBFS_SEEK_START:
	    newpos = offset;
	    break;
	case DBFS_SEEK_CURRENT:
	    newpos = file->ptr + offset;
	    break;
	case DBFS_SEEK_END:
	    newpos = file->size + offset;
	    break;
	default:
	    return 0;
    }
    if (newpos < 0 || newpos > file->size)
	return 0;
    file->ptr = newpos;
    return 1;
}

static int dbfs_read(dbfs_file *file, void *buf, UInt32 *nbytes) {
    UInt32 bytes_to_read;
    UInt16 recnum;
    UInt32 recoff;
    UInt32 left_in_file;
    char *dst;
    MemHandle h;
    char *p;

    bytes_to_read = *nbytes;
    *nbytes = 0;
    if (file->mode == DBFS_MODE_WRITEONLY)
	return 0;

    left_in_file = file->size - file->ptr;
    if (left_in_file == 0)
	return 1;

    if (bytes_to_read > left_in_file)
	bytes_to_read = left_in_file;

    recnum = file->ptr / DBFS_BLKSIZ + file->first_record;
    recoff = file->ptr % DBFS_BLKSIZ;
    dst = (char *) buf;

    while (bytes_to_read > 0) {
	UInt32 left_in_rec = DBFS_BLKSIZ - recoff;
	UInt32 n = bytes_to_read < left_in_rec ? bytes_to_read : left_in_rec;
	h = DmQueryRecord(fs_db, recnum);
	if (h == NULL)
	    return 0;
	p = (char *) MemHandleLock(h);
	MemMove(dst, p + sizeof(dbfs_header) + recoff, n);
	MemHandleUnlock(h);
	dst += n;
	file->ptr += n;
	bytes_to_read -= n;
	*nbytes += n;
	recnum++;
	recoff = 0;
    }

    return 1;
}

static int dbfs_write(dbfs_file *file, const void *buf, UInt32 *nbytes) {
    UInt32 bytes_to_write;
    UInt16 recnum;
    UInt32 recoff;
    UInt32 left_in_file;
    UInt32 timestamp;
    const char *src;
    MemHandle h;
    char *p;
    int res;
    dbfs_file *f;

    if (file->mode == DBFS_MODE_READONLY) {
	*nbytes = 0;
	return 0;
    }

    bytes_to_write = *nbytes;
    *nbytes = 0;
    src = (const char *) buf;
    recnum = file->ptr / DBFS_BLKSIZ + file->first_record;
    recoff = file->ptr % DBFS_BLKSIZ;
    left_in_file = file->size - file->ptr;

    /* In case the file pointer is not at the end of the file, this first
     * section of dbfs_write() handles overwriting existing data; the second
     * 'while' loop handles appending new data.
     */

    while (bytes_to_write > 0 && left_in_file > 0) {
	UInt32 left_in_rec = DBFS_BLKSIZ - recoff;
	UInt32 n;
	if (left_in_rec > left_in_file)
	    left_in_rec = left_in_file;
	n = bytes_to_write < left_in_rec ? bytes_to_write : left_in_rec;
	h = DmGetRecord(fs_db, recnum);
	if (h == NULL)
	    return 0;
	p = (char *) MemHandleLock(h);
	DmWrite(p, sizeof(dbfs_header) + recoff, src, n);
	timestamp = TimGetSeconds();
	DmWrite(p, offsetof(dbfs_header, modified), &timestamp, 4);
	MemHandleUnlock(h);
	DmReleaseRecord(fs_db, recnum, true);
	src += n;
	file->ptr += n;
	bytes_to_write -= n;
	left_in_file -= n;
	*nbytes += n;
	recnum++;
	recoff = 0;
    }

    /* The file pointer is at the end of the file.
     * This section of dbfs_write() deals with growing or adding records
     * and then writing the data into the newly created section of the file.
     */

    res = 0;
    while (bytes_to_write > 0) {
	UInt32 n;
	if (recoff == 0 && file->size > 0) {
	    /* Append a new record */
	    if (!dbfs_insrec(&recnum, file->name, 0))
		goto finish;
	}
	/* Grow the current record */
	n = recoff + bytes_to_write;
	if (n > DBFS_BLKSIZ)
	    n = DBFS_BLKSIZ;
	h = DmResizeRecord(fs_db, recnum, n + sizeof(dbfs_header));
	if (h == NULL)
	    goto finish;
	h = DmGetRecord(fs_db, recnum);
	if (h == NULL)
	    goto finish;
	p = (char *) MemHandleLock(h);
	n -= recoff;
	DmWrite(p, sizeof(dbfs_header) + recoff, src, n);
	timestamp = TimGetSeconds();
	DmWrite(p, offsetof(dbfs_header, modified), &timestamp, 4);
	MemHandleUnlock(h);
	DmReleaseRecord(fs_db, recnum, true);
	src += n;
	file->ptr += n;
	file->size = file->ptr;
	bytes_to_write -= n;
	*nbytes += n;
	recnum++;
	recoff = 0;
    }
    res = 1;

    finish:
    /* Update file size on other files open to this one */
    f = first_file;
    while (f != NULL) {
	if (f->mode == DBFS_MODE_READONLY
		&& StrCaselessCompare(f->name, file->name) == 0)
	    f->size = file->size;
	f = f->next;
    }

    return res;
}

void fsdump(int dummy) {
    UInt16 n = DmNumRecords(fs_db);
    UInt16 i;
    char fname[256];
    Err err;
    FileRef file;
    UInt32 size;
    MemHandle h;
    char *p;

    for (i = 0; i < n; i++) {
	StrPrintF(fname, "/Free42FileSys.%02d", i);
	err = VFSFileCreate(1, fname);
	err = VFSFileOpen(1, fname, vfsModeWrite, &file);
	h = DmQueryRecord(fs_db, i);
	size = MemHandleSize(h);
	p = (char *) MemHandleLock(h);
	err = VFSFileWrite(file, size, p, &size);
	MemHandleUnlock(h);
	err = VFSFileClose(file);
    }
}


/****************************************/
/***** Filesystem abstraction stuff *****/
/****************************************/

static int fsa_voliter_start() FILESYS_SECT;
static int fsa_voliter_next(fsa_obj **child) FILESYS_SECT;
static void fsa_getpath(fsa_obj *obj, char *buf, int buflen) FILESYS_SECT;
static int fsa_mkdir(fsa_obj *parent, const char *dirname, fsa_obj **dir)
								FILESYS_SECT;
static int fsa_list(fsa_obj *dir,
		    int (*filter)(const char *name, const char *pattern),
		    const char *pattern,
		    char ***names, int **types, int *n) FILESYS_SECT;
static int fsa_path(fsa_obj *obj, char ***names, int *n) FILESYS_SECT;
static int fsa_ancestor(fsa_obj *child, int n, fsa_obj **ancestor) FILESYS_SECT;


/* 'real_type' values */
#define FSA_TYPE_PALM_DIRECTORY 5
#define FSA_TYPE_PALM_FILE 6
#define FSA_TYPE_VFS_DIRECTORY 7
#define FSA_TYPE_VFS_FILE 8

typedef struct {
    int type;
    char name[FILENAMELEN];
    char path[FILENAMELEN];
    int real_type;
    char volname[FILENAMELEN];
    union {
	struct {
	    char dummy;
	} palm_dir;
	struct {
	    dbfs_file *file;
	} palm_file;
	struct {
	    UInt16 vol;
	} vfs_dir;
	struct {
	    UInt16 vol;
	    int is_open;
	    FileRef file;
	} vfs_file;
    } data;
} fsa_obj_internal;

static int enum_phase;
static UInt32 enum_iterator; 
static int enum_count;

static fsa_obj_internal *new_fsa() FILESYS_SECT;
static fsa_obj_internal *new_fsa() {
    return (fsa_obj_internal *) MemPtrNew(sizeof(fsa_obj_internal));
}

static int fsa_voliter_start() {
    enum_phase = 0;
    enum_count = 0;
    return FSA_ERR_NONE;
}

static int fsa_voliter_next(fsa_obj **child) {
    switch (enum_phase) {
	case 0: {
	    fsa_obj_internal *newfsa;
	    if (fs_db == 0 || enum_count > 0)
		goto phase1;
	    newfsa = new_fsa();
	    if (newfsa == NULL)
		return FSA_ERR_OUT_OF_MEMORY;
	    enum_count++;
	    newfsa->type = FSA_TYPE_PALM_VOLUME;
	    newfsa->real_type = FSA_TYPE_PALM_DIRECTORY;
	    StrCopy(newfsa->name, "Free42");
	    StrCopy(newfsa->volname, "Free42");
	    StrCopy(newfsa->path, "/");
	    *child = (fsa_obj *) newfsa;
	    return FSA_ERR_NONE;
	}
	phase1:
	    if (vfs_manager_present()) {
		enum_iterator = vfsIteratorStart;
		enum_count = 1;
	    } else
		goto phase2;
	    enum_phase = 1;
	case 1: {
	    UInt16 vol;
	    Err err;
	    fsa_obj_internal *newfsa;
	    if (enum_iterator == vfsIteratorStop)
		goto phase2;
	    err = VFSVolumeEnumerate(&vol, &enum_iterator);
	    if (err != errNone)
		goto phase2;
	    newfsa = new_fsa();
	    if (newfsa == NULL)
		return FSA_ERR_OUT_OF_MEMORY;
	    enum_count++;
	    newfsa->type = FSA_TYPE_VFS_VOLUME;
	    err = VFSVolumeGetLabel(vol, newfsa->name, FILENAMELEN);
	    if (err != errNone && err != vfsErrNameShortened)
		StrPrintF(newfsa->name, "%d", enum_count);
	    StrCopy(newfsa->volname, newfsa->name);
	    newfsa->real_type = FSA_TYPE_VFS_DIRECTORY;
	    newfsa->data.vfs_dir.vol = vol;
	    StrCopy(newfsa->path, "/");
	    *child = (fsa_obj *) newfsa;
	    return FSA_ERR_NONE;
	} 
	phase2:
	    enum_phase = 2;
	case 2:
	    return FSA_ERR_ENUMERATION_END;
	default:
	    return FSA_ERR_INVALID_ARGUMENT;
    }
}

int fsa_resolve(const char *vol_and_path, fsa_obj **obj,
			int resolve_parent, char *basename) {
    /* This function resolves names of the form [<volume>:]<abs_path>
     * and returns an fsa_obj for the named object.
     */
    char pathbuf[FILENAMELEN];
    char numbuf[10];
    char *volname, *colon, *path, *name, *p1, *p2;
    int count, pathlen;
    fsa_obj *fsa;
    fsa_obj_internal *fsaint;
    int i, n;
    
    /* Separate the volume name and the path. */
    StrNCopy(pathbuf, vol_and_path, FILENAMELEN - 1);
    pathbuf[FILENAMELEN - 1] = 0;
    pathlen = StrLen(pathbuf);
    colon = StrChr(pathbuf, ':');
    if (colon == NULL) {
	volname = "1";
	path = pathbuf;
    } else {
	volname = pathbuf;
	path = colon + 1;
	*colon = 0;
    }

    /* Make sure the path starts with a "/". Insert one if necessary. */
    if (path[0] != '/') {
	n = StrLen(path) + 1;
	if (pathlen == FILENAMELEN - 1)
	    n--;
	MemMove(path + 1, path, n);
	path[0] = '/';
	path[n] = 0;
    }

    /* Replace runs of slashes with single slashes. */
    i = n = 0;
    while (path[n] != 0) {
	if (n == 0 || path[n] != '/' || path[n - 1] != '/')
	    path[i++] = path[n];
	n++;
    }
    path[i] = 0;

    /* Find the object's base name */
    name = path + 1;
    p1 = path;
    while ((p2 = StrChr(p1, '/')) != NULL) {
	p1 = p2 + 1;
	if (*p1 != 0 && *p1 != '/')
	    name = p1;
    }
    
    /* If we're looking for the object's parent, split off its base name */
    if (resolve_parent) {
	StrCopy(basename, name);
	if (name == path + 1)
	    *name = 0;
	else
	    name[-1] = 0;
    }

    /* Find the volume. */
    fsa_voliter_start();
    count = 0;
    while (fsa_voliter_next(&fsa) == FSA_ERR_NONE) {
	count++;
	if (StrCaselessCompare(fsa->name, volname) == 0)
	    goto found_vol;
	StrPrintF(numbuf, "%d", count);
	if (StrCompare(numbuf, volname) == 0) {
	    volname = fsa->name;
	    goto found_vol;
	}
	fsa_release(fsa);
    }
    return FSA_ERR_VOLUME_NOT_FOUND;

    /* Find the object in the volume */
    found_vol:
    if (StrCompare(path, "/") == 0 &&
	    (*name == 0 || StrCompare(name, "/") == 0)) {
	/* Root directory wanted. That means we're done already;
	 * the root directory IS the volume, which we have.
	 */
	*obj = (fsa_obj *) fsa;
	return FSA_ERR_NONE;
    }

    fsaint = (fsa_obj_internal *) fsa;
    switch (fsaint->real_type) {

	case FSA_TYPE_PALM_DIRECTORY: {
	    fsa_obj_internal *newfsa;
	    int is_file;
	    UInt32 off;
	    
	    fsa_release(fsa);
	    off = dbfs_resolve(path, &is_file);
	    if (off == 0xFFFFFFFF)
		return FSA_ERR_NOT_FOUND;

	    newfsa = new_fsa();
	    if (newfsa == NULL)
		return FSA_ERR_OUT_OF_MEMORY;
	    
	    if (is_file) {
		newfsa->type = FSA_TYPE_FILE;
		newfsa->real_type = FSA_TYPE_PALM_FILE;
		newfsa->data.palm_file.file = NULL;
	    } else {
		newfsa->type = FSA_TYPE_DIRECTORY;
		newfsa->real_type = FSA_TYPE_PALM_DIRECTORY;
	    }
	    StrCopy(newfsa->name, name);
	    StrCopy(newfsa->path, path);
	    StrCopy(newfsa->volname, volname);
	    *obj = (fsa_obj *) newfsa;
	    return FSA_ERR_NONE;
	}

	case FSA_TYPE_VFS_DIRECTORY: {
	    FileRef file;
	    UInt32 attributes;
	    fsa_obj_internal *newfsa;
	    UInt16 vol = fsaint->data.vfs_dir.vol;
	    fsa_release(fsa);

	    /* Locate, don't create */
	    newfsa = new_fsa();
	    if (newfsa == NULL)
		return FSA_ERR_OUT_OF_MEMORY;
	    if (VFSFileOpen(vol, path, vfsModeRead, &file) != errNone) {
		MemPtrFree(newfsa);
		return FSA_ERR_NOT_FOUND;
	    }
	    if (VFSFileGetAttributes(file, &attributes) != errNone) {
		MemPtrFree(newfsa);
		VFSFileClose(file);
		return FSA_ERR_NOT_FOUND;
	    }
	    VFSFileClose(file);
	    /* For testing with HostFS (which never sets vfsFileAttrDirectory)
	     * change this condition to include 'resolve_parent'.
	     * You still can't browse directories, but at least exporting etc.
	     * will then work, since at least it can resolve the parent
	     * directory then.
	     */
	    if ((attributes & vfsFileAttrDirectory) == 0) {
		newfsa->type = FSA_TYPE_FILE;
		newfsa->real_type = FSA_TYPE_VFS_FILE;
		newfsa->data.vfs_file.vol = vol;
		newfsa->data.vfs_file.is_open = 0;
	    } else {
		newfsa->type = FSA_TYPE_DIRECTORY;
		newfsa->real_type = FSA_TYPE_VFS_DIRECTORY;
		newfsa->data.vfs_dir.vol = vol;
	    }
	    StrCopy(newfsa->name, name);
	    StrCopy(newfsa->path, path);
	    StrCopy(newfsa->volname, volname);
	    *obj = (fsa_obj *) newfsa;
	    return FSA_ERR_NONE;
	}

	default:
	    fsa_release(fsa);
	    return FSA_ERR_INVALID_ARGUMENT;
    }
}

static void fsa_getpath(fsa_obj *obj, char *buf, int buflen) {
    fsa_obj_internal *fsa = (fsa_obj_internal *) obj;
    StrNCopy(buf, fsa->volname, buflen - 1);
    StrNCat(buf, ":", buflen - 1);
    StrNCat(buf, fsa->path, buflen - 1);
    buf[buflen - 1] = 0;
}

int fsa_create(fsa_obj *parent, const char *filename, fsa_obj **file) {
    fsa_obj_internal *fsa = (fsa_obj_internal *) parent;
    char path[FILENAMELEN];

    StrCopy(path, fsa->path);
    if (StrLen(path) > 1)
	StrNCat(path, "/", FILENAMELEN - 1);
    StrNCat(path, filename, FILENAMELEN - 1);
    path[FILENAMELEN - 1] = 0;

    switch (fsa->real_type) {
	case FSA_TYPE_PALM_DIRECTORY: {
	    int res;
	    fsa_obj_internal *newfsa = new_fsa();
	    if (newfsa == NULL)
		return FSA_ERR_OUT_OF_MEMORY;
	    res = dbfs_create(path);
	    if (res == 0) {
		MemPtrFree(newfsa);
		return FSA_ERR_CANT_CREATE;
	    }
	    newfsa->type = FSA_TYPE_FILE;
	    newfsa->real_type = FSA_TYPE_PALM_FILE;
	    newfsa->data.palm_file.file = NULL;
	    StrCopy(newfsa->name, filename);
	    StrCopy(newfsa->path, path);
	    StrCopy(newfsa->volname, fsa->volname);
	    *file = (fsa_obj *) newfsa;
	    return res == 1 ? FSA_ERR_NONE : FSA_ERR_FILE_EXISTS;
	}
	case FSA_TYPE_PALM_FILE:
	    return FSA_ERR_NOT_SUPPORTED;
	case FSA_TYPE_VFS_DIRECTORY: {
	    Err err;
	    fsa_obj_internal *newfsa = new_fsa();
	    if (newfsa == NULL)
		return FSA_ERR_OUT_OF_MEMORY;
	    err = VFSFileCreate(fsa->data.vfs_dir.vol, path);
	    if (err != errNone && err != vfsErrFileAlreadyExists) {
		MemPtrFree(newfsa);
		return FSA_ERR_CANT_CREATE;
	    }
	    newfsa->type = FSA_TYPE_FILE;
	    newfsa->real_type = FSA_TYPE_VFS_FILE;
	    newfsa->data.vfs_file.vol = fsa->data.vfs_dir.vol;
	    newfsa->data.vfs_file.is_open = 0;
	    StrCopy(newfsa->name, filename);
	    StrCopy(newfsa->path, path);
	    StrCopy(newfsa->volname, fsa->volname);
	    *file = (fsa_obj *) newfsa;
	    return err == errNone ? FSA_ERR_NONE : FSA_ERR_FILE_EXISTS;
	}
	case FSA_TYPE_VFS_FILE:
	    return FSA_ERR_NOT_SUPPORTED;
	default:
	    return FSA_ERR_INVALID_ARGUMENT;
    }
}

static int fsa_mkdir(fsa_obj *parent, const char *dirname, fsa_obj **dir) {
    fsa_obj_internal *fsa = (fsa_obj_internal *) parent;
    switch (fsa->real_type) {
	case FSA_TYPE_PALM_FILE:
	case FSA_TYPE_VFS_FILE:
	    return FSA_ERR_NOT_SUPPORTED;
	case FSA_TYPE_PALM_DIRECTORY:
	case FSA_TYPE_VFS_DIRECTORY: {
	    char path[FILENAMELEN];
	    fsa_obj_internal *newfsa = new_fsa();
	    if (newfsa == NULL)
		return FSA_ERR_OUT_OF_MEMORY;
	    StrCopy(path, fsa->path);
	    if (StrLen(path) > 1)
		StrNCat(path, "/", FILENAMELEN - 1);
	    StrNCat(path, dirname, FILENAMELEN - 1);
	    path[FILENAMELEN - 1] = 0;
	    if (fsa->real_type == FSA_TYPE_PALM_DIRECTORY) {
		if (!dbfs_adddirentry(path, 0)) {
		    MemPtrFree(newfsa);
		    return FSA_ERR_CANT_CREATE;
		}
		newfsa->real_type = FSA_TYPE_PALM_DIRECTORY;
	    } else {
		Err err = VFSDirCreate(fsa->data.vfs_dir.vol, path);
		if (err != errNone) {
		    MemPtrFree(newfsa);
		    return FSA_ERR_CANT_CREATE;
		}
		newfsa->real_type = FSA_TYPE_VFS_DIRECTORY;
		newfsa->data.vfs_dir.vol = fsa->data.vfs_dir.vol;
	    }
	    newfsa->type = FSA_TYPE_DIRECTORY;
	    StrCopy(newfsa->name, dirname);
	    StrCopy(newfsa->path, path);
	    StrCopy(newfsa->volname, fsa->volname);
	    *dir = (fsa_obj *) newfsa;
	    return FSA_ERR_NONE;
	}
	default:
	    return FSA_ERR_INVALID_ARGUMENT;
    }
}

static int fsa_list(fsa_obj *dir,
		    int (*filter)(const char *name, const char *pattern),
		    const char *pattern, char ***names, int **types, int *n) {
    fsa_obj_internal *fsa = (fsa_obj_internal *) dir;
    switch (fsa->real_type) {
	case FSA_TYPE_PALM_DIRECTORY:
	    dbfs_list(fsa->path, filter, pattern, names, types, n);
	    return FSA_ERR_NONE;
	case FSA_TYPE_PALM_FILE:
	    return FSA_ERR_NOT_SUPPORTED;
	case FSA_TYPE_VFS_DIRECTORY: {
	    UInt32 dirIterator = vfsIteratorStart;
	    char buf[FILENAMELEN];
	    Err err;
	    int count = 0;
	    int4 totallength = 0;
	    FileRef dir;
	    int dirOpen = 0;
	    char **nameptr;
	    int *types2;
	    FileInfoType finfo;
	    finfo.nameP = buf;
	    finfo.nameBufLen = FILENAMELEN;
	    err = VFSFileOpen(fsa->data.vfs_dir.vol, fsa->path,
						    vfsModeRead, &dir);
	    if (err == errNone) {
		dirOpen = 1;
		while (dirIterator != vfsIteratorStop) {
		    err = VFSDirEntryEnumerate(dir, &dirIterator, &finfo);
		    if (err != errNone)
			break;
		    if ((finfo.attributes & vfsFileAttrDirectory) != 0
			    || filter == NULL || filter(buf, pattern)) {
			count++;
			totallength += StrLen(buf) + 1;
		    }
		}
	    }
	    if (count > 0) {
		int length = 0;
		char *namebuf;
		int finalcount = 0;

		nameptr = (char **) MemPtrNew(totallength
						    + count * sizeof(char *));
		if (nameptr == NULL) {
		    count = 0;
		    goto done;
		}
		types2 = (int *) MemPtrNew(count * sizeof(int));
		if (types2 == NULL) {
		    MemPtrFree(nameptr);
		    count = 0;
		    goto done;
		}
		namebuf = ((char *) nameptr) + count * sizeof(char *);
		dirIterator = vfsIteratorStart;
		while (finalcount < count && dirIterator != vfsIteratorStop) {
		    int blen, isDir;
		    err = VFSDirEntryEnumerate(dir, &dirIterator, &finfo);
		    if (err != errNone)
			break;
		    isDir = (finfo.attributes & vfsFileAttrDirectory) != 0;
		    if (!isDir && filter != NULL && !filter(buf, pattern))
			continue;
		    blen = StrLen(buf) + 1;
		    if (length + blen > totallength)
			break;
		    nameptr[finalcount] = namebuf + length;
		    StrCopy(nameptr[finalcount], buf);
		    length += blen;
		    types2[finalcount] = isDir;
		    finalcount++;
		}
		if (finalcount == 0) {
		    MemPtrFree(nameptr);
		    MemPtrFree(types2);
		}
		count = finalcount;
	    }
	    done:
	    *n = count;
	    if (count != 0) {
		*names = nameptr;
		*types = types2;
	    } else {
		*names = NULL;
		*types = NULL;
	    }
	    if (dirOpen)
		VFSFileClose(dir);
	    return FSA_ERR_NONE;
	}
	case FSA_TYPE_VFS_FILE:
	    return FSA_ERR_NOT_SUPPORTED;
	default:
	    return FSA_ERR_INVALID_ARGUMENT;
    }
}

static int fsa_path(fsa_obj *obj, char ***names, int *n) {
    fsa_obj_internal *fsa = (fsa_obj_internal *) obj;
    int count = 2; /* "<volumes> & volume name */
    int length = StrLen("<volumes>") + StrLen(fsa->volname)
				     + StrLen(fsa->path) + 3;
    char **nameptr;
    char *namebuf;
    int i, p;
    int pathlen;

    pathlen = StrLen(fsa->path);
    for (i = 1; i < pathlen; i++)
	if (fsa->path[i] != '/' && fsa->path[i - 1] == '/')
	    count++;

    nameptr = (char **) MemPtrNew(count * sizeof(char *) + length);
    if (nameptr == NULL)
	return FSA_ERR_OUT_OF_MEMORY;

    namebuf = ((char *) nameptr) + count * sizeof(char *);

    p = count - 3;
    for (i = 0; i <= pathlen; i++) {
	char c = fsa->path[i];
	if (i > 0 && i < pathlen && c != '/' && fsa->path[i - 1] == '/')
	    nameptr[p--] = namebuf + i;
	if (c == '/')
	    c = 0;
	namebuf[i] = c;
    }
    namebuf += pathlen + 1;

    nameptr[count - 2] = namebuf;
    StrCopy(namebuf, fsa->volname);
    namebuf += StrLen(fsa->volname) + 1;

    nameptr[count - 1] = namebuf;
    StrCopy(namebuf, "<volumes>");

    *names = nameptr;
    *n = count;
    return FSA_ERR_NONE;
}

static int fsa_ancestor(fsa_obj *child, int n, fsa_obj **ancestor) {
    fsa_obj_internal *fsa = (fsa_obj_internal *) child;
    if (StrCompare(fsa->path, "/") == 0) {
	*ancestor = NULL;
	return FSA_ERR_NONE;
    } else {
	fsa_obj_internal *newfsa;
	char *p1, *p2;
	int slashes = 0;

	p1 = fsa->path - 1;
	while ((p1 = StrChr(p1 + 1, '/')) != NULL)
	    slashes++;
	if (n > slashes) {
	    *ancestor = NULL;
	    return FSA_ERR_NONE;
	}

	newfsa = new_fsa();
	if (newfsa == NULL)
	    return FSA_ERR_OUT_OF_MEMORY;
	MemMove(newfsa, fsa, sizeof(fsa_obj_internal));
	switch (newfsa->real_type) {
	    case FSA_TYPE_VFS_FILE:
		newfsa->type = FSA_TYPE_DIRECTORY;
		newfsa->real_type = FSA_TYPE_VFS_DIRECTORY;
		newfsa->data.vfs_dir.vol = fsa->data.vfs_file.vol;
		break;
	    case FSA_TYPE_PALM_FILE:
		newfsa->type = FSA_TYPE_DIRECTORY;
		newfsa->real_type = FSA_TYPE_PALM_DIRECTORY;
		break;
	}

	p1 = newfsa->path;
	p2 = NULL;

	slashes -= n;
	while (slashes > 0) {
	    p2 = p1;
	    p1 = StrChr(p1 + 1, '/');
	    slashes--;
	}

	if (p1 == newfsa->path) {
	    /* Reached root directory */
	    switch (newfsa->real_type) {
		case FSA_TYPE_VFS_DIRECTORY:
		    newfsa->type = FSA_TYPE_VFS_VOLUME;
		    break;
		case FSA_TYPE_PALM_DIRECTORY:
		    newfsa->type = FSA_TYPE_PALM_VOLUME;
		    break;
	    }
	    newfsa->path[1] = 0;
	    StrCopy(newfsa->name, newfsa->volname);
	} else {
	    *p1 = 0;
	    StrCopy(newfsa->name, p2 + 1);
	}
	*ancestor = (fsa_obj *) newfsa;
	return FSA_ERR_NONE;
    }
}

int fsa_open(fsa_obj *obj, int mode) {
    fsa_obj_internal *fsa = (fsa_obj_internal *) obj;
    switch (fsa->real_type) {
	case FSA_TYPE_PALM_DIRECTORY:
	    return FSA_ERR_NOT_SUPPORTED;
	case FSA_TYPE_PALM_FILE: {
	    int mode2;
	    switch (mode) {
		case FSA_MODE_READONLY:
		    mode2 = DBFS_MODE_READONLY;
		    break;
		case FSA_MODE_WRITEONLY:
		    mode2 = DBFS_MODE_WRITEONLY;
		    break;
		case FSA_MODE_READWRITE:
		    mode2 = DBFS_MODE_READWRITE;
		    break;
		default:
		    return FSA_ERR_INVALID_ARGUMENT;
	    }
	    if (fsa->data.palm_file.file != NULL)
		dbfs_close(fsa->data.palm_file.file);
	    fsa->data.palm_file.file = dbfs_open(fsa->path, mode2);
	    if (fsa->data.palm_file.file == NULL)
		return FSA_ERR_NOT_FOUND;
	    else
		return FSA_ERR_NONE;
	}
	case FSA_TYPE_VFS_DIRECTORY:
	    return FSA_ERR_NOT_SUPPORTED;
	case FSA_TYPE_VFS_FILE: {
	    Err err;
	    int mode2;
	    switch (mode) {
		case FSA_MODE_READONLY:
		    mode2 = vfsModeRead;
		    break;
		case FSA_MODE_WRITEONLY:
		    mode2 = vfsModeWrite | vfsModeTruncate;
		    break;
		case FSA_MODE_READWRITE:
		    mode2 = vfsModeReadWrite;
		    break;
		default:
		    return FSA_ERR_INVALID_ARGUMENT;
	    }
	    if (fsa->data.vfs_file.is_open)
		VFSFileClose(fsa->data.vfs_file.file);
	    err = VFSFileOpen(fsa->data.vfs_file.vol, fsa->path,
			      mode2, &fsa->data.vfs_file.file);
	    fsa->data.vfs_file.is_open = err == errNone;
	    switch (err) {
		case errNone:
		    return FSA_ERR_NONE;
		case expErrCardReadOnly:
		case vfsErrFilePermissionDenied:
		    return FSA_ERR_PERMISSION_DENIED;
		default:
		    return FSA_ERR_NOT_FOUND;
	    }
	}
	default:
	    return FSA_ERR_INVALID_ARGUMENT;
    }
}

int fsa_seek(fsa_obj *obj, int mode, int4 offset) {
    fsa_obj_internal *fsa = (fsa_obj_internal *) obj;
    switch (fsa->real_type) {
	case FSA_TYPE_PALM_DIRECTORY:
	    return FSA_ERR_NOT_SUPPORTED;
	case FSA_TYPE_PALM_FILE: {
	    int mode2, res;
	    if (fsa->data.palm_file.file == NULL)
		return FSA_ERR_FILE_NOT_OPEN;
	    switch (mode) {
		case FSA_SEEK_START:
		    mode2 = DBFS_SEEK_START;
		    break;
		case FSA_SEEK_CURRENT:
		    mode2 = DBFS_SEEK_CURRENT;
		    break;
		case FSA_SEEK_END:
		    mode2 = DBFS_SEEK_END;
		    break;
		default:
		    return FSA_ERR_INVALID_ARGUMENT;
	    }
	    res = dbfs_seek(fsa->data.palm_file.file, mode2, offset);
	    return res ? FSA_ERR_NONE : FSA_ERR_SEEK_FAILED;
	}
	case FSA_TYPE_VFS_DIRECTORY:
	    return FSA_ERR_NOT_SUPPORTED;
	case FSA_TYPE_VFS_FILE: {
	    FileOrigin origin;
	    UInt32 size;
	    Err err;
	    if (!fsa->data.vfs_file.is_open)
		return FSA_ERR_FILE_NOT_OPEN;
	    switch (mode) {
		case FSA_SEEK_START:
		    origin = vfsOriginBeginning;
		    break;
		case FSA_SEEK_CURRENT:
		    origin = vfsOriginCurrent;
		    break;
		case FSA_SEEK_END:
		    origin = vfsOriginEnd;
		    break;
		default:
		    return FSA_ERR_INVALID_ARGUMENT;
	    }
	    if (mode == FSA_SEEK_END && offset == 0) {
		char c;
		UInt32 n;
		/* On my Tungsten E, it appears to be utterly Verboten to seek
		 * to the end of the file. Whether I use vfsOriginEnd with an
		 * offset of 0, or vfsOriginBeginning with an offset of 'size',
		 * or, vfsOriginEnd with an offset of -1 and then
		 * vfsOriginCurrent with an offset of 1 -- I consistently get
		 * error 0x2A07, which is vfsAtEndOfFile or something to that
		 * effect).
		 * Hence, this little hack: if the file size is 0, I do nothing
		 * because the cursor can only be in one place; otherwise, I
		 * seek to 1 byte *before* the end, then I read one byte, then
		 * I seek 1 byte backward, then I write that byte back. That
		 * does work, and leaves the cursor at the end of the file,
		 * which is where I want it.
		 */
		err = VFSFileSize(fsa->data.vfs_file.file, &size);
		if (err != errNone)
		    return FSA_ERR_SEEK_FAILED;
		if (size == 0)
		    return FSA_ERR_NONE;
		err = VFSFileSeek(fsa->data.vfs_file.file, vfsOriginEnd, -1);
		if (err != errNone)
		    return FSA_ERR_SEEK_FAILED;
		err = VFSFileRead(fsa->data.vfs_file.file, 1, &c, &n);
		if (err != errNone || n != 1)
		    return FSA_ERR_SEEK_FAILED;
		err = VFSFileSeek(fsa->data.vfs_file.file, vfsOriginCurrent,-1);
		if (err != errNone)
		    return FSA_ERR_SEEK_FAILED;
		err = VFSFileWrite(fsa->data.vfs_file.file, 1, &c, &n);
		if (err != errNone || n != 1)
		    return FSA_ERR_SEEK_FAILED;
		else
		    return FSA_ERR_NONE;
	    } else {
		err = VFSFileSeek(fsa->data.vfs_file.file, origin, offset);
		if (err != errNone)
		    return FSA_ERR_SEEK_FAILED;
		else
		    return FSA_ERR_NONE;
	    }
	}
	default:
	    return FSA_ERR_INVALID_ARGUMENT;
    }
}

int fsa_read(fsa_obj *obj, void *buf, uint4 *nbytes) {
    fsa_obj_internal *fsa = (fsa_obj_internal *) obj;
    switch (fsa->real_type) {
	case FSA_TYPE_PALM_DIRECTORY:
	    return FSA_ERR_NOT_SUPPORTED;
	case FSA_TYPE_PALM_FILE: {
	    int res;
	    if (fsa->data.palm_file.file == NULL)
		return FSA_ERR_FILE_NOT_OPEN;
	    res = dbfs_read(fsa->data.palm_file.file, buf, nbytes);
	    return res ? FSA_ERR_NONE : FSA_ERR_READ_FAILED;
	}
	case FSA_TYPE_VFS_DIRECTORY:
	    return FSA_ERR_NOT_SUPPORTED;
	case FSA_TYPE_VFS_FILE: {
	    Err err;
	    if (!fsa->data.vfs_file.is_open)
		return FSA_ERR_FILE_NOT_OPEN;
	    err = VFSFileRead(fsa->data.vfs_file.file, *nbytes, buf, nbytes);
	    if (err == errNone || err == vfsErrFileEOF)
		return FSA_ERR_NONE;
	    else
		return FSA_ERR_READ_FAILED;
	}
	default:
	    return FSA_ERR_INVALID_ARGUMENT;
    }
}

int fsa_write(fsa_obj *obj, const void *buf, uint4 *nbytes) {
    fsa_obj_internal *fsa = (fsa_obj_internal *) obj;
    switch (fsa->real_type) {
	case FSA_TYPE_PALM_DIRECTORY:
	    return FSA_ERR_NOT_SUPPORTED;
	case FSA_TYPE_PALM_FILE: {
	    int res;
	    if (fsa->data.palm_file.file == NULL)
		return FSA_ERR_FILE_NOT_OPEN;
	    res = dbfs_write(fsa->data.palm_file.file, buf, nbytes);
	    return res ? FSA_ERR_NONE : FSA_ERR_WRITE_FAILED;
	}
	case FSA_TYPE_VFS_DIRECTORY:
	    return FSA_ERR_NOT_SUPPORTED;
	case FSA_TYPE_VFS_FILE: {
	    Err err;
	    if (!fsa->data.vfs_file.is_open)
		return FSA_ERR_FILE_NOT_OPEN;
	    err = VFSFileWrite(fsa->data.vfs_file.file, *nbytes, buf, nbytes);
	    if (err == errNone)
		return FSA_ERR_NONE;
	    else
		return FSA_ERR_WRITE_FAILED;
	}
	default:
	    return FSA_ERR_INVALID_ARGUMENT;
    }
}

void fsa_delete(fsa_obj *obj) {
    fsa_obj_internal *fsa = (fsa_obj_internal *) obj;
    switch (fsa->real_type) {
	case FSA_TYPE_PALM_DIRECTORY:
	    dbfs_delete(fsa->path);
	    break;
	case FSA_TYPE_PALM_FILE:
	    if (fsa->data.palm_file.file != NULL)
		dbfs_close(fsa->data.palm_file.file);
	    dbfs_delete(fsa->path);
	    break;
	case FSA_TYPE_VFS_DIRECTORY:
	    VFSFileDelete(fsa->data.vfs_dir.vol, fsa->path);
	    break;
	case FSA_TYPE_VFS_FILE:
	    if (fsa->data.vfs_file.is_open)
		VFSFileClose(fsa->data.vfs_file.file);
	    VFSFileDelete(fsa->data.vfs_file.vol, fsa->path);
	    break;
    }
    MemPtrFree(fsa);
}

void fsa_release(fsa_obj *obj) {
    fsa_obj_internal *fsa = (fsa_obj_internal *) obj;
    switch (fsa->real_type) {
	case FSA_TYPE_PALM_FILE: {
	    if (fsa->data.palm_file.file != NULL)
		dbfs_close(fsa->data.palm_file.file);
	    break;
	}
	case FSA_TYPE_VFS_FILE: {
	    if (fsa->data.vfs_file.is_open)
		VFSFileClose(fsa->data.vfs_file.file);
	    break;
	}
    }
    MemPtrFree(fsa);
}


/*********************************/
/***** File selection dialog *****/
/*********************************/

typedef struct {
    const char *types[5];
    const char *patterns[5];
    int ntypes;
    int sel_type;
    fsa_obj *dir;
    char **path_components;
    char **dir_item_names;
    int *dir_item_types;
    ListType *path_list;
    ListType *dir_list;
    FieldType *name_field;
    ControlType *dir_trigger;
    MemHandle palmH;
    MemHandle volumeH;
    MemHandle folderH;
    MemHandle docH;
    BitmapType *palmP;
    BitmapType *volumeP;
    BitmapType *folderP;
    BitmapType *docP;
    int visible;
    int do_dir;
} select_file_struct;

static select_file_struct *sfs = NULL;

static int filename_filter(const char *name, const char *pattern) FILESYS_SECT;
static int filename_filter(const char *name, const char *pattern) {
    int name_ext_len;
    const char *namedot;
    int patlen;
    const char *p;

    if (name == NULL || *name == 0)
	/* Don't trust files with no or zero-length names */
	return 0;
    if (pattern == NULL || *pattern == 0)
	/* Missing or empty pattern means allow everything */
	return 1;

    namedot = StrChr(name, '.');
    if (namedot != NULL) {
	while ((p = StrChr(namedot + 1, '.')) != NULL)
	    namedot = p;
	namedot++;
	name_ext_len = StrLen(namedot);
    }

    p = pattern;
    while (*p != 0) {
	char *semi = StrChr(p, ';');
	if (semi == NULL)
	    patlen = StrLen(p);
	else
	    patlen = semi - p;

	if (patlen > 0) {
	    if (patlen == 1 && p[0] == '*')
		/* Star matches everything, even files w/o extension */
		return 1;
	    if (namedot != NULL && name_ext_len == patlen
			    && StrNCaselessCompare(namedot, p, patlen) == 0)
		/* File matches pattern */
		return 1;
	}

	if (semi == NULL)
	    break;
	else
	    p = semi + 1;
    }
    /* No matching pattern found */
    return 0;
}

static void update_path_list() FILESYS_SECT;
static void update_path_list() {
    int component_count;
    char **component_names;

    if (sfs->path_components != NULL) {
	MemPtrFree(sfs->path_components);
	sfs->path_components = NULL;
    }

    if (sfs->dir == NULL) {
	component_names = (char **) MemPtrNew(sizeof(char *)
					+ StrLen("<volumes>") + 1);
	if (component_names == NULL)
	    component_count = 0;
	else {
	    component_names[0] = (char *) (component_names + 1);
	    StrCopy(component_names[0], "<volumes>");
	    component_count = 1;
	}
    } else {
	int err = fsa_path(sfs->dir, &component_names, &component_count);
	if (err != FSA_ERR_NONE)
	    component_count = 0;
    }

    LstSetHeight(sfs->path_list, component_count < 8 ? component_count : 8);
    LstSetListChoices(sfs->path_list, component_names, component_count);
    if (component_count > 0) {
	sfs->path_components = component_names;
	CtlSetLabel(sfs->dir_trigger, component_names[0]);
	LstSetSelection(sfs->path_list, 0);
    }
}

static void update_dir_list() FILESYS_SECT;
static void update_dir_list() {

    int selected_item = -1;
    int dir_item_count = 0;

    if (sfs->dir_item_names != NULL) {
	MemPtrFree(sfs->dir_item_names);
	sfs->dir_item_names = NULL;
    }
    if (sfs->dir_item_types != NULL) {
	MemPtrFree(sfs->dir_item_types);
	sfs->dir_item_types = NULL;
    }

    if (sfs->dir == NULL) {
	int n = 0;
	fsa_obj *vol;
	unsigned int totallength = 0;
	char *namebuf;
	unsigned int namebuflen = 0;
	int count = 0;

	fsa_voliter_start();
	while (fsa_voliter_next(&vol) == FSA_ERR_NONE) {
	    n++;
	    totallength += StrLen(vol->name) + 1;
	    fsa_release(vol);
	}
	if (n == 0)
	    goto done;

	sfs->dir_item_names = (char **) MemPtrNew(n * sizeof(char *)
						    + totallength);
	if (sfs->dir_item_names == NULL)
	    goto done;
	namebuf = ((char *) sfs->dir_item_names) + n * sizeof(char *);
	sfs->dir_item_types = (int *) MemPtrNew(n * sizeof(int));
	if (sfs->dir_item_types == NULL) {
	    MemPtrFree(sfs->dir_item_names);
	    sfs->dir_item_names = NULL;
	    goto done;
	}
	fsa_voliter_start();
	while (n > 0 && fsa_voliter_next(&vol) == FSA_ERR_NONE) {
	    int len;
	    if (namebuflen + StrLen(vol->name) + 1 > totallength) {
		fsa_release(vol);
		break;
	    }
	    sfs->dir_item_names[count] = namebuf;
	    sfs->dir_item_types[count] = vol->type == FSA_TYPE_VFS_VOLUME;
	    StrCopy(namebuf, vol->name);
	    len = StrLen(vol->name) + 1;
	    namebuf += len;
	    namebuflen += len;
	    fsa_release(vol);
	    count++;
	    n--;
	}
	dir_item_count = count;
	if (count == 0) {
	    MemPtrFree(sfs->dir_item_types);
	    sfs->dir_item_types = NULL;
	    MemPtrFree(sfs->dir_item_names);
	    sfs->dir_item_names = NULL;
	} else
	    selected_item = 0;
    } else {
	char **item_names;
	int *item_types;
	int item_count, i, j;
	int err = fsa_list(sfs->dir,
			   filename_filter, sfs->patterns[sfs->sel_type],
			   &item_names, &item_types, &item_count);
	if (err == FSA_ERR_NONE) {
	    dir_item_count = item_count;
	    if (item_count > 0) {
		/* Sort directory items alphabetically; keep folders first */
		for (i = 0; i < item_count - 1; i++)
		    for (j = i + 1; j < item_count; j++)
			if (!item_types[i] && item_types[j]
				|| item_types[i] == item_types[j]
				    && StrCaselessCompare(item_names[i],
							 item_names[j]) > 0) {
			    int type = item_types[i];
			    item_types[i] = item_types[j];
			    item_types[j] = type;
			    char *name = item_names[i];
			    item_names[i] = item_names[j];
			    item_names[j] = name;
			}
		sfs->dir_item_names = item_names;
		sfs->dir_item_types = item_types;
		selected_item = 0;
		char *name = FldGetTextPtr(sfs->name_field);
		if (name != NULL)
		    for (i = 0; i < item_count; i++)
			if (StrCaselessCompare(item_names[i], name) == 0) {
			    selected_item = i;
			    break;
			}
	    }
	}
    }

    done:
    LstSetListChoices(sfs->dir_list, sfs->dir_item_names, dir_item_count);
    if (selected_item != -1)
	LstSetSelection(sfs->dir_list, selected_item);
    if (sfs->visible)
	LstDrawList(sfs->dir_list);
}

static void list_cell_renderer(Int16 itemNum, RectangleType *bounds,
						char **itemsText) FILESYS_SECT;
static void list_cell_renderer(Int16 itemNum, RectangleType *bounds,
						char **itemsText) {
    BitmapType *bmp;
    if (sfs->dir == NULL)
	bmp = sfs->dir_item_types[itemNum] ? sfs->volumeP : sfs->palmP;
    else
	bmp = sfs->dir_item_types[itemNum] ? sfs->folderP : sfs->docP;
    WinEraseRectangle(bounds, 0);
    WinDrawBitmap(bmp, bounds->topLeft.x,
		       bounds->topLeft.y + (bounds->extent.y - 7) / 2);
    WinGlueDrawTruncChars(itemsText[itemNum], StrLen(itemsText[itemNum]),
		      bounds->topLeft.x + 10, bounds->topLeft.y,
		      bounds->extent.x - 10);
}

static Boolean selectfile_handler(EventType *e) FILESYS_SECT;
static Boolean selectfile_handler(EventType *e) {
    switch (e->eType) {
	case ctlSelectEvent:
	    switch (e->data.ctlSelect.controlID) {
		case up_id:
		    if (sfs->dir != NULL) {
			fsa_obj *parent;
			if (fsa_ancestor(sfs->dir, 1, &parent) == FSA_ERR_NONE){
			    fsa_release(sfs->dir);
			    sfs->dir = parent;
			    update_path_list();
			    update_dir_list();
			}
		    }
		    return true;
		case mkdir_id:
		    if (sfs->dir == NULL)
			show_message("No volume selected.");
		    else {
			char *s = FldGetTextPtr(sfs->name_field);
			if (s == NULL || *s == 0)
			    show_message("No file name given.");
			else {
			    fsa_obj *newdir;
			    int err = fsa_mkdir(sfs->dir, s, &newdir);
			    if (err == FSA_ERR_NONE) {
				fsa_release(newdir);
				update_dir_list();
			    } else
				show_message("Creating directory failed.");
			}
		    }
		    return true;
		case ok_id:
		    if (sfs->dir == NULL) {
			show_message("No volume selected.");
			return true;
		    } else if (sfs->do_dir) {
			return false;
		    } else {
			char *s = FldGetTextPtr(sfs->name_field);
			if (s == NULL || *s == 0) {
			    show_message("No file name given.");
			    return true;
			} else
			    return false;
		    }
		default:
		    return false;
	    }

	case popSelectEvent:
	    switch (e->data.popSelect.controlID) {
		case dir_trigger_id: {
		    fsa_obj *ancestor;
		    int err;
		    int newsel = e->data.popSelect.selection;

		    if (newsel == 0)
			/* We keep this pop-up at 0 (top), so this is
			 * not news */
			return false;
		    if (sfs->dir == NULL)
			/* Shouldn't get here in this case */
			return false;

		    err = fsa_ancestor(sfs->dir, newsel, &ancestor);
		    if (err == FSA_ERR_NONE) {
			EventType evt;
			fsa_release(sfs->dir);
			sfs->dir = ancestor;
			update_dir_list();
			/* If I simply call update_path_list() here, the
			 * pop-up trigger's label gets messed up. I must
			 * confess I don't understand what the problem is;
			 * I was under the impression that the PalmOS controls
			 * cause actions to happen via the message queue, not
			 * callbacks, so that modifying a control while
			 * handling an event fired by that control should be
			 * safe. But what do I know?
			 * So, I send myself an event, and call
			 * update_path_list() when I receive it. That seems to
			 * prevent the problem.
			 */
			evt.eType = (eventsEnum) (firstUserEvent + 1);
			EvtAddEventToQueue(&evt);
		    }
		    return false;
		}
		case filetype_trigger_id:
		    if (sfs->sel_type != e->data.popSelect.selection) {
			sfs->sel_type = e->data.popSelect.selection;
			update_dir_list();
		    }
		    return false;
		default:
		    return false;
	    }

	case firstUserEvent + 1:
	    update_path_list();
	    return true;

	case lstSelectEvent:
	    if (e->data.lstSelect.listID == dir_list_id) {
		int n = e->data.lstSelect.selection;
		if (sfs->dir == NULL) {
		    int volnamelen = StrLen(sfs->dir_item_names[n]);
		    char *volname = (char *) MemPtrNew(volnamelen + 3);
		    if (volname != NULL) {
			int err;
			StrCopy(volname, sfs->dir_item_names[n]);
			StrCat(volname, ":/");
			err = fsa_resolve(volname, &sfs->dir, 0, NULL);
			MemPtrFree(volname);
			update_path_list();
			update_dir_list();
		    }
		} else {
		    char *name = sfs->dir_item_names[n];
		    if (sfs->dir_item_types[n] == 0) {
			if (!sfs->do_dir) {
			    set_field_text(sfs->name_field, name);
			    FldDrawField(sfs->name_field);
			}
		    } else {
			char newpath[FILENAMELEN];
			fsa_obj *newdir;
			int err, len;
			fsa_getpath(sfs->dir, newpath, FILENAMELEN);
			len = StrLen(newpath);
			if (len > 0 && newpath[len - 1] != '/')
			    StrNCat(newpath, "/", FILENAMELEN - 1);
			StrNCat(newpath, name, FILENAMELEN - 1);
			newpath[FILENAMELEN - 1] = 0;
			err = fsa_resolve(newpath, &newdir, 0, NULL);
			if (err == FSA_ERR_NONE) {
			    fsa_release(sfs->dir);
			    sfs->dir = newdir;
			    update_path_list();
			    update_dir_list();
			}
		    }
		}
	    } else
		return false;

	default:
	    return false;
    }
}

static int select_file_or_dir(const char *title, const char *ok_label,
	    const char *types, char *buf, int buflen, int do_dir) FILESYS_SECT;
static int select_file_or_dir(const char *title, const char *ok_label,
	    const char *types, char *buf, int buflen, int do_dir) {
    FormType *form;
    UInt16 idx;
    ControlType *ctl;
    ListType *list;
    int ret, i, n;
    const char *p;
    
    sfs = (select_file_struct *) MemPtrNew(sizeof(select_file_struct));
    if (sfs == NULL) {
	show_message("Insufficient memory!");
	return 0;
    }

    form = FrmInitForm(selectfile_id);
    FrmSetTitle(form, (char *) title);

    idx = FrmGetObjectIndex(form, ok_id);
    ctl = (ControlType *) FrmGetObjectPtr(form, idx);
    CtlSetLabel(ctl, ok_label);

    idx = FrmGetObjectIndex(form, dir_trigger_id);
    sfs->dir_trigger = (ControlType *) FrmGetObjectPtr(form, idx);

    idx = FrmGetObjectIndex(form, filetype_popup_id);
    list = (ListType *) FrmGetObjectPtr(form, idx);
    p = types;
    i = 0;
    while (i < 5) {
	n = StrLen(p);
	if (n == 0)
	    break;
	sfs->types[i] = p;
	p += n + 1;
	n = StrLen(p);
	if (n == 0)
	    break;
	sfs->patterns[i] = p;
	p += n + 1;
	i++;
    }
    sfs->ntypes = i;
    sfs->sel_type = 0;
    LstSetHeight(list, i);
    LstSetListChoices(list, (char **) sfs->types, i);
    if (i > 0) {
	idx = FrmGetObjectIndex(form, filetype_trigger_id);
	ctl = (ControlType *) FrmGetObjectPtr(form, idx);
	CtlSetLabel(ctl, sfs->types[0]);
    }

    sfs->palmH = DmGetResource(bitmapRsc, filesel_palm_id);
    sfs->palmP = (BitmapType *) MemHandleLock(sfs->palmH);
    sfs->volumeH = DmGetResource(bitmapRsc, filesel_volume_id);
    sfs->volumeP = (BitmapType *) MemHandleLock(sfs->volumeH);
    sfs->folderH = DmGetResource(bitmapRsc, filesel_folder_id);
    sfs->folderP = (BitmapType *) MemHandleLock(sfs->folderH);
    sfs->docH = DmGetResource(bitmapRsc, filesel_doc_id);
    sfs->docP = (BitmapType *) MemHandleLock(sfs->docH);

    idx = FrmGetObjectIndex(form, dir_popup_id);
    sfs->path_list = (ListType *) FrmGetObjectPtr(form, idx);
    idx = FrmGetObjectIndex(form, dir_list_id);
    sfs->dir_list = (ListType *) FrmGetObjectPtr(form, idx);
    LstSetDrawFunction(sfs->dir_list, list_cell_renderer);
    idx = FrmGetObjectIndex(form, filename_id);
    sfs->name_field = (FieldType *) FrmGetObjectPtr(form, idx);

    if (*buf == 0 || fsa_resolve(buf, &sfs->dir, 0, NULL) != FSA_ERR_NONE)
	sfs->dir = NULL;
    if (sfs->dir != NULL && sfs->dir->type == FSA_TYPE_FILE) {
	fsa_obj *file = sfs->dir;
	fsa_ancestor(file, 1, &sfs->dir);
	if (StrLen(file->name) > 0)
	    set_field_text(sfs->name_field, file->name);
	fsa_release(file);
    }

    sfs->path_components = NULL;
    sfs->dir_item_names = NULL;
    sfs->dir_item_types = NULL;

    sfs->visible = 0;
    update_path_list();
    update_dir_list();
    sfs->visible = 1;

    FrmSetEventHandler(form, selectfile_handler);
    idx = FrmGetObjectIndex(form, filename_id);
    FrmSetFocus(form, idx);

    sfs->do_dir = do_dir;
    if (do_dir) {
	idx = FrmGetObjectIndex(form, filenamelabel_id);
	FrmHideObject(form, idx);
	idx = FrmGetObjectIndex(form, filename_id);
	FrmHideObject(form, idx);
	idx = FrmGetObjectIndex(form, filetypelabel_id);
	FrmHideObject(form, idx);
	idx = FrmGetObjectIndex(form, filetype_trigger_id);
	FrmHideObject(form, idx);
	idx = FrmGetObjectIndex(form, mkdir_id);
	FrmHideObject(form, idx);
    }

    ret = FrmDoDialog(form) == ok_id;
    if (ret) {
	char *name = FldGetTextPtr(sfs->name_field);
	int len;
	fsa_getpath(sfs->dir, buf, buflen);
	len = StrLen(buf);
	if (do_dir) {
	    if (len == 0)
		StrCopy(buf, "/");
	} else {
	    if (len > 0 && buf[len - 1] != '/')
		StrNCat(buf, "/", buflen - 1);
	    StrNCat(buf, name, buflen - 1);
	    buf[buflen - 1] = 0;
	    if (sfs->ntypes > 0) {
		/* Append extension, if needed */
		const char *ext = sfs->patterns[sfs->sel_type];
		if (!filename_filter(name, ext)) {
		    /* The filter is rejecting the name, which must mean
		     * it doesn't have the right extension. We tack on
		     * the first sub-pattern.
		     */
		    const char *semi = StrChr(ext, ';');
		    int extlen = semi == NULL ? StrLen(ext) : semi - ext;
		    char extbuf[10];
		    StrNCopy(extbuf, ext, extlen);
		    extbuf[extlen] = 0;
		    StrNCat(buf, ".", buflen - 1);
		    StrNCat(buf, extbuf, buflen - 1);
		}
	    }
	}
    }

    MemHandleUnlock(sfs->palmH);
    DmReleaseResource(sfs->palmH);
    MemHandleUnlock(sfs->volumeH);
    DmReleaseResource(sfs->volumeH);
    MemHandleUnlock(sfs->folderH);
    DmReleaseResource(sfs->folderH);
    MemHandleUnlock(sfs->docH);
    DmReleaseResource(sfs->docH);
    FrmDeleteForm(form);

    if (sfs->dir != NULL)
	fsa_release(sfs->dir);
    if (sfs->path_components != NULL)
	MemPtrFree(sfs->path_components);
    if (sfs->dir_item_names != NULL)
	MemPtrFree(sfs->dir_item_names);
    if (sfs->dir_item_types != NULL)
	MemPtrFree(sfs->dir_item_types);
    MemPtrFree(sfs);
    return ret;
}

int select_file(const char *title, const char *ok_label,
	            const char *types, char *buf, int buflen) {
    return select_file_or_dir(title, ok_label, types, buf, buflen, 0);
}

int select_dir(const char *title, const char *ok_label,
	            const char *types, char *buf, int buflen) {
    return select_file_or_dir(title, ok_label, types, buf, buflen, 1);
}
