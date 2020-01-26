Contents of this directory:

42s.doc
The Alternative HP-42S/Free42 Manual, by José Lauro Strapasson and Russ Jones.

ComplexTest.raw
ComplexTest.txt
Test program I used to test the complex transcendentals against those in the
real HP-42S, in order to look for incorrect branch cuts and other errors. It
helped me improve those functions a lot. I doubt it will be useful again, it
really has served its purpose, but I'm keeping it for historical reasons, and
because you never know.

ScaleSkin.java
A little utility to help with resizing skins.

binary.patch
A patch for creating a binary build for Android, with a different app ID,
allowing it to be installed together with the regular decimal build. Did this
at Valentin Albillo's request when he wanted to see how binary and decimal
performance compared on Android. I doubt I'll ever use this again, but still, I
might as well keep it, so I don't have to reinvent the wheel if this need ever
comes up again.

icon
The original photograph of my real HP-42S, which I used to create the Free42
icon, plus various stages of editing that I had to do along the way. I once
managed to lose my icon sources and ended up regretting that intensely once I
started needing some additional sizes; let's not let that happen again! I'm
sure for someone who knows graphic design this stuff is easy, but for me it was
a lot of work.

max_sens_rect.c
A little utility that takes a layout and increases the keys' sensitive
rectangles as far as they can grow until they touch each other. This makes the
keys easier to hit, compared to layouts where the sensitive rectangles match
the display rectangles.

rom2raw.c
Tool for extracting user code from HP-41 ROM images.

screen-shots
The screen shots I used on the iOS App Store and Google Play Store. Keeping
those in case I ever need to re-upload them for some reason, and as references
for creating new ones, if and when that becomes necessary.

txt2html.py
Tool to convert the HISTORY file at the top level of this project to a slightly
nicer-looking HTML file. This also used to be used (or rather, the C-based
predecessor of this tool before I started looking into using Python for this
kind of thing) to perform the same conversion on the TODO file, when I still
had a public one.
