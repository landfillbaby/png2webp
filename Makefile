PREFIX ?= /usr/local
INSTALL ?= install
uname_m := ${shell uname -m}
ifeq (${uname_m},x86_64)
CFLAGS ?= -O3 -Wall -Wextra -pipe -flto=auto -DNDEBUG -march=x86-64-v2
else ifeq (${uname_m},aarch64)
CFLAGS ?= -O3 -Wall -Wextra -pipe -flto=auto -DNDEBUG -march=armv8-a+crc
else
CFLAGS ?= -O3 -Wall -Wextra -pipe -flto=auto -DNDEBUG
endif
CFLAGS += -Ilibpng -Izlib -Ilibwebp -Ilibwebp/src -DFIXEDGAMMA
CFLAGS += -DPNG_ARM_NEON -DPNG_MIPS_MSA -DPNG_INTEL_SSE -DPNG_POWERPC_VSX
ifeq (${OS},Windows_NT)
LDFLAGS ?= -s -Wl,--as-needed,--gc-sections,--no-insert-timestamp
else
LDFLAGS ?= -s -Wl,--as-needed,--gc-sections
endif
LDLIBS ?= -lm
png2webp: png2webp.c libpng/png.c libpng/pngerror.c libpng/pngget.c \
	libpng/pngmem.c libpng/pngpread.c libpng/pngread.c libpng/pngrio.c \
	libpng/pngrtran.c libpng/pngrutil.c libpng/pngset.c libpng/pngtrans.c \
	libpng/pngwio.c libpng/pngwrite.c libpng/pngwtran.c libpng/pngwutil.c \
	libpng/arm/arm_init.c libpng/arm/filter_neon_intrinsics.c \
	libpng/arm/palette_neon_intrinsics.c \
	libpng/intel/filter_sse2_intrinsics.c libpng/intel/intel_init.c \
	libpng/mips/filter_msa_intrinsics.c libpng/mips/mips_init.c \
	libpng/powerpc/filter_vsx_intrinsics.c libpng/powerpc/powerpc_init.c \
	zlib/adler32.c zlib/crc32.c zlib/deflate.c zlib/infback.c \
	zlib/inffast.c zlib/inflate.c zlib/inftrees.c zlib/trees.c \
	zlib/zutil.c libwebp/src/dec/alpha_dec.c libwebp/src/dec/buffer_dec.c \
	libwebp/src/dec/frame_dec.c libwebp/src/dec/idec_dec.c \
	libwebp/src/dec/io_dec.c libwebp/src/dec/quant_dec.c \
	libwebp/src/dec/tree_dec.c libwebp/src/dec/vp8l_dec.c \
	libwebp/src/dec/vp8_dec.c libwebp/src/dec/webp_dec.c \
	libwebp/src/dsp/alpha_processing.c \
	libwebp/src/dsp/alpha_processing_mips_dsp_r2.c \
	libwebp/src/dsp/alpha_processing_neon.c \
	libwebp/src/dsp/alpha_processing_sse2.c \
	libwebp/src/dsp/alpha_processing_sse41.c \
	libwebp/src/dsp/cost.c libwebp/src/dsp/cost_mips32.c \
	libwebp/src/dsp/cost_mips_dsp_r2.c libwebp/src/dsp/cost_neon.c \
	libwebp/src/dsp/cost_sse2.c libwebp/src/dsp/cpu.c \
	libwebp/src/dsp/dec.c libwebp/src/dsp/dec_clip_tables.c \
	libwebp/src/dsp/dec_mips32.c libwebp/src/dsp/dec_mips_dsp_r2.c \
	libwebp/src/dsp/dec_msa.c libwebp/src/dsp/dec_neon.c \
	libwebp/src/dsp/dec_sse2.c libwebp/src/dsp/dec_sse41.c \
	libwebp/src/dsp/enc.c libwebp/src/dsp/enc_mips32.c \
	libwebp/src/dsp/enc_mips_dsp_r2.c libwebp/src/dsp/enc_msa.c \
	libwebp/src/dsp/enc_neon.c libwebp/src/dsp/enc_sse2.c \
	libwebp/src/dsp/enc_sse41.c libwebp/src/dsp/filters.c \
	libwebp/src/dsp/filters_mips_dsp_r2.c libwebp/src/dsp/filters_msa.c \
	libwebp/src/dsp/filters_neon.c libwebp/src/dsp/filters_sse2.c \
	libwebp/src/dsp/lossless.c libwebp/src/dsp/lossless_enc.c \
	libwebp/src/dsp/lossless_enc_mips32.c \
	libwebp/src/dsp/lossless_enc_mips_dsp_r2.c \
	libwebp/src/dsp/lossless_enc_msa.c libwebp/src/dsp/lossless_enc_neon.c \
	libwebp/src/dsp/lossless_enc_sse2.c \
	libwebp/src/dsp/lossless_enc_sse41.c \
	libwebp/src/dsp/lossless_mips_dsp_r2.c libwebp/src/dsp/lossless_msa.c \
	libwebp/src/dsp/lossless_neon.c libwebp/src/dsp/lossless_sse2.c \
	libwebp/src/dsp/lossless_sse41.c libwebp/src/dsp/rescaler.c \
	libwebp/src/dsp/rescaler_mips32.c \
	libwebp/src/dsp/rescaler_mips_dsp_r2.c libwebp/src/dsp/rescaler_msa.c \
	libwebp/src/dsp/rescaler_neon.c libwebp/src/dsp/rescaler_sse2.c \
	libwebp/src/dsp/ssim.c libwebp/src/dsp/ssim_sse2.c \
	libwebp/src/dsp/upsampling.c libwebp/src/dsp/upsampling_mips_dsp_r2.c \
	libwebp/src/dsp/upsampling_msa.c libwebp/src/dsp/upsampling_neon.c \
	libwebp/src/dsp/upsampling_sse2.c libwebp/src/dsp/upsampling_sse41.c \
	libwebp/src/dsp/yuv.c libwebp/src/dsp/yuv_mips32.c \
	libwebp/src/dsp/yuv_mips_dsp_r2.c libwebp/src/dsp/yuv_neon.c \
	libwebp/src/dsp/yuv_sse2.c libwebp/src/dsp/yuv_sse41.c \
	libwebp/src/enc/alpha_enc.c libwebp/src/enc/analysis_enc.c \
	libwebp/src/enc/backward_references_cost_enc.c \
	libwebp/src/enc/backward_references_enc.c libwebp/src/enc/config_enc.c \
	libwebp/src/enc/cost_enc.c libwebp/src/enc/filter_enc.c \
	libwebp/src/enc/frame_enc.c libwebp/src/enc/histogram_enc.c \
	libwebp/src/enc/iterator_enc.c libwebp/src/enc/near_lossless_enc.c \
	libwebp/src/enc/picture_csp_enc.c libwebp/src/enc/picture_enc.c \
	libwebp/src/enc/picture_psnr_enc.c \
	libwebp/src/enc/picture_rescale_enc.c \
	libwebp/src/enc/picture_tools_enc.c libwebp/src/enc/predictor_enc.c \
	libwebp/src/enc/quant_enc.c libwebp/src/enc/syntax_enc.c \
	libwebp/src/enc/token_enc.c libwebp/src/enc/tree_enc.c \
	libwebp/src/enc/vp8l_enc.c libwebp/src/enc/webp_enc.c \
	libwebp/src/utils/bit_reader_utils.c \
	libwebp/src/utils/bit_writer_utils.c \
	libwebp/src/utils/color_cache_utils.c \
	libwebp/src/utils/filters_utils.c \
	libwebp/src/utils/huffman_encode_utils.c \
	libwebp/src/utils/huffman_utils.c \
	libwebp/src/utils/quant_levels_dec_utils.c \
	libwebp/src/utils/quant_levels_utils.c \
	libwebp/src/utils/random_utils.c libwebp/src/utils/rescaler_utils.c \
	libwebp/src/utils/thread_utils.c libwebp/src/utils/utils.c
exestamp: exestamp.c
png2webp_timestamped: png2webp exestamp
	./exestamp png2webp.exe ${shell git show -s --format=%ct}
install: png2webp
	${INSTALL} $^ ${DESTDIR}${PREFIX}/bin/
clean:
	${RM} png2webp exestamp
