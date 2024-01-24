:: \mingw64\bin\gcc.exe -mno-sse3 ImDisk-Dlg.c -S -o ImDisk-Dlg.s -municode -mwindows -s -Os -Wall

\mingw64\bin\windres.exe resource.rc "%TEMP%\resource.o"
\mingw64\bin\gcc.exe -mno-sse3 ImDisk-Dlg.c "%TEMP%\resource.o" -o ImDisk-Dlg64.exe -municode -mwindows -s -Os -Wall -fno-ident^
 -nostdlib -lmsvcrt -lkernel32 -lshell32 -luser32 -ladvapi32 -lshlwapi -lcomdlg32^
 -Wl,--nxcompat,--dynamicbase,--high-entropy-va -pie -e wWinMain
del "%TEMP%\resource.o"
@if "%1"=="" pause