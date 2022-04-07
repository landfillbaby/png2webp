// anti-copyright Lucy Phipps 2020
// vi: sw=2 tw=80
// TODO: REFACTOR
#define VERSION "v0.8"
#ifdef FROMWEBP
/* TODO: Try to compress somewhat better
Ideally should palette if <=256 colors (in order of appearance),
or at least try to palette when input WebP was,
but that's not part of either libpng encoding or libwebp decoding.
Maybe do this:
#include <webp/encode.h> // for WebPPicture
WEBP_EXTERN int WebPGetColorPalette( // declared in libwebp utils/utils.h
const struct WebPPicture* const, uint32_t* const); */
#include <webp/decode.h>
#define INEXT "webp"
#define INEXTCHK INEXT
#ifdef PAM
#define OUTEXT "pam"
#else
#include <png.h>
#define OUTEXT "png"
#endif
#define OUTEXTCHK "." OUTEXT
#define EXTRALETTERS
#define EXTRAHELP
#else // FROMWEBP
#include <webp/encode.h>
#ifdef PAM
#include <pam.h>
#define INEXT "pam"
#define X(x) ((argv[0][len - 2] | 32) == x)
#define ISINEXT \
	if(len > 3) { \
		uint32_t ext, extmask, extmatch; \
		memcpy(&ext, *argv + len - 4, 4); \
		memcpy(&extmask, (char[4]){"\0 \xff "}, 4); \
		memcpy(&extmatch, (char[4]){".p\xffm"}, 4); \
		if((ext | extmask) == extmatch && (X('b') || X('g') || \
			X('p') || X('n') || X('a'))) len -= 4; \
	}
#else
#include <png.h>
#define INEXT "png"
#define INEXTCHK ".png"
#endif
#define OUTEXT "webp"
#define OUTEXTCHK "webp"
#define EXTRALETTERS "e"
#define EXTRAHELP "-e: Keep RGB data on pixels where alpha is 0.\n"
#endif // FROMWEBP
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#define O(x) _##x
#else
#include <unistd.h>
#define setmode(x, y) 0
#define O(x) x
#endif
#ifndef ISINEXT
#define ISINEXT \
	if(len >= sizeof INEXT) { \
		uint32_t ext, extmask, extmatch; \
		memcpy(&ext, *argv + len - 4, 4); \
		memcpy(&extmask, (char[4]){(sizeof INEXT > 4) * 32, 32, 32, \
			32}, 4); \
		memcpy(&extmatch, (char[4]){INEXTCHK}, 4); \
		if((sizeof INEXT < 5 || argv[0][len - 5] == '.') && \
			(ext | extmask) == extmatch) len -= sizeof INEXT; \
	}
#endif
#define P(x) fputs(x "\n", stderr)
#define PF(x, ...) fprintf(stderr, x "\n", __VA_ARGS__)
// #define PV(x) if(verbose) P(x);
#define PFV(...) if(verbose) PF(__VA_ARGS__);
#define E(f, ...) \
	if(!(f)) { \
		PF("ERROR " __VA_ARGS__); \
		return 1; \
	}
#if __STDC_VERSION__ < 201112L || defined(NOFOPENX)
#include <fcntl.h>
#include <sys/stat.h>
#endif
#define HELP \
  P(INEXT "2" OUTEXT " " VERSION "\n\nUsage:\n" INEXT "2" OUTEXT \
    " [-b" EXTRALETTERS "fv-] infile." INEXT " ...\n" INEXT "2" OUTEXT \
    " [-p" EXTRALETTERS "fv-] [{infile." INEXT "|-} [outfile." OUTEXT \
    "|-]]\n\n-b: Work with many input files (Batch mode).\n" \
    "    Constructs output filenames by removing the ." INEXT \
    " extension if possible,\n    and appending \"." OUTEXT "\".\n" \
    "-p: Work with a single file, allowing Piping from stdin or to stdout,\n" \
    "    or using a different output filename to the input.\n    infile." \
    INEXT " and outfile." OUTEXT \
    " default to stdin and stdout respectively,\n" \
    "    or explicitly as \"-\".\n" \
    "    Will show this message if stdin/stdout is used and is a terminal.\n" \
    EXTRAHELP \
    "-f: Force overwrite of output files (has no effect on stdout).\n" \
    "-v: Be verbose.\n--: Explicitly stop parsing options.\n\n" \
    "Without -b or -p, and with 1 or 2 filenames, there is some ambiguity.\n" \
    "In this case it will tell you what its guess is."); \
  return -1;
#define OPENR(x) \
	PFV("%scoding %s ...", "De", x ? "stdin" : *argv); \
	if(x) fp = stdin; \
	else E(fp = fopen(*argv, "rb"), "opening \"%s\" for %s: %s", *argv, \
		"reading", strerror(errno));
#define PIPEARG(x) (*argv[x] == '-' && !argv[x][1])
#define PIPECHK(x, y) \
  if(use##y) { \
	if(!(x && skipstdoutchk) && O(isatty)(x)) { HELP } \
	E(O(setmode)(x, _O_BINARY) != -1, "setting %s to binary mode", #y); \
  }
#define URGC (unsigned)argc
#define EC(x) E(!fclose(fp), "closing %s: %s", x, strerror(errno))
#ifndef FROMWEBP
static FILE* fp;
static int w(const uint8_t* d, size_t s, const WebPPicture* x) {
	(void)x;
	return s ? (int)fwrite(d, s, 1, fp) : 1;
}
#endif
int main(int argc, char** argv) {
#ifdef FROMWEBP
  FILE* fp;
#else // FROMWEBP
#ifdef PAM
  pm_init("ERROR", 0); // TODO: maybe *argv or (INEXT "2" OUTEXT) ?
#elif !defined(USEADVANCEDPNG)
  uint32_t endian;
  memcpy(&endian, (char[4]){"\xAA\xBB\xCC\xDD"}, 4);
  E(endian == 0xAABBCCDD || endian == 0xDDCCBBAA,
	"32-bit mixed-endianness (%X) not supported", endian);
#endif
  bool exact = 0;
#endif // FROMWEBP
  bool usepipe = 0, usestdin = 0, usestdout = 0, force = 0, verbose = 0,
	chosen = 0, skipstdoutchk = 0;
  char* outname = 0;
#ifdef USEGETOPT
  int c;
  while((c = getopt(argc, argv, ":bp" EXTRALETTERS "fv")) != -1) switch(c)
#else
  while(--argc && **++argv == '-' && argv[0][1])
    while(*++*argv) switch(**argv)
#endif
    {
	case 'p': usepipe = 1; // FALLTHRU
	case 'b':
		if(chosen) { HELP }
		chosen = 1;
		break;
#ifndef FROMWEBP
	case 'e': exact = 1; break;
#endif
	case 'f': force = 1; break;
	case 'v': verbose = 1; break;
#ifndef USEGETOPT
	case '-':
	  if(!argv[0][1]) {
		argc--;
		argv++;
		goto endflagloop;
	  } // FALLTHRU
#endif
	default: HELP
    }
#ifdef USEGETOPT
  argc -= optind;
  argv += optind;
#else
endflagloop:
#endif
  if(chosen && (usepipe ? URGC > 2 : !argc)) { HELP }
  if(usepipe) {
	usestdin = !argc || PIPEARG(0);
	usestdout = URGC < 2 || PIPEARG(1);
  } else if(!chosen && URGC < 3) {
    usestdin = !argc || PIPEARG(0);
    usestdout = argc == 2 ? PIPEARG(1) : usestdin;
    if(!(usepipe = usestdin || usestdout)) {
      PF("Warning: %u file%s given and neither -b or -p specified.", URGC,
	argc == 1 ? "" : "s");
      if(argc == 1) {
	if(!O(isatty)(1)) usepipe = usestdout = skipstdoutchk = 1;
      } else {
	size_t len = strlen(argv[1]);
	if(len >= sizeof OUTEXT) {
	  uint32_t ext, extmask, extmatch;
	  memcpy(&ext, argv[1] + len - 4, 4);
	  memcpy(&extmask, (char[4]){(sizeof OUTEXT > 4) * 32, 32, 32, 32}, 4);
	  memcpy(&extmatch, (char[4]){OUTEXTCHK}, 4);
	  usepipe = (sizeof OUTEXT < 5 || argv[1][len - 5] == '.') &&
		(ext | extmask) == extmatch;
      } }
      PF("Guessed -%c.", usepipe ? 'p' : 'b');
  } }
  PIPECHK(0, stdin);
  PIPECHK(1, stdout);
  OPENR(usestdin);
  for(;;) {
#ifdef FROMWEBP
    WebPDecoderConfig c = { // TODO: memset? WebPInitDecoderConfig?
#ifdef NOTHREADS
	0
#else
	.options.use_threads = 1
#endif
    };
#ifndef IDEC_BUFSIZE
#define IDEC_BUFSIZE 65536
#endif
    uint8_t i[IDEC_BUFSIZE];
    size_t l = fread(i, 1, IDEC_BUFSIZE, fp);
    char* k[] = {"out of RAM", "invalid params", "bitstream broke",
	"unsupported feature", "suspended", "cancelled", "not enough data"};
#define F c.input
#define A F.has_alpha
    VP8StatusCode r = WebPGetFeatures(i, l, &F);
    E(!r, "reading WebP header: %u (%s)", r, r < 8 ? k[r - 1] : "???");
#ifdef LOSSYISERROR
#define FORMATSTR
#define GETFORMAT
#define ANIMARGS "%sion)", k[3], "animat"
#else
    char* formats[] = {"undefined/mixed", "lossy", "lossless"};
#define FORMATSTR "\nFormat: %s (%d)"
#define GETFORMAT , (unsigned)V < 3 ? formats[V] : "???", V
#define ANIMARGS "animation)", k[3]
#endif
#define V F.format
#define W (unsigned)F.width
#define H (unsigned)F.height
    PFV("Input WebP info:\nDimensions: %u x %u\nUses alpha: %s" FORMATSTR,
	W, H, A ? "yes" : "no" GETFORMAT);
    E(!F.has_animation, "reading WebP header: 4 (%s: " ANIMARGS);
#ifdef LOSSYISERROR
    E(V == 2, "reading WebP header: 4 (%s: %sion)", k[3], "lossy compress");
#endif
    if(A) c.output.colorspace = MODE_RGBA;
    WebPIDecoder* d = WebPIDecode(i, l, &c);
    E(d, "initializing WebP decoder: 1 (%s)", *k);
    for(size_t x = l; (r = WebPIAppend(d, i, x)); l += x) {
	E(r == 5 && !feof(fp), "reading WebP data: %d (%s)", r == 5 ? 7 : r,
		r == 5 ? k[6] : r < 8 ? k[r - 1] : "???");
	x = fread(i, 1, IDEC_BUFSIZE, fp);
    }
    WebPIDelete(d);
    PFV("Size: %zu bytes (%.15g bpp)", l, (double)l * 8 / (uint32_t)(W * H));
#else // FROMWEBP
#ifdef PAM
    struct pam i;
    pnm_readpaminit(fp, &i, PAM_STRUCT_SIZE(tuple_type));
    E(i.depth < 5, "too many channels: %u (max. 4)", i.depth);
    E((unsigned)i.width < 16384 && (unsigned)i.height < 16384,
	"image too big (%ux%u, max. 16383x16383 px)", i.width, i.height);
    if(255 % i.maxval) PF("Warning: scaling from maxval %lu to 255", i.maxval);
    tuple* r = pnm_allocpamrow(&i);
#else
#ifdef USEADVANCEDPNG
#error // TODO
#else
    png_image i = {.version = PNG_IMAGE_VERSION}; // TODO: memset?
#define EP(f, s, d) \
	E(f, "reading PNG %s: %s", s, i.message); \
	if(i.warning_or_error) { \
		PF("PNG %s warning: %s", s, i.message); \
		if(d) i.warning_or_error = 0; \
	}
    EP(png_image_begin_read_from_stdio(&i, fp), "info", 1);
    E(i.width < 16384 && i.height < 16384,
	"image too big (%ux%u, max. 16383x16383 px)", i.width, i.height);
    if(i.format & PNG_FORMAT_FLAG_LINEAR)
	P("Warning: input PNG is 16bpc, will be downsampled to 8bpc");
    bool A = !!(i.format & PNG_FORMAT_FLAG_ALPHA);
    i.format = (*(uint8_t*)&(uint16_t){1}) ? PNG_FORMAT_BGRA : PNG_FORMAT_ARGB;
#endif
#endif
#define W ((uint16_t)i.width)
#define H ((uint16_t)i.height)
    WebPPicture o = {1, .width = W, H, .writer = w};
    // ^ TODO: memset? WebPPictureInit?
    WebPAuxStats s;
    if(verbose) o.stats = &s;
    // progress_hook only reports 1, 5, 90, 100 for lossless
    char* es[VP8_ENC_ERROR_LAST - 1] = {"out of RAM",
	"out of RAM flushing bitstream", "something was null", "broken config",
	/* "image too big (max. 16383x16383 px)" */ "", "partition >512KiB",
	"partition >16MiB", "couldn't write", "output >4GiB", "cancelled"};
    E(WebPPictureAlloc(&o), "%sing WebP: %s (%u)", "allocat",
	es[o.error_code - 1], o.error_code);
#ifdef PAM
    for(unsigned y = 0; y < H; y++) {
	pnm_readpamrow(&i, r);
	pnm_scaletuplerow(&i, r, r, 255);
#define A (~i.depth & 1)
#define D (i.depth > 2)
	for(unsigned x = 0; x < W; x++) o.argb[y * W + x] = (uint32_t)(
		(((A ? r[x][i.depth - 1] : 255) & 255) << 24) |
		((*r[x] & 255) << 16) | ((r[x][D] & 255) << 8) |
		(r[x][D * 2] & 255));
    }
    pnm_freepamrow(r);
#else
#ifdef USEADVANCEDPNG
#error // TODO
#else
    EP(png_image_finish_read(&i, 0, o.argb, 0, 0), "data", 0);
#endif
#endif
#endif // FROMWEBP
    EC(usestdin ? "stdin" : *argv);
    if(usestdout) {
	PFV("%scoding %s ...", "En", "stdout");
	fp = stdout;
    } else {
      if(usepipe) outname = argv[1];
      else {
	size_t len = strlen(*argv);
	ISINEXT;
	outname = malloc(len + sizeof "." OUTEXT);
	E(outname, "adding ." OUTEXT " extension to %s: out of RAM", *argv);
	memcpy(outname, *argv, len);
	memcpy(outname + len, "." OUTEXT, sizeof "." OUTEXT);
      }
      PFV("%scoding %s ...", "En", outname);
#define EO(x) E(x, "opening \"%s\" for %s: %s", outname, \
	force ? "writing" : "creation", strerror(errno))
#if __STDC_VERSION__ < 201112L || defined(NOFOPENX)
#ifndef O_BINARY
#define O_BINARY 0
#endif
      int fd = O(open)(outname, O(O_WRONLY) | O(O_CREAT) | O(O_BINARY) |
	O(O_TRUNC) | (!force * O(O_EXCL)),
#ifdef _WIN32
	_S_IREAD | _S_IWRITE
#else
	0666
#endif
	);
      EO(fd != -1 && (fp = O(fdopen)(fd, "wb")));
#else
      EO(fp = fopen(outname, force ? "wb" : "wbx"));
#endif
    }
#ifdef FROMWEBP
#define D c.output.u.RGBA
#ifdef PAM
    fprintf(fp, "P7\nWIDTH %u\nHEIGHT %u\nDEPTH %c\nMAXVAL 255\n"
	"TUPLTYPE RGB%s\nENDHDR\n", W, H, A ? '4' : '3', A ? "_ALPHA" : "");
    fwrite(D.rgba, D.size, 1, fp);
#else
    // TODO: PNG OUTPUT INFO
    png_structp png_ptr =
	png_create_write_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
    E(png_ptr, "writing PNG: %s", *k);
    png_infop info_ptr = png_create_info_struct(png_ptr);
    E(info_ptr, "writing PNG: %s", *k);
#ifdef PNG_SETJMP_SUPPORTED
    // E(!setjmp(png_jmpbuf(png_ptr)), "writing PNG: %s", "???");
    if(setjmp(png_jmpbuf(png_ptr))) return 1;
#endif
    png_init_io(png_ptr, fp);
    png_set_filter(png_ptr, 0, PNG_ALL_FILTERS);
    png_set_compression_level(png_ptr, 9);
    // png_set_compression_memlevel(png_ptr, 9);
    png_set_IHDR(png_ptr, info_ptr, W, H, 8,
	A ? PNG_COLOR_TYPE_RGBA : PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
	PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
    png_write_info(png_ptr, info_ptr);
    png_bytep px = D.rgba;
    for(unsigned y = 0; y < H; y++) {
	png_write_row(png_ptr, px);
	px += D.stride;
    }
    png_write_end(png_ptr, info_ptr);
    png_destroy_write_struct(&png_ptr, &info_ptr);
#endif
    WebPFreeDecBuffer(&c.output);
#else // FROMWEBP
#ifdef NOTHREADS
#define THREADLEVEL
#else
#define THREADLEVEL .thread_level = 1,
#endif
    E(WebPEncode(
	&(WebPConfig){ // TODO: memset? WebpConfigInit?
		1, 100, 6, // lossless, max
		WEBP_HINT_GRAPH, /* see VP8LEncodeImage source
			16bpp is only for alpha on lossy */
		THREADLEVEL // doesn't seem to affect output
		.near_lossless = 100, // don't modify visible pixels
		.exact = exact, // see EXTRAHELP
		.pass = 1, .segments = 1 // unused, for WebPValidateConfig
	}, &o),
	"%sing WebP: %s (%u)", "encod", es[o.error_code - 1], o.error_code);
#define F s.lossless_features
#define C s.palette_size
    PFV("Output WebP info:\nDimensions: %u x %u\nSize: %u bytes (%.15g bpp)\n"
	"Header size: %u, image data size: %u\nUses alpha: %s\n"
	"Precision bits: histogram=%u transform=%u cache=%u\n"
	"Lossless features:%s%s%s%s\nColors: %s%u",
	o.width, o.height, s.coded_size,
	(unsigned)s.coded_size * 8. / (uint32_t)(o.width * o.height),
	s.lossless_hdr_size, s.lossless_data_size,
	A && WebPPictureHasTransparency(&o) ? "yes" : "no",
	s.histogram_bits, s.transform_bits, s.cache_bits,
	F ? F & 1 ? " prediction" : "" : " none",
	F && F & 2 ? " cross-color" : "", F && F & 4 ? " subtract-green" : "",
	F && F & 8 ? " palette" : "", C ? "" : ">", C ? C : 256);
    WebPPictureFree(&o);
#endif // FROMWEBP
    EC(usestdout ? "stdout" : outname);
    if(usepipe || !--argc) return 0;
    if(outname) {
	free(outname);
	outname = 0;
    }
    argv++;
    OPENR(0);
} }
