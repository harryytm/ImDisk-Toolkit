set Path_prev=%Path%
set Path=%~d0\mingw64\bin
:: gcc.exe MountImg.c -S -o MountImg64.s -municode -mwindows -s -Os -Wall -D_CRTBLD

windres.exe resource.rc "%TEMP%\resource.o"
gcc.exe MountImg.c "%TEMP%\resource.o" -o MountImg64.exe -municode -mwindows -s -Os -Wall -D_CRTBLD -fno-ident^
 -nostdlib -lmsvcrt -ladvapi32 -lkernel32 -lshell32 -luser32 -lcomdlg32 -lshlwapi -lwtsapi32^
 -Wl,--nxcompat,--dynamicbase,--high-entropy-va -pie -e wWinMain
del "%TEMP%\resource.o"
set Path=%Path_prev%
@if "%1"=="" pause