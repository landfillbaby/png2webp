// anti-copyright Lucy Phipps 2022
// vi: sw=2 tw=80
#ifdef P2WCONF
#include "p2wconf.h"
#endif
#ifndef VERSION
#define VERSION "v1.2.1"
#endif
#define _FILE_OFFSET_BITS 64
#include "pun.h"
#include <inttypes.h>
#include <setjmp.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if SIZE_MAX < 0xffffffff
#error "size_t isn't at least 32-bit"
#endif
#ifdef _WIN32
#define _CRT_NONSTDC_NO_WARNINGS
#include <fcntl.h>
#include <io.h>
#else
#include <unistd.h>
#endif
#if !defined NOFOPENX && __STDC_VERSION__ < 201112L
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
#define P(x, ...) fprintf(stderr, x "\n", __VA_ARGS__)
#define PV(...) (verbose ? P(__VA_ARGS__) : 0)
#define IP (ip ? ip : "<stdin>")
#define OP (op ? op : "<stdout>")
static FILE *openr(char *ip) {
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
static FILE *openw(char *op) {
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
static void pngread(png_struct *p, uint8_t *d, size_t s) {
  if(!fread(d, s, 1, png_get_io_ptr(p))) png_error(p, "I/O error");
  pnglen += s;
}
static void pngwrite(png_struct *p, uint8_t *d, size_t s) {
  if(!fwrite(d, s, 1, png_get_io_ptr(p))) png_error(p, "I/O error");
  pnglen += s;
}
static void pngflush(png_struct *p) {
#ifdef DOFLUSH
  fflush(png_get_io_ptr(p));
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
static int webpwrite(const uint8_t *d, size_t s, const WebPPicture *p) {
  return (int)fwrite(d, s, 1, p->custom_ptr);
}
static int progress(int percent, const WebPPicture *x) {
  (void)x;
  char h[64];
  memset(h, '#', 64);
  fprintf(stderr, "\r[%-64.*s] %u%%", (unsigned)percent * 16 / 25, h, percent);
  return 1;
}
static bool p2w(char *ip, char *op) {
  P("%s -> %s ...", IP, OP);
  FILE *fp = openr(ip);
  if(!fp) return 1;
  uint32_t *b = 0;
  png_info *n = 0;
  char *k[] = {"Out of memory",
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
  png_set_read_fn(p, fp, pngread);
  png_read_info(p, n);
  uint32_t width, height;
  int bitdepth, colortype;
  png_get_IHDR(p, n, &width, &height, &bitdepth, &colortype, 0, 0, 0);
  if(width > 16383 || height > 16383) {
    P("ERROR reading: Image too big (%" PRIu32 " x %" PRIu32
      ", max. 16383 x 16383 px)",
	width, height);
    goto p2w_close;
  }
  if((unsigned)bitdepth > 8) P("Warning: %s", "Downsampling to 8-bit");
  bool trns = png_get_valid(p, n, PNG_INFO_tRNS);
  int32_t gamma = 45455;
  if(png_get_valid(p, n, PNG_INFO_sRGB) || png_get_gAMA_fixed(p, n, &gamma)) {
    if(gamma != 45455)
      P("Warning: Nonstandard gamma: %.5g", (uint32_t)gamma / 1e5);
    png_set_gamma_fixed(p, 22e4, gamma);
  }
#define S(x) png_set_##x(p)
  S(scale_16);
  S(expand);
  S(gray_to_rgb);
  S(packing);
  if(*(uint8_t *)&(uint16_t){1}) {
    S(bgr);
    png_set_add_alpha(p, 255, PNG_FILLER_AFTER);
  } else { // TODO: see big-endian below
    S(swap_alpha);
    png_set_add_alpha(p, 255, PNG_FILLER_BEFORE);
  }
  unsigned passes = (unsigned)S(interlace_handling);
  png_read_update_info(p, n);
#ifndef NDEBUG
  size_t rowbytes = png_get_rowbytes(p, n);
  if(rowbytes != (size_t)4 * width) {
    P("ERROR reading: rowbytes is %zu, should be %zu", rowbytes,
	(size_t)4 * width);
    goto p2w_close;
  }
#endif
  b = malloc(width * height * 4);
  if(!b) {
    P("ERROR reading: %s", *k);
    goto p2w_close;
  }
  for(unsigned x = passes; x; x--) {
    uint8_t *w = (uint8_t *)b;
    for(unsigned y = height; y; y--) {
      png_read_row(p, w, 0);
      w += width * 4;
    }
  }
  png_read_end(p, 0);
  fclose(fp);
  png_destroy_read_struct(&p, &n, 0);
  char *f[] = {"grayscale", "???", "RGB", "paletted", "grayscale + alpha",
      "???", "RGBA"};
  PV("Input info:\nDimensions: %" PRIu32 " x %" PRIu32
     "\nSize: %zu bytes (%.15g bpp)\nFormat: %u-bit %s%s%s",
      width, height, pnglen, (double)pnglen * 8 / (width * height), bitdepth,
      f[(unsigned)colortype], trns ? ", with transparency" : "",
      passes > 1 ? ", interlaced" : "");
  WebPConfig c;
  if(!WebPConfigPreset(&c, WEBP_PRESET_ICON, 100)) {
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
  if(doprogress) fprintf(stderr, "[%-64.*s] %u%%", 0, "", 0);
  trns = (trns || (colortype & PNG_COLOR_MASK_ALPHA))
      && WebPPictureHasTransparency(&o);
  int r = WebPEncode(&c, &o);
  if(doprogress) fputc('\n', stderr);
  if(!r) {
    P("ERROR writing: %s", k[o.error_code - 1]);
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
Precision bits: histogram=%u transform=%u cache=%u\n\
Lossless features:%s%s%s%s\nColors: %s%u",
      s.lossless_size,
      (unsigned)s.lossless_size * 8. / (uint32_t)(o.width * o.height),
      s.lossless_hdr_size, s.lossless_data_size, trns ? "yes" : "no",
      s.histogram_bits, s.transform_bits, s.cache_bits,
      F ? F & 1 ? " prediction" : "" : " none",
      F && F & 2 ? " cross-color" : "", F && F & 4 ? " subtract-green" : "",
      F && F & 8 ? " palette" : "", C ? "" : ">", C ? C : 256);
  return 0;
}
static bool w2p(char *ip, char *op) {
  P("%s -> %s ...", IP, OP);
  FILE *fp = openr(ip);
  if(!fp) return 1;
  uint8_t i[12];
  char *k[] = {"Out of memory", "Broken config, file a bug report",
      "Invalid WebP", "???", "???", "???", "I/O error"};
  // ^ unsupported feature, suspended, canceled
#define R(x, y) !fread(x, y, 1, fp)
  if(R(i, 12)) {
    P("ERROR reading: %s", k[6]);
    fclose(fp);
    return 1;
  }
  if(u4(i) != u4("RIFF") || u4(i + 8) != u4("WEBP")) {
    P("ERROR reading: %s", k[2]);
    fclose(fp);
    return 1;
  }
  // RIFF header size
  uint32_t l = (uint32_t)(i[4] | (i[5] << 8) | (i[6] << 16) | (i[7] << 24)) + 8;
  if(l < 28 || l > 0xfffffffe) {
    P("ERROR reading: %s", k[2]);
    fclose(fp);
    return 1;
  }
  uint8_t *x = malloc(l);
  if(!x) {
    P("ERROR reading: %s", *k);
    fclose(fp);
    return 1;
  }
  memcpy(x, i, 12); // should optimize out
  if(
#if defined __ANDROID__ && __ANDROID_API__ < 34
      l - 12 > 0x7fffffff // https://issuetracker.google.com/240139009
	  ? R(x + 12, 0x7fffffff) || R(x + 12 + 0x7fffffff, l - 12 - 0x7fffffff)
	  :
#endif
	  R(x + 12, l - 12)) {
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
    P("ERROR reading: %s", k[r - 1]);
    free(x);
    return 1;
  }
#define V I.format
#define W ((uint32_t)I.width)
#define H ((uint32_t)I.height)
#define A I.has_alpha
#ifdef LOSSYISERROR
#define FMTSTR
#define FMTARG
#else
  char *f[] = {"undefined/mixed", "lossy", "lossless"};
#define FMTSTR "\nFormat: %s"
#define FMTARG , f[V]
#endif
  PV("Input info:\nDimensions: %" PRIu32 " x %" PRIu32 "\nSize: %" PRIu32
     " bytes (%.15g bpp)\nUses alpha: %s" FMTSTR,
      W, H, l, l * 8. / (W * H), A ? "yes" : "no" FMTARG);
  if(I.has_animation) {
    P("ERROR reading: %s", "Unsupported feature: animation");
    free(x);
    return 1;
  }
#ifdef LOSSYISERROR
  if(V != 2) {
    P("ERROR reading: %s", "Unsupported feature: lossy compression");
    free(x);
    return 1;
  }
#endif
#define B ((unsigned)(3 + A))
  uint8_t *b = malloc(W * H * B);
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
    P("ERROR reading: %s", k[r - 1]);
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
  pnglen = 0;
  png_set_write_fn(p, fp, pngwrite, pngflush);
  png_set_filter(p, 0, PNG_ALL_FILTERS);
  png_set_compression_level(p, 9);
  // png_set_compression_memlevel(p, 9);
  png_set_IHDR(p, n, W, H, 8, A ? 6 : 2, 0, 0, 0);
  png_write_info(p, n);
  uint8_t *w = b;
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
      (double)pnglen * 8 / (W * H), A ? "RGBA" : "RGB");
  return 0;
}
int main(int sargc, char **argv) {
  unsigned argc = (unsigned)sargc;
  { // should optimize out
    uint32_t endian = u4("\xAA\xBB\xCC\xDD");
    if(endian == 0xAABBCCDD)
      P("Warning: %s", "Big-endian support is untested"); // TODO
    else if(endian != 0xDDCCBBAA) {
      P("ERROR: 32-bit mixed-endianness (%" PRIX32 ") not supported", endian);
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
    if(argc > 2 || ((usestdin = PIPEARG(0)) && isatty(0))
	|| ((usestdout = PIPEARG(1)) && isatty(1)))
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
      if(len > 4 && argv[0][len - 5] == '.'
	  && (u4(*argv + len - 4) | u4("    ")) == u4("webp"))
	len -= 5;
#if defined __STDC_NO_VLA__ && !defined NOVLA
#define NOVLA
#endif
#ifdef NOVLA
      char *op = malloc(len + 5);
      if(!op) {
	P("ERROR adding %s extension to %s: Out of memory", ".png", *argv);
	return 1;
      }
#elif defined __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wvla"
      char op[len + 5];
#pragma GCC diagnostic pop
#else
      char op[len + 5];
#endif
      memcpy(op, *argv, len); // the only real memcpy
      memcpy(op + len, ".png", 5);
      ret = w2p(*argv, op) || ret;
#ifdef NOVLA
      free(op);
#endif
    }
  else {
    if(!doprogress) doprogress = isatty(2);
    for(; argc; argc--, argv++) {
      size_t len = strlen(*argv);
      if(len > 3 && (u4(*argv + len - 4) | u4("\0   ")) == u4(".png")) len -= 4;
#ifdef NOVLA
      char *op = malloc(len + 6);
      if(!op) {
	P("ERROR adding %s extension to %s: Out of memory", ".webp", *argv);
	return 1;
      }
#elif defined __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wvla"
      char op[len + 6];
#pragma GCC diagnostic pop
#else
      char op[len + 6];
#endif
      memcpy(op, *argv, len); // the only real memcpy
      memcpy(op + len, ".webp", 6);
      ret = p2w(*argv, op) || ret;
#ifdef NOVLA
      free(op);
#endif
    }
  }
  return ret;
}
