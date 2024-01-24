:: \mingw64\bin\gcc.exe -mno-sse3 RamDyn.c -S -o RamDyn.s -municode -s -O3 -minline-all-stringops -Wall

\mingw64\bin\windres.exe resource.rc "%TEMP%\resource.o"
\mingw64\bin\gcc.exe -mno-sse3 RamDyn.c "%TEMP%\resource.o" -o RamDyn64.exe -municode -mwindows -s -O3 -minline-all-stringops -Wall -fno-ident^
 -nostdlib -lmsvcrt -lkernel32 -lntdll -luser32 -ladvapi32 -lwtsapi32 -Wl,--nxcompat,--dynamicbase,--high-entropy-va -pie -e wWinMain
del "%TEMP%\resource.o"
@if "%1"=="" pause