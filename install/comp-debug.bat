\mingw32\bin\windres.exe resource.rc "%TEMP%\resource.o"
\mingw32\bin\gcc.exe config.c "%TEMP%\resource.o" -o config.exe -municode -s -Os -Wall^
 -nostdlib -lmsvcrt -lkernel32 -lshell32 -luser32 -ladvapi32 -lcomdlg32 -lgdi32 -lshlwapi -lversion -lsetupapi -lole32^
 -Wl,--nxcompat -Wl,--dynamicbase -pie -e _wWinMain@16
del "%TEMP%\resource.o"
pause