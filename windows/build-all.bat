call vcvars32
call copy-files
vcbuild keymap2cpp.vcproj "Release|Win32"
Release\keymap2cpp
vcbuild skin2cpp.vcproj "Release|Win32"
Release\skin2cpp
vcbuild Free42Binary.vcproj "Release|Win32"
vcbuild Free42Decimal.vcproj "Release|Win32"
