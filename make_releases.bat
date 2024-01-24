del files\RamDyn.exe files\ImDiskTk-svc.exe
copy /y /b RamDiskUI\RamDiskUI32.exe files\RamDiskUI.exe
copy /y /b RamDyn\RamDyn*.exe files
copy /y /b MountImg\MountImg32.exe files\MountImg.exe
copy /y /b ImDisk-Dlg\ImDisk-Dlg32.exe files\ImDisk-Dlg.exe
copy /y /b ImDiskTk-svc\ImDiskTk-svc*.exe files
copy /y /b install\config32.exe files\config.exe
copy /y /b install\lang\* files\lang
"%ProgramW6432%\7-Zip\7z.exe" a ImDiskTk.7z .\files\* -xr!ia64 -xr!ARM* -xr!cplcore -mm=LZMA -mx=9 -mmc=1000000000
if exist ImDiskTk.7z copy /y /b SfxSetup\Util\SfxSetup\7zS2-32.sfx + ImDiskTk.7z ImDiskTk.exe
del ImDiskTk.7z

del files\RamDyn32.exe files\ImDiskTk-svc32.exe
ren files\RamDyn64.exe RamDyn.exe
ren files\ImDiskTk-svc64.exe ImDiskTk-svc.exe
copy /y /b RamDiskUI\RamDiskUI64.exe files\RamDiskUI.exe
copy /y /b MountImg\MountImg64.exe files\MountImg.exe
copy /y /b ImDisk-Dlg\ImDisk-Dlg64.exe files\ImDisk-Dlg.exe
copy /y /b install\config64.exe files\config.exe
"%ProgramW6432%\7-Zip\7z.exe" a ImDiskTk.7z .\files\* -xr!ia64 -xr!ARM* -xr!cplcore -x!driver\awealloc\i386 -x!driver\svc\i386 -x!driver\sys\i386 -mm=LZMA -mx=9 -mmc=1000000000
if exist ImDiskTk.7z copy /y /b SfxSetup\Util\SfxSetup\7zS2-64.sfx + ImDiskTk.7z ImDiskTk-x64.exe
del ImDiskTk.7z

pause