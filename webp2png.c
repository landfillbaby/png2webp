// vi: sw=2
#include <webp/decode.h>
/*TODO: ideally should palette if <=256 colors (in order of appearance),
or at least try to palette when input webp was,
but that's not part of either libpng encoding or libwebp decoding*/
/*#include<webp/encode.h>//for WebPPicture
WEBP_EXTERN int WebPGetColorPalette(//declared in libwebp utils/utils.h
const struct WebPPicture*const,uint32_t*const);*/
// TODO: should also try to compress somewhat better
#define INEXT "webp"
#define OUTEXT Z
#include "webp2png.h"
#ifdef DEBUG_IDEC
#define S 256
#else
#define S 16384
#endif
int main(int argc, char **argv) {
  FILE *fd;
  GETARGS
  while(1) {
    WebPDecoderConfig c = {
#ifdef NOTHREADS
	0
#else
	.options.use_threads = 1
#endif
    };
    uint8_t i[S];
    int l = fread(i, 1, S, fd);
    char *k[] = {"out of RAM",		"invalid params", "bitstream broke",
		 "unsupported feature", "suspended",	  "you cancelled it",
		 "not enough data"};
#define F c.input
#define A F.has_alpha
    int r;
    E(!(r = WebPGetFeatures(i, l, &F)), "reading WebP header: %d (%s)", r,
      r & ~7 ? "???" : k[r - 1]);
#define V F.format
#define W F.width
#define H F.height
    PFV("Input WebP info:\n"
	"Dimensions: %u x %u\n"
	"Uses alpha: %s\n"
	"Format: %s (%d)",
	W, H, A ? "yes" : "no",
	(unsigned)V < 3 ?
	    (char *[]){"undefined/mixed", "lossy", "lossless"}[V] :
	    "???",
	V);
    E(!F.has_animation, "reading WebP header: 4 (%s: animation)", k[3]);
    if(A) { c.output.colorspace = MODE_RGBA; }
    WebPIDecoder *d;
    E(d = WebPIDecode(i, l, &c), "initializing WebP decoder: 1 (%s)", k[0]);
    for(int x = l; (r = WebPIAppend(d, i, x)); l += x) {
      E(r == 5 && !feof(fd), "reading WebP data: %d (%s)", r == 5 ? 7 : r,
	r == 5 ? k[6] : (r & ~7 ? "???" : k[r - 1]));
      x = fread(i, 1, S, fd);
    }
    WebPIDelete(d);
    PFV("Size: %u bytes (%.17g bpp)", l, (l * 8.) / (W * H));
    GETOUTFILE
#define D c.output.u.RGBA
#ifdef PAM
    fprintf(fd,
	    A ? "P7\n"
		"WIDTH %u\n"
		"HEIGHT %u\n"
		"DEPTH 4\n"
		"MAXVAL 255\n"
		"TUPLTYPE RGB_ALPHA\n"
		"ENDHDR\n" :
		"P6\n"
		"%u %u\n"
		"255\n",
	    W, H);
    fwrite(D.rgba, D.size, 1, fd);
#else
    png_image o = {0, PNG_IMAGE_VERSION, W, H,
		   A ? PNG_FORMAT_RGBA : PNG_FORMAT_RGB};
    E(png_image_write_to_stdio(&o, fd, 0, D.rgba, 0, 0), "writing PNG: %s",
      o.message);
    if(o.warning_or_error) { PF("Warning writing PNG: %s", o.message); }
    // TODO: PNG OUTPUT INFO
#endif
    WebPFreeDecBuffer(&c.output);
    GETINFILE
  }
}
