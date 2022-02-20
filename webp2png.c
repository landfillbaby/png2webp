/* anti-copyright Lucy Phipps 2020
vi: sw=2 tw=80
TODO: Try to compress somewhat better
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
#include "webp2png.h"
int main(int argc, char** argv) {
  FILE* fp;
  GETARGS
  for(;;) {
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
#define W F.width
#define H F.height
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
    PFV("Size: %zu bytes (%.15g bpp)", l, 8. * l / (uint32_t)(W * H));
    GETOUTFILE
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
    for(uint32_t y = 0; y < (uint32_t)H; y++) {
	png_write_row(png_ptr, px);
	px += D.stride;
    }
    png_write_end(png_ptr, info_ptr);
    png_destroy_write_struct(&png_ptr, &info_ptr);
#endif
    WebPFreeDecBuffer(&c.output);
    GETINFILE
} }
