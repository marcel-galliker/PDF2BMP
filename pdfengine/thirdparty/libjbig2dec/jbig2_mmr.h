

int
jbig2_decode_generic_mmr(Jbig2Ctx *ctx,
			 Jbig2Segment *segment,
			 const Jbig2GenericRegionParams *params,
			 const byte *data, size_t size,
			 Jbig2Image *image);

int
jbig2_decode_halftone_mmr(Jbig2Ctx *ctx,
	const Jbig2GenericRegionParams *params,
	const byte *data, size_t size,
	Jbig2Image *image, size_t* consumed_bytes);

