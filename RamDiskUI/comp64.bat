:: \mingw64\bin\gcc.exe -mno-sse3 RamDiskUI.c -S -o RamDiskUI.s -municode -mwindows -s -Os -Wall

\mingw64\bin\windres.exe resource.rc "%TEMP%\resource.o"
\mingw64\bin\gcc.exe -mno-sse3 RamDiskUI.c "%TEMP%\resource.o" -o RamDiskUI64.exe -municode -mwindows -s -Os -Wall^
 -nostdlib -lgcc -lmsvcrt -lntdll -ladvapi32 -lkernel32 -lshell32 -luser32 -lgdi32 -lcomctl32 -lshlwapi -lwtsapi32^
 -Wl,--nxcompat,--dynamicbase,--high-entropy-va -pie -e wWinMain
del "%TEMP%\resource.o"
@if "%1"=="" pause