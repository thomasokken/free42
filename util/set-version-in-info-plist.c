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
		break;
	    case 1:
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
		strcat(newline, version);
		strcat(newline, p2);
		strcpy(line, newline);
		state = 2;
		break;
	}
	fputs(line, newinfoplistfile);
    }
    success = state == 2;
done:
    fclose(infoplistfile);
    fclose(newinfoplistfile);
    if (success) { 
	rename("Info.plist.new", "Info.plist");
	return 0;
    } else {
	remove("Info.plist.new");
	fprintf(stderr, "Updating CFBundleVersion in Info.plist failed.\n");
	return 1;
    }
}
