\mingw32\bin\windres.exe resource.rc "%TEMP%\resource.o"
\mingw32\bin\gcc.exe ImDisk-Dlg.c "%TEMP%\resource.o" -o ImDisk-Dlg.exe -municode -s -Os -Wall -lshlwapi -lcomdlg32
del "%TEMP%\resource.o"
pause