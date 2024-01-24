:: \mingw64\bin\gcc.exe -mno-sse3 MountImg.c -S -o MountImg64.s -municode -mwindows -s -Os -Wall

\mingw64\bin\windres.exe resource.rc "%TEMP%\resource.o"
\mingw64\bin\gcc.exe -mno-sse3 MountImg.c "%TEMP%\resource.o" -o MountImg64.exe -municode -mwindows -s -Os -Wall -fno-ident^
 -nostdlib -lmsvcrt -ladvapi32 -lkernel32 -lshell32 -luser32 -lcomdlg32 -lshlwapi -lwtsapi32^
 -Wl,--nxcompat,--dynamicbase,--high-entropy-va -pie -e wWinMain
del "%TEMP%\resource.o"
@if "%1"=="" pause