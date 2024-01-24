:: \mingw32\bin\gcc.exe config.c -S -o config32.s -municode -mwindows -s -Os -Wall

\mingw32\bin\windres.exe resource.rc "%TEMP%\resource.o"
\mingw32\bin\gcc.exe config.c "%TEMP%\resource.o" -o config32.exe -municode -mwindows -s -Os -Wall -fno-ident^
 -nostdlib -lmsvcrt -lkernel32 -lshell32 -luser32 -ladvapi32 -lcomdlg32 -lgdi32 -lshlwapi -lversion -lsetupapi -lole32^
 -Wl,--nxcompat,--dynamicbase -pie -e _wWinMain@16
del "%TEMP%\resource.o"
@if "%1"=="" pause