:: \mingw32\bin\gcc.exe RamDyn.c -S -o RamDyn.s -municode -s -O3 -minline-all-stringops -Wall

\mingw32\bin\windres.exe resource.rc "%TEMP%\resource.o"
\mingw32\bin\gcc.exe RamDyn.c "%TEMP%\resource.o" -o RamDyn32.exe -municode -mwindows -s -O3 -minline-all-stringops -Wall -Wl,--large-address-aware^
 -nostdlib -lgcc -lmsvcrt -lkernel32 -lntdll -luser32 -ladvapi32 -lwtsapi32 -Wl,--nxcompat,--dynamicbase -pie -e _wWinMain@16
del "%TEMP%\resource.o"
@if "%1"=="" pause