call vcvars32
call copy-files
rmdir /s /q Release
msbuild keymap2cpp.vcxproj /p:Configuration=Release
Release\keymap2cpp
rmdir /s /q Release
msbuild skin2cpp.vcxproj /p:Configuration=Release
Release\skin2cpp
rmdir /s /q 32
mkdir 32
rmdir /s /q Release
msbuild Free42Binary32.vcxproj /p:Configuration=Release
move Release\Free42Binary.exe 32
rmdir /s /q Release
msbuild Free42Decimal32.vcxproj /p:Configuration=Release
move Release\Free42Decimal.exe 32
call vcvars64
rmdir /s /q Release
msbuild Free42Binary64.vcxproj /p:Configuration=Release
move Release\Free42Binary.exe .
rmdir /s /q Release
msbuild Free42Decimal64.vcxproj /p:Configuration=Release
move Release\Free42Decimal.exe .
rmdir /s /q Release
