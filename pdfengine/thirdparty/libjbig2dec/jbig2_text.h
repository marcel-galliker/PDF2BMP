

typedef enum {
    JBIG2_CORNER_BOTTOMLEFT = 0,
    JBIG2_CORNER_TOPLEFT = 1,
    JBIG2_CORNER_BOTTOMRIGHT = 2,
    JBIG2_CORNER_TOPRIGHT = 3
} Jbig2RefCorner;

typedef struct {
    bool SBHUFF;
    bool SBREFINE;
    bool SBDEFPIXEL;
    Jbig2ComposeOp SBCOMBOP;
    bool TRANSPOSED;
    Jbig2RefCorner REFCORNER;
    int SBDSOFFSET;
    
    
    uint32_t SBNUMINSTANCES;
    int LOGSBSTRIPS;
    int SBSTRIPS;
    
    
    
    
    Jbig2HuffmanTable *SBHUFFFS;
    Jbig2HuffmanTable *SBHUFFDS;
    Jbig2HuffmanTable *SBHUFFDT;
    Jbig2HuffmanTable *SBHUFFRDW;
    Jbig2HuffmanTable *SBHUFFRDH;
    Jbig2HuffmanTable *SBHUFFRDX;
    Jbig2HuffmanTable *SBHUFFRDY;
    Jbig2HuffmanTable *SBHUFFRSIZE;
    Jbig2ArithIntCtx *IADT;
    Jbig2ArithIntCtx *IAFS;
    Jbig2ArithIntCtx *IADS;
    Jbig2ArithIntCtx *IAIT;
    Jbig2ArithIaidCtx *IAID;
    Jbig2ArithIntCtx *IARI;
    Jbig2ArithIntCtx *IARDW;
    Jbig2ArithIntCtx *IARDH;
    Jbig2ArithIntCtx *IARDX;
    Jbig2ArithIntCtx *IARDY;
    bool SBRTEMPLATE;
    int8_t sbrat[4];
} Jbig2TextRegionParams;

int
jbig2_decode_text_region(Jbig2Ctx *ctx, Jbig2Segment *segment,
                             const Jbig2TextRegionParams *params,
                             const Jbig2SymbolDict * const *dicts, const int n_dicts,
                             Jbig2Image *image,
                             const byte *data, const size_t size,
			     Jbig2ArithCx *GR_stats,
			     Jbig2ArithState *as, Jbig2WordStream *ws);
