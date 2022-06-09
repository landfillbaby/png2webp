rem TODO: zlib 1.2.12 hardware crc32, pam, optimizations
pushd "%~dp0"
copy libpng\scripts\pnglibconf.h.prebuilt libpng\pnglibconf.h
del libpng\pngtest.c
cl.exe /nologo /std:c11 /W2 /Brepro /O2 /Ob3 /GL /Gw /Qpar /DNDEBUG ^
    /Ilibpng /Izlib /Ilibwebp /Ilibwebp\src /DWEBP_USE_THREAD ^
    %* png2webp.c libpng\png*.c zlib\*.c ^
    libwebp\src\enc\*.c libwebp\src\utils\*.c libwebp\src\dsp\*.c
cl.exe /nologo /std:c11 /W2 /Brepro /O2 /Ob3 /GL /Gw /Qpar /DNDEBUG ^
    /Ilibpng /Izlib /Ilibwebp /Ilibwebp\src /DWEBP_USE_THREAD ^
    /DFROMWEBP /Fewebp2png %* png2webp.c libpng\png*.c zlib\*.c ^
    libwebp\src\dec\*.c libwebp\src\utils\*.c libwebp\src\dsp\*.c
del *.obj
popd
