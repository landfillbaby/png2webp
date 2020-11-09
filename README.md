# png2webp
Simple command-line batch PNG to WebP converter (and vice versa).

`png2webp` writes optimized lossless WebP versions of every PNG given,
equivalent to `cwebp -z 9`.

`webp2png` writes PNG versions of every WebP given, using default `libpng`
settings, since properly optimizing PNGs is an overcomplicated mess that
requires several different programs
(and even then they're bigger than the equivalent WebPs).

# Usage

    png2webp [-befv-] infile.png ...
    png2webp [-pefv-] [{infile.png|-} [outfile.webp|-]]

`-b`: Default when at least 1 file is given.
    Work with many input files (Batch mode).
    Constructs output filenames by removing the `.png` extension if possible,
    and appending `.webp`.

`-p`: Default when no files are given.
    Work with a single file, allowing Piping from stdin or to stdout,
    or using a different output filename to the input.
    `infile.png` and `outfile.webp` default to stdin and stdout respectively,
    or explicitly as `-`.
    Will error if stdin/stdout is used and is a terminal.

`-e`: Keep RGB data on pixels where alpha is 0.
    Equivalent to `cwebp -z 9 -exact`.

`-f`: Force overwrite of output files (has no effect on stdout).

`-v`: Be verbose.

`--`: Explicitly stop parsing options.

`webp2png` has the same syntax, but lacks `-e` as output is always exact.

For drag-and-drop usage, the provided `.bat` (Windows) and `.sh` (Unix-like)
wrappers do 2 things:
* Specify `-bv --`.
* Wait for user input if errors happen.

# Compiling
## Windows with Visual Studio / MSVC
* Download the latest release sources of libpng, libwebp, and zlib,
and extract them to this directory.
* Rename the folders to remove the version numbers.
* Search the start menu for "Native Tools Command Prompt",
and select the appropriate one for your system.
* Run `compile_msvc.bat` from this prompt
(you should be able to click and drag it into the window).
* Put the resulting `.exe`s somewhere on your `%PATH%`.

## Other
Install `libwebp` and `libpng`, then just run `make install`.

## Problems?
In either case, if you get any warnings or errors, just
[open an issue](https://github.com/landfillbaby/png2webp/issues/new)
and put the errors in a code block in the comment box:

    ```
    errors go here
    ```

## Compilation flags
Define these as preprocessor macros:

`NOFOPENX`: Defined automatically on compilers without C11 support.
If C11's fopen() "wbx" parameter isn't supported on your system,
problems happen without `-f`.
(Errors on Windows/MSVC, other platforms may overwrite.)
This attempts a workaround with `open()` and `fdopen()`.
Honestly a standard from 9 years ago should be supported by now! :(

`USEGETOPT`: Use `getopt` for command-line parsing instead of a simple loop.

`NOTHREADS`: Use single-threaded WebP encoding/decoding.
I'm pretty sure it only uses 2 threads anyway.

`IDEC_BUFSIZE`: Define this to a positive integer, the size in bytes of the
buffer for decoding WebP files. Defaults to 65536 (64 kibibytes).

`LOSSYISERROR`: Give an error when trying to decode lossy WebP files.
This means no lossy VP8 code in static globally optimized builds!

`PAM`: Read/write `netpbm` PAM files instead of PNGs.
Link `pam2webp` against `libnetpbm` instead of `libpng`.
`webp2pam` doesn't need `libnetpbm`, it constructs the output file itself.

# Why?
I wanted a smaller, faster, and less platform dependent way to convert old
PNG images to the more efficient lossless WebP format,
than having to use a wrapper script with `cwebp` or `imagemagick` or something.

I don't care about lossy WebP stuff, so I wrote a program that doesn't encode
it at all, and uses maximum compression by default.

As stated above this makes very tiny static executables when globally optimized,
for platforms like Windows that don't provide `libwebp` or `libpng`.
