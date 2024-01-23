:: \mingw64\bin\gcc.exe -mno-sse3 SfxSetup-mod.c -S -o SfxSetup-mod.s -municode -mwindows -s -Os -Wall

\mingw64\bin\windres.exe resource-mod.rc "%TEMP%\resource.o"
\mingw64\bin\gcc.exe -mno-sse3 ..\..\7zAlloc.c ..\..\7zArcIn.c ..\..\7zBuf.c ..\..\7zBuf2.c ..\..\7zCrc.c ..\..\7zCrcOpt.c ..\..\7zFile.c^
 ..\..\7zDec.c ..\..\7zStream.c ..\..\Bcj2.c ..\..\Bra.c ..\..\BraIA64.c ..\..\Bra86.c ..\..\CpuArch.c ..\..\Delta.c ..\..\LzmaDec.c^
 -D_7Z_NO_METHOD_LZMA2 SfxSetup-mod.c "%TEMP%\resource.o" -o 7zS2-64.sfx -municode -mwindows -s -Os -flto -Wall^
 -nostdlib -lgcc -lmsvcrt -lkernel32 -lshell32 -luser32 -Wl,--nxcompat,--dynamicbase,--high-entropy-va -pie -e wWinMain
del "%TEMP%\resource.o"
@if "%1"=="" pause