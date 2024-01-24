:: \mingw32\bin\gcc.exe RamDiskUI.c -S -o RamDiskUI.s -municode -mwindows -s -Os -Wall

\mingw32\bin\windres.exe resource.rc "%TEMP%\resource.o"
\mingw32\bin\gcc.exe RamDiskUI.c "%TEMP%\resource.o" -o RamDiskUI32.exe -municode -mwindows -s -Os -Wall -fno-ident^
 -nostdlib -lgcc -lmsvcrt -lntdll -ladvapi32 -lkernel32 -lshell32 -luser32 -lgdi32 -lcomctl32 -lshlwapi -lwtsapi32^
 -Wl,--nxcompat,--dynamicbase -pie -e _wWinMain@16
del "%TEMP%\resource.o"
@if "%1"=="" pause