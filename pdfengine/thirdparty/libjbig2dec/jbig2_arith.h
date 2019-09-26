

typedef struct _Jbig2ArithState Jbig2ArithState;

typedef unsigned char Jbig2ArithCx;

Jbig2ArithState *
jbig2_arith_new (Jbig2Ctx *ctx, Jbig2WordStream *ws);

bool
jbig2_arith_decode (Jbig2ArithState *as, Jbig2ArithCx *pcx);

