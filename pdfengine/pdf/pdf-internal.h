#ifndef PDF_INTERNAL_H
#define PDF_INTERNAL_H

#include "pdf.h"

struct base_warn_context_s
{
	char message[256];
	int count;
};

base_context *base_clone_context_internal(base_context *ctx);

void base_new_aa_context(base_context *ctx);
void base_free_aa_context(base_context *ctx);
void base_copy_aa_context(base_context *dst, base_context *src);

extern base_alloc_context base_alloc_default;

extern base_locks_context base_locks_default;

#if defined(DEBUG)
#define PDF_DEBUG_LOCKING
#endif

#ifdef PDF_DEBUG_LOCKING

void base_assert_lock_held(base_context *ctx, int lock);
void base_assert_lock_not_held(base_context *ctx, int lock);
void base_lock_debug_lock(base_context *ctx, int lock);
void base_lock_debug_unlock(base_context *ctx, int lock);

#else

#define base_assert_lock_held(A,B) do { } while (0)
#define base_assert_lock_not_held(A,B) do { } while (0)
#define base_lock_debug_lock(A,B) do { } while (0)
#define base_lock_debug_unlock(A,B) do { } while (0)

#endif 

static inline void
base_lock(base_context *ctx, int lock)
{
	base_lock_debug_lock(ctx, lock);
	ctx->locks->lock(ctx->locks->user, lock);
}

static inline void
base_unlock(base_context *ctx, int lock)
{
	base_lock_debug_unlock(ctx, lock);
	ctx->locks->unlock(ctx->locks->user, lock);
}

float base_atof(const char *s);

typedef struct base_hash_table_s base_hash_table;

base_hash_table *base_new_hash_table(base_context *ctx, int initialsize, int keylen, int lock);
void base_print_hash(base_context *ctx, FILE *out, base_hash_table *table);
void base_empty_hash(base_context *ctx, base_hash_table *table);
void base_free_hash(base_context *ctx, base_hash_table *table);

void *base_hash_find(base_context *ctx, base_hash_table *table, void *key);
void *base_hash_insert(base_context *ctx, base_hash_table *table, void *key, void *val);
void base_hash_remove(base_context *ctx, base_hash_table *table, void *key);

int base_hash_len(base_context *ctx, base_hash_table *table);
void *base_hash_get_key(base_context *ctx, base_hash_table *table, int idx);
void *base_hash_get_val(base_context *ctx, base_hash_table *table, int idx);

static inline int base_mul255(int a, int b)
{
	
	int x = a * b + 128;
	x += x >> 8;
	return x >> 8;
}

#define base_EXPAND(A) ((A)+((A)>>7))

#define base_COMBINE(A,B) (((A)*(B))>>8)

#define base_COMBINE2(A,B,C,D) (base_COMBINE((A), (B)) + base_COMBINE((C), (D)))

#define base_BLEND(SRC, DST, AMOUNT) ((((SRC)-(DST))*(AMOUNT) + ((DST)<<8))>>8)

void base_gridfit_matrix(base_matrix *m);
float base_matrix_max_expansion(base_matrix m);

typedef struct base_md5_s base_md5;

struct base_md5_s
{
	unsigned int state[4];
	unsigned int count[2];
	unsigned char buffer[64];
};

void base_md5_init(base_md5 *state);
void base_md5_update(base_md5 *state, const unsigned char *input, unsigned inlen);
void base_md5_final(base_md5 *state, unsigned char digest[16]);

typedef struct base_sha256_s base_sha256;

struct base_sha256_s
{
	unsigned int state[8];
	unsigned int count[2];
	union {
		unsigned char u8[64];
		unsigned int u32[16];
	} buffer;
};

void base_sha256_init(base_sha256 *state);
void base_sha256_update(base_sha256 *state, const unsigned char *input, unsigned int inlen);
void base_sha256_final(base_sha256 *state, unsigned char digest[32]);

typedef struct base_arc4_s base_arc4;

struct base_arc4_s
{
	unsigned x;
	unsigned y;
	unsigned char state[256];
};

void base_arc4_init(base_arc4 *state, const unsigned char *key, unsigned len);
void base_arc4_encrypt(base_arc4 *state, unsigned char *dest, const unsigned char *src, unsigned len);

typedef struct base_aes_s base_aes;

#define AES_DECRYPT 0
#define AES_ENCRYPT 1

struct base_aes_s
{
	int nr; 
	unsigned long *rk; 
	unsigned long buf[68]; 
};

void aes_setkey_enc( base_aes *ctx, const unsigned char *key, int keysize );
void aes_setkey_dec( base_aes *ctx, const unsigned char *key, int keysize );
void aes_crypt_cbc( base_aes *ctx, int mode, int length,
	unsigned char iv[16],
	const unsigned char *input,
	unsigned char *output );

typedef struct base_storable_s base_storable;

typedef void (base_store_free_fn)(base_context *, base_storable *);

struct base_storable_s {
	int refs;
	base_store_free_fn *free;
};

#define base_INIT_STORABLE(S_,RC,FREE) \
	do { base_storable *S = &(S_)->storable; S->refs = (RC); \
	S->free = (FREE); \
	} while (0)

void *base_keep_storable(base_context *, base_storable *);
void base_drop_storable(base_context *, base_storable *);

typedef struct base_store_hash_s base_store_hash;

struct base_store_hash_s
{
	base_store_free_fn *free;
	union
	{
		struct
		{
			int i0;
			int i1;
		} i;
		struct
		{
			void *ptr;
			int i;
		} pi;
	} u;
};

typedef struct base_store_type_s base_store_type;

struct base_store_type_s
{
	int (*make_hash_key)(base_store_hash *, void *);
	void *(*keep_key)(base_context *,void *);
	void (*drop_key)(base_context *,void *);
	int (*cmp_key)(void *, void *);
	void (*debug)(void *);
};

void base_new_store_context(base_context *ctx, unsigned int max);

void base_drop_store_context(base_context *ctx);

base_store *base_keep_store_context(base_context *ctx);

void base_print_store(base_context *ctx, FILE *out);

void *base_store_item(base_context *ctx, void *key, void *val, unsigned int itemsize, base_store_type *type);

void *base_find_item(base_context *ctx, base_store_free_fn *free, void *key, base_store_type *type);

void base_remove_item(base_context *ctx, base_store_free_fn *free, void *key, base_store_type *type);

void base_empty_store(base_context *ctx);

int base_store_scavenge(base_context *ctx, unsigned int size, int *phase);

struct base_buffer_s
{
	int refs;
	unsigned char *data;
	int cap, len;
};

base_buffer *base_new_buffer(base_context *ctx, int capacity);

void base_resize_buffer(base_context *ctx, base_buffer *buf, int capacity);

void base_grow_buffer(base_context *ctx, base_buffer *buf);

void base_trim_buffer(base_context *ctx, base_buffer *buf);

struct base_stream_s
{
	base_context *ctx;
	int refs;
	int error;
	int eof;
	int pos;
	int avail;
	int bits;
	int locked;
	unsigned char *bp, *rp, *wp, *ep;
	void *state;
	int (*read)(base_stream *stm, unsigned char *buf, int len);
	void (*close)(base_context *ctx, void *state);
	void (*seek)(base_stream *stm, int offset, int whence);
	unsigned char buf[4096];
};

void base_lock_stream(base_stream *stm);

base_stream *base_new_stream(base_context *ctx, void*, int(*)(base_stream*, unsigned char*, int), void(*)(base_context *, void *));
base_stream *base_keep_stream(base_stream *stm);
void base_fill_buffer(base_stream *stm);

void base_read_line(base_stream *stm, char *buf, int max);

static inline int base_read_byte(base_stream *stm)
{
	if (stm->rp == stm->wp)
	{
		base_fill_buffer(stm);
		return stm->rp < stm->wp ? *stm->rp++ : EOF;
	}
	return *stm->rp++;
}

static inline int base_peek_byte(base_stream *stm)
{
	if (stm->rp == stm->wp)
	{
		base_fill_buffer(stm);
		return stm->rp < stm->wp ? *stm->rp : EOF;
	}
	return *stm->rp;
}

static inline void base_unread_byte(base_stream *stm)
{
	if (stm->rp > stm->bp)
		stm->rp--;
}

static inline int base_is_eof(base_stream *stm)
{
	if (stm->rp == stm->wp)
	{
		if (stm->eof)
			return 1;
		return base_peek_byte(stm) == EOF;
	}
	return 0;
}

static inline unsigned int base_read_bits(base_stream *stm, int n)
{
	unsigned int x;

	if (n <= stm->avail)
	{
		stm->avail -= n;
		x = (stm->bits >> stm->avail) & ((1 << n) - 1);
	}
	else
	{
		x = stm->bits & ((1 << stm->avail) - 1);
		n -= stm->avail;
		stm->avail = 0;

		while (n > 8)
		{
			x = (x << 8) | base_read_byte(stm);
			n -= 8;
		}

		if (n > 0)
		{
			stm->bits = base_read_byte(stm);
			stm->avail = 8 - n;
			x = (x << n) | (stm->bits >> stm->avail);
		}
	}

	return x;
}

static inline void base_sync_bits(base_stream *stm)
{
	stm->avail = 0;
}

static inline int base_is_eof_bits(base_stream *stm)
{
	return base_is_eof(stm) && (stm->avail == 0 || stm->bits == EOF);
}

base_stream *base_open_copy(base_stream *chain);
base_stream *base_open_null(base_stream *chain, int len);
base_stream *base_open_arc4(base_stream *chain, unsigned char *key, unsigned keylen);
base_stream *base_open_aesd(base_stream *chain, unsigned char *key, unsigned keylen);
base_stream *base_open_a85d(base_stream *chain);
base_stream *base_open_ahxd(base_stream *chain);
base_stream *base_open_rld(base_stream *chain);
base_stream *base_open_dctd(base_stream *chain, int color_transform);
base_stream *base_open_resized_dctd(base_stream *chain, int color_transform, int factor);
base_stream *base_open_faxd(base_stream *chain,
	int k, int end_of_line, int encoded_byte_align,
	int columns, int rows, int end_of_block, int black_is_1);
base_stream *base_open_flated(base_stream *chain);
base_stream *base_open_lzwd(base_stream *chain, int early_change);
base_stream *base_open_predict(base_stream *chain, int predictor, int columns, int colors, int bpc);
base_stream *base_open_jbig2d(base_stream *chain, base_buffer *global);

enum { base_MAX_COLORS = 32 };

int base_lookup_blendmode(char *name);
char *base_blendmode_name(int blendmode);

struct base_bitmap_s
{
	int refs;
	int w, h, stride, n;
	unsigned char *samples;
};

base_bitmap *base_new_bitmap(base_context *ctx, int w, int h, int n);

void base_bitmap_details(base_bitmap *bitmap, int *w, int *h, int *n, int *stride);

void base_clear_bitmap(base_context *ctx, base_bitmap *bit);

struct base_pixmap_s
{
	base_storable storable;
	int x, y, w, h, n;
	int interpolate;
	int xres, yres;
	base_colorspace *colorspace;
	unsigned char *samples;
	int free_samples;
};

void base_free_pixmap_imp(base_context *ctx, base_storable *pix);

void base_clear_pixmap_rect_with_value(base_context *ctx, base_pixmap *pix, int value, base_bbox r);
void base_copy_pixmap_rect(base_context *ctx, base_pixmap *dest, base_pixmap *src, base_bbox r);
void base_premultiply_pixmap(base_context *ctx, base_pixmap *pix);
base_pixmap *base_alpha_from_gray(base_context *ctx, base_pixmap *gray, int luminosity);
unsigned int base_pixmap_size(base_context *ctx, base_pixmap *pix);

base_pixmap *base_scale_pixmap(base_context *ctx, base_pixmap *src, float x, float y, float w, float h, base_bbox *clip);

base_bbox base_pixmap_bbox_no_ctx(base_pixmap *src);

struct base_image_s
{
	base_storable storable;
	int w, h;
	base_image *mask;
	base_colorspace *colorspace;
	base_pixmap *(*get_pixmap)(base_context *, base_image *, int w, int h);
};

base_pixmap *base_load_jpx(base_context *ctx, unsigned char *data, int size, base_colorspace *cs, int indexed);
base_pixmap *base_load_jpeg(base_context *doc, unsigned char *data, int size);
base_pixmap *base_load_png(base_context *doc, unsigned char *data, int size);
base_pixmap *base_load_tiff(base_context *doc, unsigned char *data, int size);

struct base_halftone_s
{
	int refs;
	int n;
	base_pixmap *comp[1];
};

base_halftone *base_new_halftone(base_context *ctx, int num_comps);
base_halftone *base_default_halftone(base_context *ctx, int num_comps);
void base_drop_halftone(base_context *ctx, base_halftone *half);
base_halftone *base_keep_halftone(base_context *ctx, base_halftone *half);

struct base_colorspace_s
{
	base_storable storable;
	unsigned int size;
	char name[16];
	int n;
	void (*to_rgb)(base_context *ctx, base_colorspace *, float *src, float *rgb);
	void (*from_rgb)(base_context *ctx, base_colorspace *, float *rgb, float *dst);
	void (*free_data)(base_context *Ctx, base_colorspace *);
	void *data;
};

base_colorspace *base_new_colorspace(base_context *ctx, char *name, int n);
base_colorspace *base_keep_colorspace(base_context *ctx, base_colorspace *colorspace);
void base_drop_colorspace(base_context *ctx, base_colorspace *colorspace);
void base_free_colorspace_imp(base_context *ctx, base_storable *colorspace);

void base_convert_color(base_context *ctx, base_colorspace *dsts, float *dstv, base_colorspace *srcs, float *srcv);

char *ft_error_string(int err);

struct base_font_s
{
	int refs;
	char name[32];

	void *ft_face; 
	int ft_substitute; 
	int ft_bold; 
	int ft_italic; 
	int ft_hint; 

	
	char *ft_file;
	unsigned char *ft_data;
	int ft_size;

	base_matrix t3matrix;
	void *t3resources;
	base_buffer **t3procs; 
	float *t3widths; 
	char *t3flags; 
	void *t3doc; 
	void (*t3run)(void *doc, void *resources, base_buffer *contents, base_device *dev, base_matrix ctm, void *gstate);
	void (*t3freeres)(void *doc, void *resources);

	base_rect bbox;	

	
	int use_glyph_bbox;
	int bbox_count;
	base_rect *bbox_table;

	
	int width_count;
	int *width_table; 
};

void base_new_font_context(base_context *ctx);
base_font_context *base_keep_font_context(base_context *ctx);
void base_drop_font_context(base_context *ctx);

base_font *base_new_type3_font(base_context *ctx, char *name, base_matrix matrix);

base_font *base_new_font_from_memory(base_context *ctx, unsigned char *data, int len, int index, int use_glyph_bbox);
base_font *base_new_font_from_file(base_context *ctx, char *path, int index, int use_glyph_bbox);

base_font *base_keep_font(base_context *ctx, base_font *font);
void base_drop_font(base_context *ctx, base_font *font);

void base_print_font(base_context *ctx, FILE *out, base_font *font);

void base_set_font_bbox(base_context *ctx, base_font *font, float xmin, float ymin, float xmax, float ymax);
base_rect base_bound_glyph(base_context *ctx, base_font *font, int gid, base_matrix trm);
int base_glyph_cacheable(base_context *ctx, base_font *font, int gid);

typedef struct base_path_s base_path;
typedef struct base_stroke_state_s base_stroke_state;

typedef union base_path_item_s base_path_item;

typedef enum base_path_item_kind_e
{
	base_MOVETO,
	base_LINETO,
	base_CURVETO,
	base_CLOSE_PATH
} base_path_item_kind;

typedef enum base_linecap_e
{
	base_LINECAP_BUTT = 0,
	base_LINECAP_ROUND = 1,
	base_LINECAP_SQUARE = 2,
	base_LINECAP_TRIANGLE = 3
} base_linecap;

typedef enum base_linejoin_e
{
	base_LINEJOIN_MITER = 0,
	base_LINEJOIN_ROUND = 1,
	base_LINEJOIN_BEVEL = 2,
	base_LINEJOIN_MITER_XPS = 3
} base_linejoin;

union base_path_item_s
{
	base_path_item_kind k;
	float v;
};

struct base_path_s
{
	int len, cap;
	base_path_item *items;
	int last;
};

struct base_stroke_state_s
{
	int refs;
	base_linecap start_cap, dash_cap, end_cap;
	base_linejoin linejoin;
	float linewidth;
	float miterlimit;
	float dash_phase;
	int dash_len;
	float dash_list[32];
};

base_path *base_new_path(base_context *ctx);
void base_moveto(base_context*, base_path*, float x, float y);
void base_lineto(base_context*, base_path*, float x, float y);
void base_curveto(base_context*,base_path*, float, float, float, float, float, float);
void base_curvetov(base_context*,base_path*, float, float, float, float);
void base_curvetoy(base_context*,base_path*, float, float, float, float);
void base_closepath(base_context*,base_path*);
void base_free_path(base_context *ctx, base_path *path);

void base_transform_path(base_context *ctx, base_path *path, base_matrix transform);

base_path *base_clone_path(base_context *ctx, base_path *old);

base_rect base_bound_path(base_context *ctx, base_path *path, base_stroke_state *stroke, base_matrix ctm);
void base_print_path(base_context *ctx, FILE *out, base_path *, int indent);

base_stroke_state *base_new_stroke_state(base_context *ctx);
base_stroke_state *base_new_stroke_state_with_len(base_context *ctx, int len);
base_stroke_state *base_keep_stroke_state(base_context *ctx, base_stroke_state *stroke);
void base_drop_stroke_state(base_context *ctx, base_stroke_state *stroke);
base_stroke_state *base_unshare_stroke_state(base_context *ctx, base_stroke_state *shared);
base_stroke_state *base_unshare_stroke_state_with_len(base_context *ctx, base_stroke_state *shared, int len);

void base_new_glyph_cache_context(base_context *ctx);
base_glyph_cache *base_keep_glyph_cache(base_context *ctx);
void base_drop_glyph_cache_context(base_context *ctx);
void base_purge_glyph_cache(base_context *ctx);

base_pixmap *base_render_ft_glyph(base_context *ctx, base_font *font, int cid, base_matrix trm, int aa);
base_pixmap *base_render_t3_glyph(base_context *ctx, base_font *font, int cid, base_matrix trm, base_colorspace *model);
base_pixmap *base_render_ft_stroked_glyph(base_context *ctx, base_font *font, int gid, base_matrix trm, base_matrix ctm, base_stroke_state *state);
base_pixmap *base_render_glyph(base_context *ctx, base_font*, int, base_matrix, base_colorspace *model);
base_pixmap *base_render_stroked_glyph(base_context *ctx, base_font*, int, base_matrix, base_matrix, base_stroke_state *stroke);
void base_render_t3_glyph_direct(base_context *ctx, base_device *dev, base_font *font, int gid, base_matrix trm, void *gstate);

typedef struct base_text_s base_text;
typedef struct base_text_item_s base_text_item;

struct base_text_item_s
{
	float x, y;
	int gid; 
	int ucs; 
};

struct base_text_s
{
	base_font *font;
	base_matrix trm;
	int wmode;
	int len, cap;
	base_text_item *items;
};

base_text *base_new_text(base_context *ctx, base_font *face, base_matrix trm, int wmode);
void base_add_text(base_context *ctx, base_text *text, int gid, int ucs, float x, float y);
void base_free_text(base_context *ctx, base_text *text);
base_rect base_bound_text(base_context *ctx, base_text *text, base_matrix ctm);
base_text *base_clone_text(base_context *ctx, base_text *old);
void base_print_text(base_context *ctx, FILE *out, base_text*);

enum
{
	base_LINEAR,
	base_RADIAL,
	base_MESH,
};

typedef struct base_shade_s base_shade;

struct base_shade_s
{
	base_storable storable;

	base_rect bbox;		
	base_colorspace *colorspace;

	base_matrix matrix;	
	int use_background;	
	float background[base_MAX_COLORS];

	int use_function;
	float function[256][base_MAX_COLORS + 1];

	int type; 
	int extend[2];

	int mesh_len;
	int mesh_cap;
	float *mesh; 
};

base_shade *base_keep_shade(base_context *ctx, base_shade *shade);
void base_drop_shade(base_context *ctx, base_shade *shade);
void base_free_shade_imp(base_context *ctx, base_storable *shade);
void base_print_shade(base_context *ctx, FILE *out, base_shade *shade);

base_rect base_bound_shade(base_context *ctx, base_shade *shade, base_matrix ctm);
void base_paint_shade(base_context *ctx, base_shade *shade, base_matrix ctm, base_pixmap *dest, base_bbox bbox);

typedef struct base_gel_s base_gel;

base_gel *base_new_gel(base_context *ctx);
void base_insert_gel(base_gel *gel, float x0, float y0, float x1, float y1);
void base_reset_gel(base_gel *gel, base_bbox clip);
void base_sort_gel(base_gel *gel);
base_bbox base_bound_gel(base_gel *gel);
void base_free_gel(base_gel *gel);
int base_is_rect_gel(base_gel *gel);

void base_scan_convert(base_gel *gel, int eofill, base_bbox clip, base_pixmap *pix, unsigned char *colorbv);

void base_flatten_fill_path(base_gel *gel, base_path *path, base_matrix ctm, float flatness);
void base_flatten_stroke_path(base_gel *gel, base_path *path, base_stroke_state *stroke, base_matrix ctm, float flatness, float linewidth);
void base_flatten_dash_path(base_gel *gel, base_path *path, base_stroke_state *stroke, base_matrix ctm, float flatness, float linewidth);

base_device *base_new_draw_device_type3(base_context *ctx, base_pixmap *dest);

enum
{
	
	base_IGNORE_IMAGE = 1,
	base_IGNORE_SHADE = 2,

	
	base_DEVFLAG_MASK = 1,
	base_DEVFLAG_COLOR = 2,
	base_DEVFLAG_UNCACHEABLE = 4,
	base_DEVFLAG_FILLCOLOR_UNDEFINED = 8,
	base_DEVFLAG_STROKECOLOR_UNDEFINED = 16,
	base_DEVFLAG_STARTCAP_UNDEFINED = 32,
	base_DEVFLAG_DASHCAP_UNDEFINED = 64,
	base_DEVFLAG_ENDCAP_UNDEFINED = 128,
	base_DEVFLAG_LINEJOIN_UNDEFINED = 256,
	base_DEVFLAG_MITERLIMIT_UNDEFINED = 512,
	base_DEVFLAG_LINEWIDTH_UNDEFINED = 1024,
	base_DEVFLAG_ONLYIMAGE = 2048,
	
};

struct base_device_s
{
	int hints;
	int flags;

	void *user;
	void (*free_user)(base_device *);
	void (*flush_user)(base_device *);
	base_context *ctx;

	void (*fill_path)(base_device *, base_path *, int even_odd, base_matrix, base_colorspace *, float *color, float alpha);
	void (*stroke_path)(base_device *, base_path *, base_stroke_state *, base_matrix, base_colorspace *, float *color, float alpha);
	void (*clip_path)(base_device *, base_path *, base_rect *rect, int even_odd, base_matrix);
	void (*clip_stroke_path)(base_device *, base_path *, base_rect *rect, base_stroke_state *, base_matrix);

	void (*my_fill_text)(base_device *, base_text *, base_matrix, base_colorspace *, float *color, float alpha, base_bbox org_bbox, base_bbox scissor, double dAngle);
	void (*fill_text)(base_device *, base_text *, base_matrix, base_colorspace *, float *color, float alpha);
	void (*stroke_text)(base_device *, base_text *, base_stroke_state *, base_matrix, base_colorspace *, float *color, float alpha);
	void (*clip_text)(base_device *, base_text *, base_matrix, int accumulate);
	void (*clip_stroke_text)(base_device *, base_text *, base_stroke_state *, base_matrix);
	void (*ignore_text)(base_device *, base_text *, base_matrix);

	void (*fill_shade)(base_device *, base_shade *shd, base_matrix ctm, float alpha);
	void (*fill_image)(base_device *, base_image *img, base_matrix ctm, float alpha);
	void (*fill_image_mask)(base_device *, base_image *img, base_matrix ctm, base_colorspace *, float *color, float alpha);
	void (*clip_image_mask)(base_device *, base_image *img, base_rect *rect, base_matrix ctm);

	void (*pop_clip)(base_device *);

	void (*begin_mask)(base_device *, base_rect, int luminosity, base_colorspace *, float *bc);
	void (*end_mask)(base_device *);
	void (*begin_group)(base_device *, base_rect, int isolated, int knockout, int blendmode, float alpha);
	void (*end_group)(base_device *);

	void (*begin_tile)(base_device *, base_rect area, base_rect view, float xstep, float ystep, base_matrix ctm);
	void (*end_tile)(base_device *);
};

void base_fill_path(base_device *dev, base_path *path, int even_odd, base_matrix ctm, base_colorspace *colorspace, float *color, float alpha);
void base_stroke_path(base_device *dev, base_path *path, base_stroke_state *stroke, base_matrix ctm, base_colorspace *colorspace, float *color, float alpha);
void base_clip_path(base_device *dev, base_path *path, base_rect *rect, int even_odd, base_matrix ctm);
void base_clip_stroke_path(base_device *dev, base_path *path, base_rect *rect, base_stroke_state *stroke, base_matrix ctm);
void my_base_fill_text(base_device *dev, base_text *text, base_matrix ctm, base_colorspace *colorspace, float *color, float alpha, base_bbox org_bbox, base_bbox scissor, double dAngle);
void base_fill_text(base_device *dev, base_text *text, base_matrix ctm, base_colorspace *colorspace, float *color, float alpha);
void base_stroke_text(base_device *dev, base_text *text, base_stroke_state *stroke, base_matrix ctm, base_colorspace *colorspace, float *color, float alpha);
void base_clip_text(base_device *dev, base_text *text, base_matrix ctm, int accumulate);
void base_clip_stroke_text(base_device *dev, base_text *text, base_stroke_state *stroke, base_matrix ctm);
void base_ignore_text(base_device *dev, base_text *text, base_matrix ctm);
void base_pop_clip(base_device *dev);
void base_fill_shade(base_device *dev, base_shade *shade, base_matrix ctm, float alpha);
void base_fill_image(base_device *dev, base_image *image, base_matrix ctm, float alpha);
void base_fill_image_mask(base_device *dev, base_image *image, base_matrix ctm, base_colorspace *colorspace, float *color, float alpha);
void base_clip_image_mask(base_device *dev, base_image *image, base_rect *rect, base_matrix ctm);
void base_begin_mask(base_device *dev, base_rect area, int luminosity, base_colorspace *colorspace, float *bc);
void base_end_mask(base_device *dev);
void base_begin_group(base_device *dev, base_rect area, int isolated, int knockout, int blendmode, float alpha);
void base_end_group(base_device *dev);
void base_begin_tile(base_device *dev, base_rect area, base_rect view, float xstep, float ystep, base_matrix ctm);
void base_end_tile(base_device *dev);

base_device *base_new_device(base_context *ctx, void *user);

void base_decode_tile(base_pixmap *pix, float *decode);
void base_decode_indexed_tile(base_pixmap *pix, float *decode, int maxval);
void base_unpack_tile(base_pixmap *dst, unsigned char * restrict src, int n, int depth, int stride, int scale);

void base_paint_solid_alpha(unsigned char * restrict dp, int w, int alpha);
void base_paint_solid_color(unsigned char * restrict dp, int n, int w, unsigned char *color);

void base_paint_span(unsigned char * restrict dp, unsigned char * restrict sp, int n, int w, int alpha);
void base_paint_span_with_color(unsigned char * restrict dp, unsigned char * restrict mp, int n, int w, unsigned char *color);

void base_paint_image(base_pixmap *dst, base_bbox scissor, base_pixmap *shape, base_pixmap *img, base_matrix ctm, int alpha);
void base_paint_image_with_color(base_pixmap *dst, base_bbox scissor, base_pixmap *shape, base_pixmap *img, base_matrix ctm, unsigned char *colorbv);

void base_paint_pixmap(base_pixmap *dst, base_pixmap *src, int alpha);
void base_paint_pixmap_with_mask(base_pixmap *dst, base_pixmap *src, base_pixmap *msk);
void base_paint_pixmap_with_rect(base_pixmap *dst, base_pixmap *src, int alpha, base_bbox bbox);

void base_blend_pixmap(base_pixmap *dst, base_pixmap *src, int alpha, int blendmode, int isolated, base_pixmap *shape);
void base_blend_pixel(unsigned char dp[3], unsigned char bp[3], unsigned char sp[3], int blendmode);

enum
{
	
	base_BLEND_NORMAL,
	base_BLEND_MULTIPLY,
	base_BLEND_SCREEN,
	base_BLEND_OVERLAY,
	base_BLEND_DARKEN,
	base_BLEND_LIGHTEN,
	base_BLEND_COLOR_DODGE,
	base_BLEND_COLOR_BURN,
	base_BLEND_HARD_LIGHT,
	base_BLEND_SOFT_LIGHT,
	base_BLEND_DIFFERENCE,
	base_BLEND_EXCLUSION,

	
	base_BLEND_HUE,
	base_BLEND_SATURATION,
	base_BLEND_COLOR,
	base_BLEND_LUMINOSITY,

	
	base_BLEND_MODEMASK = 15,
	base_BLEND_ISOLATED = 16,
	base_BLEND_KNOCKOUT = 32
};

struct base_document_s
{
	void (*close)(base_document *);
	int (*needs_password)(base_document *doc);
	int (*authenticate_password)(base_document *doc, char *password);
	base_outline *(*load_outline)(base_document *doc);
	int (*count_pages)(base_document *doc);
	base_page *(*load_page)(base_document *doc, int number);
	base_link *(*load_links)(base_document *doc, base_page *page);
	base_rect (*bound_page)(base_document *doc, base_page *page);
	void (*run_page)(base_document *doc, base_page *page, base_device *dev, base_matrix transform, base_cookie *cookie);
	void (*free_page)(base_document *doc, base_page *page);
	int (*meta)(base_document *doc, int key, void *ptr, int size);
};

typedef struct base_display_node_s base_display_node;

#define STACK_SIZE 96

typedef enum base_display_command_e
{
	base_CMD_FILL_PATH,
	base_CMD_STROKE_PATH,
	base_CMD_CLIP_PATH,
	base_CMD_CLIP_STROKE_PATH,
	base_CMD_FILL_TEXT,
	base_CMD_STROKE_TEXT,
	base_CMD_CLIP_TEXT,
	base_CMD_CLIP_STROKE_TEXT,
	base_CMD_IGNORE_TEXT,
	base_CMD_FILL_SHADE,
	base_CMD_FILL_IMAGE,
	base_CMD_FILL_IMAGE_MASK,
	base_CMD_CLIP_IMAGE_MASK,
	base_CMD_POP_CLIP,
	base_CMD_BEGIN_MASK,
	base_CMD_END_MASK,
	base_CMD_BEGIN_GROUP,
	base_CMD_END_GROUP,
	base_CMD_BEGIN_TILE,
	base_CMD_END_TILE
} base_display_command;

#define NON_TYPE				0
#define TABLE_TYPE				1
#define UNDERLINE_TYPE			2
#define SHAPE_TYPE				3
#define PATH_TYPE				4
#define IMAGE_TYPE				5

struct base_display_node_s
{
	base_display_command cmd;
	base_display_node *next;
	base_rect rect;
	union {
		base_path *path;
		base_text *text;
		base_shade *shade;
		base_image *image;
		int blendmode;
	} item;
	base_stroke_state *stroke;
	int flag; 
	int isFillPath;
	base_matrix ctm;
	base_colorspace *colorspace;
	float alpha;
	float color[base_MAX_COLORS];

	int index;
	int type;
};

struct base_display_list_s
{
	base_display_node *first;
	base_display_node *last;

	int top;
	struct {
		base_rect *update;
		base_rect rect;
	} stack[STACK_SIZE];
	int tiled;
};

void base_save_one_image(base_device *devp, base_image *image, base_matrix ctm, float alpha, wchar_t *filename, wchar_t *format);

#endif
