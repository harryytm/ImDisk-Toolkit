These sources are compiled with MinGW 12.2.1, available here: https://gcc-mcf.lhmouse.com/
64-bit:
https://gcc-mcf.lhmouse.com/mingw-w64-gcc-mcf_20220823_12.2.1_x64-msvcrt_e94c5578c707d2784c6279bca30fcf692e84a180.7z
32-bit:
https://gcc-mcf.lhmouse.com/mingw-w64-gcc-mcf_20220823_12.2.1_x86-msvcrt_cb1d274e635aad28e23acb786294abb9d950ac7e.7z

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