These sources are compiled with MinGW 7.3.0, available here:

https://sourceforge.net/projects/mingw-w64/files/Toolchains targetting Win32/Personal Builds/mingw-builds/7.3.0/threads-win32/dwarf/
https://sourceforge.net/projects/mingw-w64/files/Toolchains targetting Win64/Personal Builds/mingw-builds/7.3.0/threads-win32/seh/

Source files without copyright notice can be used without any restriction and
are provided without any warranty.

The sources of the Imdisk Virtual Disk Driver and the DiscUtils library are
available separately on the website of their respective authors.


Building a release requires the following steps:
- MinGW must be extracted in the root of the same drive of the sources. Otherwise, batch files must be adapted.
- comp_all.bat can be used to compile both the 32 and 64-bit executables.
- The driver (http://www.ltr-data.se/opencode.html/#ImDisk) must be extracted in the "files" folder.
- The DiscUtils library and related tools must be copied in the "MountImg" folder. See the FAQ of the driver for the links: http://reboot.pro/topic/15593-faqs-and-how-tos/
- Finally, make_releases.bat can be used to produce the final install executable. It assumes that the x64 version of 7-Zip is installed: https://www.7-zip.org/