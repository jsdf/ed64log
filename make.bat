REM download and extract to ../libftdi: https://sourceforge.net/projects/picusb/files/libftdi1-1.4git_devkit_x86_x64_14June2018.zip/download
gcc ed64log.c gopt.c -o ed64log -v -o windows/ed64log.exe -I../libftdi/include/libftdi   -L../libftdi/lib32/  -lusb-1.0 -lftdi1
