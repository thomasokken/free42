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

include $(CLEAR_VARS)

LOCAL_MODULE    := free42
LOCAL_SRC_FILES := free42glue.cc bcd.cc bcd.h bcd2.h bcdfloat.cc bcdfloat.h bcdfloat2.cc bcdfloat2.h bcdmath.cc bcdmath.h core_commands1.cc core_commands1.h core_commands2.cc core_commands2.h core_commands3.cc core_commands3.h core_commands4.cc core_commands4.h core_commands5.cc core_commands5.h core_commands6.cc core_commands6.h core_commands7.cc core_commands7.h core_display.cc core_display.h core_globals.cc core_globals.h core_helpers.cc core_helpers.h core_keydown.cc core_keydown.h core_linalg1.cc core_linalg1.h core_linalg2.cc core_linalg2.h core_main.cc core_main.h core_math1.cc core_math1.h core_math2.cc core_math2.h core_phloat.cc core_phloat.h core_sto_rcl.cc core_sto_rcl.h core_tables.cc core_tables.h core_variables.cc core_variables.h free42.h shell.h shell_loadimage.cc shell_loadimage.h shell_spool.cc shell_spool.h
LOCAL_CPP_EXTENSION := .cc
LOCAL_CPPFLAGS := -Wall -fno-exceptions -fno-rtti

include $(BUILD_SHARED_LIBRARY)
