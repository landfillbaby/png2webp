# png2webp
Simple command-line batch PNG to WebP converter (and vice versa).

`png2webp` writes optimized lossless WebP versions of every PNG given,
equivalent to `cwebp -z 9`.

`webp2png` writes PNG versions of every WebP given, using default `libpng`
settings, since properly optimizing PNGs is an overcomplicated mess that
requires several different programs
(and even then they're bigger than the equivalent WebPs).

# Usage

    png2webp [-fv] [--] file.png ...
    png2webp [-p[fv] [--] [{infile.png|-} [outfile.webp|-]]]

For each `file.png`, outputs an equivalent `file.webp`

`-f`: Force overwrite of output files (has no effect on stdout, see `-p`).

`-v`: Be verbose.

`-p`: Default when no arguments are given.
      Work with a single file, allowing piping from stdin or to stdout,
      or using a different output filename to the input.
      `infile.png` and `outfile.webp` default to stdin or stdout respectively,
      or explicitly as `-`.
      Will return a usage error if stdin or stdout is used and is a terminal.

`webp2png` has the same syntax.

# Compiling
Just use your favourite C compiler:
* Link against `libwebp` and `libpng` (in either order), or even compile them
in at the same time for a tiny static executable.
* Turn on whatever optimization level you want (preferably more than none).

For example: GCC, dynamically linked:

    gcc -O2 -o png2webp png2webp.c -lwebp -lpng
    gcc -O2 -o webp2png webp2png.c -lwebp -lpng

## Compilation flags
Define these as preprocessor macros:

`USEGETOPT`: Use `getopt` for command-line parsing instead of a simple loop.

`NOTHREADS`: Use single-threaded WebP encoding/decoding.
I'm pretty sure it only uses 2 threads anyway.

`DEBUG_IDEC`: Use a 256 byte buffer for decoding WebP files,
instead of a 16 kibibyte one.

`PAM`: Read/write `netpbm` PAM files instead of PNGs.
Link `pam2webp` against `libnetpbm` instead of `libpng`.
`webp2pam` doesn't need `libnetpbm`, it constructs the file itself:
PAM if alpha is used, PPM otherwise.
They use and expect `.pam` file extensions even though they do PNMs too,
because I wrote them as an afterthought.

# Why?
I wanted a smaller, faster, and less platform dependent way to convert old
PNG images to the more efficient lossless WebP format,
than having to use a wrapper script with `cwebp` or `imagemagick` or something.

I don't care about lossy WebP stuff, so I wrote a program that doesn't encode
it at all, and uses maximum compression by default.

As stated above this makes very tiny static executables when globally optimized,
for platforms like Windows that don't provide `libwebp` or `libpng`.
