

typedef struct {
  bool MMR;
  
  
  int GBTEMPLATE;
  bool TPGDON;
  bool USESKIP;
  
  int8_t gbat[8];
} Jbig2GenericRegionParams;

int
jbig2_generic_stats_size(Jbig2Ctx *ctx, int template);

int
jbig2_decode_generic_region(Jbig2Ctx *ctx,
			    Jbig2Segment *segment,
			    const Jbig2GenericRegionParams *params,
			    Jbig2ArithState *as,
			    Jbig2Image *image,
			    Jbig2ArithCx *GB_stats);

typedef struct {
  
  
  bool GRTEMPLATE;
  Jbig2Image *reference;
  int32_t DX, DY;
  bool TPGRON;
  int8_t grat[4];
} Jbig2RefinementRegionParams;

int
jbig2_decode_refinement_region(Jbig2Ctx *ctx,
                            Jbig2Segment *segment,
                            const Jbig2RefinementRegionParams *params,
                            Jbig2ArithState *as,
                            Jbig2Image *image,
                            Jbig2ArithCx *GB_stats);
