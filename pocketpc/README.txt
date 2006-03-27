About Free42

Free42 is a complete re-implementation of the HP-42S scientific programmable
RPN calculator, which was made from 1988 until 1995 by Hewlett-Packard.
Free42 is a complete rewrite and contains no HP code whatsoever.
At this time, the author supports versions that run on Pocket PC, Microsoft
Windows, PalmOS, and Unix.


Installing Free42:

Copy Free42Decimal.cab (or Free42Binary.cab, or both) to your Pocket PC and run
it. The easiest is probably to copy everything onto an SD card and run it from
there (run File Explorer, navigate to the appropriate directory, and click on
Free42Binary or Free42Decimal).
Running the *.cab file will install the executable in the Program Files
directory, and it will create a shortcut in the Start menu.

Free42 stores its 'state.bin' and 'keymap.txt' files in the same directory
where the executable is located, that is, in \Program Files\Free42 (unless you
moved it someplace else manually).
It also looks for additional skins in that location. Note that installing skins
is optional, since Free42 already has two 240x268 skins built in.


Uninstalling Free42:

Remove \Program Files\Free42 and everything in it.
Remove the shortcut(s) to Free42 from the Start menu (\Windows\Start Menu).


Documentation

The ultimate documentation for Free42 is the manual for the HP-42S. You can
obtain this manual in PDF format by purchasing the CD or DVD set from The
Museum of HP Calculators (http://hpmuseum.org/). Alternatively, there is an
independently written HP-42S/Free42 manual, by Jose Lauro Strapasson, which
you can download free at http://joselauro.com/42s.pdf.


What's the deal with the "Decimal" and "Binary"?

Starting with version 1.4, Free42 comes in decimal and binary versions. The two
look and behave identically; the only difference is the way they represent
numbers internally.
Free42 Decimal uses Hugh Steers' 7-digit base-10000 BCDFloat class, which
effectively gives 25 decimal digits of precision, with exponents ranging from
-10000 to +9999. Transcendental functions are evaluated to about 20 digits;
each number consumes 16 bytes of memory.
Free42 Binary uses the PC's FPU; it represents numbers as IEEE-754 compatible
double precision binary floating point, which consumes 8 bytes per number, and
gives an effective precision of nearly 16 decimal digits, with exponents
ranging from -308 to +307 (actually, exponents can be less than -308; such
small numbers are "denormalized" and don't have the full precision of
"normalized" numbers).
The binary version has the advantage of being much faster than the decimal
version; also, it uses less memory. However, numbers such as 0.1 (one-tenth)
cannot be represented exactly in binary, since they are repeating fractions
then. This inexactness can cause some HP-42S programs to fail.
If you understand the issues surrounding binary floating point, and you do not
rely on legacy software that may depend on the exactness of decimal fractions,
you may use Free42 Binary and enjoy its speed advantage. If, on the other hand,
you need full HP-42S compatibility, you should use Free42 Decimal.
If you don't fully understand the above, it is best to play safe and use
Free42 Decimal.


Free42 is (C) 2004-2006, by Thomas Okken
BCD support (C) 2005, by Hugh Steers / voidware
Contact the author at thomas_okken@yahoo.com
Look for updates, and versions for other operating systems, at
http://home.planet.nl/~demun000/thomas_projects/free42/
