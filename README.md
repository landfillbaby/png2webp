# png2webp
simple batch PNG to WebP converter (and vice versa)

writes optimized lossless WebP versions of every PNG given on the command line

# usage

    png2webp [-fv] [--] file.png ...
    png2webp [-p[fv] [--] [{infile.png|-} [outfile.webp|-]]]

    for each file.png, outputs an equivalent file.webp

    -f: force overwrite of output files (has no effect on stdout, see below).
    -v: be verbose.
    -p: default without arguments.
        work with a single file, allowing piping from stdin or to stdout,
        or using a different output filename to the input.
        infile.png and outfile.webp default to stdin or stdout respectively,
        or explicitly as "-".
        will error if stdin/stdout is used and is a terminal.

webp2png works the same way, using default libpng settings,
since properly optimizing PNGs is an overcomplicated mess.

there's also versions that use Netpbm files instead of PNG:
PAM if alpha is used, or PPM otherwise, but uses and expects .pam file
extensions anyway because I wrote it as an afterthought

# compiling
compiling is easy:
just use your C compiler.
link against libwebp and libpng,
or even compile them in at the same time for a tiny static executable,
and turn on whatever optimization level you want (preferably more than none).
e.g. for GCC, dynamically linked:

    gcc -O2 -o png2webp png2webp.c -lwebp -lpng
    gcc -O2 -o webp2png webp2png.c -lpng -lwebp

for the PAM versions, just define PAM as a preprocessor macro,
and link pam2webp against libnetpbm instead of libpng
webp2pam doesn't need libnetpbm, it constructs the PAM file itself

# why?
I wanted a smaller, faster, and less platform dependent way to convert old
PNG images to the more efficient lossless WebP format,
than having to use a wrapper script with cwebp or imagemagick or something.

I don't care about lossy WebP stuff, so I wrote a program that doesn't use it
at all, and uses maximum compression by default.

As stated above this makes very tiny static execuables when globally optimized,
for platforms like Windows that don't provide libwebp or libpng.
