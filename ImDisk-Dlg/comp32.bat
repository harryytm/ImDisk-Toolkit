:: \mingw32\bin\gcc.exe ImDisk-Dlg.c -S -o ImDisk-Dlg.s -municode -mwindows -s -Os -Wall

\mingw32\bin\windres.exe resource.rc "%TEMP%\resource.o"
\mingw32\bin\gcc.exe ImDisk-Dlg.c "%TEMP%\resource.o" -o ImDisk-Dlg32.exe -municode -mwindows -s -Os -Wall^
 -nostdlib -lgcc -lmsvcrt -lkernel32 -lshell32 -luser32 -ladvapi32 -lshlwapi -lcomdlg32^
 -Wl,--nxcompat,--dynamicbase -pie -e _wWinMain@16
del "%TEMP%\resource.o"
@if "%1"=="" pause