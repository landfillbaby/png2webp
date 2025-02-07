# 2025 update: this was dumb. just use `cwebp` and `dwebp` like google wants you to

# PNG2WebP
Simple command-line batch PNG to WebP converter (and vice versa).

`png2webp` writes optimized lossless WebP versions of every PNG given,
equivalent to `cwebp -z 9`.

# Warning
The following data will be lost in conversion:
- Animation. Support may be added in a future release.
- Metadata. Ditto.
- Color depth if an input PNG isn't 8 bits per channel,
  or if its gamma value isn't the sRGB standard of 2.2.
  These are limits of the WebP format. The program will issue a warning.
- Color values of invisible pixels (alpha = 0) in input PNGs,
  unless `-e` flag is given.
- Color palettes. The colors themselves are preserved,
  but the knowledge of previous presence or absence of a palette,
  including its order and any unused colors, are lost.
  Palettes are part of the WebP format,
  and paletted PNGs will probably be indirectly converted into paletted WebPs,
  but palettes are not exposed for reading or writing by `libwebp`.
  [I sent a feature request in 2019](https://crbug.com/webp/437)
  but it looks like they won't be implementing it.
- Other minutiae of the internal format of the input images.

# Usage

    png2webp [-refv] [--] INFILE ...
    png2webp -p[refv] [--] [INFILE [OUTFILE]]

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
The
[`png2webp.bat`](https://github.com/landfillbaby/png2webp/raw/main/png2webp.bat)
and
[`webptopng.bat`](https://github.com/landfillbaby/png2webp/raw/main/webptopng.bat)
(Windows) and
[`png2webp.sh`](https://github.com/landfillbaby/png2webp/raw/main/png2webp.sh)
and
[`webptopng.sh`](https://github.com/landfillbaby/png2webp/raw/main/webptopng.sh)
 (Unix-like) wrappers do 2 things:
* Specify `-v --` (`-rv --` in the case of `webptopng.*`).
* Wait for user input if errors happen.

Just keep them in the same folder as the program itself,
and drag the image files onto the corresponding batch file.
Rename the wrappers if you want, but don't rename the program.

# Compiling
Download the sources for libpng, zlib, and libwebp,
and place them in the supplied folders, or run

    git submodule update --init --depth 1

Then run `./configure`. (Try `./configure --help` to see some optional flags.)

## Static
Simply run `make` or `make png2webp`.

## Dynamic
To dynamically link against `libpng` and `libwebp`, either
install them through your package manager, or run `make submodules_dynamic`.

Then run `make png2webp_dynamic`.

## For Windows
Optionally run `make png2webp_timestamped`
(or `make png2webp_dynamic_timestamped`), to use the git commit timestamp
instead of 1/1/1970 in the `.exe`'s PE32(+) header.

# Installing
- `make install` to install the static build.
- `make install_dynamic`
to install the dynamic build as `png2webp_dynamic` (rename it if you want).
- `make install_submodules_dynamic` to install `zlib`, `libpng`, and `libwebp`.

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
