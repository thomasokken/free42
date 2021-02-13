call vcvars32
call copy-files
rmdir /s /q Release
msbuild keymap2cpp.vcxproj /p:Configuration=Release
Release\keymap2cpp
rmdir /s /q Release
msbuild skin2cpp.vcxproj /p:Configuration=Release
Release\skin2cpp
rmdir /s /q Release
msbuild Free42Binary.vcxproj /p:Configuration=Release
move Release\Free42Binary.exe .
rmdir /s /q Release
msbuild Free42Decimal.vcxproj /p:Configuration=Release
move Release\Free42Decimal.exe .
rmdir /s /q Release
