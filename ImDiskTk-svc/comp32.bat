\mingw32\bin\windres.exe resource.rc "%TEMP%\resource.o"
\mingw32\bin\gcc.exe ImDiskTk-svc.c "%TEMP%\resource.o" -o ImDiskTk-svc32.exe -municode -mwindows -s -Os -Wall^
 -nostdlib -lkernel32 -lntdll -ladvapi32 -Wl,--nxcompat,--dynamicbase -pie -e _wWinMain@16
del "%TEMP%\resource.o"
@if "%1"=="" pause