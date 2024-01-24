These sources are compiled with MinGW 10.2.1, available here: https://gcc-mcf.lhmouse.com/
64-bit:
https://gcc-mcf.lhmouse.com/mingw-w64-gcc-mcf_20201010_10.2.1_x64_554c631b4f4c3b9d9de8721f04ee2287d6b7ad0d.7z
32-bit:
https://gcc-mcf.lhmouse.com/mingw-w64-gcc-mcf_20201010_10.2.1_x86_3b362c8b23ad5514b005aa15ad0eb55ae76276a1.7z

Source files without copyright notice can be used without any restriction and
are provided without any warranty.

The sources of the Imdisk Virtual Disk Driver and the DiscUtils library are
available separately on the website of their respective authors.


Building a release requires the following steps:
- MinGW must be extracted in the root of the same drive of the sources. Otherwise, batch files must be adapted.
- comp_all.bat can be used to compile both the 32 and 64-bit executables.
- The driver (http://www.ltr-data.se/opencode.html/#ImDisk) must be extracted in the "files" folder.
- The DiscUtils library and related tools must be copied in the "MountImg" folder. See the FAQ of the driver for the links: http://reboot.pro/topic/15593-faqs-and-how-tos/
- Finally, make_releases.bat can be used to produce the final install packages. It assumes that the x64 version of 7-Zip is installed: https://www.7-zip.org/