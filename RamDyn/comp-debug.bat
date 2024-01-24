set Path_prev=%Path%
set Path=%~d0\mingw64\bin
gcc.exe RamDyn.c -S -o RamDyn64.s -municode -s -O3 -minline-all-stringops -Wall -D_CRTBLD

windres.exe resource.rc "%TEMP%\resource.o"
gcc.exe RamDyn.c "%TEMP%\resource.o" -o RamDyn64.exe^
 -municode -s -O3 -minline-all-stringops -Wall -D_CRTBLD -fno-ident -lmsvcrt -lkernel32 -lntdll -luser32 -lshlwapi -lwtsapi32

del "%TEMP%\resource.o"


set Path=%~d0\mingw32\bin
gcc.exe RamDyn.c -S -o RamDyn32.s -municode -s -O3 -minline-all-stringops -Wall -D_CRTBLD

windres.exe resource.rc "%TEMP%\resource.o"
gcc.exe RamDyn.c "%TEMP%\resource.o" -o RamDyn32.exe^
 -municode -s -O3 -minline-all-stringops -Wall -D_CRTBLD -fno-ident -lmingwex -lgcc -lmsvcrt -lkernel32 -lntdll -luser32 -lshlwapi -lwtsapi32

del "%TEMP%\resource.o"
set Path=%Path_prev%
pause