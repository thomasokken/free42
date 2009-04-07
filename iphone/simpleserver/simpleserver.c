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

#include <dirent.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#ifdef FREE42
#include "../../common/core_main.h"
#endif

#ifndef VERSION
#define VERSION ""
#endif

/* TODO:
 * The code is currently not thread-safe; it uses static
 * buffers. That should be fixed before it can be considered ready
 * for embedding in a multithreaded environment (i.e., iPhone).
 */

typedef struct {
    char *buf;
    int size;
    int capacity;
} textbuf;

#define LINEBUFSIZE 1024

/* Stuff defined in icons.c */
extern int icon_count;
extern const char *icon_name[];
extern long icon_size[];
extern unsigned char *icon_data[];

#ifdef STANDALONE
static
#endif
void handle_client(int csock);

static void sockprintf(int sock, char *fmt, ...);
static void tbwrite(textbuf *tb, const char *data, int size);
static void tbprintf(textbuf *tb, const char *fmt, ...);
static void do_get(int csock, const char *url);
static void do_post(int csock, const char *url);
static const char *canonicalize_url(const char *url);
static int open_item(const char *url, void **ptr, int *type, int *filesize);
static const char *get_mime(const char *ext);
static void http_error(int csock, int err);

static void read_line(int csock, char *buf, int bufsize) {
    int p = 0;
    int afterCR = 0;
    while (1) {
	int n = recv(csock, buf + p, 1, 0);
	if (n == -1) {
	    buf[p] = 0;
	    break;
	}
	if (afterCR) {
	    if (buf[p] == '\n') {
		buf[p] = 0;
		break;
	    } else {
		afterCR = 0;
		if (p < bufsize - 1)
		    p++;
	    }
	} else {
	    if (buf[p] == '\r') {
		afterCR = 1;
	    } else {
		if (p < bufsize - 1)
		    p++;
	    }
	}
    }
    buf[p] = 0;
}

#ifdef STANDALONE
static
#endif
void handle_client(int csock) {
    char *req;
    char *url;
    char *protocol;

    req = (char *) malloc(LINEBUFSIZE);
    if (req == NULL) {
	fprintf(stderr, "Memory allocation failure while allocating line buffer\n");
	shutdown(csock, SHUT_WR);
	return;
    }

    read_line(csock, req, LINEBUFSIZE);
    fprintf(stderr, "%s\n", req);

    url = strchr(req, ' ');
    if (url == NULL) {
	fprintf(stderr, "Malformed HTTP request: \"%s\"\n", req);
	shutdown(csock, SHUT_WR);
	free(req);
	return;
    }

    protocol = strchr(url + 1, ' ');
    if (protocol == NULL) {
	fprintf(stderr, "Malformed HTTP request: \"%s\"\n", req);
	shutdown(csock, SHUT_WR);
	free(req);
	return;
    }

    *url++ = 0;
    *protocol++ = 0;
    if (strncmp(protocol, "HTTP/", 5) != 0) {
	fprintf(stderr, "Unsupported protocol: \"%s\"\n", protocol);
	shutdown(csock, SHUT_WR);
	free(req);
	return;
    }

    if (strcmp(req, "GET") == 0)
	do_get(csock, url);
    else if (strcmp(req, "POST") == 0)
	do_post(csock, url);
    else
	fprintf(stderr, "Unsupported method: \"%s\"\n", req);
    shutdown(csock, SHUT_WR);
    free(req);
}

static void sockprintf(int sock, char *fmt, ...) {
    va_list ap;
    char text[LINEBUFSIZE];
    ssize_t sent;
    int err;
    va_start(ap, fmt);
    vsprintf(text, fmt, ap);
    sent = send(sock, text, strlen(text), 0);
    err = errno;
    if (sent != strlen(text))
	fprintf(stderr, "send() only sent %d out of %d bytes: %s (%d)\n", sent, strlen(text),
		strerror(err), err);
    va_end(ap);
}

static void tbwrite(textbuf *tb, const char *data, int size) {
    if (tb->size + size > tb->capacity) {
	int newcapacity = tb->capacity == 0 ? 1024 : (tb->capacity << 1);
	while (newcapacity < tb->size + size)
	    newcapacity <<= 1;
	char *newbuf = (char *) realloc(tb->buf, newcapacity);
	if (newbuf == NULL) {
	    /* Bummer! Let's just append as much as we can */
	    memcpy(tb->buf + tb->size, data, tb->capacity - tb->size);
	    tb->size = tb->capacity;
	} else {
	    tb->buf = newbuf;
	    tb->capacity = newcapacity;
	    memcpy(tb->buf + tb->size, data, size);
	    tb->size += size;
	}
    } else {
	memcpy(tb->buf + tb->size, data, size);
	tb->size += size;
    }
}

static void tbprintf(textbuf *tb, const char *fmt, ...) {
    va_list ap;
    char text[LINEBUFSIZE];
    va_start(ap, fmt);
    vsprintf(text, fmt, ap);
    tbwrite(tb, text, strlen(text));
    va_end(ap);
}

typedef struct dir_item {
    char *name;
    int size;
    int type; /* 0=unknown, 1=file, 2=dir */
    char mtime[64];
    struct dir_item *next;
} dir_item;

static int dir_item_compare(const void *va, const void *vb) {
    dir_item *a = *((dir_item **) va);
    dir_item *b = *((dir_item **) vb);
    return strcmp(a->name, b->name);
}

static void do_get(int csock, const char *url) {
    int err;
    void *ptr;
    int type;
    int filesize;
    DIR *dir;
    char buf[LINEBUFSIZE];
    int n;

    url = canonicalize_url(url);
    if (url == NULL) {
	http_error(csock, 403);
	return;
    }

    err = open_item(url, &ptr, &type, &filesize);
    if (err != 200) {
	free((void *) url);
	http_error(csock, err);
	return;
    }

    if (type == 0 || type == 1) {
	sockprintf(csock, "HTTP/1.0 200 OK\r\n");
	sockprintf(csock, "Connection: close\r\n");
	sockprintf(csock, "Content-Type: %s\r\n", get_mime(url));
	sockprintf(csock, "Content-Length: %d\r\n", filesize);
	sockprintf(csock, "\r\n");
	if (type == 0)
	    send(csock, ptr, filesize, 0);
	else {
	    FILE *file = (FILE *) ptr;
	    while ((n = fread(buf, 1, LINEBUFSIZE, file)) > 0)
		send(csock, buf, n, 0);
	    fclose(file);
	}
    } else if (url[strlen(url) - 1] != '/') {
	sockprintf(csock, "HTTP/1.0 302 Moved Temporarily\r\n");
	sockprintf(csock, "Connection: close\r\n");
	sockprintf(csock, "Location: %s/\r\n", url);
	sockprintf(csock, "\r\n");
	if (type == 3)
	    closedir((DIR *) ptr);
    } else {
	struct dir_item *dir_list = NULL;
	int dir_length = 0;
	struct dir_item **dir_array;
	textbuf tb = { NULL, 0, 0 };
	int i;

	if (type == 3) {
	    struct dirent *d;
	    dir = (DIR *) ptr;

	    while ((d = readdir(dir)) != NULL) {
		struct stat s;
		struct tm stm;
		dir_item *di = (dir_item *) malloc(sizeof(dir_item));
		
		if (strcmp(d->d_name, ".") == 0 || strcmp(d->d_name, "..") == 0)
		    continue;
		if (strlen(url) == 1)
		    err = stat(d->d_name, &s);
		else {
		    char *p = (char *) malloc(strlen(url) + strlen(d->d_name) + 1);
		    strcpy(p, url + 1);
		    strcat(p, "/");
		    strcat(p, d->d_name);
		    err = stat(p, &s);
		    free(p);
		}
		di->name = (char *) malloc(strlen(d->d_name) + 1);
		strcpy(di->name, d->d_name);
		if (err == 0) {
		    localtime_r(&s.st_mtime, &stm);
		    strftime(di->mtime, sizeof(di->mtime), "%d-%b-%Y %H:%M:%S", &stm);
		    if (S_ISREG(s.st_mode)) {
			di->type = 1;
			di->size = s.st_size;
		    } else if (S_ISDIR(s.st_mode))
			di->type = 2;
		    else
			di->type = 0;
		} else
		    di->type = 0;
		di->next = dir_list;
		dir_list = di;
		dir_length++;
	    }
	    closedir(dir);
#ifdef FREE42
	    if (strcmp(url, "/") == 0) {
		/* Make sure there is a directory called "memory"; override
		 * whatever real item exists with that name, if necessary
		 */
		int found = 0;
		dir_item *di = dir_list;
		while (di != NULL) {
		    if (strcmp(di->name, "memory") == 0) {
			di->type = 2;
			strcpy(di->mtime, "?");
			found = 2;
			break;
		    }
		    di = di->next;
		}
		if (!found) {
		    di = (dir_item *) malloc(sizeof(dir_item));
		    di->name = (char *) malloc(7);
		    strcpy(di->name, "memory");
		    strcpy(di->mtime, "?");
		    di->type = 2;
		    di->next = dir_list;
		    dir_list = di;
		    dir_length++;
		}
	    }
#endif
	} else {
	    /* type == 2: fake directory for /memory */
	    char **name = (char **) ptr;
	    dir_item *dir_tail;
	    while (*name != NULL) {
		dir_item *di = (dir_item *) malloc(sizeof(dir_item));
		di->name = *name;
		di->size = 0;
		di->type = 1;
		strcpy(di->mtime, "?");
		di->next = NULL;
		if (dir_list == NULL)
		    dir_list = di;
		else
		    dir_tail->next = di;
		dir_tail = di;
		name++;
		dir_length++;
	    }
	}		
	
	dir_array = (dir_item **) malloc(dir_length * sizeof(dir_item *));
	for (i = 0; i < dir_length; i++) {
	    dir_array[i] = dir_list;
	    dir_list = dir_list->next;
	}
	
	if (type == 3)
	    qsort(dir_array, dir_length, sizeof(dir_item *), dir_item_compare);
	
	tbprintf(&tb, "<html>\n");
	tbprintf(&tb, " <head>\n");
	tbprintf(&tb, "  <title>Index of %s</title>\n", url);
	tbprintf(&tb, "  <style type=\"text/css\">\n");
	tbprintf(&tb, "   td { padding-left: 10px }\n");
	tbprintf(&tb, "  </style>\n");
	tbprintf(&tb, " </head>\n");
	tbprintf(&tb, " <body>\n");
	tbprintf(&tb, "  <h1>Index of %s</h1>\n", url);
	tbprintf(&tb, "  <table><tr><th><img src=\"/icons/blank.gif\"></th><th>Name</th><th>Last modified</th><th>Size</th></tr><tr><th colspan=\"4\"><hr></th></tr>\n");
	tbprintf(&tb, "   <tr><td valign=\"top\"><img src=\"/icons/back.gif\"></td><td><a href=\"..\">Parent directory</a></td><td>&nbsp;</td><td align=\"right\">&nbsp;</td></tr>\n");

	for (i = 0; i < dir_length; i++) {
	    dir_item *di = dir_array[i];
	    switch (di->type) {
		case 0:
		    tbprintf(&tb, "   <tr><td valign=\"top\"><img src=\"/icons/unknown.gif\"></td><td><a href=\"%s\">%s</a></td><td>?</td><td align=\"right\">?</td></tr>\n", di->name, di->name);
		    break;
		case 1:
		    if (type == 2)
			tbprintf(&tb, "   <tr><td valign=\"top\"><img src=\"/icons/text.gif\"></td><td><a href=\"%d\">%s</a></td><td>%s</td><td align=\"right\">%d</td></tr>\n", i, di->name, di->mtime, di->size);
		    else
			tbprintf(&tb, "   <tr><td valign=\"top\"><img src=\"/icons/text.gif\"></td><td><a href=\"%s\">%s</a></td><td>%s</td><td align=\"right\">%d</td></tr>\n", di->name, di->name, di->mtime, di->size);
		    break;
		case 2:
		    tbprintf(&tb, "   <tr><td valign=\"top\"><img src=\"/icons/folder.gif\"></td><td><a href=\"%s\">%s</a></td><td>%s</td><td align=\"right\">-</td></tr>\n", di->name, di->name, di->mtime);
		    break;
	    }
	    if (type == 3)
		free(di->name);
	    free(di);
	}
	free(dir_array);

	tbprintf(&tb, "   <tr><th colspan=\"4\"><hr></th></tr>\n");
	tbprintf(&tb, "   <tr><td colspan=\"4\"><form method=\"post\" enctype=\"multipart/form-data\">\n");
	tbprintf(&tb, "    Upload file:<p>\n");
	tbprintf(&tb, "    <input type=\"file\" name=\"filedata\"><p>\n");
	tbprintf(&tb, "    <input type=\"submit\" value=\"Submit\">\n");
	tbprintf(&tb, "   </form></td></tr>\n");
	tbprintf(&tb, "   <tr><th colspan=\"4\"><hr></th></tr></table>\n");
	tbprintf(&tb, "  <address>Free42 " VERSION " HTTP Server</address>\n");
	tbprintf(&tb, " </body>\n");
	tbprintf(&tb, "</html>\n");

	sockprintf(csock, "HTTP/1.0 200 OK\r\n");
	sockprintf(csock, "Connection: close\r\n");
	sockprintf(csock, "Content-Type: text/html\r\n");
	sockprintf(csock, "Content-Length: %d\r\n", tb.size);
	sockprintf(csock, "\r\n");
	send(csock, tb.buf, tb.size, 0);
	free(tb.buf);
    }
    free((void *) url);
}

// TODO: I could use one textbuffer for import *and* export;
// those two things can't happen at the same time.
// NOTE: We only read from this textbuf, we don't write;
// we use 'capacity' as the read position.

static textbuf import_tb = { NULL, 0, 0 };

int shell_read(char *buf, int nbytes) {
    if (import_tb.buf == NULL || import_tb.capacity >= import_tb.size)
	return -1;
    int bytes_copied = import_tb.size - import_tb.capacity;
    if (nbytes < bytes_copied)
	bytes_copied = nbytes;
    memcpy(buf, import_tb.buf + import_tb.capacity, bytes_copied);
    import_tb.capacity += bytes_copied;
    return bytes_copied;
}

void do_post(int csock, const char *url) {
    char line[LINEBUFSIZE];
    char boundary[LINEBUFSIZE] = "";
    int blen;

    url = canonicalize_url(url);
    if (url == NULL) {
	http_error(csock, 403);
	return;
    }

    while (1) {
	read_line(csock, line, LINEBUFSIZE);
	if (strlen(line) == 0)
	    break;
	if (strncasecmp(line, "Content-Type:", 13) == 0) {
	    char *p = line + 13;
	    char q;
	    while (*p == ' ')
		p++;
	    if (strncmp(p, "multipart/form-data;", 20) != 0) {
		http_error(csock, 415);
		return;
	    }
	    p += 20;
	    while (*p == ' ')
		p++;
	    if (strncmp(p, "boundary=", 9) != 0) {
		http_error(csock, 415);
		return;
	    }
	    p += 9;
	    if (*p == '\'' || *p == '"')
		q = *p++;
	    else
		q = 0;
	    strcpy(boundary, "\r\n--");
	    strcat(boundary, p);
	    p = boundary;
	    while (*p != 0)
		p++;
	    if (q != 0 && p[-1] == q)
		*(--p) = 0;
	    blen = strlen(boundary);
	}
    }

    /* Now we're going to read the request body; this will be
     * delimited using the boundary we found above...
     */
    read_line(csock, line, LINEBUFSIZE);
    if (strlen(line) + 2 != blen || strncmp(line, boundary + 2, blen - 2) != 0) {
	http_error(csock, 415);
	return;
    }

    while (1) {
	/* Loop over message parts */
	char filename[LINEBUFSIZE] = "";
	textbuf tb;
	int bpos;

	while (1) {
	    /* Loop over part headers */
	    read_line(csock, line, LINEBUFSIZE);
	    if (strlen(line) == 0)
		break;
	    if (strncasecmp(line, "Content-Disposition: form-data; name=\"filedata\";", 48) == 0) {
		char *p = strstr(line + 48, "filename=");
		char *p2;
		char q;
		if (p == NULL) {
		    http_error(csock, 415);
		    return;
		}
		p += 9;
		if (*p == '\'' || *p == '"')
		    q = *p++;
		else
		    q = 0;
		p2 = p + strlen(p);
		while (p2 >= p)
		    if (*p2 == '\\' || *p2 == '/') {
			p = p2 + 1;
			break;
		    } else
			p2--;
		strcpy(filename, p);
		if (q != 0 && filename[strlen(filename) - 1] == q)
		    filename[strlen(filename) - 1] = 0;
	    }
	}

	/* Read part body until we find a "\r\n" followed by a boundary string */
	tb.buf = NULL;
	tb.size = 0;
	tb.capacity = 0;
	bpos = 0;

	while (1) {
	    char c;
	    int n = recv(csock, &c, 1, 0);
	    if (n != 1)
		break;
	    if (*filename != 0)
		tbwrite(&tb, &c, 1);
	    if (bpos == blen && (c == '\r' || c == '-'))
		bpos++;
	    else if (bpos == blen + 1 && (c == '\n' || c == '-'))
		bpos++;
	    else if (c == boundary[bpos])
		bpos++;
	    else if (c == boundary[0])
		bpos = 1;
	    else
		bpos = 0;
	    if (bpos == blen + 2) {
		/* Found the body delimiter! */
		if (*filename != 0) {
		    tb.size -= blen + 2;
		    if (strcmp(url, "/memory/") == 0) {
			// Import program straight to memory
			import_tb.buf = tb.buf;
			import_tb.size = tb.size;
			import_tb.capacity = 0;
			// TODO -- error message on failure
			core_import_programs(NULL);
		    } else {
			// Upload to file
			strcpy(line, url + 1);
			strcat(line, filename);
			if (tb.size == 0)
			    // Uploading zero-length file: delete destination
			    unlink(line);
			else {
			    // Uploading nonempty file: create it
			    FILE *f = fopen(line, "w");
			    if (f == NULL) {
				http_error(csock, 403);
				free(tb.buf);
				return;
			    }
			    fwrite(tb.buf, 1, tb.size, f);
			    fclose(f);
			}
		    }
		    free(tb.buf);
		}
		if (*filename != 0 || c == '-')
		    goto done;
		else
		    break;
	    }
	}
    }
    done:

    sockprintf(csock, "HTTP/1.0 302 Moved Temporarily\r\n");
    sockprintf(csock, "Connection: close\r\n");
    sockprintf(csock, "Location: %s\r\n", url);
    sockprintf(csock, "\r\n");
    free((void *) url);
}

static const char *canonicalize_url(const char *url) {
    /* This function:
     * enforces that the url starts with a "/"
     * substitutes "//" => "/"
     * substitutes "/./" => "/"
     * substitutes "/something/../" => "/"
     * substitutes trailing "/." => "/"
     * substitutes trailing "/something/.." => "/"
     * forbids ascending above docroot
     */
    char *ret, *dst;
    const char *src;
    int state;

    if (url[0] != '/')
	return NULL;
    ret = (char *) malloc(strlen(url) + 1);
    src = url;
    dst = ret;
    state = 0;

    while (1) {
	char c = *src++;
	*dst++ = c;
	if (c == 0) {
	    if (state == 2)
		/* Trailing "/." => "/" */
		dst[-2] = 0;
	    else if (state == 3) {
		/* Trailing "/.." */
		if (dst == ret + 4) {
		    /* Attempt to go to the parent of our docroot! */
		    free(ret);
		    return NULL;
		}
		dst -= 5;
		while (*dst != '/')
		    dst--;
		dst[1] = 0;
	    }
	    return ret;
	}
	switch (state) {
	    case 0:
		if (c == '/')
		    state = 1;
		break;
	    case 1:
		if (c == '/')
		    dst--; /* "//" => "/" */
		else
		    state = c == '.' ? 2 : 0;
		break;
	    case 2:
		if (c == '/') {
		    dst -= 2; /* "/./" => "/" */
		    state = 1;
		} else
		    state = c == '.' ? 3 : 0;
		break;
	    case 3:
		if (c == '/') {
		    /* Found "/../"; move back two slashes */
		    if (dst == ret + 4) {
			/* Attempt to go to the parent of our docroot! */
			free(ret);
			return NULL;
		    }
		    dst -= 6;
		    while (*dst != '/')
			dst--;
		    dst++;
		    state = 1;
		} else
		    state = 0;
		break;
	}
    }
}

static textbuf export_tb = { NULL, 0, 0 };

int shell_write(const char *buf, int nbytes) {
    tbwrite(&export_tb, buf, nbytes);
    return 1;
}


/*
 * Returns: an HTTP status code: 200, 403, 404, or 500.
 * 200 means everything is fine; 302 is used for redirects from /dirname to /dirname/;
 * everything else is an error (duh).
 * The return parameters ptr, type, and filesize are only meaningful for status 200:
 * type = 0: built-in icon; *ptr points to icon data; filesize is icon data size;
 * type = 1: regular file; *ptr points to FILE*, filesize is file size;
 * type = 2: fake directory; *ptr points to char**; dir end flagged by NULL
 * type = 3: real directory; *ptr points to DIR*
 * The caller must close the FILE* or DIR* returned when type = 1 or 3.
 */
static int open_item(const char *url, void **ptr, int *type, int *filesize) {
    struct stat statbuf;
    int err;
    int i;

    /* We know that the url starts with '/'; strip it so that
     * it becomes a path relative to the current directory.
     * Of course the current directory should be set to docroot;
     * the -d command line option does this, and if it is
     * unspecified, it is the user's responsibility to run this
     * code after making sure some other way that the current
     * directory is docroot.
     */
    url++;
    if (strlen(url) == 0)
	url = ".";

#ifdef FREE42
    /* We map /memory/ to the simulator's program memory,  */
    if (strcmp(url, "memory") == 0) {
	*type = 2;
	/* We don't bother actually populating anything further,
	 * since the caller, do_get(), is going to ignore the
	 * other return values anyway and send a redirect.
	 */
	return 200;
    }
    if (strcmp(url, "memory/") == 0) {
#define NAMEBUFSIZE 1024
#define MAXPROGS 255
	static char buf[NAMEBUFSIZE];
	static char *names[MAXPROGS + 1];
	int p = 0, i;
	int n = core_list_programs(buf, NAMEBUFSIZE);
	buf[NAMEBUFSIZE - 1] = 0;
	for (i = 0; i < n; i++) {
	    names[i] = buf + p;
	    p += strlen(names[i]) + 1;
	    if (p >= NAMEBUFSIZE - 2 || i == MAXPROGS)
		break;
	}
	names[i] = NULL;
	*type = 2;
	*ptr = names;
	return 200;
    }
    if (strncmp(url, "memory/", 7) == 0) {
	/* We treat the remainder of the URL as a decimal number,
	 * representing a 0-based index into the list of programs.
	 * TODO: In the other versions of Free42, we can always be
	 * sure that core_export_programs() is only called with
	 * valid program indexes, but not here, and that's a bit
	 * of a concern since out-of-range values can cause bad
	 * things to happen. Not destructively bad things per se,
	 * I think, but dereferencing bad pointers and/or writing
	 * insane amounts of data are definite possibilities.
	 */
	int n;
	if (sscanf(url + 7, "%d", &n) != 1)
	    return 404;
	if (n < 0)
	    return 404;
	/* TODO -- this is where the range check would be nice! */
	if (export_tb.buf != NULL)
	    free(export_tb.buf);
	export_tb.buf = NULL;
	export_tb.size = 0;
	export_tb.capacity = 0;
	core_export_programs(1, &n, NULL);
	if (export_tb.size == 0)
	    return 404;
	else {
	    *ptr = export_tb.buf;
	    *type = 0;
	    *filesize = export_tb.size;
	    return 200;
	}
    }
#endif

    /* Look for hard-coded icons from icons.c */
    for (i = 0; i < icon_count; i++) {
	if (strcmp(url, icon_name[i]) == 0) {
	    *ptr = icon_data[i];
	    *type = 0;
	    *filesize = icon_size[i];
	    return 200;
	}
    }

    err = stat(url, &statbuf);
    if (err != 0) {
	err = errno;
	switch (err) {
	    case EACCES: return 403;
	    case EBADF: return 500;
	    case EFAULT: return 500;
	    case ELOOP: return 500;
	    case ENAMETOOLONG: return 404;
	    case ENOENT: return 404;
	    case ENOMEM: return 500;
	    case ENOTDIR: return 404;
	    default: return 500;
	}
    }

    if (S_ISREG(statbuf.st_mode)) {
	*ptr = fopen(url, "r");
	*type = 1;
	*filesize = statbuf.st_size;
	if (*ptr == NULL) {
	    /* We already know the file exists and is reachable, so
	     * we only check for EACCES; any other error is reported
	     * as an internal server error (500).
	     */
	    err = errno;
	    return err == EACCES ? 403 : 500;
	}
	return 200;
    } else if (S_ISDIR(statbuf.st_mode)) {
	*ptr = opendir(url);
	*type = 3;
	if (*ptr == NULL) {
	    /* We already know the file exists and is reachable, so
	     * we only check for EACCES; any other error is reported
	     * as an internal server error (500).
	     */
	    err = errno;
	    return err == EACCES ? 403 : 500;
	}
	return 200;
    } else
	return 403;
}

typedef struct {
    const char *ext;
    const char *mime;
} mime_rec;

static mime_rec mime_list[] = {
    { "gif", "image/gif" },
    { "jpg", "image/jpeg" },
    { "png", "image/png" },
    { "ico", "image/vnd.microsoft.icon" },
    { "txt", "text/plain" },
    { "htm", "text/html" },
    { "html", "text/html" },
    { "raw", "application/octet-stream" },
    { NULL, "application/octet-stream" }
};

static const char *get_mime(const char *filename) {
    int i = 0;
    int filenamelen = strlen(filename);
    int extlen;
    while (1) {
	mime_rec *mr = &mime_list[i++];
	if (mr->ext == NULL)
	    return mr->mime;
	extlen = strlen(mr->ext);
	if (filenamelen >= extlen && strcmp(filename + filenamelen - extlen, mr->ext) == 0)
	    return mr->mime;
    }
}

static void http_error(int csock, int err) {
    const char *msg;
    switch (err) {
	case 200: msg = "OK"; break;
	case 403: msg = "Forbidden"; break;
	case 404: msg = "Not Found"; break;
	case 415: msg = "Unsupported Media Type"; break;
	case 500: msg = "Internal Server Error"; break;
	case 501: msg = "Not Implemented"; break;
	default: msg = "Internal Server Error"; break;
    }
    sockprintf(csock, "HTTP/1.0 %d %s\r\n", err, msg);
    sockprintf(csock, "Connection: close\r\n");
    sockprintf(csock, "\r\n");
    /* TODO: Descriptive response body */
    sockprintf(csock, "<h1>%d %s</h1>\r\n", err, msg);
    sockprintf(csock, "This error page is under construction.\r\n");
}

#ifdef STANDALONE
int main(int argc, char *argv[]) {
    int i;
    int port = 9090;
    int backlog = 32;
    int ssock, csock;
    struct sockaddr_in sa, ca;
    int err;

    for (i = 1; i < argc; i++) {
	if (strcmp(argv[i], "-p") == 0) {
	    if (i == argc - 1 || sscanf(argv[i + 1], "%d", &port) != 1) {
		fprintf(stderr, "Can't parse port number \"%s\"\n", argv[i + 1]);
		return 1;
	    }
	    i++;
	} else if (strcmp(argv[i], "-b") == 0) {
	    if (i == argc - 1 || sscanf(argv[i + 1], "%d", &backlog) != 1) {
		fprintf(stderr, "Can't parse backlog number \"%s\"\n", argv[i + 1]);
		return 1;
	    }
	    i++;
	} else if (strcmp(argv[i], "-d") == 0) {
	    if (i == argc - 1)
		err = -1;
	    else
		err = chdir(argv[i + 1]);
	    if (err != 0) {
		fprintf(stderr, "Can't chdir to docroot \"%s\"\n", argv[i + 1]);
		return 1;
	    }
	    i++;
	} else {
	    fprintf(stderr, "Unrecognized option: \"%s\"\n", argv[i]);
	    return 1;
	}
    }

    ssock = socket(AF_INET, SOCK_STREAM, 0);
    if (ssock == -1) {
	err = errno;
	fprintf(stderr, "Could not create socket: %s (%d)\n", strerror(err), err);
	return 1;
    }

    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);
    sa.sin_addr.s_addr = INADDR_ANY;
    err = bind(ssock, (struct sockaddr *) &sa, sizeof(sa));
    if (err != 0) {
	err = errno;
	fprintf(stderr, "Could not bind socket to port %d: %s (%d)\n", port, strerror(err), err);
	return 1;
    }

    err = listen(ssock, backlog);
    if (err != 0) {
	err = errno;
	fprintf(stderr, "Could not listen (backlog = %d): %s (%d)\n", backlog, strerror(err), err);
	return 1;
    }

    while (1) {
	unsigned int n = sizeof(ca);
	char cname[256];
	csock = accept(ssock, (struct sockaddr *) &ca, &n);
	if (csock == -1) {
	    err = errno;
	    fprintf(stderr, "Could not accept connection from client: %s (%d)\n", strerror(err), err);
	    return 1;
	}
	inet_ntop(AF_INET, &ca.sin_addr, cname, sizeof(cname));
	/*fprintf(stderr, "Accepted connection from %s\n", cname);*/
	handle_client(csock);
    }
    
    return 0;
}
#endif
