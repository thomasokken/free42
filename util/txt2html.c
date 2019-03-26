#include <stdio.h>

int main(int argc, char *argv[]) {
    int c;
    printf("<html>\n"
           "<head>\n"
           "  <title>%s</title>\n"
           "</head>\n"
           "<body>\n"
           "<h3>%s</h3>\n"
           "<pre>", argv[1], argv[1]);
    while ((c = getchar()) != EOF) {
        switch (c) {
            case '<': printf("&lt;"); break;
            case '>': printf("&gt;"); break;
            case '&': printf("&amp;"); break;
            default: putchar(c);
        }
    }
    printf("</pre>\n"
           "<p>\n"
           "<a href=\".\">Go to Free42 home page</a>\n"
           "</body>\n"
           "</html>\n");
    return 0;
}
