# Microsoft Developer Studio Project File - Name="Free42" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=Free42 - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Free42.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Free42.mak" CFG="Free42 - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Free42 - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "Free42 - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Free42 - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "WINDOWS" /FR /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386

!ELSEIF  "$(CFG)" == "Free42 - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "WINDOWS" /FR /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "Free42 - Win32 Release"
# Name "Free42 - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\core_commands1.cpp
# End Source File
# Begin Source File

SOURCE=.\core_commands2.cpp
# End Source File
# Begin Source File

SOURCE=.\core_commands3.cpp
# End Source File
# Begin Source File

SOURCE=.\core_commands4.cpp
# End Source File
# Begin Source File

SOURCE=.\core_decimal.cpp
# End Source File
# Begin Source File

SOURCE=.\core_display.cpp
# End Source File
# Begin Source File

SOURCE=.\core_globals.cpp
# End Source File
# Begin Source File

SOURCE=.\core_helpers.cpp
# End Source File
# Begin Source File

SOURCE=.\core_keydown.cpp
# End Source File
# Begin Source File

SOURCE=.\core_linalg.cpp
# End Source File
# Begin Source File

SOURCE=.\core_main.cpp
# End Source File
# Begin Source File

SOURCE=.\core_math.cpp
# End Source File
# Begin Source File

SOURCE=.\core_tables.cpp
# End Source File
# Begin Source File

SOURCE=.\core_variables.cpp
# End Source File
# Begin Source File

SOURCE=.\Free42.rc
# End Source File
# Begin Source File

SOURCE=.\keymap.cpp
# End Source File
# Begin Source File

SOURCE=.\mathfudge.c
# End Source File
# Begin Source File

SOURCE=.\msg2string.cpp
# End Source File
# Begin Source File

SOURCE=.\shell.cpp
# End Source File
# Begin Source File

SOURCE=.\shell_loadimage.cpp
# End Source File
# Begin Source File

SOURCE=.\shell_skin.cpp
# End Source File
# Begin Source File

SOURCE=.\shell_spool.cpp
# End Source File
# Begin Source File

SOURCE=.\skins.cpp
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\core_commands1.h
# End Source File
# Begin Source File

SOURCE=.\core_commands2.h
# End Source File
# Begin Source File

SOURCE=.\core_commands3.h
# End Source File
# Begin Source File

SOURCE=.\core_commands4.h
# End Source File
# Begin Source File

SOURCE=.\core_decimal.h
# End Source File
# Begin Source File

SOURCE=.\core_display.h
# End Source File
# Begin Source File

SOURCE=.\core_globals.h
# End Source File
# Begin Source File

SOURCE=.\core_helpers.h
# End Source File
# Begin Source File

SOURCE=.\core_keydown.h
# End Source File
# Begin Source File

SOURCE=.\core_linalg.h
# End Source File
# Begin Source File

SOURCE=.\core_main.h
# End Source File
# Begin Source File

SOURCE=.\core_math.h
# End Source File
# Begin Source File

SOURCE=.\core_tables.h
# End Source File
# Begin Source File

SOURCE=.\core_variables.h
# End Source File
# Begin Source File

SOURCE=.\free42.h
# End Source File
# Begin Source File

SOURCE=.\msg2string.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\shell.h
# End Source File
# Begin Source File

SOURCE=.\shell_loadimage.h
# End Source File
# Begin Source File

SOURCE=.\shell_skin.h
# End Source File
# Begin Source File

SOURCE=.\shell_spool.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\Free42.ico
# End Source File
# Begin Source File

SOURCE=.\small.ico
# End Source File
# End Group
# Begin Source File

SOURCE=.\ReadMe.txt
# End Source File
# End Target
# End Project
