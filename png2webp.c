// vi: sw=2 tw=80
#ifndef _FILE_OFFSET_BITS
#define _FILE_OFFSET_BITS 64
#endif
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809
#endif
#ifdef P2WCONF
#include "p2wconf.h"
#endif
#ifndef VERSION
#define VERSION "v1.2.2"
#endif
#include "pun.h"
#include <inttypes.h>
#include <setjmp.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if SIZE_MAX < 0xffffffffu
#error "size_t isn't at least 32-bit"
#endif
#ifdef _WIN32
#ifndef _CRT_NONSTDC_NO_WARNINGS
#define _CRT_NONSTDC_NO_WARNINGS
#endif
#include <fcntl.h>
#include <io.h>
#define eprintf(...) fprintf(stderr, __VA_ARGS__)
#else
#include <unistd.h>
#define eprintf(...) dprintf(2, __VA_ARGS__)
#endif
#if !defined NOFOPENX && __STDC_VERSION__ < 201112
#define NOFOPENX
#endif
#ifdef NOFOPENX
#include <fcntl.h>
#include <sys/stat.h>
#ifndef O_BINARY
#define O_BINARY 0
#endif
#endif
#include "png.h"
#include "webp/decode.h"
#include "webp/encode.h"
#if (' ' == '\40' && '0' == '\60' && 'A' == '\101' && 'a' == '\141') \
    || (' ' == '\100' && '0' == '\360' && 'A' == '\301' && 'a' == '\201')
#define ASCII_OR_EBCDIC
#endif
static int help(void) {
  fputs("PNG2WebP " VERSION "\n\
\n\
Usage:\n\
png2webp [-refv] [--] INFILE ...\n\
png2webp -p[refv] [--] [INFILE [OUTFILE]]\n\
\n\
-p: Work with a single file, allowing piping from stdin or to stdout,\n\
    or using a different output filename to the input.\n\
    `INFILE` and `OUTFILE` default to stdin and stdout respectively,\n\
    or explicitly as \"-\".\n\
    Will show this message if stdin/stdout is used and is a terminal.\n\
-r: Convert from WebP to PNG instead.\n\
-e: Keep RGB data on pixels where alpha is 0. Always enabled for `-r`.\n\
-f: Force overwrite of output files (has no effect on stdout).\n\
-v: Be verbose.\n\
-t: Print a progress bar even when stderr isn't a terminal (not for `-r`).\n\
--: Explicitly stop parsing options.\n",
      stderr);
  return -1;
}
static bool exact, force, verbose, doprogress;
#define P(x, ...) eprintf(x "\n", __VA_ARGS__)
#define PV(...) (verbose ? P(__VA_ARGS__) : 0)
#define IP (ip ? ip : "<stdin>")
#define OP (op ? op : "<stdout>")
static FILE *openr(const char *ip) {
  if(!ip) return stdin;
  FILE *fp;
#ifdef NOFOPENX
  int fd = open(ip, O_RDONLY | O_BINARY);
  if(fd == -1) {
    perror("ERROR reading");
    return 0;
  }
  if(!(fp = fdopen(fd, "rb"))) {
    perror("ERROR reading");
    close(fd);
    return 0;
  }
#else
  if(!(fp = fopen(ip, "rb"))) {
    perror("ERROR reading");
    return 0;
  }
#endif
  return fp;
}
static FILE *openw(const char *op) {
  if(verbose) fputs("Encoding ...\n", stderr);
  if(!op) return stdout;
  FILE *fp;
#ifdef NOFOPENX
  int fd = open(op, O_WRONLY | O_BINARY | O_CREAT | (force ? O_TRUNC : O_EXCL),
#ifdef _WIN32
      S_IREAD | S_IWRITE
#else
      S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH
#endif
  );
  if(fd == -1) {
    perror("ERROR writing");
    return 0;
  }
  if(!(fp = fdopen(fd, "wb"))) {
    perror("ERROR writing");
    close(fd);
    unlink(op);
    return 0;
  }
#else
  if(!(fp = fopen(op, force ? "wb" : "wbx"))) {
    perror("ERROR writing");
    return 0;
  }
#endif
  return fp;
}
static size_t pnglen;
static void pngread(png_struct *p, U *d, size_t s) {
  if(!fread(d, s, 1u, png_get_io_ptr(p))) png_error(p, "I/O error");
  pnglen += s;
}
static void pngwrite(png_struct *p, U *d, size_t s) {
  if(!fwrite(d, s, 1u, png_get_io_ptr(p))) png_error(p, "I/O error");
  pnglen += s;
}
static void pngflush(png_struct *p) {
#ifdef DOFLUSH
  if(fflush(png_get_io_ptr(p))) png_error(p, "I/O error");
#else
  (void)p;
#endif
}
static PNG_NORETURN void pngrerr(png_struct *p, const char *s) {
  P("ERROR reading: %s", s);
  png_longjmp(p, 1);
}
static PNG_NORETURN void pngwerr(png_struct *p, const char *s) {
  P("ERROR writing: %s", s);
  png_longjmp(p, 1);
}
static void pngwarn(png_struct *p, const char *s) {
  (void)p;
  P("Warning: %s", s);
}
static int webpwrite(const U *d, size_t s, const WebPPicture *p) {
  return (int)fwrite(d, s, 1u, p->custom_ptr);
}
static int progress(int percent, const WebPPicture *x) {
  (void)x;
  char h[64];
  memset(h, '#', 64u);
  eprintf(
      "\r[%-64.*s] %u%%", (unsigned)percent * 16u / 25u, h, (unsigned)percent);
  return 1;
}
static bool p2w(const char *ip, const char *op) {
  P("%s -> %s ...", IP, OP);
  FILE *fp = openr(ip);
  if(!fp) return 1;
  U4 *b = 0;
  png_info *n = 0;
  const char *k[] = {"Out of memory",
      "???", // oom flushing bitstream, unused in libwebp
      "???", // null param
      "Broken config, file a bug report",
      "???", // image too big (checked on PNG input)
      "???", "???", "I/O error", "???", // lossy stuff
      "???"}; // canceled
  png_struct *p
      = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, pngrerr, pngwarn);
  if(!p) {
    P("ERROR reading: %s", *k);
    goto p2w_close;
  }
  n = png_create_info_struct(p);
  if(!n) {
    P("ERROR reading: %s", *k);
    goto p2w_close;
  }
  if(setjmp(png_jmpbuf(p))) {
  p2w_close:
    fclose(fp);
    png_destroy_read_struct(&p, &n, 0);
  p2w_free:
    if(b) free(b);
    return 1;
  }
  pnglen = 0;
#define E(x) png_set_##x(p)
#define S(x, ...) png_set_##x(p, __VA_ARGS__)
  S(read_fn, fp, pngread);
  png_read_info(p, n);
  U4 width, height;
  int bitdepth, colortype;
  png_get_IHDR(p, n, &width, &height, &bitdepth, &colortype, 0, 0, 0);
  if(width > 16383u || height > 16383u) {
    P("ERROR reading: Image too big (%" PRIu32 " x %" PRIu32
      ", max. 16383 x 16383 px)",
	width, height);
    goto p2w_close;
  }
  if((unsigned)bitdepth > 8u) P("Warning: %s", "Downsampling to 8-bit");
  bool trns = png_get_valid(p, n, PNG_INFO_tRNS);
  int32_t gamma = 45455;
  if(png_get_valid(p, n, PNG_INFO_sRGB) || png_get_gAMA_fixed(p, n, &gamma)) {
    if(gamma != 45455)
      P("Warning: Nonstandard gamma: %" PRIu32 ".%05" PRIu32,
	  (U4)gamma / 100000u, (U4)gamma % 100000u);
    S(gamma_fixed, 220000, gamma);
  }
  E(scale_16);
  E(expand);
  E(gray_to_rgb);
  E(packing);
  if(hl(1u) != 1u) { // TODO: test big-endian
    E(swap_alpha);
    S(add_alpha, 255u, PNG_FILLER_BEFORE);
  } else {
    E(bgr);
    S(add_alpha, 255u, PNG_FILLER_AFTER);
  }
  unsigned passes = (unsigned)E(interlace_handling);
  png_read_update_info(p, n);
#ifndef NDEBUG
  size_t rowbytes = png_get_rowbytes(p, n);
  if(rowbytes != width * (size_t)4u) {
    P("ERROR reading: rowbytes is %zu, should be %zu", rowbytes,
	width * (size_t)4u);
    goto p2w_close;
  }
#endif
  b = malloc(width * height * (size_t)4u);
  if(!b) {
    P("ERROR reading: %s", *k);
    goto p2w_close;
  }
  for(unsigned x = passes; x; x--) {
    U *w = (U *)b;
    for(unsigned y = height; y; y--) {
      png_read_row(p, w, 0);
      w += width * (size_t)4u;
    }
  }
  png_read_end(p, 0);
  fclose(fp);
  png_destroy_read_struct(&p, &n, 0);
  const char *f[] = {"grayscale", "???", "RGB", "paletted", "grayscale + alpha",
      "???", "RGBA"};
  PV("Input info:\nDimensions: %" PRIu32 " x %" PRIu32
     "\nSize: %zu bytes (%.15g bpp)\nFormat: %u-bit %s%s%s",
      width, height, pnglen, (double)pnglen * 8u / (width * height),
      (unsigned)bitdepth, f[(unsigned)colortype],
      trns ? ", with transparency" : "", passes > 1u ? ", interlaced" : "");
  WebPConfig c;
  if(!WebPConfigPreset(&c, WEBP_PRESET_ICON, 100u)) {
    P("ERROR writing: %s", k[3]);
    goto p2w_free;
  }
  if(!(fp = openw(op))) goto p2w_free;
  c.lossless = 1;
  c.method = 6;
#ifndef NOTHREADS
  c.thread_level = 1; // doesn't seem to affect output
#endif
  c.exact = exact;
  WebPAuxStats s;
  WebPPicture o = {1, .width = (int)width, (int)height, .argb = b,
      .argb_stride = (int)width, .writer = webpwrite, .custom_ptr = fp,
      .stats = verbose ? &s : 0, .progress_hook = doprogress ? progress : 0};
  if(doprogress) eprintf("[%-64.*s] %u%%", 0, "", 0u);
  trns = (trns || (colortype & PNG_COLOR_MASK_ALPHA))
      && WebPPictureHasTransparency(&o);
  int r = WebPEncode(&c, &o);
  if(doprogress) fputs("\n", stderr);
  if(!r) {
    P("ERROR writing: %s", k[o.error_code - 1u]);
    fclose(fp);
  p2w_rm:
    if(op) unlink(op);
    goto p2w_free;
  }
  if(fclose(fp)) {
    perror("ERROR writing");
    goto p2w_rm;
  }
  free(b);
#define F s.lossless_features
#define C s.palette_size
  PV("Output info:\nSize: %u bytes (%.15g bpp)\n\
Header size: %u, image data size: %u\nUses alpha: %s\n\
Precision bits: histogram=%u prediction=%u cross-color=%u cache=%u\n\
Lossless features:%s%s%s%s\nColors: %s%u",
      (unsigned)s.coded_size,
      (double)(unsigned)s.coded_size * 8u / (U4)(o.width * o.height),
      (unsigned)s.lossless_hdr_size, (unsigned)s.lossless_data_size,
      trns ? "yes" : "no", (unsigned)s.histogram_bits,
      (unsigned)s.transform_bits, (unsigned)s.cross_color_transform_bits,
      (unsigned)s.cache_bits, F ? F & 1u ? " prediction" : "" : " none",
      F && F & 2u ? " cross-color" : "", F && F & 4u ? " subtract-green" : "",
      F && F & 8u ? " palette" : "", C ? "" : ">", C ? (unsigned)C : 256u);
  return 0;
}
static bool w2p(const char *ip, const char *op) {
  P("%s -> %s ...", IP, OP);
  FILE *fp = openr(ip);
  if(!fp) return 1;
  U4 i[3];
  const char *k[] = {"Out of memory", "Broken config, file a bug report",
      "Invalid WebP", "???", "???", "???", "I/O error"};
  // ^ unsupported feature, suspended, canceled
#define R(x, y) !fread(x, y, 1u, fp)
  if(R(i, 12u)) {
    P("ERROR reading: %s", k[6]);
    fclose(fp);
    return 1;
  }
  if(*i != hl(0x46464952u) || i[2] != hl(0x50424557u)) {
    P("ERROR reading: %s", k[2]);
    fclose(fp);
    return 1;
  }
  U4 l = lh(i[1]) + 8u; // RIFF header size
  if(l < 28u || l > 0xfffffffeu) {
    P("ERROR reading: %s", k[2]);
    fclose(fp);
    return 1;
  }
  U *x = malloc(l);
  if(!x) {
    P("ERROR reading: %s", *k);
    fclose(fp);
    return 1;
  }
  memcpy(x, i, 12u); // should optimize out
  if(
#if defined __ANDROID__ && __ANDROID_API__ < 34
      l > 0x8000000bu // https://issuetracker.google.com/240139009
	  ? R(x + 12u, (size_t)0x7fffffffu)
	      || R(x + 0x8000000bu, l - (size_t)0x8000000bu)
	  :
#endif
	  R(x + 12u, l - (size_t)12u)) {
    P("ERROR reading: %s", k[6]);
    fclose(fp);
    free(x);
    return 1;
  }
  fclose(fp);
#if defined LOSSYISERROR || defined NOTHREADS
  WebPBitstreamFeatures I;
#else
  WebPDecoderConfig c = {.options.use_threads = 1};
#define I c.input
#endif
  VP8StatusCode r = WebPGetFeatures(x, l, &I);
  if(r) {
    P("ERROR reading: %s", k[r - 1u]);
    free(x);
    return 1;
  }
#define V I.format
#define W ((U4)I.width)
#define H ((U4)I.height)
#define A I.has_alpha
#ifdef LOSSYISERROR
#define FMTSTR
#define FMTARG
#else
  const char *f[] = {"undefined/mixed", "lossy", "lossless"};
#define FMTSTR "\nFormat: %s"
#define FMTARG , f[V]
#endif
  PV("Input info:\nDimensions: %" PRIu32 " x %" PRIu32 "\nSize: %" PRIu32
     " bytes (%.15g bpp)\nUses alpha: %s" FMTSTR,
      W, H, l, (double)l * 8u / (W * H), A ? "yes" : "no" FMTARG);
  if(I.has_animation) {
    P("ERROR reading: %s", "Unsupported feature: animation");
    free(x);
    return 1;
  }
#ifdef LOSSYISERROR
  if(V != 2u) {
    P("ERROR reading: %s", "Unsupported feature: lossy compression");
    free(x);
    return 1;
  }
#endif
#define B (3u + (unsigned)A) // assume 0 or 1
  U *b = malloc(W * H * B);
  if(!b) {
    P("ERROR reading: %s", *k);
    free(x);
    return 1;
  }
#if defined LOSSYISERROR || defined NOTHREADS
  if(!(A ? WebPDecodeRGBAInto : WebPDecodeRGBInto)(
	 x, l, b, W * H * B, (int)(W * B))) {
    P("ERROR reading: %s", k[2]);
    free(b);
    free(x);
    return 1;
  }
#else
  c.output.colorspace = A ? MODE_RGBA : MODE_RGB;
  c.output.is_external_memory = 1;
#define D c.output.u.RGBA
  D.rgba = b;
  D.stride = (int)(W * B);
  D.size = W * H * B;
  r = WebPDecode(x, l, &c);
  if(r) {
    P("ERROR reading: %s", k[r - 1u]);
    free(b);
    free(x);
    return 1;
  }
#endif
  free(x);
  if(!(fp = openw(op))) {
    free(b);
    return 1;
  }
  png_info *n = 0;
  png_struct *p
      = png_create_write_struct(PNG_LIBPNG_VER_STRING, 0, pngwerr, pngwarn);
  if(!p) {
    P("ERROR writing: %s", *k);
    goto w2p_close;
  }
  n = png_create_info_struct(p);
  if(!n) {
    P("ERROR writing: %s", *k);
    goto w2p_close;
  }
  if(setjmp(png_jmpbuf(p))) {
  w2p_close:
    fclose(fp);
  w2p_rm:
    if(op) unlink(op);
    png_destroy_write_struct(&p, &n);
    free(b);
    return 1;
  }
  pnglen = 0u;
  S(write_fn, fp, pngwrite, pngflush);
  S(filter, 0, PNG_ALL_FILTERS);
  S(compression_level, 9);
  // S(compression_memlevel, 9);
  S(IHDR, n, W, H, 8, A ? 6 : 2, 0, 0, 0);
  png_write_info(p, n);
  U *w = b;
  for(unsigned y = H; y; y--) {
    png_write_row(p, w);
    w += W * B;
  }
  png_write_end(p, n);
  if(fclose(fp)) {
    perror("ERROR writing");
    goto w2p_rm;
  }
  png_destroy_write_struct(&p, &n);
  free(b);
  PV("Output info:\nSize: %zu bytes (%.15g bpp)\nFormat: 8-bit %s", pnglen,
      (double)pnglen * 8u / (W * H), A ? "RGBA" : "RGB");
  return 0;
}
int main(int sargc, char **argv) {
  unsigned argc = (unsigned)sargc;
  {
    const U4 x = lh(u4("4321"));
    if(x == u4("1234"))
      P("Warning: %s", "Big-endian support is untested"); // TODO
    else if(x != u4("4321")) {
      P("ERROR: system is mixed-endian (%.4s)", (const char *)&x);
      return 1;
    }
  }
  bool pipe = 0, usestdin = 0, usestdout = 0, reverse = 0;
#ifdef USEGETOPT
  for(int c; (c = getopt(sargc, argv, ":prefvt")) != -1;)
    switch(c)
#else
  while(--argc && **++argv == '-' && argv[0][1])
    if(argv[0][1] == '-') {
      if(argv[0][2]) return help();
      argc--;
      argv++;
      break;
    } else
      while(*++*argv)
	switch(**argv)
#endif
    {
      case 'p': pipe = 1; break;
      case 'r': reverse = 1; break;
      case 'e': exact = 1; break;
      case 'f': force = 1; break;
      case 'v': verbose = 1; break;
      case 't': doprogress = 1; break;
      default: return help();
    }
#ifdef USEGETOPT
  argc -= (unsigned)optind;
  argv += (unsigned)optind;
#endif
#define PIPEARG(x) (argc <= x || (*argv[x] == '-' && !argv[x][1]))
  if(pipe) {
    if(argc > 2u || ((usestdin = PIPEARG(0u)) && isatty(0))
	|| ((usestdout = PIPEARG(1u)) && isatty(1)))
      return help();
#ifdef _WIN32
    if(usestdin) setmode(0, O_BINARY);
    if(usestdout) setmode(1, O_BINARY);
#endif
    if(!reverse && !doprogress) doprogress = isatty(2);
    return (reverse ? w2p : p2w)(usestdin ? 0 : *argv, usestdout ? 0 : argv[1]);
  }
  if(!argc) return help();
  bool ret = 0;
  if(reverse)
    for(; argc; argc--, argv++) {
      size_t len = strlen(*argv);
#define K(x, y) (argv[0][len - x] == y)
      if(len > 4u && K(5u, '.') &&
#ifdef ASCII_OR_EBCDIC
	  (u4(*argv + len - 4u) | u4("    ")) == (u4("webp") | u4("    "))
#else
#define J(x, y) (K(x, *y) || K(x, y[1]))
	  J(4u, "wW") && J(3u, "eE") && J(2u, "bB") && J(1u, "pP")
#endif
      )
	len -= 5u;
#if defined __STDC_NO_VLA__ && !defined NOVLA
#define NOVLA
#endif
#ifdef NOVLA
      char *op = malloc(len + 5u);
      if(!op) {
	P("ERROR adding %s extension to %s: %s", ".png", *argv,
	    "Out of memory");
	return 1;
      }
#elif defined __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wvla"
      char op[len + 5u];
#pragma GCC diagnostic pop
#else
      char op[len + 5u];
#endif
      memcpy(op + len, ".png", 5u);
      memcpy(op, *argv, len); // the only real memcpy
      ret = w2p(*argv, op) || ret;
#ifdef NOVLA
      free(op);
#endif
    }
  else {
    if(!doprogress) doprogress = isatty(2);
    for(; argc; argc--, argv++) {
      size_t len = strlen(*argv);
      if(len > 3u &&
#ifdef ASCII_OR_EBCDIC
	  (u4(*argv + len - 4u) | u4("\0   ")) == (u4(".png") | u4("\0   "))
#else
	  K(4u, '.') && J(3u, "pP") && J(2u, "nN") && J(1u, "gG")
#endif
      )
	len -= 4u;
#ifdef NOVLA
      char *op = malloc(len + 6u);
      if(!op) {
	P("ERROR adding %s extension to %s: %s", ".webp", *argv,
	    "Out of memory");
	return 1;
      }
#elif defined __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wvla"
      char op[len + 6u];
#pragma GCC diagnostic pop
#else
      char op[len + 6u];
#endif
      memcpy(op + len, ".webp", 6u);
      memcpy(op, *argv, len); // the only real memcpy
      ret = p2w(*argv, op) || ret;
#ifdef NOVLA
      free(op);
#endif
    }
  }
  return ret;
}
