#include <stdio.h>
#include <string.h>
#include <ctype.h>

int main() {
    FILE *versionfile = fopen("../VERSION", "r");
    if (versionfile == NULL) {
        fprintf(stderr, "Can't open ../VERSION\n");
        return 1;
    }
    FILE *infoplistfile = fopen("Info.plist", "r");
    if (infoplistfile == NULL) {
        fprintf(stderr, "Can't open Info.plist\n");
        return 1;
    }
    char version[100];
    if (fscanf(versionfile, "%99s", version) != 1) {
        fprintf(stderr, "Can't read version number\n");
        return 1;
    }
    int vlen = strlen(version);
    if (vlen == 0) {
        fprintf(stderr, "Version is empty\n");
        return 1;
    }
    char c = version[vlen - 1];
    if (!isdigit(c)) {
        version[vlen - 1] = '.';
        char *p = version + vlen;
        sprintf(p, "%d", ((int) c) - 96);
    }
    char short_version[100];
    char *dot = strchr(version, '.');
    strncpy(short_version, version, dot - version);
    strcat(short_version, dot + 1);
    int state = 0;
    char line[1024], newline[1024];
    FILE *newinfoplistfile = fopen("Info.plist.new", "w");
    int success = 0;
    char *p, *p2;
    int len;
    while (fgets(line, 1024, infoplistfile) != NULL) {
        switch (state) {
            case 0:
                if (strstr(line, "<key>CFBundleVersion</key>") != NULL)
                    state = 1;
                else if (strstr(line, "<key>CFBundleShortVersionString</key>") != NULL)
                    state = 2;
                break;
            case 1:
            case 2:
                p = strstr(line, "<string>");
                if (p == NULL)
                    goto done;
                p += 8;
                p2 = strstr(line, "</string>");
                if (p2 == NULL)
                    goto done;
                len = p - line;
                memcpy(newline, line, len);
                newline[len] = 0;
                strcat(newline, state == 1 ? version : short_version);
                strcat(newline, p2);
                strcpy(line, newline);
                success |= state;
                state = 0;
                break;
        }
        fputs(line, newinfoplistfile);
    }
done:
    fclose(infoplistfile);
    fclose(newinfoplistfile);
    if (success == 3) { 
        rename("Info.plist.new", "Info.plist");
        return 0;
    } else {
        remove("Info.plist.new");
        fprintf(stderr, "Updating CFBundleVersion or CFBundleShortVersionString in Info.plist failed.\n");
        return 1;
    }
}
