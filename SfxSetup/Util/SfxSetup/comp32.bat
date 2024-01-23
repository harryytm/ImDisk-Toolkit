:: \mingw32\bin\gcc.exe SfxSetup-mod.c -S -o SfxSetup-mod.s -municode -mwindows -s -Os -Wall

\mingw32\bin\windres.exe resource-mod.rc "%TEMP%\resource.o"
\mingw32\bin\gcc.exe -D_7Z_NO_METHOD_LZMA2 ..\..\7zAlloc.c ..\..\7zArcIn.c ..\..\7zBuf.c ..\..\7zBuf2.c ..\..\7zCrc.c ..\..\7zCrcOpt.c ..\..\7zFile.c^
 ..\..\7zDec.c ..\..\7zStream.c ..\..\Bcj2.c ..\..\Bra.c ..\..\BraIA64.c ..\..\Bra86.c ..\..\CpuArch.c ..\..\Delta.c ..\..\LzmaDec.c^
 SfxSetup-mod.c "%TEMP%\resource.o" -o 7zS2-32.sfx -municode -mwindows -s -Os -flto -Wall^
 -nostdlib -lgcc -lmsvcrt -lkernel32 -lshell32 -luser32 -Wl,--nxcompat,--dynamicbase -pie -e _wWinMain@16
del "%TEMP%\resource.o"
@if "%1"=="" pause