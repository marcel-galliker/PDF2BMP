

typedef struct {
    int n_symbols;
    Jbig2Image **glyphs;
} Jbig2SymbolDict;

int
jbig2_symbol_dictionary(Jbig2Ctx *ctx, Jbig2Segment *segment,
			const byte *segment_data);

Jbig2Image *
jbig2_sd_glyph(Jbig2SymbolDict *dict, unsigned int id);

Jbig2SymbolDict *
jbig2_sd_new(Jbig2Ctx *ctx, int n_symbols);

void
jbig2_sd_release(Jbig2Ctx *ctx, Jbig2SymbolDict *dict);

Jbig2SymbolDict *
jbig2_sd_cat(Jbig2Ctx *ctx, int n_dicts,
			Jbig2SymbolDict **dicts);

int
jbig2_sd_count_referred(Jbig2Ctx *ctx, Jbig2Segment *segment);

Jbig2SymbolDict **
jbig2_sd_list_referred(Jbig2Ctx *ctx, Jbig2Segment *segment);

