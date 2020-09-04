// vi: sw=2
#include <webp/encode.h>
#ifdef PAM
#include <pam.h>
#endif
#define INEXT Z
#define OUTEXT "webp"
#include "png2webp.h"
static FILE *fp;
static int w(const uint8_t *d, size_t s, const WebPPicture *x) {
  (void)x;
  return s ? fwrite(d, s, 1, fp) : 1;
}
int main(int argc, char **argv) {
  GETARGS
  while(1) {
#ifdef PAM
    pm_init("ERROR", 0); // TODO: maybe *argv or (INEXT "2" OUTEXT) ?
    struct pam i;
    pnm_readpaminit(fp, &i, PAM_STRUCT_SIZE(opacity_plane));
    E(i.visual, "nonstandard tuple type: \"%s\"", i.tuple_type);
    tuple *r = pnm_allocpamrow(&i);
#else
    png_image i = {.version = 1};
#define EP(f, s, d) \
  E(f, "reading PNG %s: %s", s, i.message); \
  if(i.warning_or_error) { \
    PF("PNG info warning: %s", i.message); \
    if(d) { i.warning_or_error = 0; } \
  }
    EP(png_image_begin_read_from_stdio(&i, fp), "info", 1);
    if(i.format & PNG_FORMAT_FLAG_LINEAR) {
      P("Warning: input PNG is 16bpc, will be downsampled to 8bpc");
    }
    bool A = !!(i.format & PNG_FORMAT_FLAG_ALPHA);
    i.format = (*(uint8_t *)&(uint16_t){1}) ? PNG_FORMAT_BGRA : PNG_FORMAT_ARGB;
#endif
    WebPPicture o = {1, .width = i.width, i.height, .writer = w};
    WebPAuxStats s;
    if(verbose) { o.stats = &s; }
    // progress_hook only reports 1,5,90,100 for lossless
#define EW(f, s) \
  E(f, "%sing WebP: %s (%d)", s, es[o.error_code - 1], o.error_code)
    char *es[VP8_ENC_ERROR_LAST - 1] = {"out of RAM",
					"out of RAM flushing bitstream",
					"something was null",
					"broken config",
					"image too big (max. 16383x16383 px)",
					"partition >512KiB",
					"partition >16MiB",
					"couldn't write",
					"output >4GiB",
					"you cancelled it"};
    EW(WebPPictureAlloc(&o), "allocat");
#ifdef PAM
    if(255 % i.maxval) {
      PF("Warning: scaling from maxval %lu to 255", i.maxval);
    }
    for(int y = 0; y < i.height; y++) {
      pnm_readpamrow(&i, r);
      pnm_scaletuplerow(&i, r, r, 255);
#define A i.have_opacity
#define D (i.depth > 2)
      for(int x = 0; x < i.width; x++) {
	o.argb[y * i.width + x] = ((A ? r[x][i.opacity_plane] : 255) << 24) |
				  (r[x][0] << 16) | (r[x][D ? 1 : 0] << 8) |
				  r[x][D ? 2 : 0];
	// don't need &255, libnetpbm errors out on values >maxval
      }
    }
    pnm_freepamrow(r);
#else
    EP(png_image_finish_read(&i, 0, o.argb, 0, 0), "data", 0);
#endif
#ifdef NOTHREADS
#define THREADLEVEL
#else
#define THREADLEVEL .thread_level = 1, // doesn't seem to affect output
#endif
    GETOUTFILE
    EW(WebPEncode(&(WebPConfig){1, 100, 6,	 // lossless,max
				WEBP_HINT_GRAPH, /*see vp8l_enc.c#L1841
				 should be default imo
				 16bpp is only for alpha on lossy*/
				THREADLEVEL
				.near_lossless = 100,
				// ^ don't modify visible pixels
				// .exact=1, // don't delete invisible pixels
				.pass = 1, // unused, for WebPValidateConfig
				.segments = 1}, // ditto
		  &o),
       "encod");
#define F s.lossless_features
#define C s.palette_size
    PFV("Output WebP info:\n"
	"Dimensions: %u x %u\n"
	"Size: %u bytes (%.17g bpp)\n"
	"Header size: %u, image data size: %u\n"
	"Uses alpha: %s\n"
	"Precision bits: histogram=%u transform=%u cache=%u\n"
	"Lossless features:%s%s%s%s\n"
	"Colors: %s%u",
	o.width, o.height,
	s.coded_size, s.coded_size * 8. / (o.width * o.height),
	s.lossless_hdr_size, s.lossless_data_size,
	A && WebPPictureHasTransparency(&o) ? "yes" : "no",
	s.histogram_bits, s.transform_bits, s.cache_bits,
	F ? F & 1 ? " prediction" : "" : " none",
	F && F & 2 ? " cross-color" : "",
	F && F & 4 ? " subtract-green" : "",
	F && F & 8 ? " palette" : "",
	C ? "" : ">", C ? C : 256);
    WebPPictureFree(&o);
    GETINFILE
  }
}
