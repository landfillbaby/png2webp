// anti-copyright Lucy Phipps 2020
// vi: sw=2 tw=80
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
#endif
#define OUTEXT "webp"
#define OUTEXTCHK OUTEXT
#define EXTRALETTERS "e"
#define EXTRAHELP "-e: Keep RGB data on pixels where alpha is 0.\n"
#define EXTRAFLAGS case 'e': exact = 1; break;
#include "png2webp.h"
static FILE* fp;
static int w(const uint8_t* d, size_t s, const WebPPicture* x) {
	(void)x;
	return s ? (int)fwrite(d, s, 1, fp) : 1;
}
int main(int argc, char** argv) {
#if !defined(PAM) && !defined(USEADVANCEDPNG)
  uint32_t endian;
  memcpy(&endian, (char[4]){"\xAA\xBB\xCC\xDD"}, 4);
  E(endian == 0xAABBCCDD || endian == 0xDDCCBBAA,
	"32-bit mixed-endianness (%X) not supported", endian);
#endif
#ifdef PAM
  pm_init("ERROR", 0); // TODO: maybe *argv or (INEXT "2" OUTEXT) ?
#endif
  bool exact = 0;
  GETARGS
  for(;;) {
#ifdef PAM
    struct pam i;
    pnm_readpaminit(fp, &i, PAM_STRUCT_SIZE(opacity_plane));
    E(i.visual, "nonstandard tuple type: \"%s\"", i.tuple_type);
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
#define A i.have_opacity
#define D (i.depth > 2)
	for(unsigned x = 0; x < W; x++) o.argb[y * W + x] =
		(((A ? r[x][i.opacity_plane] : 255) & 255) << 24) |
		((*r[x] & 255) << 16) | ((r[x][D] & 255) << 8) |
		(r[x][D * 2] & 255);
    }
    pnm_freepamrow(r);
#else
#ifdef USEADVANCEDPNG
#error // TODO
#else
    EP(png_image_finish_read(&i, 0, o.argb, 0, 0), "data", 0);
#endif
#endif
#ifdef NOTHREADS
#define THREADLEVEL
#else
#define THREADLEVEL .thread_level = 1,
#endif
    GETOUTFILE
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
	8. * (unsigned)s.coded_size / (uint32_t)(o.width * o.height),
	s.lossless_hdr_size, s.lossless_data_size,
	A && WebPPictureHasTransparency(&o) ? "yes" : "no",
	s.histogram_bits, s.transform_bits, s.cache_bits,
	F ? F & 1 ? " prediction" : "" : " none",
	F && F & 2 ? " cross-color" : "", F && F & 4 ? " subtract-green" : "",
	F && F & 8 ? " palette" : "", C ? "" : ">", C ? C : 256);
    WebPPictureFree(&o);
    GETINFILE
} }
