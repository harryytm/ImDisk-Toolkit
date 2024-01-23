\mingw32\bin\windres.exe resource.rc "%TEMP%\resource.o"
\mingw32\bin\gcc.exe RamDiskUI.c "%TEMP%\resource.o" -o RamDiskUI.exe^
 -lntdll -lgdi32 -lcomctl32 -lcomdlg32 -lshlwapi -lwtsapi32 -municode -s -Os -Wall
del "%TEMP%\resource.o"
pause