pushd "%~dp0"
copy libpng\scripts\pnglibconf.h.prebuilt libpng\pnglibconf.h
del libpng\pngtest.c
cl.exe /nologo /std:c11 /W2 /Brepro /O2 /Ob3 /GL /Gw /Qpar /DNDEBUG ^
    /Ilibpng /Izlib /Ilibwebp /Ilibwebp\src ^
    %* png2webp.c libpng\png*.c zlib\*.c ^
    libwebp\src\enc\*.c libwebp\src\utils\*.c libwebp\src\dsp\*.c
cl.exe /nologo /std:c11 /W2 /Brepro /O2 /Ob3 /GL /Gw /Qpar /DNDEBUG ^
    /Ilibpng /Izlib /Ilibwebp /Ilibwebp\src ^
    %* webp2png.c libpng\png*.c zlib\*.c ^
    libwebp\src\dec\*.c libwebp\src\utils\*.c libwebp\src\dsp\*.c
del *.obj
popd
