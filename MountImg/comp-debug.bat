\mingw32\bin\windres.exe resource.rc "%TEMP%\resource.o"
\mingw32\bin\gcc.exe MountImg.c "%TEMP%\resource.o" -o MountImg32.exe -municode -s -Os -Wall^
 -nostdlib -lmsvcrt -lkernel32 -lshell32 -luser32 -ladvapi32 -lcomdlg32 -lshlwapi -lwtsapi32^
 -Wl,--nxcompat -Wl,--dynamicbase -pie -e _wWinMain@16
del "%TEMP%\resource.o"

\mingw64\bin\windres.exe resource.rc "%TEMP%\resource.o"
\mingw64\bin\gcc.exe -mno-sse3 MountImg.c "%TEMP%\resource.o" -o MountImg64.exe -municode -s -Os -Wall^
 -nostdlib -lmsvcrt -lkernel32 -lshell32 -luser32 -ladvapi32 -lcomdlg32 -lshlwapi -lwtsapi32^
 -Wl,--nxcompat -Wl,--dynamicbase -pie -e wWinMain
del "%TEMP%\resource.o"

pause