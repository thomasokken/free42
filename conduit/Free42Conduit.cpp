/*****************************************************************************
 * Free42 -- an HP-42S calculator simulator
 * Copyright (C) 2004-2009  Thomas Okken
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
 * along with this program; if not, see http://www.gnu.org/licenses/.
 *****************************************************************************/

#include <windows.h>
#include <shlobj.h>

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>

#include <stddef.h>
#include <direct.h>
#include <search.h>

#include <Syncmgr.h>
#include <Palm_Cmn.h>
#include <Condapi.h>
#include <HSLog.h>
#include <UserData.h>


#define DllImport   __declspec( dllimport )
#define DllExport   __declspec( dllexport )

#define CONDUIT_NAME "Free42"

#define FILENAMELEN 256

#pragma pack(push, 4)
typedef struct {
	char name[FILENAMELEN];
	DWORD created;
	DWORD modified;
	WORD seqno;
	WORD flags;
} dbfs_header;
#pragma pack(pop)

#define DBFS_FLAG_FIRST 0x0001
#define DBFS_FLAG_LAST 0x0002

/* Number of data bytes per record, excluding the header */
#define DBFS_BLKSIZ 60000

#pragma pack(push, 2)
typedef struct {
	size_t datasize;
	WORD index;
	dbfs_header hdr;
	char data[1];
} blk_wrapper;
#pragma pack(pop)


static int __cdecl blk_wrapper_compare(const void *a, const void *b) {
	WORD ai = (*(blk_wrapper **) a)->index;
	WORD bi = (*(blk_wrapper **) b)->index;
	return ai < bi ? -1 : ai > bi ? 1 : 0;
}

static void set_local_dir(const char *path) {
	HKEY k1, k2, k3;
	DWORD disp;
	if (RegOpenKeyEx(HKEY_CURRENT_USER, "Software", NULL, KEY_QUERY_VALUE, &k1) == ERROR_SUCCESS) {
		if (RegCreateKeyEx(k1, "Thomas Okken Software", 0, "", REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &k2, &disp) == ERROR_SUCCESS) {
			if (RegCreateKeyEx(k2, "Free42", 0, "", REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &k3, &disp) == ERROR_SUCCESS) {
				RegSetValueEx(k3, "LocalDir", 0, REG_SZ, (const unsigned char *) path, strlen(path) + 1);
				RegCloseKey(k3);
			}
			RegCloseKey(k2);
		}
		RegCloseKey(k1);
	}
}

static void get_local_dir(char *path, int pathlen) {
	HKEY k1, k2, k3;
	path[0] = 0;
	if (RegOpenKeyEx(HKEY_CURRENT_USER, "Software", NULL, KEY_QUERY_VALUE, &k1) == ERROR_SUCCESS) {
		if (RegOpenKeyEx(k1, "Thomas Okken Software", NULL, KEY_QUERY_VALUE, &k2) == ERROR_SUCCESS) {
			if (RegOpenKeyEx(k2, "Free42", NULL, KEY_QUERY_VALUE, &k3) == ERROR_SUCCESS) {
				DWORD type, len = pathlen;
				if (RegQueryValueEx(k3, "LocalDir", NULL, &type, (unsigned char *) path, &len) != ERROR_SUCCESS || type != REG_SZ)
					path[0] = 0;
				RegCloseKey(k3);
			}
			RegCloseKey(k2);
		}
		RegCloseKey(k1);
	}
	if (path[0] == 0) {
		strncpy(path, "C:\\Free42FileSys", pathlen - 1);
		path[pathlen - 1] = 0;
	}
}

static void config_local_dir() {
	BROWSEINFO binfo;
	char buf[MAX_PATH];
	get_local_dir(buf, MAX_PATH);
	char msg[MAX_PATH + 100];
	sprintf(msg, "Select local Free42 directory\n(currently %s)", buf);
	binfo.hwndOwner = NULL;
	binfo.pidlRoot = NULL;
	binfo.pszDisplayName = buf;
	binfo.lpszTitle = msg;
	binfo.ulFlags = BIF_RETURNONLYFSDIRS;
	binfo.lpfn = NULL;
	binfo.lParam = 0;
	LPITEMIDLIST idlist = SHBrowseForFolder(&binfo);
	if (idlist != NULL) {
		if (SHGetPathFromIDList(idlist, buf))
			set_local_dir(buf);
		LPMALLOC imalloc;
		if (SHGetMalloc(&imalloc) == NOERROR)
			imalloc->Free(idlist);
	}
}

static int read_blk_wrapper(BYTE db, WORD index, blk_wrapper **blk_ret) {
	int blksize = DBFS_BLKSIZ + sizeof(dbfs_header) + offsetof(blk_wrapper, hdr);
	blk_wrapper *blk = (blk_wrapper *) malloc(blksize);
	if (blk == NULL) {
		LogAddEntry("Free42: Memory allocation failure during download.", slWarning, false);
		return 0;
	}

	CRawRecordInfo rri;
	rri.m_pBytes = (BYTE *) &blk->hdr;
	rri.m_FileHandle = db;
	rri.m_TotalBytes = blksize - offsetof(blk_wrapper, hdr);
	rri.m_dwReserved = 0;

	long res;
	if (index == 0)
		res = SyncReadNextModifiedRec(rri);
	else {
		rri.m_RecIndex = index;
		res = SyncReadRecordByIndex(rri);
	}

	if (index == 0 && res == SYNCERR_NOT_FOUND) {
		free(blk);
		*blk_ret = NULL;
		return 1;
	}
	if (res != 0) {
		free(blk);
		LogAddFormattedEntry(slWarning, false, "Free42: Error while retrieving modified records (0x%08lx).", res);
		return 0;
	}

	if (rri.m_RecSize > rri.m_TotalBytes) {
		// Dang! My buffer was too small. Create one that will fit,
		// and try again.
		free(blk);
		blksize = rri.m_RecSize + offsetof(blk_wrapper, hdr);
		blk = (blk_wrapper *) malloc(blksize);
		if (blk == NULL) {
			LogAddEntry("Free42: Memory allocation failure during download.", slWarning, false);
			return 0;
		}
		rri.m_pBytes = (BYTE *) &blk->hdr;
		rri.m_TotalBytes = (WORD) rri.m_RecSize;
		res = SyncReadRecordByIndex(rri);
		if (res != 0) {
			free(blk);
			LogAddFormattedEntry(slWarning, false, "Free42: Error while retrieving modified records (0x%08lx).", res);
			return 0;
		}
		blk->datasize = blksize - offsetof(blk_wrapper, data);
	} else if (rri.m_RecSize < rri.m_TotalBytes) {
		// Reallocate smaller buffer, to minimize memory usage
		blksize = rri.m_RecSize + offsetof(blk_wrapper, hdr);
		blk_wrapper *b = (blk_wrapper *) realloc(blk, blksize);
		if (b != NULL)
			// Can't think why reallocating a *smaller* block would ever fail,
			// but what the heck...
			blk = b;
		blk->datasize = blksize - offsetof(blk_wrapper, data);
	} else {
		// Buffer was just the right size.
		blk->datasize = DBFS_BLKSIZ;
	}

	blk->index = rri.m_RecIndex;
	if (rri.m_RecSize >= sizeof(dbfs_header)) {
		blk->hdr.created = SyncHHToHostDWord(blk->hdr.created);
		blk->hdr.modified = SyncHHToHostDWord(blk->hdr.modified);
		blk->hdr.seqno = SyncHHToHostWord(blk->hdr.seqno);
		blk->hdr.flags = SyncHHToHostWord(blk->hdr.flags);
	}
	*blk_ret = blk;
	return 1;
}

static void make_parent_dir(const char *path_) {
	char path[MAX_PATH + FILENAMELEN + 3];
	strcpy(path, path_);
	for (char *p = path; *p != 0; p++) {
		if (*p == '\\') {
			*p = 0;
			mkdir(path);
			*p = '\\';
		}
	}
}

static void set_timestamps(const char *path, DWORD created, DWORD modified) {
	HANDLE h = CreateFile(path, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (h == NULL)
		return;

	// PalmOS time is in seconds since 1/1/1904, 0:00 local (i.e. like MacOS time);
	// Windows time is in hundreds of nanoseconds since 1/1/1601, 0:00 UTC.
	// (I'd prefer to use UTC timestamps in Free42, but time zones only appeared in
	// PalmOS version 4.0, and I want it to have the same behavior on all versions.)

	// SetFileTime() does not allow time stamps before 1/1/1980 0:00 local.
	// That cutoff moment corresponds with the time stamp 119600064000000000,
	// which I have to convert to UTC before applying it:

	__int64 limit = 119600064000000000;
	__int64 bias = 0;

	TIME_ZONE_INFORMATION tzinfo;
	if (GetTimeZoneInformation(&tzinfo) != TIME_ZONE_ID_UNKNOWN)
		bias = ((__int64) tzinfo.Bias) * 600000000;

	limit += bias;

	// Between 1/1/1601 and 1/1/1904, there are three centuries plus three non-leap years;
	// in the three centuries, there are three centennials, none of which is a leap year,
	// so there are 300 * 365.25 - 3 + 3 * 365 = 110667 days between those dates.

	__int64 wincreated = ((__int64) created) * 10000000 + 95616288000000000 + bias;
	if (wincreated < limit)
		wincreated = limit;
	__int64 winmodified = ((__int64) modified) * 10000000 + 95616288000000000 + bias;
	if (winmodified < limit)
		winmodified = limit;

	SetFileTime(h, (FILETIME *) &wincreated, (FILETIME *) &winmodified, (FILETIME *) &winmodified);
	CloseHandle(h);
}

static int upload2(int *db_open, BYTE *db, WORD *seqno, char *path, int basepathlen) {
	int pathlen = strlen(path);
	strncat(path, "\\*.*", MAX_PATH - 1);
	path[MAX_PATH - 1] = 0;

	WIN32_FIND_DATA wfd;
	HANDLE search = FindFirstFile(path, &wfd);
	path[pathlen] = 0;

	if (search == INVALID_HANDLE_VALUE)
		return 1;

	do {
		strncat(path, "\\", MAX_PATH - 1);
		strncat(path, wfd.cFileName, MAX_PATH - 1);
		path[MAX_PATH - 1] = 0;

		if ((wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
			// Directory: recurse, but avoid "." and ".."
			if (strcmp(wfd.cFileName, ".") != 0 && strcmp(wfd.cFileName, "..") != 0) {
				if (!upload2(db_open, db, seqno, path, basepathlen)) {
					FindClose(search);
					return 0;
				}
			}
		} else {
			// File: upload it
			int success = 0;
			FILE *file;
			__int64 bias = 0;

			if (!*db_open) {
				long res = SyncOpenDB("Free42FileSys", 0, *db, eDbRead | eDbWrite);
				if (res == SYNCERR_NOT_FOUND) {
					CDbCreateDB dbinfo;
					dbinfo.m_Creator = 'Fk42';
					dbinfo.m_Type = 'FSys';
					dbinfo.m_Flags = eRecord;
					strcpy(dbinfo.m_Name, "Free42FileSys");
					dbinfo.m_CardNo = 0;
					dbinfo.m_Version = 0;
					dbinfo.m_dwReserved = 0;
					res = SyncCreateDB(dbinfo);
					if (res != 0) {
						LogAddFormattedEntry(slWarning, false, "Free42: Could not create Free42FileSys database (0x%08lx).", res);
						return 0;
					}
					*db = dbinfo.m_FileHandle;
					*db_open = 1;
				} else if (res != 0) {
					LogAddFormattedEntry(slWarning, false, "Free42: Could not open Free42FileSys database (0x%08lx).", res);
					return 0;
				} else {
					*db_open = 1;
					// In order to allow multiple uploads to happen without having to
					// run Free42 in between to absorb them, I use category IDs 1-15.
					// Category 0 are absorbed records; category 1 is the most recent
					// upload; category 2 is the next most recent download, etc.
					// If more than 15 uploads take place without running Free42, we
					// run out of categories; in this case, I log a warning and abort.
					res = SyncResetRecordIndex(*db);
					if (res != 0) {
						LogAddFormattedEntry(slWarning, false, "Free42: Unexpected error during upload (0x%08lx).", res);
						return 0;
					}
					CRawRecordInfo rri;
					BYTE buf[1];
					rri.m_pBytes = buf;
					rri.m_FileHandle = *db;
					rri.m_CatId = 15;
					rri.m_TotalBytes = 1;
					rri.m_dwReserved = 0;
					res = SyncReadNextRecInCategory(rri);
					if (res == 0) {
						LogAddEntry("Free42: There are too many uploads queued up on the handheld.", slWarning, false);
						LogAddEntry("Free42: You must run Free42 at least once on the handheld, so it can process", slWarning, false);
						LogAddEntry("Free42: the queued-up uploads, before you can send another.", slWarning, false);
						return 0;
					}
					if (res != SYNCERR_NOT_FOUND) {
						LogAddFormattedEntry(slWarning, false, "Free42: Unexpected error during upload (0x%08lx).", res);
						return 0;
					}
					for (int i = 14; i >= 1; i--) {
						res = SyncChangeCategory(*db, i, i + 1);
						if (res != 0) {
							LogAddFormattedEntry(slWarning, false, "Free42: Unexpected error during upload (0x%08lx).", res);
							return 0;
						}
					}
				}
				char c = path[basepathlen];
				path[basepathlen] = 0;
				LogAddFormattedEntry(slText, false, "Free42: Uploading from \"%s\"...", path);
				path[basepathlen] = c;
			}

			dbfs_header *hdr = (dbfs_header *) malloc(sizeof(dbfs_header) + DBFS_BLKSIZ);
			if (hdr == NULL) {
				LogAddEntry("Free42: Memory allocation failure during upload.", slWarning, false);
				goto abort3;
			}
			int i;
			char c;
			i = basepathlen;
			do {
				c = path[i];
				if (c == '\\')
					c = '/';
				hdr->name[i - basepathlen] = c;
				i++;
			} while (c != 0);
			LogAddFormattedEntry(slText, false, "Free42: Uploading \"%s\"", hdr->name);

			TIME_ZONE_INFORMATION tzinfo;
			if (GetTimeZoneInformation(&tzinfo) != TIME_ZONE_ID_UNKNOWN)
				bias = ((__int64) tzinfo.Bias) * 600000000;

			hdr->created = SyncHostToHHDWord((DWORD) ((*(__int64 *) &wfd.ftCreationTime - 95616288000000000 - bias) / 10000000));
			hdr->modified = SyncHostToHHDWord((DWORD) ((*(__int64 *) &wfd.ftLastWriteTime - 95616288000000000 - bias) / 10000000));
			hdr->flags = SyncHostToHHWord(DBFS_FLAG_FIRST);

			CRawRecordInfo rri;
			rri.m_FileHandle = *db;
			rri.m_Attribs = 0;
			rri.m_CatId = 1; // Indicates newly uploaded record
			rri.m_pBytes = (BYTE *) hdr;
			rri.m_dwReserved = 0;

			file = fopen(path, "rb");
			if (file == NULL) {
				LogAddFormattedEntry(slWarning, false, "Can't open \"%s\" for reading.", path);
				goto abort2;
			}
			size_t n;
			while ((n = fread(hdr + 1, 1, DBFS_BLKSIZ, file)) != 0) {
				long res = SyncYieldCycles(1);
				if (res != 0) {
					LogAddFormattedEntry(slWarning, false, "Free42: Unexpected error during upload (0x%08lx).", res);
					goto abort1;
				}
				if (feof(file))
					hdr->flags |= SyncHostToHHWord(DBFS_FLAG_LAST);
				rri.m_RecSize = sizeof(dbfs_header) + n;
				rri.m_RecId = 0;
				hdr->seqno = SyncHostToHHWord((*seqno)++);
				res = SyncWriteRec(rri);
				if (res != 0) {
					LogAddFormattedEntry(slWarning, false, "Error uploading \"%s\".", hdr->name);
					goto abort1;
				}
				hdr->flags &= SyncHostToHHWord(~DBFS_FLAG_FIRST);
			}
			success = 1;

			abort1:
			fclose(file);

			abort2:
			free(hdr);

			abort3:
			if (!success) {
				FindClose(search);
				return 0;
			}
		}
		path[pathlen] = 0;

	} while (FindNextFile(search, &wfd));

	FindClose(search);
	return 1;
}

static int upload() {
	char path[MAX_PATH];
	get_local_dir(path, MAX_PATH);
	int pathlen = strlen(path);
	if (path[pathlen - 1] != '\\') 
		strncat(path, "\\", MAX_PATH - 1);
	strncat(path, "Upload", MAX_PATH - 1);
	path[MAX_PATH - 1] = 0;
	pathlen = strlen(path);

	struct _stat statbuf;
	if (_stat(path, &statbuf) == -1) {
		// Directory does not exist? Create it, just to remind
		// the user of where they're supposed to put their uploadables.
		mkdir(path);
		return 1;
	}
	if ((statbuf.st_mode & _S_IFDIR) == 0) {
		// There's something called "Upload", but it's not a directory.
		LogAddFormattedEntry(slWarning, false, "Free42: NOTE: \"%s\" is not a directory!", path);
		return 1;
	}

	BYTE db;
	int db_open = 0;
	WORD seqno = 0;
	int res = upload2(&db_open, &db, &seqno, path, pathlen);
	if (db_open) {
		SyncCloseDB(db);
		// Rename "Upload" directory to "Uploaded YYYY.MM.DD HH.MM.SS",
		// but only if the upload had no errors, and only if something
		// actually got uploaded (don't want to rename "Upload" if it
		// was empty!).
		if (res && seqno > 0) {
			path[pathlen] = 0; // In case upload2() left something tacked on
			char newpath[MAX_PATH];
			get_local_dir(newpath, MAX_PATH);
			int newpathlen = strlen(newpath);
			if (newpath[newpathlen - 1] != '\\') 
				strncat(newpath, "\\", MAX_PATH - 1);
			strncat(newpath, "Uploaded", MAX_PATH - 1);
			newpath[MAX_PATH - 1] = 0;
			newpathlen = strlen(newpath);
			if (newpathlen < MAX_PATH - 2) {
				time_t t = time(NULL);
				struct tm *tim = localtime(&t);
				sprintf(newpath + newpathlen, " %04d.%02d.%02d %02d.%02d.%02d",
						tim->tm_year + 1900, tim->tm_mon + 1, tim->tm_mday,
						tim->tm_hour, tim->tm_min, tim->tm_sec);
			} else if (newpathlen < MAX_PATH - 4) {
				// Not enough room to add on the full time stamp;
				// try to create a unique name by tacking on a 3-digit
				// sequence number.
				int j = 0;
				do {
					j++;
					sprintf(newpath + newpathlen, ".%03d", j);
				} while (_stat(path, &statbuf) == 0 && j < 999);
			} // else: just try "Uploaded" by itself.

			if (MoveFile(path, newpath))
				// Create new "Upload" directory to replace the one we just renamed
				mkdir(path);
		}
	}
	return res;
}


extern "C" DllExport long GetConduitInfo(ConduitInfoEnum infoType, void *pInArgs,
										 void *pOut, DWORD *pdwOutSize) {
	// Do some error checking on passed-in parameters.
	// Do not modify these lines.
	if (!pOut)
		return CONDERR_INVALID_PTR;
	if (!pdwOutSize)
		return CONDERR_INVALID_OUTSIZE_PTR;

	// HotSync Manager requests information from your conduit through
	// repeated calls to GetConduitInfo.
	// The type of information requested is determined by infoType.
	// Your conduit responds appropriately.
	switch (infoType) {
		case eConduitName: {
			// HotSync Manager wants to know the conduit name.
			char *s = (char *) pOut;
			strncpy(s, CONDUIT_NAME, *pdwOutSize - 1);
			s[*pdwOutSize - 1] = 0;
			*pdwOutSize = strlen(s);
			break;
		}

		case eDefaultAction:
			// HotSync Manager wants to know the default sync action.
			if (*pdwOutSize != sizeof(eSyncTypes))
				return CONDERR_INVALID_BUFFER_SIZE;
			*(eSyncTypes *) pOut = eHHtoPC; 
			break; 

		case eMfcVersion:
			// HotSync Manager wants to know the version of MFC.
			if (*pdwOutSize != sizeof(DWORD))
				return CONDERR_INVALID_BUFFER_SIZE;
			*(DWORD *) pOut = MFC_NOT_USED;
			break;

		default:
			return CONDERR_UNSUPPORTED_CONDUITINFO_ENUM;
	}
	return 0;
}

extern "C" DllExport DWORD GetConduitVersion() {
	return 0x00000101; 
}

extern "C" DllExport long OpenConduit(PROGRESSFN pFn, CSyncProperties &rProps) {
	CONDHANDLE ch;
	if (SyncRegisterConduit(ch) != 0) {
		LogAddEntry("Free42: Could not register the conduit.", slWarning, false);
		return CONDERR_NONE;
	}

	BYTE db;
	int db_open = 0;
	int i;
	WORD numrecs;
	int ok = 1;

	long res = SyncOpenDB("Free42FileSys", 0, db, eDbRead | eDbWrite);
	if (res != 0) {
		if (res == SYNCERR_NOT_FOUND) {
			// Not an error; the user has installed the conduit and Free42,
			// but apparently has not used Free42 yet, or maybe deleted the
			// database manually. We don't exit yet; we skip to the "upload"
			// stage, and if we have stuff to upload, we'll just create
			// the db then.
			goto do_upload;
		} else {
			LogAddFormattedEntry(slWarning, false, "Free42: Could not open Free42FileSys database (0x%08lx).", res);
			SyncUnRegisterConduit(ch);
		}
		return CONDERR_NONE;
	}
	res = SyncGetDBRecordCount(db, numrecs);
	if (res != 0) {
		LogAddFormattedEntry(slWarning, false, "Free42: Could not get Free42FileSys DB record count (0x%08lx).", res);
		SyncCloseDB(db);
		SyncUnRegisterConduit(ch);
		return CONDERR_NONE;
	}

	// The Free42FileSys database is open; let's find out if there's anything to download.
	int blk_cap;
	int blk_count;
	blk_wrapper **blocks;
	blk_cap = 100;
	blk_count = 0;
	blocks = (blk_wrapper **) malloc(blk_cap * sizeof(blk_wrapper *));
	// TODO - handle memory allocation failure

	while (1) {
		if ((res = SyncYieldCycles(1)) != 0) {
			LogAddFormattedEntry(slWarning, false, "Free42: Unexpected error during download (0x%08lx).", res);
			goto abort_download;
		}

		if (blk_count == blk_cap) {
			blk_cap += 100;
			blk_wrapper **newblocks = (blk_wrapper **) realloc(blocks, blk_cap * sizeof(blk_wrapper *));
			if (newblocks == NULL) {
				LogAddEntry("Free42: Memory allocation failure during download.", slWarning, false);
				goto abort_download;
			}
			blocks = newblocks;
		}

		blk_wrapper *blk;
		if (!read_blk_wrapper(db, 0, &blk))
			goto abort_download;
		if (blk == NULL)
			break;
		if (blk->index == 0 || blk->index == 1)
			free(blk);
		else
			blocks[blk_count++] = blk;
	}


	// We're done reading *modified* records; now we have to download all the other
	// records that belong to the files that were touched.
	// Since the DBFS filesystem stores files as runs of consecutive blocks, it
	// helps to sort the blocks array by index now; that way, we can then traverse
	// the array sequentially, dealing with only one file at a time.

	qsort(blocks, blk_count, sizeof(blk_wrapper *), blk_wrapper_compare);

	char filename[MAX_PATH];
	filename[0] = 0;
	char basepath[MAX_PATH];
	get_local_dir(basepath, MAX_PATH);
	int basepathlen;
	basepathlen = strlen(basepath);

	// Force base directory to be created. I want it to exist, even if it turns
	// out we don't create any new files.
	strcat(basepath, "\\");
	make_parent_dir(basepath);
	basepath[basepathlen] = 0;

	char path[MAX_PATH + FILENAMELEN + 3];
	WORD last_rec;
	int logged_download_message;
	logged_download_message = 0;
	DWORD created, modified;

	if (blk_count == 0) {
		res = 0;
		goto nothing_to_download;
	} 

	i = 0;
	FILE *file;
	while (1) {
		if (i < blk_count && filename[0] == 0) {
			// New file
			strcpy(filename, blocks[i]->hdr.name);
			strcpy(path, basepath);
			char *dst = path + basepathlen;
			char *src = filename;
			char c;
			while ((c = *src++) != 0) {
				if (c == '/')
					c = '\\';
				*dst++ = c;
			}
			*dst = 0;
			
			// If file exists, rename it from "foo" to "foo.001" (try "foo.002",
			// "foo.003", etc., until a name is found that is not yet taken).
			struct _stat statbuf;
			if (_stat(path, &statbuf) == 0) {
				// 'path' exists.
				int j = 0;
				do {
					j++;
					sprintf(dst, ".%03d", j);
				} while (_stat(path, &statbuf) == 0 && j < 999);
				char newname[MAX_PATH + FILENAMELEN + 3];
				strcpy(newname, path);
				*dst = 0;
				rename(path, newname);
			} else
				make_parent_dir(path);

			if (!logged_download_message) {
				LogAddFormattedEntry(slText, false, "Free42: Downloading to \"%s\"...", basepath);
				logged_download_message = 1;
			}
			LogAddFormattedEntry(slText, false, "Free42: Downloading \"%s\"", filename);
			file = fopen(path, "wb");
			if (file == NULL) {
				LogAddFormattedEntry(slWarning, false, "Free42: Could not create local file \"%s\" to download \"%s\".", path, filename);
				goto abort_download;
			}
			created = blocks[i]->hdr.created;
			modified = blocks[i]->hdr.modified;

			last_rec = blocks[i]->index;
			if (last_rec > 2
					&& (blocks[i]->hdr.flags & DBFS_FLAG_FIRST) == 0
					&& !(i > 0 && blocks[i - 1]->index == last_rec - 1)) {
				// The first dirty record for this new file is not the
				// first record of the file. We load previous records, going
				// backwards by index, until we encounter one that has the
				// 'first' flag set (or until we encounter one whose name
				// does not match, just to be safe).
				blk_wrapper **blocks2 = (blk_wrapper **) malloc(100 * sizeof(blk_wrapper *));
				// TODO - handle memory allocation failure
				int blk2_count = 0;
				int blk2_cap = 100;
				WORD idx = last_rec;
				int success = 0;
				while (1) {
					if ((res = SyncYieldCycles(1)) != 0) {
						fclose(file);
						LogAddFormattedEntry(slWarning, false, "Free42: Unexpected error during download (0x%08lx).", res);
						goto abort_download_2;
					}

					if (blk2_count == blk2_cap) {
						blk2_cap += 100;
						blk_wrapper **newblocks2 = (blk_wrapper **) realloc(blocks2, blk2_cap * sizeof(blk_wrapper *));
						if (newblocks2 == NULL) {
							fclose(file);
							LogAddEntry("Free42: Memory allocation failure during download.", slWarning, false);
							goto abort_download_2;
						}
						blocks2 = newblocks2;
					}

					blk_wrapper *blk2;
					idx--;
					if (idx == 1)
						break;
					if (!read_blk_wrapper(db, idx, &blk2)) {
						fclose(file);
						goto abort_download_2;
					}
					if (_stricmp(blk2->hdr.name, filename) != 0) {
						free(blk2);
						break;
					}
					blocks2[blk2_count++] = blk2;
					if ((blk2->hdr.flags & DBFS_FLAG_FIRST) != 0)
						break;
				}
				int n;
				for (n = blk2_count - 1; n >= 0; n--) {
					size_t written = fwrite(&blocks2[n]->data, 1, blocks2[n]->datasize, file);
					if (written != blocks2[n]->datasize) {
						LogAddFormattedEntry(slWarning, false, "Free42: Error writing local file \"%s\" while downloading \"%s\".", path, filename);
						goto abort_download_2;
					}
					if (blocks2[n]->hdr.created < created)
						created = blocks2[n]->hdr.created;
					if (blocks2[n]->hdr.modified > modified)
						modified = blocks2[n]->hdr.modified;
				}
				success = 1;

				abort_download_2:
				for (n = 0; n < blk2_count; n++)
					free(blocks2[n]);
				free(blocks2);
				if (!success)
					goto abort_download;
			}

			// Write block[i]
			if (fwrite(&blocks[i]->data, 1, blocks[i]->datasize, file) != blocks[i]->datasize) {
				LogAddFormattedEntry(slWarning, false, "Free42: Error writing local file \"%s\" while downloading \"%s\".", path, filename);
				goto abort_download;
			}
			if ((blocks[i]->hdr.flags & DBFS_FLAG_LAST) != 0) {
				fclose(file);
				set_timestamps(path, created, modified);
				filename[0] = 0;
			}
			i++;

		} else {
			
			// Have an open file already
			if (i == blk_count || _stricmp(blocks[i]->hdr.name, filename) != 0) {
				// Must finish the current file, then start the next
				if (i < blk_count && blocks[i]->index == last_rec + 1)
					goto file_done;
				another:
				last_rec++;
				if (last_rec == numrecs)
					goto file_done;
				blk_wrapper *blk2;
				if (!read_blk_wrapper(db, last_rec, &blk2)) {
					fclose(file);
					goto abort_download;
				}
				if (_stricmp(blk2->hdr.name, filename) == 0) {
					if (fwrite(&blk2->data, 1, blk2->datasize, file) != blk2->datasize) {
						fclose(file);
						LogAddFormattedEntry(slWarning, false, "Free42: Error writing local file \"%s\" while downloading \"%s\".", path, filename);
						goto abort_download;
					}
					if (blk2->hdr.created < created)
						created = blk2->hdr.created;
					if (blk2->hdr.modified > modified)
						modified = blk2->hdr.modified;
				}
				if ((blk2->hdr.flags & DBFS_FLAG_LAST) != 0 || _stricmp(blk2->hdr.name, filename) != 0) {
					file_done:
					fclose(file);
					set_timestamps(path, created, modified);
					filename[0] = 0;
					if (i == blk_count)
						break;
					else
						continue; // iterating over the 'blocks' array
				} else
					goto another; // block in the tail of the current file
			}

			// We have an open file; block[i] belongs to that file; we're not
			// at the end of the file (unless block[i] happens to be the last;
			// we'll deal with that after writing it).

			WORD curr_rec = blocks[i]->index;
			if (curr_rec > last_rec + 1) {
				for (WORD k = last_rec + 1; k < curr_rec; k++) {
					blk_wrapper *blk2;
					if (!read_blk_wrapper(db, k, &blk2)) {
						fclose(file);
						goto abort_download;
					}
					// No check on the file name this time. Should always match,
					// since we have blocks from before AND after index 'k' with
					// matching names, but I guess a check and a log entry would
					// be nice. TODO.
					if (fwrite(&blk2->data, 1, blk2->datasize, file) != blk2->datasize) {
						fclose(file);
						LogAddFormattedEntry(slWarning, false, "Free42: Error writing local file \"%s\" while downloading \"%s\".", path, filename);
						goto abort_download;
					}
					if (blk2->hdr.created < created)
						created = blk2->hdr.created;
					if (blk2->hdr.modified > modified)
						modified = blk2->hdr.modified;
				}
			}
			if (fwrite(&blocks[i]->data, 1, blocks[i]->datasize, file) != blocks[i]->datasize) {
				fclose(file);
				LogAddFormattedEntry(slWarning, false, "Free42: Error writing local file \"%s\" while downloading \"%s\".", path, filename);
				goto abort_download;
			}
			if (blocks[i]->hdr.created < created)
				created = blocks[i]->hdr.created;
			if (blocks[i]->hdr.modified > modified)
				modified = blocks[i]->hdr.modified;

			if ((blocks[i]->hdr.flags & DBFS_FLAG_LAST) != 0) {
				fclose(file);
				set_timestamps(path, created, modified);
				filename[0] = 0;
			} else
				last_rec = curr_rec;

			i++;
		}
	}

	// The download phase was finished successfully; we can now clear the 'dirty' bits
	// in the Free42FileSys database.

	res = SyncResetSyncFlags(db);
	
	nothing_to_download:
	if (res != 0)
		LogAddFormattedEntry(slWarning, false, "Free42: Unexpected error while finishing up download (0x%08lx).", res);
	goto still_ok;

	abort_download:
	ok = 0;

	still_ok:
	for (i = 0; i < blk_count; i++)
		free(blocks[i]);
	free(blocks);
	SyncCloseDB(db);

	// The upload code is in a separate function. There are limits
	// to the amount of spaghetti code I can manage all at once. :-)

	do_upload:
	if (!upload())
		ok = 0;

	if (ok)
		LogAddEntry("OK Free42", slText, false);
	else
		LogAddEntry("Free42: Done.", slText, false);

	SyncUnRegisterConduit(ch);
	return CONDERR_NONE;
}

extern "C" DllExport long CfgConduit(ConduitCfgEnum cfgType, void *pArgs, DWORD *pdwArgsSize) {
	config_local_dir();
	return 0;
}

extern "C" DllExport long ConfigureConduit(CSyncPreference &syncPrefs) {
	config_local_dir();
	return 0;
}

extern "C" DllExport long GetConduitName(char *pszName, WORD nLen) {
	strncpy(pszName, CONDUIT_NAME, nLen - 1);
	pszName[nLen - 1] = 0;
	return 0;
}
