About Free42

Free42 is a free re-implementation of the HP-42S scientific programmable RPN
calculator, which was made from 1988 until 1995 by Hewlett-Packard.
Free42 is a complete rewrite and contains no HP code whatsoever.
At this time, the author supports versions that run on PalmOS, Microsoft
Windows, and Linux.


Installing Free42:

Copy Free42.exe to wherever you want it, e.g. create a directory "C:\Program
Files\Free42" and put it there. When Free42 runs, it will create three
additional files, in "My Documents"\Free42\ (this directory will be created if
it does not already exist); the additional files are state.bin, print.bin, and
keymap.txt, and they are used to store the calculator's internal state, the
contents of the print-out window, and the PC keyboard map. Also, if you want to
use a non-standard skin with Free42, this directory is where you have to store
the skin's layout and bitmap files.
For convenience, create a shortcut to Free42.exe and put it on the desktop or
in the Start menu.


Upgrading from Free42 version 1.1.14 or earlier:

Free42 1.1.14 and older stored persistent state in a file called
Free42State.bin, and the print-out image in Free42Print.bin; these files are
in the same directory as the executable. Free42 1.1.15 and later use files
called state.bin and print.bin, stored in "My Documents"\Free42.
To upgrade from 1.1.14 or older to 1.2.7 or later, you should move the state
and print-out files to "My Documents"\Free42, and rename them to state.bin and
print.bin. Note that it is not critical to do so, but if you don't, you'll
start with brand-new state.bin and print.bin files, meaning "Memory Clear" and
a blank print-out.
Starting with version 1.2.7, you can change the location of the Free42
directory, using the Preferences dialog. When you do, and if you want to keep
your old state file etc., you should exit Free42, then move the files from the
old to the new Free42 directory.


Uninstalling Free42:

Remove Free42.exe, and "My Documents"\Free42\ and its contents, and any
shortcuts you have created to point to Free42.exe.


Free42 is (C) 2004-2006, by Thomas Okken
Contact the author at thomas_okken@yahoo.com
Look for updates, and versions for other operating systems, at
http://home.planet.nl/~demun000/thomas_projects/free42/
