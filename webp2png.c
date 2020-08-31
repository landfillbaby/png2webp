// vi: sw=2
#include <webp/decode.h>
/*TODO: ideally should palette if <=256 colors (in order of appearance),
or at least try to palette when input webp was,
but that's not part of either libpng encoding or libwebp decoding*/
/*#include<webp/encode.h>//for WebPPicture
WEBP_EXTERN int WebPGetColorPalette(//declared in libwebp utils/utils.h
const struct WebPPicture*const,uint32_t*const);*/
// TODO: should also try to compress somewhat better
#define inext "webp"
#define outext z
#include "webp2png.h"
#ifdef DEBUG_IDEC
#define s 256
#else
#define s 16384
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
    uint8_t i[s];
    int l = fread(i, 1, s, fd);
    char *k[] = {"out of RAM",		"invalid params", "bitstream broke",
		 "unsupported feature", "suspended",	  "you cancelled it",
		 "not enough data"};
#define f c.input
#define a f.has_alpha
    int r;
    e(!(r = WebPGetFeatures(i, l, &f)), "reading WebP header: %d (%s)", r,
      r & ~7 ? "???" : k[r - 1]);
#define v f.format
#define w f.width
#define h f.height
    pfv("input WebP info:\ndimensions: %u x %u\nuses alpha: %s\n"
	"format: %s (%d)",
	w, h, a ? "yes" : "no",
	(unsigned)v < 3 ?
	    (char *[]){"undefined/mixed", "lossy", "lossless"}[v] :
	    "???",
	v);
    e(!f.has_animation, "reading WebP header: 4 (%s: animation)", k[3]);
    if(a) { c.output.colorspace = MODE_RGBA; }
    WebPIDecoder *d;
    e(d = WebPIDecode(i, l, &c), "initializing WebP decoder: 1 (%s)", k[0]);
    for(int x = l; (r = WebPIAppend(d, i, x)); l += x) {
      e(r == 5 && !feof(fd), "reading WebP data: %d (%s)", r == 5 ? 7 : r,
	r == 5 ? k[6] : (r & ~7 ? "???" : k[r - 1]));
      x = fread(i, 1, s, fd);
    }
    WebPIDelete(d);
    pfv("size: %u bytes (%.17g bpp)", l, (l * 8.) / (w * h));
    GETOUTFILE
#define d c.output.u.RGBA
#ifdef PAM
    fprintf(fd,
	    a ? "P7\nWIDTH %u\nHEIGHT %u\nDEPTH 4\nMAXVAL 255\n"
		"TUPLTYPE RGB_ALPHA\nENDHDR\n" :
		"P6\n%u %u\n255\n",
	    w, h);
    fwrite(d.rgba, d.size, 1, fd);
#else
    png_image o = {0, PNG_IMAGE_VERSION, w, h,
		   a ? PNG_FORMAT_RGBA : PNG_FORMAT_RGB};
    e(png_image_write_to_stdio(&o, fd, 0, d.rgba, 0, 0), "writing PNG: %s",
      o.message);
    if(o.warning_or_error) { pf("warning writing PNG: %s", o.message); }
    // TODO: PNG OUTPUT INFO
#endif
    WebPFreeDecBuffer(&c.output);
    GETINFILE
  }
}
