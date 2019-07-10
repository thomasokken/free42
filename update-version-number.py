#!/usr/bin/python

import sys
import os
import subprocess

# On the command line, we expect one argument, which is the version number, in
# 1.2.3a notation: one, two, or three numerical components, separated by dots,
# and an optional lowercase letter, attached to the final numerical component
# without any separators.
# In addition, the option -a may be specified, which indicates that the Android
# version code should not be bumped.

try:
    sys.argv.remove("-a")
    bump_android_version_code = False
except:
    bump_android_version_code = True
        
version_raw = sys.argv[1]
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

patch_plist("mac/Info.plist")

# Insert version for Mac Dashboard

patch_plist("macdashboard/Info.plist")

# Inset version for iOS

patch_plist("iphone/Info.plist")

# Update Android version.code -- this is just a sequence, not related to the version number --
# and insert that code, and the version number, into the Android build

if bump_android_version_code:
    vc_file = open("android/version.code")
    vc = int(vc_file.read())
    vc_file.close()
    vc += 1
    vc_file = open("android/version.code", "w");
    vc_file.write(str(vc));
    vc_file.close()
    subprocess.call(["sed", "-i",  "", "s/versionCode [0-9]*/versionCode " + str(vc) + "/", "android/app/build.gradle"])
        
subprocess.call(["sed", "-i", "", "s/versionName \"[^\"]*\"/versionName \"" + version_name + "\"/", "android/app/build.gradle"])

# Insert the version number into VERSION and VERSION.rc

v_file = open("VERSION", "w")
v_file.write(version_name + "\n")
v_file.close()
v_file = open("windows/VERSION.rc", "w")
v_file.write("#define FREE42_VERSION_1 \"Free42 " + version_name + "\"\n")
v_file.write("#define FREE42_VERSION_2 \"" + version_name + "\\0\"\n")
v_file.write("#define FREE42_VERSION_3 " + str(version_comps[0]) + "," + str(version_comps[1]) + "," + str(version_comps[2]) + "," + str(version_comps[3]) + "\n")
v_file.write("#define FREE42_VERSION_4 \"Release " + version_name + "\"\n")
v_file.close()
