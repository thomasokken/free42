/*****************************************************************************
 * Free42 -- an HP-42S calculator simulator
 * Copyright (C) 2004-2020  Thomas Okken
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
#include <utime.h>
#include <pthread.h>
#include <limits.h>

#include "minizip_zip.h"
#include "minizip_unzip.h"

#ifdef FREE42

#include "../../common/core_main.h"
#include "../../common/core_globals.h"

const char *get_version();
char *make_temp_file();
static pthread_mutex_t shell_mutex = PTHREAD_MUTEX_INITIALIZER;

#else

static char *make_temp_file() {
    char *path = (char *) malloc(19);
    int fd;
    strcpy(path, "/tmp/Free42.XXXXXX");
    fd = mkstemp(path);
    if (fd == -1) {
        free(path);
        return NULL;
    } else {
        close(fd);
        return path;
    }
}

#endif

/* TODO:
 * The code is currently not thread-safe; it uses static
 * buffers. That should be fixed before it can be considered ready
 * for embedding in a multithreaded environment (i.e., iPhone).
 */

typedef struct {
    char *buf;
    ssize_t size;
    ssize_t capacity;
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

#ifndef FREE42
static
#endif
void errprintf(const char *fmt, ...);

typedef struct cleaner_base {
    void (*doit)(struct cleaner_base *This);
} cleaner_base;

static void sockprintf(int sock, const char *fmt, ...);
static void tbwrite(textbuf *tb, const char *data, ssize_t size);
static void tbprintf(textbuf *tb, const char *fmt, ...);
static void do_get(int csock, const char *url);
static void do_post(int csock, const char *url);
static const char *canonicalize_url(const char *url);
static int open_item(const char *url, void **ptr, int *type, int *filesize, const char **real_name, cleaner_base **cleaner);
static const char *get_mime(const char *ext);
static void http_error(int csock, int err);

static void read_line(int csock, char *buf, int bufsize) {
    int p = 0;
    int afterCR = 0;
    while (1) {
        ssize_t n = recv(csock, buf + p, 1, 0);
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
        errprintf("Memory allocation failure while allocating line buffer\n");
        shutdown(csock, SHUT_WR);
        return;
    }

    read_line(csock, req, LINEBUFSIZE);
    errprintf("%s\n", req);

    url = strchr(req, ' ');
    if (url == NULL) {
        errprintf("Malformed HTTP request: \"%s\"\n", req);
        goto finish;
    }

    protocol = strchr(url + 1, ' ');
    if (protocol == NULL) {
        errprintf("Malformed HTTP request: \"%s\"\n", req);
        goto finish;
    }

    *url++ = 0;
    *protocol++ = 0;
    if (strncmp(protocol, "HTTP/", 5) != 0) {
        errprintf("Unsupported protocol: \"%s\"\n", protocol);
        goto finish;
    }

    if (strcmp(req, "GET") == 0) {
        do_get(csock, url);
        //errprintf("GET %s DONE\n", url);
    } else if (strcmp(req, "POST") == 0) {
        do_post(csock, url);
        //errprintf("POST %s DONE\n", url);
    } else
        errprintf("Unsupported method: \"%s\"\n", req);

    finish:
    shutdown(csock, SHUT_WR);
    while (recv(csock, req, LINEBUFSIZE, 0) > 0);
    close(csock);
    free(req);
    return;
}

#ifndef FREE42
static void errprintf(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
}
#endif

static void sockprintf(int sock, const char *fmt, ...) {
    va_list ap;
    char text[LINEBUFSIZE];
    ssize_t sent;
    int err;
    va_start(ap, fmt);
    vsprintf(text, fmt, ap);
    sent = send(sock, text, strlen(text), 0);
    err = errno;
    if (sent != strlen(text))
        errprintf("send() only sent %d out of %d bytes: %s (%d)\n", sent, strlen(text),
                strerror(err), err);
    va_end(ap);
}

static void tbwrite(textbuf *tb, const char *data, ssize_t size) {
    if (tb->size + size > tb->capacity) {
        ssize_t newcapacity = tb->capacity == 0 ? 1024 : (tb->capacity << 1);
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
    size_t size;
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
    size_t n;

    url = canonicalize_url(url);
    if (url == NULL) {
        http_error(csock, 403);
        return;
    }

    const char *real_name = NULL;
    cleaner_base *cleaner = NULL;
    err = open_item(url, &ptr, &type, &filesize, &real_name, &cleaner);
    if (err != 200) {
        free((void *) url);
        http_error(csock, err);
        return;
    }

    if (type == 0 || type == 1) {
        sockprintf(csock, "HTTP/1.1 200 OK\r\n");
        sockprintf(csock, "Connection: close\r\n");
        sockprintf(csock, "Content-Type: %s\r\n", get_mime(url));
        sockprintf(csock, "Content-Length: %d\r\n", filesize);
        if (real_name != NULL)
            sockprintf(csock, "Content-Disposition: attachment; filename=%s.raw\r\n", real_name);
        sockprintf(csock, "\r\n");
        if (type == 0) {
            send(csock, ptr, filesize, 0);
        } else {
            FILE *file = (FILE *) ptr;
            while ((n = fread(buf, 1, LINEBUFSIZE, file)) > 0)
                send(csock, buf, n, 0);
        }
    } else if (url[strlen(url) - 1] != '/') {
        sockprintf(csock, "HTTP/1.1 302 Moved Temporarily\r\n");
        sockprintf(csock, "Connection: close\r\n");
        sockprintf(csock, "Location: %s/\r\n", url);
        sockprintf(csock, "\r\n");
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
                        strcpy(di->mtime, "-");
                        found = 2;
                        break;
                    }
                    di = di->next;
                }
                if (!found) {
                    di = (dir_item *) malloc(sizeof(dir_item));
                    di->name = (char *) malloc(7);
                    strcpy(di->name, "memory");
                    strcpy(di->mtime, "-");
                    di->type = 2;
                    di->next = dir_list;
                    dir_list = di;
                    dir_length++;
                }
            }
        } else {
            /* type == 2: fake directory for /memory */
            char **name = (char **) ptr;
            dir_item *dir_tail = NULL;
            int prgm_index = 0;
            while (*name != NULL) {
                dir_item *di = (dir_item *) malloc(sizeof(dir_item));
                di->name = *name;
                di->size = core_program_size(prgm_index++) + 3;
                di->type = 1;
                strcpy(di->mtime, "-");
                di->next = NULL;
                if (dir_list == NULL)
                    dir_list = di;
                else
                    dir_tail->next = di;
                dir_tail = di;
                name++;
                dir_length++;
            }
#endif
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
        tbprintf(&tb, "  <script type=\"text/javascript\">\n");
        tbprintf(&tb, "    function itemsSelected(name) {\n");
        tbprintf(&tb, "        var count = 0\n");
        tbprintf(&tb, "        var items = document.getElementsByName(name);\n");
        tbprintf(&tb, "        for (var i = 0; i < items.length; i++)\n");
        tbprintf(&tb, "            if (items[i].checked)\n");
        tbprintf(&tb, "                count++;\n");
        tbprintf(&tb, "        return count;\n");
        tbprintf(&tb, "    }\n");
        tbprintf(&tb, "    function selectAll() {\n");
        tbprintf(&tb, "        var selAll = document.getElementsByName(\"selAll\")[0];\n");
        tbprintf(&tb, "        var fns = document.getElementsByName(\"fn\");\n");
        tbprintf(&tb, "        for (var i = 0; i < fns.length; i++)\n");
        tbprintf(&tb, "            fns[i].checked = selAll.checked;\n");
        tbprintf(&tb, "        var dns = document.getElementsByName(\"dn\");\n");
        tbprintf(&tb, "        for (var i = 0; i < dns.length; i++)\n");
        tbprintf(&tb, "            dns[i].checked = selAll.checked;\n");
        tbprintf(&tb, "    }\n");
        tbprintf(&tb, "    function selectOne() {\n");
        tbprintf(&tb, "        var everythingSelected = true;\n");
        tbprintf(&tb, "        var fns = document.getElementsByName(\"fn\");\n");
        tbprintf(&tb, "        for (var i = 0; i < fns.length; i++)\n");
        tbprintf(&tb, "            if (!fns[i].checked) {\n");
        tbprintf(&tb, "                everythingSelected = false;\n");
        tbprintf(&tb, "                break;\n");
        tbprintf(&tb, "            }\n");
        tbprintf(&tb, "        if (everythingSelected) {\n");
        tbprintf(&tb, "            var dns = document.getElementsByName(\"dn\");\n");
        tbprintf(&tb, "            for (var i = 0; i < dns.length; i++)\n");
        tbprintf(&tb, "                if (!dns[i].checked) {\n");
        tbprintf(&tb, "                    everythingSelected = false;\n");
        tbprintf(&tb, "                    break;\n");
        tbprintf(&tb, "                }\n");
        tbprintf(&tb, "        }\n");
        tbprintf(&tb, "        var selAll = document.getElementsByName(\"selAll\")[0]\n");
        tbprintf(&tb, "        selAll.checked = everythingSelected;\n");
        tbprintf(&tb, "    }\n");
        tbprintf(&tb, "    function deselectAll() {\n");
        tbprintf(&tb, "        var fns = document.getElementsByName(\"fn\");\n");
        tbprintf(&tb, "        for (var i = 0; i < fns.length; i++)\n");
        tbprintf(&tb, "            fns[i].checked = false;\n");
        tbprintf(&tb, "        var dns = document.getElementsByName(\"dn\");\n");
        tbprintf(&tb, "        for (var i = 0; i < dns.length; i++)\n");
        tbprintf(&tb, "            dns[i].checked = false;\n");
        tbprintf(&tb, "    }\n");
        tbprintf(&tb, "    function clearFile() {\n");
        tbprintf(&tb, "        var file = document.getElementsByName(\"filedata\")[0];\n");
        tbprintf(&tb, "        file.value = \"\";\n");
        tbprintf(&tb, "    }\n");
#ifdef FREE42
        if (strcmp(url, "/memory/") != 0) {
#endif
            tbprintf(&tb, "    function clearDir() {\n");
            tbprintf(&tb, "        var newDir = document.getElementsByName(\"newDir\")[0];\n");
            tbprintf(&tb, "        newDir.value = \"\";\n");
            tbprintf(&tb, "    }\n");
#ifdef FREE42
        }
#endif
        tbprintf(&tb, "    function doUpload() {\n");
        tbprintf(&tb, "        var file = document.getElementsByName(\"filedata\")[0].value;\n");
        tbprintf(&tb, "        if (file == \"\" || file == null) {\n");
        tbprintf(&tb, "            alert(\"You haven't selected a file to upload.\");\n");
        tbprintf(&tb, "            return;\n");
        tbprintf(&tb, "        }\n");
#ifdef FREE42
        if (strcmp(url, "/memory/") == 0) {
            tbprintf(&tb, "        file = file.toLowerCase();\n");
            tbprintf(&tb, "        var p = file.length - 4;\n");
            tbprintf(&tb, "        if (p < 0 || (file.substr(p, 4) != \".raw\" && file.substr(p, 4) != \".zip\")) {\n");
            tbprintf(&tb, "            alert(\"You can only upload *.raw or *.zip files to /memory/.\");\n");
            tbprintf(&tb, "            return;\n");
            tbprintf(&tb, "        }\n");
        }
#endif
        tbprintf(&tb, "        deselectAll();\n");
#ifdef FREE42
        if (strcmp(url, "/memory/") != 0)
#endif
            tbprintf(&tb, "        clearDir();\n");
        tbprintf(&tb, "        document.forms[0].submit();\n");
        tbprintf(&tb, "    }\n");
        tbprintf(&tb, "    function doDownload() {\n");
        tbprintf(&tb, "        var nfiles = itemsSelected(\"fn\")\n");
        tbprintf(&tb, "        var ndirs = itemsSelected(\"dn\")\n");
        tbprintf(&tb, "        if (nfiles == 0 && ndirs == 0) {\n");
        tbprintf(&tb, "            alert(\"You haven't selected anything to download.\");\n");
        tbprintf(&tb, "            return;\n");
        tbprintf(&tb, "        }\n");
#ifdef FREE42
        if (strcmp(url, "/memory/") != 0)
#endif
            tbprintf(&tb, "        clearDir();\n");
        tbprintf(&tb, "        clearFile();\n");
        tbprintf(&tb, "        document.forms[0].what.value = \"download\";\n");
        tbprintf(&tb, "        document.forms[0].submit();\n");
        tbprintf(&tb, "    }\n");
        tbprintf(&tb, "    function doDelete() {\n");
#ifdef FREE42
        if (strcmp(url, "/") == 0) {
            tbprintf(&tb, "        var dirChecks = document.getElementsByName(\"dn\");\n");
            tbprintf(&tb, "        for (var i = 0; i < dirChecks.length; i++) {\n");
            tbprintf(&tb, "            var dirCheck = dirChecks[i];\n");
            tbprintf(&tb, "            if (dirCheck.checked && dirCheck.value == \"memory\") {\n");
            tbprintf(&tb, "                alert(\"You can't delete /memory/.\");\n");
            tbprintf(&tb, "                return;\n");
            tbprintf(&tb, "            }\n");
            tbprintf(&tb, "        }\n");
        }
#endif
        tbprintf(&tb, "        var nfiles = itemsSelected(\"fn\")\n");
        tbprintf(&tb, "        var ndirs = itemsSelected(\"dn\")\n");
        tbprintf(&tb, "        if (nfiles == 0 && ndirs == 0) {\n");
        tbprintf(&tb, "            alert(\"You haven't selected anything to delete.\");\n");
        tbprintf(&tb, "            return;\n");
        tbprintf(&tb, "        }\n");
        tbprintf(&tb, "        var prompt = \"Are you sure you want to delete\";\n");
        tbprintf(&tb, "        if (nfiles != 0)\n");
        tbprintf(&tb, "            prompt += \" \" + nfiles + \" \" + (nfiles == 1 ? \"file\" : \"files\");\n");
        tbprintf(&tb, "        if (nfiles != 0 && ndirs != 0)\n");
        tbprintf(&tb, "            prompt += \" and\";\n");
        tbprintf(&tb, "        if (ndirs != 0)\n");
        tbprintf(&tb, "            prompt += \" \" + ndirs + \" \" + (ndirs == 1 ? \"directory\" : \"directories\");\n");
        tbprintf(&tb, "        prompt += \"?\";\n");
        tbprintf(&tb, "        if (confirm(prompt)) {\n");
#ifdef FREE42
        if (strcmp(url, "/memory/") != 0)
#endif
            tbprintf(&tb, "            clearDir();\n");
        tbprintf(&tb, "            clearFile();\n");
        tbprintf(&tb, "            document.forms[0].what.value = \"delete\";\n");
        tbprintf(&tb, "            document.forms[0].submit();\n");
        tbprintf(&tb, "        }\n");
        tbprintf(&tb, "    }\n");
        tbprintf(&tb, "    function doMkdir() {\n");
        tbprintf(&tb, "        var newDir = document.getElementsByName(\"newDir\")[0];\n");
        tbprintf(&tb, "        if (newDir.value == \"\")\n");
        tbprintf(&tb, "            alert(\"You haven't entered a name for the new directory.\");\n");
        tbprintf(&tb, "        else {\n");
        tbprintf(&tb, "            deselectAll();\n");
        tbprintf(&tb, "            clearFile();\n");
        tbprintf(&tb, "            document.forms[0].submit();\n");
        tbprintf(&tb, "        }\n");
        tbprintf(&tb, "    }\n");
        tbprintf(&tb, "  </script>\n");
        tbprintf(&tb, " </head>\n");
        tbprintf(&tb, " <body>\n");
        tbprintf(&tb, "  <h1>Index of %s</h1>\n", url);
        tbprintf(&tb, "  <form method=\"post\" enctype=\"multipart/form-data\"><input type=\"hidden\" name=\"what\" value=\"download\"><table><tr><td valign=\"top\"><img src=\"/icons/blank.gif\"></td><td><b>Name</b></td><td><b>Last modified</b></td><td align=\"right\"><b>Size</b></td><td><input type=\"checkbox\" name=\"selAll\" value=\"foo\" onclick=\"selectAll()\"></td></tr><tr><th colspan=\"5\"><hr></th></tr>\n");
        tbprintf(&tb, "   <tr><td valign=\"top\"><img src=\"/icons/back.gif\"></td><td><a href=\"..\">Parent directory</a></td><td>&nbsp;</td><td align=\"right\">&nbsp;</td><td>&nbsp;</td></tr>\n");

        for (i = 0; i < dir_length; i++) {
            dir_item *di = dir_array[i];
            switch (di->type) {
                case 0:
                    tbprintf(&tb, "   <tr><td valign=\"top\"><img src=\"/icons/unknown.gif\"></td><td><a href=\"%s\">%s</a></td><td>?</td><td align=\"right\">?</td><td>&nbsp</td></tr>\n", di->name, di->name);
                    break;
                case 1:
                    if (type == 2)
                        tbprintf(&tb, "   <tr><td valign=\"top\"><img src=\"/icons/text.gif\"></td><td><a href=\"%d\">%s</a></td><td>%s</td><td align=\"right\">%d</td><td><input type=\"checkbox\" name=\"fn\" value=\"%d\" onclick=\"selectOne()\"></td></tr>\n", i, di->name, di->mtime, di->size, i);
                    else
                        tbprintf(&tb, "   <tr><td valign=\"top\"><img src=\"/icons/text.gif\"></td><td><a href=\"%s\">%s</a></td><td>%s</td><td align=\"right\">%d</td><td><input type=\"checkbox\" name=\"fn\" value=\"%s\" onclick=\"selectOne()\"></td></tr>\n", di->name, di->name, di->mtime, di->size, di->name);
                    break;
                case 2:
                    tbprintf(&tb, "   <tr><td valign=\"top\"><img src=\"/icons/folder.gif\"></td><td><a href=\"%s/\">%s</a></td><td>%s</td><td align=\"right\">-</td><td><input type=\"checkbox\" name=\"dn\" value=\"%s\" onclick=\"selectOne()\"></td></tr>\n", di->name, di->name, di->mtime, di->name);
                    break;
            }
            if (type == 3)
                free(di->name);
            free(di);
        }
        free(dir_array);

        tbprintf(&tb, "   <tr><th colspan=\"5\"><hr></th></tr>\n");
        tbprintf(&tb, "   <tr><td colspan=\"5\"><table>\n");
        tbprintf(&tb, "    <tr><td align=\"right\">Upload file:</td><td><input type=\"file\" name=\"filedata\">&nbsp;<input type=\"button\" value=\"Upload\" onclick=\"doUpload()\"></td></tr>\n");
        if (strncmp(url, "/memory/", 8) != 0)
            tbprintf(&tb, "    <tr><td align=\"right\">New directory:</td><td><input type=\"text\" name=\"newDir\">&nbsp;<input type=\"button\" value=\"Create\" onclick=\"doMkdir()\"></td></tr>");
        tbprintf(&tb, "    <tr><td align=\"right\">Selected items:</td><td><input type=\"button\" value=\"Download\" onclick=\"doDownload()\">&nbsp;<input type=\"button\" value=\"Delete\" onclick=\"doDelete()\"></td></tr>");
        tbprintf(&tb, "   </table></td></tr>\n");
        tbprintf(&tb, "   <tr><th colspan=\"5\"><hr></th></tr></table></form>\n");
#ifdef FREE42
        tbprintf(&tb, "  <address>Free42 %s HTTP Server</address>\n", get_version());
#else
        tbprintf(&tb, "  <address>Thomas Okken's Simple Server</address>\n");
#endif
        tbprintf(&tb, " </body>\n");
        tbprintf(&tb, "</html>\n");

        sockprintf(csock, "HTTP/1.1 200 OK\r\n");
        sockprintf(csock, "Connection: close\r\n");
        sockprintf(csock, "Content-Type: text/html; charset=utf-8\r\n");
        sockprintf(csock, "Content-Length: %d\r\n", tb.size);
        sockprintf(csock, "\r\n");
        send(csock, tb.buf, tb.size, 0);
        free(tb.buf);
    }
    free((void *) url);
    if (cleaner != NULL)
        cleaner->doit(cleaner);
}

static int recursive_remove(const char *path) {
    // We assume that 'path' refers to a directory
    struct dirent *d;
    struct stat statbuf;
    DIR *dir = opendir(path);
    if (dir == NULL)
        return 0;
    while ((d = readdir(dir)) != NULL) {
        if (strcmp(d->d_name, ".") == 0 || strcmp(d->d_name, "..") == 0)
            continue;
        char *newpath = (char *) malloc(strlen(path) + strlen(d->d_name) + 2);
        strcpy(newpath, path);
        strcat(newpath, "/");
        strcat(newpath, d->d_name);
        int success;
        if (stat(newpath, &statbuf) == 0)
            if (S_ISDIR(statbuf.st_mode))
                success = recursive_remove(newpath);
            else
                success = remove(newpath) == 0;
        else
            success = 0;
        if (!success) {
            closedir(dir);
            free(newpath);
            return 0;
        }
        free(newpath);
    }
    closedir(dir);
    return remove(path) == 0;
}

#ifdef FREE42

typedef struct prgm_name_list {
    char *name;
    struct prgm_name_list *next;
} prgm_name_list;

static void prgm_name_list_clear(prgm_name_list *n) {
    while (n != NULL) {
        prgm_name_list *n2 = n->next;
        if (n->name != NULL)
            free(n->name);
        free(n);
        n = n2;
    }
}

static char *prgm_name_list_find(prgm_name_list *n, const char *name) {
    prgm_name_list *head = n;
    while (n != NULL) {
        if (n->name != NULL && strcmp(n->name, name) == 0)
            return NULL;
        n = n->next;
    }
    prgm_name_list *n2 = (prgm_name_list *) malloc(sizeof(prgm_name_list));
    n2->next = head->next;
    n2->name = strdup(name);
    head->next = n2;
    return n2->name;
}

static char *prgm_name_list_make_unique(prgm_name_list *n, const char *name) {
    char *name2 = prgm_name_list_find(n, name);
    if (name2 != NULL)
        return name2;
    int i = 0;
    char tmpname[12];
    while (1) {
        sprintf(tmpname, "%s.%d", name, i);
        name2 = prgm_name_list_find(n, tmpname);
        if (name2 != NULL)
            return name2;
        i++;
    }
}

// NOTE call this only when shell_mutex is locked
static char *prgm_index_to_name(int prgm_index) {
    char *buf = core_list_programs();
    if (buf == NULL)
        return NULL;
    int n = ((buf[0] & 255) << 24) | ((buf[1] & 255) << 16) | ((buf[2] & 255) << 8) | (buf[3] & 255);
    if (prgm_index < 0 || prgm_index >= n) {
        free(buf);
        return NULL;
    }
    char *p = buf + 4;
    while (prgm_index > 0) {
        p += strlen(p) + 1;
        prgm_index--;
    }
    if (*p != '"') {
        // END or .END.
        free(buf);
        return strdup("END");
    }
    char *q = strchr(p + 1, '"');
    intptr_t len = q - p - 1;
    char *name = (char *) malloc(len + 1);
    memcpy(name, p + 1, len);
    name[len] = 0;
    free(buf);
    return name;
}

static int zip_program_2(int prgm_index, int is_all, zipFile z, char *buf, int bufsize, prgm_name_list *namelist) {
    char *name = prgm_index_to_name(prgm_index);
    if (name == NULL)
        return 0;
    ssize_t psize = core_program_size(prgm_index) + 3;
    char *pbuf = (char *) malloc(psize);
    if (pbuf == NULL) {
        free(name);
        return 0;
    }

    char bufparam[21];
    strcpy(bufparam, "mem:");
    memcpy(bufparam + 5, &pbuf, sizeof(char *));
    memcpy(bufparam + 13, &psize, sizeof(ssize_t));
    core_export_programs(1, &prgm_index, bufparam);
    char *name2 = prgm_name_list_make_unique(namelist, name);
    time_t t = time(NULL);
    struct tm stm;
    localtime_r(&t, &stm);
    zip_fileinfo zfi;
    zfi.tmz_date.tm_sec = stm.tm_sec;
    zfi.tmz_date.tm_min = stm.tm_min;
    zfi.tmz_date.tm_hour = stm.tm_hour;
    zfi.tmz_date.tm_mday = stm.tm_mday;
    zfi.tmz_date.tm_mon = stm.tm_mon;
    zfi.tmz_date.tm_year = stm.tm_year + 1900;
    zfi.dosDate = 0;
    zfi.internal_fa = 0;
    zfi.external_fa = 0;
    char *name3 = (char *) malloc(strlen(name2) + 12);
    strcpy(name3, is_all ? "memory/" : "");
    strcat(name3, name2);
    strcat(name3, ".raw");
    /* int ret = */ zipOpenNewFileInZip(z, name3, &zfi, NULL, 0, NULL, 0, NULL, Z_DEFLATED, Z_DEFAULT_COMPRESSION);
    // TODO: How to deal with the return value? zip.h doesn't say.
    zipWriteInFileInZip(z, pbuf, (unsigned int) psize);
    zipCloseFileInZip(z);
    free(name3);
    free(pbuf);
    free(name);
    return 1;
}

static void zip_program(int prgm_index, zipFile z, char *buf, int bufsize, prgm_name_list *namelist) {
    pthread_mutex_lock(&shell_mutex);
    if (prgm_index == -1)
        while (zip_program_2(++prgm_index, 1, z, buf, bufsize, namelist));
    else
        zip_program_2(prgm_index, 0, z, buf, bufsize, namelist);
    pthread_mutex_unlock(&shell_mutex);
}

#endif
    
static void zip_one_file(const char *file, const char *path, zipFile z, char *buf, int bufsize) {
    zip_fileinfo zfi;
    struct stat st;
    FILE *f;
    struct tm stm;
    int ret;
    ssize_t n;

    ret = stat(file, &st);
    if (ret != 0 || !S_ISREG(st.st_mode))
        return;
    f = fopen(file, "r");
    if (f == NULL)
        return;

    localtime_r(&st.st_mtime, &stm);
    zfi.tmz_date.tm_sec = stm.tm_sec;
    zfi.tmz_date.tm_min = stm.tm_min;
    zfi.tmz_date.tm_hour = stm.tm_hour;
    zfi.tmz_date.tm_mday = stm.tm_mday;
    zfi.tmz_date.tm_mon = stm.tm_mon;
    zfi.tmz_date.tm_year = stm.tm_year + 1900;
    zfi.dosDate = 0;
    zfi.internal_fa = 0;
    zfi.external_fa = 0;

    ret = zipOpenNewFileInZip(z, path, &zfi, NULL, 0, NULL, 0, NULL, Z_DEFLATED, Z_DEFAULT_COMPRESSION);
    // TODO: How to deal with the return value? zip.h doesn't say.
    while ((n = fread(buf, 1, bufsize, f)) > 0)
        zipWriteInFileInZip(z, buf, (unsigned int) n);
    fclose(f);
    zipCloseFileInZip(z);
}

static void recursive_zip(const char *base_path, const char *zip_path, zipFile z, char *buf, int bufsize) {
    char *full_path, *new_path;
    DIR *dir;
    struct dirent *d;
    
    full_path = (char *) malloc(strlen(base_path) + strlen(zip_path) + 2);
    strcpy(full_path, base_path);
    if (strlen(base_path) > 0)
        strcat(full_path, "/");
    strcat(full_path, zip_path);
    dir = opendir(full_path);
    while ((d = readdir(dir)) != NULL) {
        struct stat st;
        if (strcmp(d->d_name, ".") == 0 || strcmp(d->d_name, "..") == 0)
            continue;
        new_path = (char *) malloc(strlen(full_path) + strlen(d->d_name) + 2);
        strcpy(new_path, full_path);
        strcat(new_path, "/");
        strcat(new_path, d->d_name);
        if (stat(new_path, &st) == 0) {
            char *new_zip_path = (char *) malloc(strlen(zip_path) + strlen(d->d_name) + 2);
            strcpy(new_zip_path, zip_path);
            strcat(new_zip_path, "/");
            strcat(new_zip_path, d->d_name);
            if (S_ISDIR(st.st_mode))
                recursive_zip(base_path, new_zip_path, z, buf, bufsize);
            else if (S_ISREG(st.st_mode))
                zip_one_file(new_path, new_zip_path, z, buf, bufsize);
            free(new_zip_path);
        }
        free(new_path);
    }
    closedir(dir);
    free(full_path);
}

void do_post(int csock, const char *url) {
    char line[LINEBUFSIZE];
    char boundary[LINEBUFSIZE] = "";
    size_t blen;

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
                http_error(csock, 400);
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
        http_error(csock, 400);
        return;
    }

    int what = 1; // 1=download 2=delete
    zipFile z = NULL;
    char *zip_name;
    char buf[8192];

#ifdef FREE42
    // for deleting programs
    int program_index_offset = 0;
    // for preventing name clashes when exporting programs to zip file
    prgm_name_list *namelist = (prgm_name_list *) malloc(sizeof(prgm_name_list));
    namelist->name = NULL;
    namelist->next = NULL;
#endif

    while (1) {
        /* Loop over message parts */
        char filename[LINEBUFSIZE] = "";
        // action values:
        // 0 = upload
        // 1 = download or delete file
        // 2 = download or delete dir
        // 3 = mkdir
        // 4 = parse "what"
        int action;
        textbuf tb;
        int bpos;

        while (1) {
            /* Loop over part headers */
            read_line(csock, line, LINEBUFSIZE);
            if (strlen(line) == 0)
                break;
            if (strncasecmp(line, "Content-Disposition: form-data; name=\"", 38) == 0) {
                char *p, *p2, q;
                p = line + 38;
                p2 = strchr(p, '"');
                if (p2 == NULL) {
#ifdef FREE42
                    prgm_name_list_clear(namelist);
#endif
                    http_error(csock, 400);
                    return;
                }
                *p2 = 0;
                if (strcmp(p, "fn") == 0)
                    action = 1;
                else if (strcmp(p, "dn") == 0)
                    action = 2;
                else if (strcmp(p, "newDir") == 0)
                    action = 3;
                else if (strcmp(p, "what") == 0)
                    action = 4;
                else if (strcmp(p, "filedata") == 0) {
                    action = 0;
                    p = strstr(p2 + 1, "filename=");
                    if (p == NULL) {
#ifdef FREE42
                        prgm_name_list_clear(namelist);
#endif
                        http_error(csock, 400);
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
                } else
                    action = -1;
            }
        }

        /* Read part body until we find a "\r\n" followed by a boundary string */
        tb.buf = NULL;
        tb.size = 0;
        tb.capacity = 0;
        bpos = 0;

        while (1) {
            char c;
            ssize_t n = recv(csock, &c, 1, 0);
            if (n != 1)
                break;
            if (*filename != 0 || action != 0)
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
                if (*filename != 0 || action != 0) {
                    tb.size -= blen + 2;
                    if (action == 0) {
                        size_t fnlen = strlen(filename);
                        if (fnlen < 4 || strcasecmp(filename + fnlen - 4, ".zip") != 0) {
                            // Upload to file
#ifdef FREE42
                            if (strcmp(url, "/memory/") == 0) {
                                if (fnlen < 4 || strcasecmp(filename + fnlen - 4, ".raw") != 0) {
                                    // No need to be very specific with the error message here; the
                                    // web interface has JS code to catch this situation and prevent
                                    // the form from even being submitted.
                                    prgm_name_list_clear(namelist);
                                    http_error(csock, 403);
                                    return;
                                }
                                // Import program straight to memory
                                pthread_mutex_lock(&shell_mutex);
                                char bufparam[21];
                                strcpy(bufparam, "mem:");
                                memcpy(bufparam + 5, &tb.buf, sizeof(char *));
                                memcpy(bufparam + 13, &tb.size, sizeof(ssize_t));
                                core_import_programs(0, bufparam);
                                // TODO -- error message on failure
                                pthread_mutex_unlock(&shell_mutex);
                            } else {
#endif
                                strcpy(line, url + 1);
                                strcat(line, filename);
                                FILE *f = fopen(line, "w");
                                if (f == NULL) {
                                    free(tb.buf);
#ifdef FREE42
                                    prgm_name_list_clear(namelist);
#endif
                                    http_error(csock, 403);
                                    return;
                                }
                                fwrite(tb.buf, 1, tb.size, f);
                                fclose(f);
#ifdef FREE42
                            }
#endif
                        } else {
                            // Uploading zip file: unpack it on the fly
                            char *fn = make_temp_file();
                            FILE *f = fopen(fn, "w");
                            if (f == NULL)
                                goto unzip_failed_1;
                            fwrite(tb.buf, 1, tb.size, f);
                            fclose(f);
                            zipFile zf;
                            zf = unzOpen(fn);
                            if (zf == NULL)
                                goto unzip_failed_2;
                            if (unzGoToFirstFile(zf) != UNZ_OK) {
                                unzip_failed_3:
                                unzClose(zf);
                                unzip_failed_2:
                                remove(fn);
                                unzip_failed_1:
                                free(tb.buf);
                                free(fn);
#ifdef FREE42
                                prgm_name_list_clear(namelist);
#endif
                                http_error(csock, 403);
                                return;
                            }
                            do {
                                unz_file_info ufi;
                                char filename[_POSIX_PATH_MAX];
                                unzGetCurrentFileInfo(zf, &ufi, filename, _POSIX_PATH_MAX, NULL, 0, NULL, 0);
                                while (filename[0] == '/')
                                    memmove(filename, filename + 1, strlen(filename) - 1);
                                char *b = filename;
                                while (1) {
                                    char *slash = strchr(b, '/');
                                    if (slash == NULL)
                                        break;
                                    if (slash == b) {
                                        b++;
                                        continue;
                                    }
                                    *slash = 0;
                                    strcpy(line, url + 1);
                                    if (line[0] != 0)
                                        strcat(line, "/");
                                    strcat(line, filename);
#ifdef FREE42
                                    if (strcmp(line, "memory") != 0 && strncmp(line, "memory/", 7) != 0)
#endif
                                        mkdir(line, 0755);
                                    *slash = '/';
                                    b = slash + 1;
                                }
                                strcpy(line, url + 1);
                                if (line[0] != 0)
                                    strcat(line, "/");
                                strcat(line, filename);
#ifdef FREE42
                                if (strncmp(line, "memory/", 7) == 0) {
                                    if (strcasecmp(line + strlen(line) - 4, ".raw") == 0) {
                                        if (unzOpenCurrentFile(zf) != UNZ_OK)
                                            goto unzip_failed_3;
                                        char *buf = (char *) malloc(1024);
                                        int n;
                                        pthread_mutex_lock(&shell_mutex);
                                        textbuf tb;
                                        tb.buf = NULL;
                                        tb.size = 0;
                                        tb.capacity = 0;
                                        while ((n = unzReadCurrentFile(zf, buf, 1024)) > 0)
                                            tbwrite(&tb, buf, n);
                                        char bufparam[21];
                                        strcpy(bufparam, "mem:");
                                        memcpy(bufparam + 5, &tb.buf, sizeof(char *));
                                        memcpy(bufparam + 13, &tb.size, sizeof(ssize_t));
                                        // TODO -- error message on failure
                                        core_import_programs(0, bufparam);
                                        if (tb.buf != NULL)
                                            free(tb.buf);
                                        pthread_mutex_unlock(&shell_mutex);
                                    }
                                } else {
#endif
                                    FILE *f = fopen(line, "w");
                                    if (f != NULL) {
                                        if (unzOpenCurrentFile(zf) != UNZ_OK) {
                                            fclose(f);
                                            remove(line);
                                            goto unzip_failed_3;
                                        }
                                        char *buf = (char *) malloc(8192);
                                        int n;
                                        while ((n = unzReadCurrentFile(zf, buf, 8192)) > 0)
                                            fwrite(buf, 1, n, f);
                                        free(buf);
                                        unzCloseCurrentFile(zf);
                                        fclose(f);
                                        struct tm stm;
                                        stm.tm_sec = ufi.tmu_date.tm_sec;
                                        stm.tm_min = ufi.tmu_date.tm_min;
                                        stm.tm_hour = ufi.tmu_date.tm_hour;
                                        stm.tm_mday = ufi.tmu_date.tm_mday;
                                        stm.tm_mon = ufi.tmu_date.tm_mon;
                                        stm.tm_year = ufi.tmu_date.tm_year - 1900;
                                        stm.tm_isdst = -1;
                                        time_t t = mktime(&stm);
                                        struct utimbuf ub;
                                        ub.actime = t;
                                        ub.modtime = t;
                                        utime(line, &ub);
                                    }
#ifdef FREE42
                                }
#endif
                            } while (unzGoToNextFile(zf) == UNZ_OK);
                            unzClose(zf);
                            remove(fn);
                            free(fn);
                        }
                    } else if (action == 4) {
                        if (tb.size > 0) {
                            tb.buf[tb.size] = 0;
                            what = strcmp(tb.buf, "delete") == 0 ? 2 : 1;
                        }
                    } else if (action != -1) {
                        // action = 1 : download or delete file
                        // action = 2 : download or delete directory
                        // action = 3 : make directory
                        // the filename is in the part body
                        if (tb.size > 0) {
                            tb.buf[tb.size] = 0;
                            if (strchr(tb.buf, '/') != NULL || strcmp(tb.buf, ".") == 0
                                    || strcmp(tb.buf, "..") == 0) {
                                // Trying to go outside the current directory
                                // or trying to delete . or ..
                                free(tb.buf);
#ifdef FREE42
                                prgm_name_list_clear(namelist);
#endif
                                http_error(csock, 403);
                                return;
                            }
                            if ((action == 1 || action == 2) && what == 1 && z == NULL) {
                                zip_name = make_temp_file();
                                z = zipOpen(zip_name, APPEND_STATUS_CREATE);
                                sockprintf(csock, "HTTP/1.1 200 OK\r\n");
                                sockprintf(csock, "Connection: close\r\n");
                                sockprintf(csock, "Content-Type: application/zip\r\n");
                                sockprintf(csock, "Transfer-Encoding: chunked\r\n");
#ifdef FREE42
                                if (strncmp(url, "/memory/", 8) == 0) {
                                    int prgm_index;
                                    char *prgm_name;
                                    if (sscanf(tb.buf, "%d", &prgm_index) == 1 && (prgm_name = prgm_index_to_name(prgm_index)) != NULL) {
                                        sockprintf(csock, "Content-Disposition: attachment; filename=%s.zip\r\n", prgm_name);
                                        free(prgm_name);
                                    } else
                                        sockprintf(csock, "Content-Disposition: attachment; filename=program.zip\r\n");
                                } else
#endif
                                    sockprintf(csock, "Content-Disposition: attachment; filename=%s.zip\r\n", tb.buf);
                                sockprintf(csock, "\r\n");
                            }
                            strcpy(line, url + 1);
                            strcat(line, tb.buf);
                            if (action == 1) {
#ifdef FREE42
                                if (strncmp(url, "/memory/", 8) == 0) {
                                    if (z == NULL) {
                                        int prgm_index;
                                        if (sscanf(tb.buf, "%d", &prgm_index) == 1) {
                                            prgm_index -= program_index_offset;
                                            pthread_mutex_lock(&shell_mutex);
                                            if (clear_prgm_by_index(prgm_index) == 0)

                                                program_index_offset++;
                                            pthread_mutex_unlock(&shell_mutex);
                                        }
                                    } else {
                                        int prgm_index;
                                        if (sscanf(tb.buf, "%d", &prgm_index) == 1)
                                            zip_program(prgm_index, z, buf, 8192, namelist);
                                    }
                                } else {
#endif
                                    if (z == NULL)
                                        remove(line);
                                    else
                                        zip_one_file(line, tb.buf, z, buf, 8192);
#ifdef FREE42
                                }
#endif
                            } else if (action == 2) {
#ifdef FREE42
                                if (strcmp(line, "memory") == 0 || strncmp(line, "memory/", 7) == 0) {
                                    if (z == NULL) {
                                        free(tb.buf);
                                        prgm_name_list_clear(namelist);
                                        http_error(csock, 403);
                                        return;
                                    } else {
                                        zip_program(-1, z, buf, 8192, namelist);
                                    }
                                } else {
#endif
                                    struct stat statbuf;
                                    if (stat(line, &statbuf) == 0 && S_ISDIR(statbuf.st_mode)) {
                                        if (z == NULL)
                                            recursive_remove(line);
                                        else
                                            recursive_zip(url + 1, tb.buf, z, buf, 8192);
                                    }
#ifdef FREE42
                                }
#endif
                            } else {
#ifdef FREE42
                                if (strncmp(url, "/memory/", 8) == 0) {
                                    free(tb.buf);
                                    prgm_name_list_clear(namelist);
                                    http_error(csock, 403);
                                    return;
                                }
#endif
                                mkdir(line, 0755);
                            }
                        }
                    }
                    free(tb.buf);
                }
                if (c == '-')
                    goto done;
                else
                    break;
            }
        }
    }
    done:

    if (z != NULL) {
        zipClose(z, NULL);
        FILE *f = fopen(zip_name, "r");
        ssize_t n;
        while ((n = fread(buf, 1, 8192, f)) > 0) {
            sockprintf(csock, "%X\r\n", n);
            send(csock, buf, n, 0);
            sockprintf(csock, "\r\n");
        }
        sockprintf(csock, "0\r\n\r\n");
        fclose(f);
        remove(zip_name);
        free(zip_name);
    } else {
        sockprintf(csock, "HTTP/1.1 302 Moved Temporarily\r\n");
        sockprintf(csock, "Connection: close\r\n");
        sockprintf(csock, "Location: %s\r\n", url);
        sockprintf(csock, "\r\n");
    }
    free((void *) url);
#ifdef FREE42
    prgm_name_list_clear(namelist);
#endif
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

#ifdef FREE42

typedef struct {
    cleaner_base base;
    char *buf;
    char **names;
    void *ptr;
} program_list_cleaner;

static void program_list_cleaner_doit(cleaner_base *This) {
    program_list_cleaner *c = (program_list_cleaner *) This;
    free(c->buf);
    free(c->names);
    if (c->ptr != NULL)
        free(c->ptr);
    free(c);
}

static cleaner_base *new_program_list_cleaner(char *buf, char **names, void *ptr) {
    program_list_cleaner *c = (program_list_cleaner *) malloc(sizeof(program_list_cleaner));
    c->base.doit = program_list_cleaner_doit;
    c->buf = buf;
    c->names = names;
    c->ptr = ptr;
    return (cleaner_base *) c;
}

#endif

typedef struct {
    cleaner_base base;
    FILE *f;
} file_cleaner;

static void file_cleaner_doit(cleaner_base *This) {
    file_cleaner *c = (file_cleaner *) This;
    fclose(c->f);
    free(c);
}

static cleaner_base *new_file_cleaner(FILE *f) {
    file_cleaner *c = (file_cleaner *) malloc(sizeof(file_cleaner));
    c->base.doit = file_cleaner_doit;
    c->f = f;
    return (cleaner_base *) c;
}

typedef struct {
    cleaner_base base;
    DIR *d;
} dir_cleaner;

static void dir_cleaner_doit(cleaner_base *This) {
    dir_cleaner *c = (dir_cleaner *) This;
    closedir(c->d);
    free(c);
}

static cleaner_base *new_dir_cleaner(DIR *d) {
    dir_cleaner *c = (dir_cleaner *) malloc(sizeof(dir_cleaner));
    c->base.doit = dir_cleaner_doit;
    c->d = d;
    return (cleaner_base *) c;
}

/*
 * Returns: an HTTP status code: 200, 403, 404, or 500.
 * 200 means everything is fine; 302 is used for redirects from /dirname to /dirname/;
 * everything else is an error (duh).
 * The return parameters ptr, type, and filesize are only meaningful for status 200:
 * type = 0: in-memory data; *ptr points to data; filesize is data size;
 * type = 1: regular file; *ptr points to FILE*, filesize is file size;
 * type = 2: fake directory; *ptr points to char**; dir end flagged by NULL
 * type = 3: real directory; *ptr points to DIR*
 * The 'cleaner' parameter is return pointer to a cleanup structure. If open_item()
 * sets it to something non-NULL, the caller must call cleaner->doit() before returning.
 */
static int open_item(const char *url, void **ptr, int *type, int *filesize, const char **real_name, cleaner_base **cleaner) {
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
    if (strncmp(url, "memory/", 7) == 0) {
        pthread_mutex_lock(&shell_mutex);
        char *buf = core_list_programs();
        pthread_mutex_unlock(&shell_mutex);
        int n = buf == NULL ? 0 : ((buf[0] & 255) << 24) | ((buf[1] & 255) << 16) | ((buf[2] & 255) << 8) | (buf[3] & 255);
        char **names = (char **) malloc((n + 1) * sizeof(char *));
        int p = 4, i;
        for (i = 0; i < n; i++) {
            names[i] = buf + p;
            p += strlen(names[i]) + 1;
        }
        names[i] = NULL;
        
        if (strcmp(url, "memory/") == 0) {
            /* GET /memory/ => return the directory listing */
            *type = 2;
            *ptr = names;
            *cleaner = new_program_list_cleaner(buf, names, NULL);
            return 200;
        }

        /* GET /memory/<program_number> => return the program */
        
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
        int idx;
        if (sscanf(url + 7, "%d", &idx) != 1)
            goto return_404;
        if (idx < 0 || idx >= n)
            goto return_404;
        pthread_mutex_lock(&shell_mutex);
	ssize_t psize;
	psize = core_program_size(idx) + 3;
	char *pbuf;
	pbuf = (char *) malloc(psize);
        if (pbuf == NULL) {
            pthread_mutex_unlock(&shell_mutex);
            return_404:
            free(buf);
            free(names);
            return 404;
        } else {
            char bufparam[21];
            strcpy(bufparam, "mem:");
            memcpy(bufparam + 5, &pbuf, sizeof(char *));
            memcpy(bufparam + 13, &psize, sizeof(ssize_t));
            core_export_programs(1, &idx, bufparam);
            *ptr = pbuf;
            *type = 0;
            *filesize = (int) psize;
            pthread_mutex_unlock(&shell_mutex);
            /* names[idx] is one of:
             * 1) "NAME1" ["NAME2" ...]
             * 2) END
             * 3) .END.
             * we're tweaking this to return, respectively:
             * 1) NAME1
             * 2) END
             * 3) END
             */
            if (names[idx][0] == '"') {
                /* One or more of "LBLNAME", separated by spaces */
                char *secondquote = strchr(names[idx] + 1, '"');
                *secondquote = 0;
                *real_name = names[idx] + 1;
            } else if (names[idx][0] == '.') {
                /* .END. */
                names[idx][4] = 0;
                *real_name = names[idx] + 1;
            } else {
                /* END */
                *real_name = names[idx];
            }
            *cleaner = new_program_list_cleaner(buf, names, *ptr);
            return 200;
        }
    }
#endif

    /* Look for hard-coded icons from icons.c */
    for (i = 0; i < icon_count; i++) {
        if (strcmp(url, icon_name[i]) == 0) {
            *ptr = icon_data[i];
            *type = 0;
            *filesize = (int) icon_size[i];
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
        FILE *f = fopen(url, "r");
        *type = 1;
        *filesize = (int) statbuf.st_size;
        if (f == NULL) {
            /* We already know the file exists and is reachable, so
             * we only check for EACCES; any other error is reported
             * as an internal server error (500).
             */
            err = errno;
            return err == EACCES ? 403 : 500;
        }
        *ptr = f;
        *cleaner = new_file_cleaner(f);
        return 200;
    } else if (S_ISDIR(statbuf.st_mode)) {
        DIR *d = opendir(url);
        *type = 3;
        if (d == NULL) {
            /* We already know the file exists and is reachable, so
             * we only check for EACCES; any other error is reported
             * as an internal server error (500).
             */
            err = errno;
            return err == EACCES ? 403 : 500;
        }
        *ptr = d;
        *cleaner = new_dir_cleaner(d);
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
    { "layout", "text/plain" },
    { "raw", "application/octet-stream" },
    { NULL, "application/octet-stream" }
};

static const char *get_mime(const char *filename) {
    int i = 0;
    size_t filenamelen = strlen(filename);
    size_t extlen;
    while (1) {
        mime_rec *mr = &mime_list[i++];
        if (mr->ext == NULL)
            return mr->mime;
        extlen = strlen(mr->ext);
        if (filenamelen > extlen
                && filename[filenamelen - extlen - 1] == '.'
                && strcmp(filename + filenamelen - extlen, mr->ext) == 0)
            return mr->mime;
    }
}

static void http_error(int csock, int err) {
    const char *msg;
    switch (err) {
        case 200: msg = "OK"; break;
        case 400: msg = "Bad Request"; break;
        case 403: msg = "Forbidden"; break;
        case 404: msg = "Not Found"; break;
        case 415: msg = "Unsupported Media Type"; break;
        case 500: msg = "Internal Server Error"; break;
        case 501: msg = "Not Implemented"; break;
        default: msg = "Internal Server Error"; break;
    }
    sockprintf(csock, "HTTP/1.1 %d %s\r\n", err, msg);
    sockprintf(csock, "Connection: close\r\n");
    sockprintf(csock, "\r\n");
    /* TODO: Descriptive response body */
    sockprintf(csock, "<html>\r\n");
    sockprintf(csock, "<head>\r\n");
    sockprintf(csock, "<title>%d %s</title>\r\n", err, msg);
    sockprintf(csock, "</head>\r\n");
    sockprintf(csock, "<body>\r\n");
    sockprintf(csock, "<h1>%d %s</h1>\r\n", err, msg);
    sockprintf(csock, "This error page is under construction.\r\n");
    sockprintf(csock, "</body>\r\n");
    sockprintf(csock, "</html>\r\n");
}

#ifdef STANDALONE

static void *handle_client_2(void *param) {
    handle_client((int) (intptr_t) param);
    return NULL;
}

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
                errprintf("Can't parse port number \"%s\"\n", argv[i + 1]);
                return 1;
            }
            i++;
        } else if (strcmp(argv[i], "-b") == 0) {
            if (i == argc - 1 || sscanf(argv[i + 1], "%d", &backlog) != 1) {
                errprintf("Can't parse backlog number \"%s\"\n", argv[i + 1]);
                return 1;
            }
            i++;
        } else if (strcmp(argv[i], "-d") == 0) {
            if (i == argc - 1)
                err = -1;
            else
                err = chdir(argv[i + 1]);
            if (err != 0) {
                errprintf("Can't chdir to docroot \"%s\"\n", argv[i + 1]);
                return 1;
            }
            i++;
        } else {
            errprintf("Unrecognized option: \"%s\"\n", argv[i]);
            return 1;
        }
    }

    ssock = socket(AF_INET, SOCK_STREAM, 0);
    if (ssock == -1) {
        err = errno;
        errprintf("Could not create socket: %s (%d)\n", strerror(err), err);
        return 1;
    }

    // TODO: This doesn't seem to work on iOS:
    // Try starting the HTTP server, connect with a browser on the PC, click
    // on the 'memory' link, kill the browser, kill Free42, restart Free42,
    // restart the HTTP server.
    // It will say that the address is already in use.
    // Why? Isn't that exactly what SO_REUSEADDR is supposed to prevent?
    int optval = 1;
    setsockopt(ssock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
    // END(TODO)

    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);
    sa.sin_addr.s_addr = INADDR_ANY;
    err = bind(ssock, (struct sockaddr *) &sa, sizeof(sa));
    if (err != 0) {
        err = errno;
        errprintf("Could not bind socket to port %d: %s (%d)\n", port, strerror(err), err);
        return 1;
    }

    err = listen(ssock, backlog);
    if (err != 0) {
        err = errno;
        errprintf("Could not listen (backlog = %d): %s (%d)\n", backlog, strerror(err), err);
        return 1;
    }

    while (1) {
        socklen_t n = sizeof(ca);
        char cname[256];
        pthread_t thread;
        csock = accept(ssock, (struct sockaddr *) &ca, &n);
        if (csock == -1) {
            err = errno;
            errprintf("Could not accept connection from client: %s (%d)\n", strerror(err), err);
            return 1;
        }
        inet_ntop(AF_INET, &ca.sin_addr, cname, sizeof(cname));
        /*errprintf("Accepted connection from %s\n", cname);*/
        pthread_create(&thread, NULL, handle_client_2, (void *) csock);
        pthread_detach(thread);
    }
    
    return 0;
}
#endif
