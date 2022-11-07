# PNG2WebP
Simple command-line batch PNG to WebP converter (and vice versa).

`png2webp` writes optimized lossless WebP versions of every PNG given,
equivalent to `cwebp -z 9`.

# Usage

    png2webp [-refv-] INFILE ...
    png2webp -p[refv-] [INFILE [OUTFILE]]

`-p`: Work with a single file, allowing Piping from stdin or to stdout,
    or using a different output filename to the input.
    `INFILE` and `OUTFILE` default to stdin and stdout respectively,
    or explicitly as `-`.
    Will show this message if stdin/stdout is used and is a terminal.

`-r`: Convert from WebP to PNG instead, using default `libpng` settings,
    since properly optimizing PNGs is an overcomplicated mess
    that requires several different programs
    (and even then they're bigger than the equivalent WebPs).

`-e`: Keep RGB data on pixels where alpha is 0.
    Equivalent to `cwebp -z 9 -exact`. Always enabled for `-r`.

`-f`: Force overwrite of output files (has no effect on stdout).

`-v`: Be verbose.

`-t`: Print a progress bar even when stderr isn't a terminal (not for `-r`).

`--`: Explicitly stop parsing options.

## Drag and drop
The `.bat` (Windows) and `.sh` (Unix-like) wrappers do 2 things:
* Specify `-v --` (`-rv --` in the case of `webptopng.*`).
* Wait for user input if errors happen.

Just keep them in the same folder as the program itself,
and drag the image files onto the corresponding batch file.
Rename the wrappers if you want, but don't rename the program.

# Compiling
## Prebuilt libpng and libwebp
Install `libpng` and `libwebp`, then just run:

    cc -O3 -DNDEBUG -s -o png2webp png2webp.c -lpng -lwebp

Or on Windows MSVC:

    cl.exe /nologo /std:c11 /O2 /Ob3 /DNDEBUG /I libpng /I libwebp/src png2webp.c libpng16.lib zlib.lib libwebp.lib

## Static build
Download the sources for libpng, zlib, and libwebp,
and place them in the supplied folders, or run

    git submodule update --init --depth 1

Then run `./configure && make`.

`./configure --help` to see some optional flags.

### For Windows
Optionally run `make png2webp_timestamped`
to use the git commit timestamp instead of 1/1/1970.

## Problems?
In either case, if you get any warnings or errors, just
[open an issue](https://github.com/landfillbaby/png2webp/issues/new)
and put the errors in a code block in the comment box:

    ```
    errors go here
    ```

## Compilation flags
Define these as preprocessor macros:

`NOFOPENX`: If C11's `fopen("wbx")` parameter isn't supported on your system,
problems happen without `-f`: system errors, overwriting anyway, etc.
It's undefined behavior. This forces a workaround, used automatically
when compiling with no C11 support, with POSIX `open()` and `fdopen()`.

`NOVLA`: Use `malloc()`/`free()` instead of a C99 variable-length array
for output filenames in batch mode. May be on by default in C11 onwards.

`USEGETOPT`: Use `getopt` for command-line parsing instead of a simple loop.

`NOTHREADS`: Use single-threaded WebP encoding/decoding.
I'm pretty sure it only uses 2 threads anyway.

`LOSSYISERROR`: Give an error when trying to decode lossy WebP files.
This means no lossy VP8 code in static globally optimized builds!

# Why?
I wanted a smaller, faster, and less platform dependent way to convert old
PNG images to the more efficient lossless WebP format,
than having to use a wrapper script with `cwebp` or `imagemagick` or something.

I don't care about lossy WebP stuff, so I wrote a program that doesn't encode
it at all, and uses maximum compression by default.

As stated above this makes tiny static executables when globally optimized,
for platforms like Windows that don't provide `libwebp` or `libpng`.
