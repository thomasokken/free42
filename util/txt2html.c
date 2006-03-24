#include <stdio.h>

int main(int argc, char *argv[]) {
    int c;
    printf("<html>\n"
	   "<head>\n"
	   "  <title>%s</title>\n"
	   "<head>\n"
	   "<body>\n"
	   "  <table border=\"0\" cellpadding=\"0\" cellspacing=\"0\">\n"
	   "    <tr><td valign=\"top\" align=\"center\" bgcolor=\"#ff60a0\"><a href=\"../index.html\"><img border=\"0\" src=\"../title.png\"></a></td>\n"
	   "    <td><img src=\"../spacer.gif\" width=\"10\"></td><td>\n"
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
	   "</td></tr></table>\n"
	   "</body>\n"
	   "</html>\n");
    return 0;
}
