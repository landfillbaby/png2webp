// vi: sw=2
#include <webp/decode.h>
/*
TODO: Try to compress somewhat better
Ideally should palette if <=256 colors (in order of appearance),
or at least try to palette when input WebP was,
but that's not part of either libpng encoding or libwebp decoding.
Maybe do this:
#include <webp/encode.h> // for WebPPicture
WEBP_EXTERN int WebPGetColorPalette( // declared in libwebp utils/utils.h
const struct WebPPicture *const, uint32_t *const);
*/
#define INEXT "webp"
#define OUTEXT Z
#include "webp2png.h"
int main(int argc, char **argv) {
  FILE *fp;
  GETARGS
  while(1) {
    WebPDecoderConfig c = {
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
    char *k[] = {"out of RAM", "invalid params", "bitstream broke",
	"unsupported feature", "suspended", "you cancelled it",
	"not enough data"};
#define F c.input
#define A F.has_alpha
    int r = WebPGetFeatures(i, l, &F);
    E(!r, "reading WebP header: %d (%s)", r, r & ~7 ? "???" : k[r - 1]);
    char *formats[] = {"undefined/mixed", "lossy", "lossless"};
#define V F.format
#define W F.width
#define H F.height
    PFV("Input WebP info:\n"
	"Dimensions: %u x %u\n"
	"Uses alpha: %s\n"
	"Format: %s (%d)",
	W, H, A ? "yes" : "no", (unsigned)V < 3 ? formats[V] : "???", V);
#ifdef LOSSYISERROR
    E(V == 2,
	"reading WebP header: 4 (%s:\n"
	"                              compression is %s (%d),\n"
	"                              instead of lossless (2))",
	k[3], (unsigned)V < 3 ? formats[V] : "???", V);
#endif
    E(!F.has_animation, "reading WebP header: 4 (%s: animation)", k[3]);
    if(A) { c.output.colorspace = MODE_RGBA; }
    WebPIDecoder *d = WebPIDecode(i, l, &c);
    E(d, "initializing WebP decoder: 1 (%s)", k[0]);
    for(size_t x = l; (r = WebPIAppend(d, i, x)); l += x) {
      E(r == 5 && !feof(fp), "reading WebP data: %d (%s)", r == 5 ? 7 : r,
	  r == 5 ? k[6] : (r & ~7 ? "???" : k[r - 1]));
      x = fread(i, 1, IDEC_BUFSIZE, fp);
    }
    WebPIDelete(d);
    PFV("Size: %zu bytes (%.17g bpp)", l,
	(l * 8.) / ((uint32_t)W * (uint32_t)H));
    GETOUTFILE
#define D c.output.u.RGBA
#ifdef PAM
    fprintf(fp,
	"P7\n"
	"WIDTH %u\n"
	"HEIGHT %u\n"
	"DEPTH 4\n"
	"MAXVAL 255\n"
	"TUPLTYPE RGB%s\n"
	"ENDHDR\n",
	W, H, A ? "_ALPHA" : "");
    fwrite(D.rgba, D.size, 1, fp);
#else
#ifdef USEADVANCEDPNG
    png_structp png_ptr =
	png_create_write_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
    ED(png_ptr, "writing PNG: %s", k[0]);
    png_infop info_ptr = png_create_info_struct(png_ptr);
    ED(info_ptr, "writing PNG: %s", k[0]);
#ifdef PNG_SETJMP_SUPPORTED
    // E(!setjmp(png_jmpbuf(png_ptr)), "writing PNG: %s", "???");
    if(setjmp(png_jmpbuf(png_ptr))) { Q(1); }
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
      png_write_rows(png_ptr, &px, 1);
      px += D.stride;
    }
    png_write_end(png_ptr, info_ptr);
    png_destroy_write_struct(&png_ptr, &info_ptr);
#else
    png_image o = {.version = 1, W, H, A ? PNG_FORMAT_RGBA : PNG_FORMAT_RGB};
    ED(png_image_write_to_stdio(&o, fp, 0, D.rgba, 0, 0), "writing PNG: %s",
	o.message);
    if(o.warning_or_error) { PF("Warning writing PNG: %s", o.message); }
#endif
    // TODO: PNG OUTPUT INFO
#endif
    WebPFreeDecBuffer(&c.output);
    GETINFILE
} }
