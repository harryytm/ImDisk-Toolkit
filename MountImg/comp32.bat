:: \mingw32\bin\gcc.exe MountImg.c -S -o MountImg32.s -municode -mwindows -s -Os -Wall

\mingw32\bin\windres.exe resource.rc "%TEMP%\resource.o"
\mingw32\bin\gcc.exe MountImg.c "%TEMP%\resource.o" -o MountImg32.exe -municode -mwindows -s -Os -Wall -fno-ident^
 -nostdlib -lmsvcrt -ladvapi32 -lkernel32 -lshell32 -luser32 -lcomdlg32 -lshlwapi -lwtsapi32^
 -Wl,--nxcompat,--dynamicbase -pie -e _wWinMain@16
del "%TEMP%\resource.o"
@if "%1"=="" pause