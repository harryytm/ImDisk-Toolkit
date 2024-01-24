\mingw64\bin\gcc.exe -mno-sse3 RamDyn.c -S -o RamDyn64.s -municode -s -O3 -minline-all-stringops -Wall

\mingw64\bin\windres.exe resource.rc "%TEMP%\resource.o"
\mingw64\bin\gcc.exe -mno-sse3 RamDyn.c "%TEMP%\resource.o" -o RamDyn64.exe^
 -municode -s -O3 -minline-all-stringops -Wall -fno-ident -lmsvcrt -lkernel32 -lntdll -luser32 -lshlwapi -lwtsapi32

del "%TEMP%\resource.o"


\mingw32\bin\gcc.exe RamDyn.c -S -o RamDyn32.s -municode -s -O3 -minline-all-stringops -Wall

\mingw32\bin\windres.exe resource.rc "%TEMP%\resource.o"
\mingw32\bin\gcc.exe RamDyn.c "%TEMP%\resource.o" -o RamDyn32.exe^
 -municode -s -O3 -minline-all-stringops -Wall -fno-ident -lmingwex -lgcc -lmsvcrt -lkernel32 -lntdll -luser32 -lshlwapi -lwtsapi32

del "%TEMP%\resource.o"
pause