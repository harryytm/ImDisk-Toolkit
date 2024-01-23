:: \mingw64\bin\gcc.exe -mno-sse3 ImDiskTk-svc.c -S -o ImDiskTk-svc.s -municode -mwindows -s -Os -Wall

\mingw64\bin\windres.exe resource.rc "%TEMP%\resource.o"
\mingw64\bin\gcc.exe -mno-sse3 ImDiskTk-svc.c "%TEMP%\resource.o" -o ImDiskTk-svc64.exe -municode -mwindows -s -Os -Wall^
 -nostdlib -lkernel32 -lntdll -ladvapi32 -Wl,--nxcompat,--dynamicbase,--high-entropy-va -pie -e wWinMain
del "%TEMP%\resource.o"
@if "%1"=="" pause