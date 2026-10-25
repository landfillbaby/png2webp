// vi: sw=2 tw=80
#ifndef _FILE_OFFSET_BITS
#define _FILE_OFFSET_BITS 64
#endif
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#include "png.h"
#include "webp/decode.h"
#include "webp/encode.h"
#ifdef P2WCONF
#include "p2wconf.h"
#endif
#ifndef VERSION
#define VERSION "v1.3.0-dev"
#endif
#ifdef _WIN32
#ifndef _CRT_NONSTDC_NO_WARNINGS
#define _CRT_NONSTDC_NO_WARNINGS
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#include <fcntl.h>
#include <io.h>
#else
#include <unistd.h>
#endif
#include <inttypes.h>
#include <setjmp.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if SIZE_MAX < 0xffffffffu
#error "size_t isn't at least 32-bit"
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
#define P(...) fprintf(stderr, __VA_ARGS__)
#define PV(...) (verbose ? P(__VA_ARGS__) : 0)
#define PR(x) P("ERROR reading %s: %s\n", ip, x)
#define PW(x) P("ERROR writing %s: %s\n", op, x)
#ifdef DOPROGRESS
#define DOPROGRESS_CHAR "t"
#define DOPROGRESS_CHK (doprogress = doprogress || isatty(2))
static int progress(int percent, const WebPPicture *x) {
  (void)x;
  char h[64];
  for(size_t n = 0; n < 64u;) h[n++] = '#';
  P("\r[%-64.*s] %u%%", (unsigned)percent * 16u / 25u, h, (unsigned)percent);
  return 1;
}
#else
#define DOPROGRESS_CHAR
#define DOPROGRESS_CHK ((void)0)
#endif
static int help(void) {
  // PNG2WebP v1.x.y-zz-g1234567 NOFOPENX USEGETOPT LOSSYISERROR DOFLUSH
  fputs("PNG2WebP " VERSION
#ifdef NOFOPENX
      " NOFOPENX"
#endif
#ifdef USEGETOPT
      " USEGETOPT"
#endif
#ifdef LOSSYISERROR
      " LOSSYISERROR"
#endif
#ifdef DOFLUSH
      " DOFLUSH"
#endif
      "\n\
\n\
Usage:\n\
png2webp [-refv" DOPROGRESS_CHAR "] [--] INFILE ...\n\
png2webp -p[refv" DOPROGRESS_CHAR "] [--] [INFILE [OUTFILE]]\n\
\n\
-p: Work with a single file, allowing piping from stdin or to stdout,\n\
    or using a different output filename to the input.\n\
    `INFILE` and `OUTFILE` default to stdin and stdout respectively,\n\
    or explicitly as \"-\".\n\
    Will show this message if stdin/stdout is used and is a terminal.\n\
-r: Convert from WebP to PNG instead.\n\
-e: Keep RGB data on pixels where alpha is 0. Always enabled for `-r`.\n\
-f: Force overwrite of output files (has no effect on stdout).\n\
-v: Be verbose.\n"
#ifdef DOPROGRESS
"-" DOPROGRESS_CHAR
": Print a progress bar even when stderr isn't a terminal (not for `-r`).\n"
#endif
"--: Explicitly stop parsing options.\n", stderr);
  return -1;
}
static bool force, verbose; // TODO: no global vars
static FILE *openr(const char *ip) {
  P("Decoding %s ...\n", ip ? ip : "<stdin>");
  if(!ip) return stdin;
  FILE *fp;
#ifdef NOFOPENX
  int fd = open(ip, O_RDONLY | O_BINARY);
  if(fd == -1) {
    PR("Couldn't open");
    return 0;
  }
  if(!(fp = fdopen(fd, "rb"))) {
    PR("Couldn't open");
    close(fd);
    return 0;
  }
#else
  if(!(fp = fopen(ip, "rb"))) {
    PR("Couldn't open");
    return 0;
  }
#endif
  return fp;
}
#ifdef __FreeBSD__ // why isn't this in POSIX?
#define UNLINK(path, fd) funlinkat(AT_FDCWD, (path), (fd), 0)
#elif defined _WIN32
#define UNLINK(unused, fd) SetFileInformationByHandle( \
    (HANDLE)_get_osfhandle(fd), FileDispositionInfo, \
    &(FILE_DISPOSITION_INFO){.DeleteFile = TRUE}, sizeof(FILE_DISPOSITION_INFO))
#else // TODO: find fixes for other OSes
#define UNLINK(path, unused) unlink(path) // TOCTOU race condition :(
#endif
static FILE *openw(const char *op) {
  P("Encoding %s ...\n", op ? op : "<stdout>");
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
    PW("Couldn't open");
    return 0;
  }
  if(!(fp = fdopen(fd, "wb"))) {
    PW("Couldn't open");
    UNLINK(op, fd);
    close(fd);
    return 0;
  }
#else
  if(!(fp = fopen(op, force ? "wb" : "wbx"))) {
    PW("Couldn't open");
    return 0;
  }
#endif
  return fp;
}
typedef struct {
  FILE *const fp;
  const char *const path;
  size_t len;
} pngptr;
static void pngread(png_struct *p, uint8_t *d, size_t s) {
  pngptr *i = png_get_io_ptr(p);
  if(!fread(d, s, 1, i->fp)) png_error(p, "I/O error");
  i->len += s;
}
static void pngwrite(png_struct *p, uint8_t *d, size_t s) {
  pngptr *i = png_get_io_ptr(p);
  if(!fwrite(d, s, 1, i->fp)) png_error(p, "I/O error");
  i->len += s;
}
static void pngflush(png_struct *p) {
#ifdef DOFLUSH
  pngptr *i = png_get_io_ptr(p);
  if(fflush(i->fp)) png_error(p, "I/O error");
#else
  (void)p;
#endif
}
static PNG_NORETURN void pngrerr(png_struct *p, const char *s) {
  pngptr *i = png_get_io_ptr(p);
  const char *ip = i->path;
  PR(s);
  png_longjmp(p, 1);
}
static PNG_NORETURN void pngwerr(png_struct *p, const char *s) {
  pngptr *i = png_get_io_ptr(p);
  const char *op = i->path;
  PW(s);
  png_longjmp(p, 1);
}
static void pngwarn(png_struct *p, const char *s) {
  pngptr *i = png_get_io_ptr(p);
  P("Warning: %s: %s\n", i->path, s);
}
static int webpwrite(const uint8_t *d, size_t s, const WebPPicture *p) {
  return (int)fwrite(d, s, 1, p->custom_ptr);
}
static bool init_webpconfig(WebPConfig *c, bool exact) {
  if(!WebPConfigPreset(c, WEBP_PRESET_ICON, 100)) {
    P("ERROR: %s\n", "Broken config, file a bug report");
    return 1;
  }
  c->lossless = 1;
  c->method = 6;
  c->image_hint = WEBP_HINT_GRAPH; // init VP8LBitWriter to 8 bpp
  c->exact = exact;
  return 0;
}
static bool p2w(const char *ip, const char *op, const WebPConfig *c
#ifdef DOPROGRESS
    , bool doprogress
#endif
    ) {
  pngptr pi = {openr(ip), ip, 0};
  if(!pi.fp) return 1;
  uint32_t *b = 0;
  png_info *n = 0;
  png_struct *p
      = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, pngrerr, pngwarn);
#define P2W_CLOSE \
  do { \
    fclose(pi.fp); \
    png_destroy_read_struct(&p, &n, 0); \
    free(b); /* only non-null in row read loop */ \
    return 1; \
  } while(0)
  if(!p || !(n = png_create_info_struct(p))) {
    PR("Out of memory");
    P2W_CLOSE;
  }
  if(setjmp(png_jmpbuf(p))) P2W_CLOSE;
  png_set_read_fn(p, &pi, pngread);
  png_read_info(p, n);
  uint32_t width, height;
  int bitdepth, colortype;
  png_get_IHDR(p, n, &width, &height, &bitdepth, &colortype, 0, 0, 0);
  if(width > 16383u || height > 16383u) {
    P("ERROR reading %s: Image too big (%" PRIu32 " x %" PRIu32
      ", max. 16383 x 16383 px)\n",
	ip, width, height);
    P2W_CLOSE;
  }
  if((unsigned)bitdepth > 8u)
    P("Warning: %s: %s\n", ip, "Downsampling to 8-bit");
  bool trns = png_get_valid(p, n, PNG_INFO_tRNS);
  int32_t gamma = 45455;
  if(png_get_valid(p, n, PNG_INFO_sRGB) || png_get_gAMA_fixed(p, n, &gamma)) {
    if(gamma != 45455)
      P("Warning: %s: Nonstandard gamma: %" PRIu32 ".%05" PRIu32 "\n",
	  ip, (uint32_t)gamma / 100000u, (uint32_t)gamma % 100000u);
    png_set_gamma_fixed(p, 220000, gamma);
  }
  png_set_scale_16(p);
  png_set_expand(p);
  png_set_gray_to_rgb(p);
  png_set_packing(p);
  if(!*(uint8_t *)(uint16_t[]){1}) { // TODO: test big-endian
    png_set_swap_alpha(p);
    png_set_add_alpha(p, 255, PNG_FILLER_BEFORE);
  } else {
    png_set_bgr(p);
    png_set_add_alpha(p, 255, PNG_FILLER_AFTER);
  }
  unsigned passes = (unsigned)png_set_interlace_handling(p);
  png_read_update_info(p, n);
#ifndef NDEBUG
  size_t rowbytes = png_get_rowbytes(p, n);
  if(rowbytes != (size_t)4 * width) {
    P("ERROR reading %s: rowbytes is %zu, should be %zu\n", ip, rowbytes,
	(size_t)4 * width);
    P2W_CLOSE;
  }
#endif
  b = malloc((size_t)4 * width * height);
  if(!b) {
    PR("Out of memory");
    P2W_CLOSE;
  }
  for(unsigned x = passes; x; x--) {
    uint8_t *w = (uint8_t *)b; // it's fine
    for(unsigned y = height; y; y--) {
      png_read_row(p, w, 0);
      w += (size_t)4 * width;
    }
  }
  png_read_end(p, 0);
  fclose(pi.fp);
  png_destroy_read_struct(&p, &n, 0);
  const char *f[] = {"grayscale", "???", "RGB", "paletted", "grayscale + alpha",
      "???", "RGBA"};
  PV("Input info:\n%s\nDimensions: %" PRIu32 " x %" PRIu32
     "\nSize: %zu bytes (%.15g bpp)\nFormat: %u-bit %s%s%s\n",
      ip, width, height, pi.len, (double)pi.len * 8u / (width * height),
      (unsigned)bitdepth, f[(unsigned)colortype],
      trns ? ", with transparency" : "", passes > 1u ? ", interlaced" : "");
  WebPAuxStats s;
  WebPPicture o = {1, .width = (int)width, (int)height, .argb = b, (int)width,
      .writer = webpwrite, .custom_ptr = openw(op), .stats = verbose ? &s : 0,
#ifdef DOPROGRESS
      .progress_hook = doprogress ? progress : 0,
#endif
      };
  if(!o.custom_ptr) {
    free(b);
    return 1;
  }
#ifdef DOPROGRESS
  if(doprogress) P("[%-64.*s] %u%%", 0, "", 0u);
#endif
  trns = (trns || (colortype & PNG_COLOR_MASK_ALPHA))
      && WebPPictureHasTransparency(&o);
  int r = WebPEncode(c, &o);
#ifdef DOPROGRESS
  if(doprogress) fputs("\n", stderr);
#endif
  if(!r) {
    switch(o.error_code) {
      case VP8_ENC_ERROR_OUT_OF_MEMORY: PW("Out of memory"); break;
      case VP8_ENC_ERROR_INVALID_CONFIGURATION:
	PW("Broken config, file a bug report");
	break;
      case VP8_ENC_ERROR_BAD_WRITE: PW("I/O error"); break;
      default: PW("???");
    }
    if(op) UNLINK(op, fileno(o.custom_ptr));
    fclose(o.custom_ptr);
    free(b);
    return 1;
  }
  if(fflush(o.custom_ptr)) {
    PW("I/O error");
    if(op) UNLINK(op, fileno(o.custom_ptr));
    fclose(o.custom_ptr);
    free(b);
    return 1;
  }
  fclose(o.custom_ptr);
  free(b);
#define F s.lossless_features
#define C s.palette_size
#define W ((uint32_t)o.width)
#define H ((uint32_t)o.height)
  PV("Output info:\n%s\nSize: %u bytes (%.15g bpp)\n\
Header size: %u, image data size: %u\nUses alpha: %s\n\
Precision bits: histogram=%u prediction=%u cross-color=%u cache=%u\n\
Lossless features:%s%s%s%s\nColors: %s%u\n",
      op, (unsigned)s.coded_size,
      (double)(unsigned)s.coded_size * 8u / (W * H),
      (unsigned)s.lossless_hdr_size, (unsigned)s.lossless_data_size,
      trns ? "yes" : "no", (unsigned)s.histogram_bits,
      (unsigned)s.transform_bits, (unsigned)s.cross_color_transform_bits,
      (unsigned)s.cache_bits, F ? F & 1u ? " prediction" : "" : " none",
      F && F & 2u ? " cross-color" : "", F && F & 4u ? " subtract-green" : "",
      F && F & 8u ? " palette" : "", C ? "" : ">", C ? (unsigned)C : 256u);
  return 0;
}
static bool w2p(const char *ip, const char *op) {
  FILE *fp = openr(ip);
  if(!fp) return 1;
  uint8_t i[12];
  if(!fread(i, 12, 1, fp)) {
    PR("I/O error");
    fclose(fp);
    return 1;
  }
#define U(x) ((uint32_t)(*(x) | (uint32_t)(x)[1] << 8 \
      | (uint32_t)(x)[2] << 16 | (uint32_t)(x)[3] << 24))
  uint32_t l = U(i + 4u) + 8u; // RIFF header size
  if(U(i) != U("RIFF") || U(i + 8u) != U("WEBP") || -~l < 29u) {
    PR("Invalid WebP");
    fclose(fp);
    return 1;
  }
  uint8_t *x = malloc(l);
  if(!x) {
    PR("Out of memory");
    fclose(fp);
    return 1;
  }
  for(size_t n = 0; n < 12u; n++) x[n] = i[n];
  if(!fread(x + 12u, l - 12u, 1, fp)) {
    PR("I/O error");
    fclose(fp);
    free(x);
    return 1;
  }
  fclose(fp);
  WebPBitstreamFeatures o;
  VP8StatusCode r = WebPGetFeatures(x, l, &o);
  if(r) {
    switch(r) {
      case VP8_STATUS_INVALID_PARAM:
	PR("Broken config, file a bug report");
	break;
      case VP8_STATUS_BITSTREAM_ERROR:
      case VP8_STATUS_NOT_ENOUGH_DATA: PR("Invalid WebP"); break;
      default: PR("???");
    }
    free(x);
    return 1;
  }
#define A (!!o.has_alpha)
#define B ((uint32_t)3 + A)
#define L ((size_t)B * W * H)
#ifdef LOSSYISERROR
#define FMTSTR
#define FMTARG
#else
  const char *f[] = {"undefined/mixed", "lossy", "lossless"};
#define FMTSTR "Format: %s\n"
#define FMTARG , f[(unsigned)o.format]
#endif
  PV("Input info:\n%s\nDimensions: %" PRIu32 " x %" PRIu32 "\nSize: %" PRIu32
     " bytes (%.15g bpp)\nUses alpha: %s\n" FMTSTR,
      ip, W, H, l, (double)l * 8u / (W * H), A ? "yes" : "no" FMTARG);
  if(o.has_animation) {
    PR("Unsupported feature: animation");
    free(x);
    return 1;
  }
#ifdef LOSSYISERROR
  if(o.format != 2) {
    PR("Unsupported feature: lossy compression");
    free(x);
    return 1;
  }
#endif
  uint8_t *b = malloc(L);
  if(!b) {
    PR("Out of memory");
    free(x);
    return 1;
  }
  if(!(A ? WebPDecodeRGBAInto : WebPDecodeRGBInto)(x, l, b, L, (int)(B * W))) {
    PR("Invalid WebP");
    free(b);
    free(x);
    return 1;
  }
  free(x);
  pngptr pi = {openw(op), op, 0};
  if(!pi.fp) {
    free(b);
    return 1;
  }
  png_info *n = 0;
  png_struct *p
      = png_create_write_struct(PNG_LIBPNG_VER_STRING, 0, pngwerr, pngwarn);
#define W2P_RM \
  do { \
    if(op) UNLINK(op, fileno(pi.fp)); \
    fclose(pi.fp); \
    png_destroy_write_struct(&p, &n); \
    free(b); \
    return 1; \
  } while(0)
  if(!p || !(n = png_create_info_struct(p))) {
    PW("Out of memory");
    W2P_RM;
  }
  if(setjmp(png_jmpbuf(p))) W2P_RM;
  png_set_write_fn(p, &pi, pngwrite, pngflush);
  png_set_filter(p, 0, PNG_ALL_FILTERS);
  png_set_compression_level(p, 9);
  // png_set_compression_memlevel(p, 9);
  png_set_IHDR(p, n, W, H, 8, A ? 6 : 2, 0, 0, 0);
  png_write_info(p, n);
  uint8_t *w = b;
  for(unsigned y = H; y; y--) {
    png_write_row(p, w);
    w += (size_t)B * W;
  }
  png_write_end(p, n);
  if(fflush(pi.fp)) {
    PW("I/O error");
    W2P_RM;
  }
  fclose(pi.fp);
  png_destroy_write_struct(&p, &n);
  free(b);
  PV("Output info:\n%s\nSize: %zu bytes (%.15g bpp)\nFormat: 8-bit %s\n",
      op, pi.len, (double)pi.len * 8u / (W * H), A ? "RGBA" : "RGB");
  return 0;
}
int main(int sargc, char **argv) {
  unsigned argc = (unsigned)sargc;
  {
    const uint8_t *x = (const uint8_t *)(const uint32_t[]){U("4321")};
    if(U(x) == U("1234"))
      P("Warning: %s\n", "Big-endian support is untested"); // TODO
    else if(U(x) != U("4321")) {
      P("ERROR: system is mixed-endian (%.4s)\n", x);
      return 1;
    }
  }
  bool pipe = 0, usestdin = 0, usestdout = 0, reverse = 0, exact = 0;
#ifdef DOPROGRESS
  bool doprogress = 0;
#endif
#ifdef USEGETOPT
  for(int c; (c = getopt(sargc, argv, ":prefv" DOPROGRESS_CHAR)) != -1;)
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
#ifdef DOPROGRESS
      case *(DOPROGRESS_CHAR): doprogress = 1; break;
#endif
      default: return help();
    }
#ifdef USEGETOPT
  argc -= (unsigned)optind;
  argv += (unsigned)optind;
#endif
#define PIPEARG(x) (argc <= (x) || (*argv[x] == '-' && !argv[x][1]))
  if(pipe) {
    if(argc > 2u || ((usestdin = PIPEARG(0u)) && isatty(0))
	|| ((usestdout = PIPEARG(1u)) && isatty(1)))
      return help();
#ifdef _WIN32
    if(usestdin) setmode(0, O_BINARY);
    if(usestdout) setmode(1, O_BINARY);
#endif
    if(reverse) return w2p(usestdin ? 0 : *argv, usestdout ? 0 : argv[1]);
    WebPConfig c;
    if(init_webpconfig(&c, exact)) return 1;
    DOPROGRESS_CHK;
    return p2w(usestdin ? 0 : *argv, usestdout ? 0 : argv[1], &c
#ifdef DOPROGRESS
	, doprogress
#endif
	);
  }
  if(!argc) return help();
  bool ret = 0;
#define K(x, y) (argv[i][len - (x)] == (y))
#define J(x, y) (K(x, *(y)) || K(x, (y)[1]))
  if(reverse) {
#pragma omp parallel for // TODO: pthread, fix stderr output
    for(unsigned i = 0; i < argc; i++) {
      size_t len = strlen(argv[i]);
      if(len > 4u && K(5u, '.') && J(4u, "wW") && J(3u, "eE") && J(2u, "bB")
	  && J(1u, "pP"))
	len -= 5u;
      char *op = malloc(len + 5u);
      if(!op) {
	P("ERROR adding %s extension to %s: %s\n", ".png", argv[i],
	    "Out of memory");
	ret = 1;
      } else {
	memcpy(op, argv[i], len);
	for(size_t n = 0; n < 5u; n++) (op + len)[n] = ".png"[n];
	ret = w2p(argv[i], op) || ret;
	free(op);
      }
    }
  } else {
    WebPConfig c;
    if(init_webpconfig(&c, exact)) return 1;
    DOPROGRESS_CHK;
#pragma omp parallel for // TODO: ditto
    for(unsigned i = 0; i < argc; i++) {
      size_t len = strlen(argv[i]);
      if(len > 3u && K(4u, '.') && J(3u, "pP") && J(2u, "nN") && J(1u, "gG"))
	len -= 4u;
      char *op = malloc(len + 6u);
      if(!op) {
	P("ERROR adding %s extension to %s: %s\n", ".webp", argv[i],
	    "Out of memory");
	ret = 1;
      } else {
	memcpy(op, argv[i], len);
	for(size_t n = 0; n < 6u; n++) (op + len)[n] = ".webp"[n];
	ret = p2w(argv[i], op, &c
#ifdef DOPROGRESS
	    , doprogress
#endif
	    ) || ret;
	free(op);
      }
    }
  }
  return ret;
}
