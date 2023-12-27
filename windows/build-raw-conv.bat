call vcvars32
call copy-files
rmdir /s /q Release
msbuild raw2txt.vcxproj /p:Configuration=Release
move Release\raw2txt.exe ..\packages
rmdir /s /q Release
msbuild txt2raw.vcxproj /p:Configuration=Release
move Release\txt2raw.exe ..\packages
rmdir /s /q Release
