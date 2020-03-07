call vcvars32
call copy-files
msbuild keymap2cpp.vcxproj /p:Configuration=Release
Release\keymap2cpp
msbuild skin2cpp.vcxproj /p:Configuration=Release
Release\skin2cpp
msbuild Free42Binary.vcxproj /p:Configuration=Release
move Release\Free42Binary.exe .
rmdir /s /q Release
msbuild Free42Decimal.vcxproj /p:Configuration=Release
move Release\Free42Decimal.exe .
rmdir /s /q Release
