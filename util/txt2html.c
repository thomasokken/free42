#include <stdio.h>

int main(int argc, char *argv[]) {
    int c;
    printf("<html>\n"
	   "<head>\n"
	   "  <title>%s</title>\n"
	   "<head>\n"
	   "<body style=\"background: no-repeat fixed left top; background-image: url(title.png); padding-left: 62px;\">\n"
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
	   "Go <a href=\"index.html\">back</a>.\n"
	   "<p>\n"
	   "<a href=\"http://sourceforge.net/\"><img src=\"http://sflogo.sourceforge.net/sflogo.php?group_id=211716&type=5\" width=\"210\" height=\"62\" border=\"0\" alt=\"SourceForge.net Logo\"/></a>\n"
	   "</body>\n"
	   "</html>\n");
    return 0;
}
