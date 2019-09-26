

#ifndef JBIG2_HUFFMAN_H
#define JBIG2_HUFFMAN_H

typedef struct _Jbig2HuffmanEntry Jbig2HuffmanEntry;
typedef struct _Jbig2HuffmanState Jbig2HuffmanState;
typedef struct _Jbig2HuffmanTable Jbig2HuffmanTable;
typedef struct _Jbig2HuffmanParams Jbig2HuffmanParams;

struct _Jbig2HuffmanEntry {
  union {
    int32_t RANGELOW;
    Jbig2HuffmanTable *ext_table;
  } u;
  byte PREFLEN;
  byte RANGELEN;
  byte flags;
};

struct _Jbig2HuffmanTable {
  int log_table_size;
  Jbig2HuffmanEntry *entries;
};

typedef struct _Jbig2HuffmanLine Jbig2HuffmanLine;

struct _Jbig2HuffmanLine {
  int PREFLEN;
  int RANGELEN;
  int RANGELOW;
};

struct _Jbig2HuffmanParams {
  bool HTOOB;
  int n_lines;
  const Jbig2HuffmanLine *lines;
};

Jbig2HuffmanState *
jbig2_huffman_new (Jbig2Ctx *ctx, Jbig2WordStream *ws);

void
jbig2_huffman_free (Jbig2Ctx *ctx, Jbig2HuffmanState *hs);

void
jbig2_huffman_skip(Jbig2HuffmanState *hs);

void jbig2_huffman_advance(Jbig2HuffmanState *hs, int offset);

int
jbig2_huffman_offset(Jbig2HuffmanState *hs);

int32_t
jbig2_huffman_get (Jbig2HuffmanState *hs,
		   const Jbig2HuffmanTable *table, bool *oob);

int32_t
jbig2_huffman_get_bits (Jbig2HuffmanState *hs, const int bits);

#ifdef JBIG2_DEBUG
void jbig2_dump_huffman_state(Jbig2HuffmanState *hs);
void jbig2_dump_huffman_binary(Jbig2HuffmanState *hs);
#endif

Jbig2HuffmanTable *
jbig2_build_huffman_table (Jbig2Ctx *ctx, const Jbig2HuffmanParams *params);

void
jbig2_release_huffman_table (Jbig2Ctx *ctx, Jbig2HuffmanTable *table);

extern const Jbig2HuffmanParams jbig2_huffman_params_A; 
extern const Jbig2HuffmanParams jbig2_huffman_params_B; 
extern const Jbig2HuffmanParams jbig2_huffman_params_C; 
extern const Jbig2HuffmanParams jbig2_huffman_params_D; 
extern const Jbig2HuffmanParams jbig2_huffman_params_E; 
extern const Jbig2HuffmanParams jbig2_huffman_params_F; 
extern const Jbig2HuffmanParams jbig2_huffman_params_G; 
extern const Jbig2HuffmanParams jbig2_huffman_params_H; 
extern const Jbig2HuffmanParams jbig2_huffman_params_I; 
extern const Jbig2HuffmanParams jbig2_huffman_params_J; 
extern const Jbig2HuffmanParams jbig2_huffman_params_K; 
extern const Jbig2HuffmanParams jbig2_huffman_params_L; 
extern const Jbig2HuffmanParams jbig2_huffman_params_M; 
extern const Jbig2HuffmanParams jbig2_huffman_params_N; 
extern const Jbig2HuffmanParams jbig2_huffman_params_O; 

int
jbig2_table(Jbig2Ctx *ctx, Jbig2Segment *segment, const byte *segment_data);

void
jbig2_table_free(Jbig2Ctx *ctx, Jbig2HuffmanParams *params);

const Jbig2HuffmanParams *
jbig2_find_table(Jbig2Ctx *ctx, Jbig2Segment *segment, int index);

#endif 
