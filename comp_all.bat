@for /f %%I in ('wmic OS get LocalDateTime ^| find "."') do @set D=%%I
echo #define APP_VERSION "%D:~0,8%">inc\build.h
echo #define FILE_VER %D:~0,4%,%D:~4,2%,%D:~6,2%,^0>>inc\build.h
echo #define PROD_VER "%D:~0,4%.%D:~4,2%.%D:~6,2%.0">>inc\build.h
cd ImDisk-Dlg
call comp32.bat -
call comp64.bat -
cd ..\ImDiskTk-svc
call comp32.bat -
call comp64.bat -
cd ..\install
call comp32.bat -
call comp64.bat -
cd ..\MountImg
call comp32.bat -
call comp64.bat -
cd ..\RamDiskUI
call comp32.bat -
call comp64.bat -
cd ..\RamDyn
call comp32.bat -
call comp64.bat -
cd ..\SfxSetup\Util\SfxSetup
call comp32.bat -
call comp64.bat -
cd ..\..\..
upx --ultra-brute SfxSetup\Util\SfxSetup\7zS2-32.sfx
upx --ultra-brute SfxSetup\Util\SfxSetup\7zS2-64.sfx
@pause