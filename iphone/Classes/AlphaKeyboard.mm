/*****************************************************************************
 * Free42 -- an HP-42S calculator simulator
 * Copyright (C) 2004-2025  Thomas Okken
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

#import "AlphaKeyboard.h"
#import "CalcView.h"


struct key {
    int x, y, w, h;
    const char *normal, *shifted, *num, *sym;
    key *next;
    key(int x, int y, int w, int h, const char *normal, const char *shifted, const char *num, const char *sym)
        : x(x), y(y), w(w), h(h), normal(normal), shifted(shifted), num(num), sym(sym) {}
    ~key() {
        free(normal);
        free(shifted);
        free(num);
        free(sym);
        delete next;
    }
};

static key *kbMap = NULL;
static int kbWidth, kbHeight;


@implementation AlphaKeyboard

- (id)initWithFrame:(CGRect)frame {
    if (self = [super initWithFrame:frame]) {
        // Initialization code
    }
    return self;
}

- (void) awakeFromNib {
    [super awakeFromNib];
    if (kbMap == NULL) {
        // Read alphakeyboard.txt
        char buf[1024];
        NSString *path = [[NSBundle mainBundle] pathForResource:@"alphakeyboard" ofType:@"txt"];
        [path getCString:buf maxLength:1024 encoding:NSUTF8StringEncoding];
        FILE *mapfile = fopen(buf, "r");
        if (mapfile == NULL)
            goto no_file;

        char *def[4] = { NULL, NULL, NULL, NULL };

        while (fgets(buf, 1024, mapfile) != NULL) {
            char *p = buf;
            while (isspace(*p))
                p++;
            char *hash = strchr(p, '#');
            if (hash != NULL)
                *hash = 0;
            int n = strlen(p) - 1;
            while (n >= 0 && isspace(p[n]))
                p[n--] = 0;
            if (strncasecmp(p, "size:", 5) == 0) {
                if (sscanf(p + 5, "%d %d", &kbWidth, &kbHeight) != 2) {
                    fail:
                    delete kbMap;
                    kbMap = NULL;
                    for (int i = 0; i < 4; i++)
                        free(def[i]);
                    break;
                }
            } else {
                char *c = strchr(p, ':');
                if (c == NULL)
                    goto fail;
                *c++ = 0;
                int x, y, w, h;
                if (sscanf(p, "%d,%d,%d,%d", &x, &y, &w, &h) != 4)
                    goto fail;
                char defcount = 0;
                char defbuf[32];
                char defptr;
                bool in_str = false;
                int len = strlen(c);
                for (int i = 0; i < len; i++) {
                    int ch = c[i] & 255;
                    if (!in_str) {
                        if (ch == '"') {
                            if (defcount == 4)
                                goto fail;
                            in_str = true;
                            defptr = 0;
                        }
                        continue;
                    }
                    if (ch == '"') {
                        char *s = (char *) malloc(defptr + 1);
                        if (s == NULL)
                            goto fail;
                        memcpy(s, defbuf, defptr);
                        s[defptr] = 0;
                        def[defcount++] = s;
                        in_str = false;
                        continue;
                    }
                    if (ch == '\\') {
                        int ch2 = c[++i] & 255;
                        if (ch2 == '\\' || ch2 == '"')
                            ch = ch2;
                        else
                            goto fail;
                    }
                    defbuf[defptr++] = ch;
                }
                if (defcount != 4)
                    goto fail;
                key *k = new key(x, y, w, h, def[0], def[1], def[2], def[3]);
                k->next = kbMap;
                kbMap = k;
            }
        }

        fclose(mapfile);
        no_file:
    }
}

- (void) raised {
    // Set everything to initial state
}

- (void)drawRect:(CGRect)rect {
    // Drawing code
}

- (void) touchesBegan: (NSSet *) touches withEvent: (UIEvent *) event {
    // Handle key presses
    [super touchesBegan:touches withEvent:event];
}

@end
