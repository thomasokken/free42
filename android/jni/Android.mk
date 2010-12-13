#############################################################################
# Free42 -- an HP-42S calculator simulator
# Copyright (C) 2004-2010  Thomas Okken
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 2,
# as published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, see http://www.gnu.org/licenses/.
#############################################################################

LOCAL_PATH := $(call my-dir)
VERSION = "\"$(shell cat ../../VERSION)\""

include $(CLEAR_VARS)

# For the Decimal build, add bcd.cc, bcdfloat.cc, bcdfloat2.cc, and bcdmath.cc
# to LOCAL_SRC_FILES, and add -DBCD_MATH to LOCAL_CPPFLAGS.

LOCAL_MODULE    := free42
LOCAL_SRC_FILES := free42glue.cc bcd.cc bcdfloat.cc bcdfloat2.cc bcdmath.cc core_commands1.cc core_commands2.cc core_commands3.cc core_commands4.cc core_commands5.cc core_commands6.cc core_commands7.cc core_display.cc core_globals.cc core_helpers.cc core_keydown.cc core_linalg1.cc core_linalg2.cc core_main.cc core_math1.cc core_math2.cc core_phloat.cc core_sto_rcl.cc core_tables.cc core_variables.cc shell_spool.cc
LOCAL_CPP_EXTENSION := .cc
LOCAL_CPPFLAGS := -DBCD_MATH -DANDROID -DVERSION=$(VERSION) -DNO_SINCOS -Wall -fno-exceptions -fno-rtti -fsigned-char -g

include $(BUILD_SHARED_LIBRARY)
