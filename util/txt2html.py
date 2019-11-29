#!/usr/bin/python

import sys

hist = sys.stdin.read()
hist = hist.replace("&", "&amp;");
hist = hist.replace("<", "&lt;");
hist = hist.replace(">", "&gt;");

print("<html>\n"
        + "<head>\n"
        + "  <title>" + sys.argv[1] + "</title>\n"
        + "  <link rel=\"icon\" type=\"image/png\" href=\"images/free42-icon.png\">\n"
        + "</head>\n"
        + "<body>\n"
        + "<h3>" + sys.argv[1] + "</h3>\n"
        + "<pre>" + hist + "</pre>\n"
        + "<p>\n"
        + "<a href=\".\">Go to Free42 home page</a>\n"
        + "</body>\n"
        + "</html>");
