#!/usr/bin/python

import sys
import os
import subprocess
from datetime import datetime

# On the command line, we expect one argument, which is the version number, in
# 1.2.3a notation: one, two, or three numerical components, separated by dots,
# and an optional lowercase letter, attached to the final numerical component
# without any separators.
# Additional arguments are "android", "ios", "windows", "macos", "linux", and
# "all". The OS names may be abbreviated, but "all" must be spelled out fully.

version_raw = None
do_android = False
do_ios = False
do_windows = False
do_macos = False
do_linux = False
do_nothing = True

for arg in sys.argv[1:]:
    if len(arg) == 0:
        continue
    orig_arg = arg
    arg = arg.lower()
    if arg[0].isdigit():
        version_raw = arg
    elif arg == "all":
        do_android = True
        do_ios = True
        do_windows = True
        do_macos = True
        do_linux = True
        do_nothing = False
    elif "android".startswith(arg):
        do_android = True
        do_nothing = False
    elif "ios".startswith(arg):
        do_ios = True
        do_nothing = False
    elif "windows".startswith(arg):
        do_windows = True
        do_nothing = False
    elif "macos".startswith(arg):
        do_macos = True
        do_nothing = False
    elif "linux".startswith(arg):
        do_linux = True
        do_nothing = False
    elif "gtk".startswith(arg):
        do_linux = True
        do_nothing = False
    else:
        raise Exception("Unrecognized argument: \"" + orig_arg + "\"")

if version_raw == None:
    raise Exception("Version number must be specified.");
if do_nothing:
    do_android = True
    do_ios = True
    do_windows = True
    do_macos = True
    do_linux = True
        
version_name = version_raw
if not version_raw[-1].isdigit():
    version_final = ord(version_raw[-1]) - ord('a') + 1;
    version_raw = version_raw[0:-1]
else:
    version_final = 0
version_comps = map(int, version_raw.split('.'))
while len(version_comps) < 3:
    version_comps.append(0)
version_comps.append(version_final)

# For Apple targets, we need to insert the version number into Info.plist
# files, which we'll accomplish using this function, which takes the plist
# file name as its argument

def patch_plist(plist_name):
    in_file = open(plist_name)
    out_file = open(plist_name + ".new", "w")
    state = 0
    success = 0
    version = str(version_comps[0])
    short_version = str(version_comps[0])
    if version_comps[1] != 0 or version_comps[3] != 0:
        version += "." + str(version_comps[1])
        short_version += str(version_comps[1])
        if version_comps[2] != 0 or version_comps[3] != 0:
            version += "." + str(version_comps[2])
            short_version += "." + str(version_comps[2])
            if version_comps[3] != 0:
                version += "." + str(version_comps[3])
                short_version += "." + str(version_comps[3])
    for line in in_file:
        if state == 0:
            if "<key>CFBundleVersion</key>" in line:
                state = 1
            elif "<key>CFBundleShortVersionString</key>" in line:
                state = 2
        else:
            p = line.find("<string>")
            if p == -1:
                break
            p += 8
            p2 = line.find("</string>")
            if p2 == -1:
                break
            line = line[0:p] + (version if state == 1 else short_version) + line[p2:]
            success |= state
            state = 0
        out_file.write(line)
    in_file.close()
    out_file.close()
    if success == 3:
        os.rename(plist_name + ".new", plist_name)
    else:
        os.remove(plist_name + ".new")
        raise Exception("Could not set version numbers in " + plist_name + "; status = " + success)

# Insert version for Mac

if do_macos:
    patch_plist("mac/Info.plist")

# Insert version for iOS

if do_ios:
    patch_plist("iphone/Info.plist")

# Insert version for Android and increment versionCode

if do_android:
    vc = int(os.popen("grep versionCode android/app/build.gradle").read().split()[1]);
    vc += 1
    subprocess.call(["sed", "-i", "", "s/versionCode [0-9]*/versionCode " + str(vc) + "/", "android/app/build.gradle"])
    subprocess.call(["sed", "-i", "", "s/versionName \"[^\"]*\"/versionName \"" + version_name + "\"/", "android/app/build.gradle"])

# Insert version for Linux

if do_linux:
    v_file = open("gtk/VERSION", "w")
    v_file.write(version_name + "\n")
    v_file.close()

# Insert version for Windows

if do_windows:
    v_file = open("windows/VERSION.rc", "w")
    v_file.write("#define FREE42_VERSION_1 \"Free42 " + version_name + "\"\n")
    v_file.write("#define FREE42_VERSION_2 \"" + version_name + "\\0\"\n")
    v_file.write("#define FREE42_VERSION_3 " + str(version_comps[0]) + "," + str(version_comps[1]) + "," + str(version_comps[2]) + "," + str(version_comps[3]) + "\n")
    v_file.write("#define FREE42_VERSION_4 \"Release " + version_name + "\"\n")
    v_file.close()

# Create a new entry at the top of HISTORY, appropriately labeled, and dump the commit history since the latest tag.

f = open("_temp", "w")
f.write(datetime.today().strftime('%Y-%m-%d') + ": release " + version_name)

oses = [ ]
if do_android:
    oses.append("Android")
if do_ios:
    oses.append("iOS")
if do_windows:
    oses.append("Windows")
if do_macos:
    oses.append("MacOS")
if do_linux:
    oses.append("Linux")

if len(oses) == 5:
    pass # Write nothing
elif len(oses) == 1:
    f.write(" (" + oses[0] + " only)")
elif len(oses) == 2:
    f.write(" (" + oses[0] + " and " + oses[1] + ")")
else:
    f.write(" (")
    for i in range(len(oses) - 1):
        f.write(oses[i] + ", ")
    f.write("and " + oses[-1] + ")")
f.write("\n\n")

v = os.popen("git describe --tags").read().split("-")[-2]
f.write(os.popen("git log -" + v).read())
f.write("\n")
h = open("HISTORY", "r")
f.write(h.read())
h.close()
f.close()
os.rename("_temp", "HISTORY")
