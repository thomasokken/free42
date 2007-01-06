About Free42 and the Free42 conduit

Free42 is a complete re-implementation of the HP-42S scientific programmable
RPN calculator, which was made from 1988 until 1995 by Hewlett-Packard.
Free42 is a complete rewrite and contains no HP code whatsoever.
At this time, the author supports versions that run on Pocket PC, Microsoft
Windows, PalmOS, and Unix.

The PalmOS version of Free42 incorporates code that lets it create a virtual
filesystem inside a PalmOS database. Within Free42, you can use this filesystem
just like any VFS filesystem, such as a FAT32 filesystem on a Secure Digital
(SD) card. The Free42 filesystem is available on all PalmOS versions that
Free42 runs on, regardless of whether they have VFS support or not.
There are two ways to get files into and out of the Free42 filesystem:
1) In the Free42 application, use the Copy File command in the Util menu, to
copy files to or from an SD card or any other VFS filesystem;
2) Use the Free42 conduit (Windows only, for now).

If the Free42 conduit is installed on the PC you use for HotSyncing your PalmOS
hand-held, any files in the Free42 filesystem that were created or modified
since the last HotSync operation, will be copied to a directory on the PC. You
can tell the Free42 conduit which folder you wish to download to, by clicking
on the HotSync Manager icon in the task bar (the icon that looks like a red and
a blue arrow chasing each other's tails), and select the Custom option from the
menu. In the dialog box that appears next, scroll down until you find Free42,
select it, and click Change; another dialog box will pop up where you can
browse through the directories on your PC. Select the one you want to use for
downloads, and click OK; then click Done to dismiss the Custom dialog.

You can also upload files from the PC to the Palm; to do this, move them to a
directory named "Upload" within your download directory. Whatever is in the
Upload directory is copied to the Free42 filesystem on the hand-held during the
next HotSync operation; the Upload folder itself is then renamed "Upload <date>
<time>" and replaced with a new, empty, Upload folder. The reason for the
rename step is to prevent files from being uploaded over and over again in case
you forget to remove them.

NOTE: when a file is downloaded, and there is a file with the same name in the
download directory already, the old file is first renamed by appending ".001"
to its name (or if that name is also already taken, ".002" is appended instead,
etc.), so that the downloaded file does not overwrite any existing files.
During uploads, however, uploaded files simply replace any existing files in
the Free42 filesystem if they have the same name. This may be changed in the
future, to make uploads behave more like downloads, but until then, be careful
when performing uploads.

ALSO NOTE: files are downloaded only if they were created or changed since the
last HotSync; files are uploaded from the Upload folder regardless of when they
were last modified. If you have files in the Free42 filesystem on your
hand-held that you want to download, but they have been downloaded previously
and have not been changed since then, you can use the Force Download command in
Free42's Util menu; this will schedule ALL files in the Free42 filesystem for
download during the next HotSync.

ALSO ALSO NOTE: if you do not install the Free42 conduit on your PC, you can
still use the Free42 filesystem on the hand-held, but you won't be able to
upload or download files to or from it. You will be able to copy files between
it and a VFS filesystem, if available. The Free42 conduit is not required for
backing up Free42's internal state, either, so any programs and variables,
CUSTOM key assignments, etc., in short, everything that lives inside the
calculator's memory, will still be backed up during HotSync, whether the
conduit is installed or not.


Manifest:

The Free42-for-PalmOS package, Free42PalmOS.zip, should contain these files:

1) README.txt                   The document you're reading now
2) free42dec_68k.prc            The PalmOS version of Free42 Decimal (68K)
3) free42bin_68k.prc            The PalmOS version of Free42 Binary  (68K)
4) free42dec_arm.prc            The PalmOS version of Free42 Decimal (ARM)
5) free42bin_arm.prc            The PalmOS version of Free42 Binary  (ARM)
6) Free42ConduitInstaller.exe   Installer for the Free42 conduit
7) Free42Conduit.dll            The Free42 conduit
8) condmgr.dll                  Dynamic Link Library needed by the installer
9) HSAPI.dll                    Dynamic Link Library needed by the installer


Installing the Free42 application:

* Double-click one of 'free42dec_68k.prc', 'free42bin_68k.prc',
  'free42dec_arm.prc', or 'free42bin_arm.prc' to schedule it for installation
  to the hand-held. This will copy it to Quick Install; from there, it will be
  installed to the hand-held the next time you perform a HotSync operation.
  Note that you cannot install more than one version of Free42 at the same
  time. The differences between the four versions are explained below; see the
  sections about "Decimal" vs. "Binary" and "68K" vs. "ARM" at the end of this
  document.
* The 68K version of Free42 Binary (free42bin_68k.prc) requires the MathLib
  library. In case you don't already have it, you can download it from Rick
  Huebner's MathLib page at http://www.radiks.net/~rhuebner/mathlib.html.
  The other versions of Free42 (the Decimal 68K version, and the Binary and
  Decimal ARM versions) do NOT require MathLib.

Installing the Free42 conduit:

* Double-click 'Free42ConduitInstaller.exe' to start the installer.
* If you already have an older version of the Free42 conduit installed, first
  select the "Uninstall" option and click OK.
* Select the "Install" option and click OK.
* Click Exit to leave the installer.
* NOTE: You should not need to restart HotSync Manager.

NOTE: once the above installation steps are completed, you can remove the
Free42 package, even before running the next HotSync; the application and
conduit are copied to <Palm>\<User>\QuickInstall and <Palm>, respectively, and
the files in the Free42 package (i.e. where you found this README file) are no
longer needed.
You should, however, archive Free42PalmOS.zip someplace safe, since you will
need it in order to uninstall the Free42 conduit, should you decide to do so at
some time in the future.

Removing the Free42 application:

* On the hand-held, go to the Launcher (click the 'house' icon next to the
  Graffiti area to leave any application that may be running).
* Tap on the 'menu' icon (below the 'house' icon) to activate the menu bar.
* Select the App menu, then select the Delete command.
* Scroll down until you find Free42 in the list; select it, and click Delete.
* Click Yes to confirm.
* Click Done.

Removing the Free42 conduit:

* Unpack the Free42PalmOS.zip file again, in case you had deleted the extracted
  folder after you finished the installation, before.
* Double-click 'Free42ConduitInstaller.exe' to start the installer.
* Select the "Uninstall" option and click OK.
* Click Exit to leave the installer.
* NOTE: You should not need to restart HotSync Manager.


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
Free42 Decimal (free42dec_68k.prc and free42dec_arm.prc) uses Hugh Steers'
7-digit base-10000 BCD20 library, which effectively gives 25 decimal digits of
precision, with exponents ranging from -10000 to +9999. Transcendental
functions are evaluated to about 20 digits; each number consumes 16 bytes of
memory.
Free42 Binary (free42bin_68k.prc and free42bin_arm.prc) uses a software
implementation of IEEE-754 double precision binary floating point. This
consumes 8 bytes per number, and gives an effective precision of nearly 16
decimal digits, with exponents ranging from -308 to +307 (actually, exponents
can be less than -308; such small numbers are "denormalized" and don't have the
full precision of "normalized" numbers).
The binary version has the advantage of being much faster than the decimal
version; also, it uses less memory. However, numbers such as 0.1 (one-tenth)
cannot be represented exactly in binary, since they are repeating fractions
then. This inexactness can cause some HP-42S programs to fail.
If you understand the issues surrounding binary floating point, and you do not
rely on legacy software that may depend on the exactness of decimal fractions,
you may use Free42 Binary and enjoy its speed advantage. If, on the other hand,
you need full HP-42S compatibility, you should use Free42 Decimal.
If you don't fully understand the above, it is best to play safe and use
Free42 Decimal (free42dec_68k.prc or free42dec_arm.prc).


What's the deal with the "68K" and "ARM"?

Starting with version 1.4.13, Free42 for PalmOS comes in 68K and ARM versions.
They look and behave identically, but the ARM version runs much faster.
To be able to use the ARM version, you need a PalmOS PDA with an ARM CPU;
currently, this includes all Palm models running PalmOS 5.0 or later (e.g.
the Tungsten E, Tungsten T, Zire 21, and many others).
The ARM version does have a couple of drawbacks compared to the 68K version: it
is larger, so it consumes about 300 kilobytes more memory; and it takes longer
to start up (more than 2 seconds on a Zire 21; less on faster models).
NOTE: the state file format is different between the 68K and ARM versions. This
means that when you switch from using the 68K version (including all releases
prior to 1.4.13) to the ARM version, or back, you will get the "State File
Corrupt" message, and all the information in the calculator will be lost.
Be sure to save (export) all important programs before switching!


Free42 is (C) 2004-2007, by Thomas Okken
BCD support (C) 2005, by Hugh Steers / voidware
Contact the author at thomas_okken@yahoo.com
Look for updates, and versions for other operating systems, at
http://home.planet.nl/~demun000/thomas_projects/free42/
