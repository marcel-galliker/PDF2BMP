#ifndef PDFENGINE_INTERNAL_H
#define PDFENGINE_INTERNAL_H

#include "pdfengine.h"
#include "pdf-internal.h"

void pdf_set_str_len(pdf_obj *obj, int newlen);
void *pdf_get_indirect_document(pdf_obj *obj);

typedef struct pdf_image_params_s pdf_image_params;

struct pdf_image_params_s
{
	int type;
	base_colorspace *colorspace;
	union
	{
		struct
		{
			int columns;
			int rows;
			int k;
			int eol;
			int eba;
			int eob;
			int bi1;
		}
		fax;
		struct
		{
			int ct;
		}
		jpeg;
		struct
		{
			int columns;
			int colors;
			int predictor;
			int bpc;
		}
		flate;
		struct
		{
			int columns;
			int colors;
			int predictor;
			int bpc;
			int ec;
		}
		lzw;
	}
	u;
};

typedef struct pdf_image_s pdf_image;

struct pdf_image_s
{
	base_image base;
	base_pixmap *tile;
	int n, bpc;
	pdf_image_params params;
	base_buffer *buffer;
	int colorkey[base_MAX_COLORS * 2];
	float decode[base_MAX_COLORS * 2];
	int imagemask;
	int interpolate;
	int usecolorkey;
};

enum
{
	PDF_IMAGE_RAW,
	PDF_IMAGE_FAX,
	PDF_IMAGE_JPEG,
	PDF_IMAGE_RLD,
	PDF_IMAGE_FLATE,
	PDF_IMAGE_LZW,
	PDF_IMAGE_JPX
};

enum
{
	PDF_TOK_ERROR, PDF_TOK_EOF,
	PDF_TOK_OPEN_ARRAY, PDF_TOK_CLOSE_ARRAY,
	PDF_TOK_OPEN_DICT, PDF_TOK_CLOSE_DICT,
	PDF_TOK_OPEN_BRACE, PDF_TOK_CLOSE_BRACE,
	PDF_TOK_NAME, PDF_TOK_INT, PDF_TOK_REAL, PDF_TOK_STRING, PDF_TOK_KEYWORD,
	PDF_TOK_R, PDF_TOK_TRUE, PDF_TOK_FALSE, PDF_TOK_NULL,
	PDF_TOK_OBJ, PDF_TOK_ENDOBJ,
	PDF_TOK_STREAM, PDF_TOK_ENDSTREAM,
	PDF_TOK_XREF, PDF_TOK_TRAILER, PDF_TOK_STARTXREF,
	PDF_NUM_TOKENS
};

enum
{
	PDF_LEXBUF_SMALL = 256,
	PDF_LEXBUF_LARGE = 65536
};

typedef struct pdf_lexbuf_s pdf_lexbuf;
typedef struct pdf_lexbuf_large_s pdf_lexbuf_large;

struct pdf_lexbuf_s
{
	int size;
	int len;
	int i;
	float f;
	char scratch[PDF_LEXBUF_SMALL];
};

struct pdf_lexbuf_large_s
{
	pdf_lexbuf base;
	char scratch[PDF_LEXBUF_LARGE - PDF_LEXBUF_SMALL];
};

int pdf_lex(base_stream *f, pdf_lexbuf *lexbuf);

pdf_obj *pdf_parse_array(pdf_document *doc, base_stream *f, pdf_lexbuf *buf);
pdf_obj *pdf_parse_dict(pdf_document *doc, base_stream *f, pdf_lexbuf *buf);
pdf_obj *pdf_parse_stm_obj(pdf_document *doc, base_stream *f, pdf_lexbuf *buf);
pdf_obj *pdf_parse_ind_obj(pdf_document *doc, base_stream *f, pdf_lexbuf *buf, int *num, int *gen, int *stm_ofs);

typedef struct pdf_xref_entry_s pdf_xref_entry;

struct pdf_xref_entry_s
{
	int ofs;	
	int gen;	
	int stm_ofs;	
	pdf_obj *obj;	
	int type;	
};

typedef struct pdf_crypt_s pdf_crypt;
typedef struct pdf_ocg_descriptor_s pdf_ocg_descriptor;
typedef struct pdf_ocg_entry_s pdf_ocg_entry;

struct pdf_ocg_entry_s
{
	int num;
	int gen;
	int state;
};

struct pdf_ocg_descriptor_s
{
	int len;
	pdf_ocg_entry *ocgs;
	pdf_obj *intent;
};

struct pdf_document_s
{
	base_document super;

	base_context *ctx;
	base_stream *file;

	int version;
	int startxref;
	int file_size;
	pdf_crypt *crypt;
	pdf_obj *trailer;
	pdf_ocg_descriptor *ocg;

	int len;
	pdf_xref_entry *table;

	int page_len;
	int page_cap;
	pdf_obj **page_objs;
	pdf_obj **page_refs;

	pdf_lexbuf_large lexbuf;
};

void pdf_cache_object(pdf_document *doc, int num, int gen);

base_stream *pdf_open_inline_stream(pdf_document *doc, pdf_obj *stmobj, int length, base_stream *chain, pdf_image_params *params);
base_buffer *pdf_load_image_stream(pdf_document *doc, int num, int gen, pdf_image_params *params);
base_stream *pdf_open_image_stream(pdf_document *doc, int num, int gen, pdf_image_params *params);
base_stream *pdf_open_stream_with_offset(pdf_document *doc, int num, int gen, pdf_obj *dict, int stm_ofs);
base_stream *pdf_open_image_decomp_stream(base_context *ctx, base_buffer *, pdf_image_params *params, int *factor);

void pdf_repair_xref(pdf_document *doc, pdf_lexbuf *buf);
void pdf_repair_obj_stms(pdf_document *doc);
void pdf_print_xref(pdf_document *);
void pdf_resize_xref(pdf_document *doc, int newcap);

pdf_crypt *pdf_new_crypt(base_context *ctx, pdf_obj *enc, pdf_obj *id);
void pdf_free_crypt(base_context *ctx, pdf_crypt *crypt);

void pdf_crypt_obj(base_context *ctx, pdf_crypt *crypt, pdf_obj *obj, int num, int gen);
base_stream *pdf_open_crypt(base_stream *chain, pdf_crypt *crypt, int num, int gen);
base_stream *pdf_open_crypt_with_filter(base_stream *chain, pdf_crypt *crypt, char *name, int num, int gen);

int pdf_crypt_revision(pdf_document *doc);
char *pdf_crypt_method(pdf_document *doc);
int pdf_crypt_length(pdf_document *doc);
unsigned char *pdf_crypt_key(pdf_document *doc);

void pdf_print_crypt(pdf_crypt *crypt);

typedef struct pdf_function_s pdf_function;

pdf_function *pdf_load_function(pdf_document *doc, pdf_obj *ref);
void pdf_eval_function(base_context *ctx, pdf_function *func, float *in, int inlen, float *out, int outlen);
pdf_function *pdf_keep_function(base_context *ctx, pdf_function *func);
void pdf_drop_function(base_context *ctx, pdf_function *func);
unsigned int pdf_function_size(pdf_function *func);

base_colorspace *pdf_load_colorspace(pdf_document *doc, pdf_obj *obj);
base_pixmap *pdf_expand_indexed_pixmap(base_context *ctx, base_pixmap *src);

base_shade *pdf_load_shading(pdf_document *doc, pdf_obj *obj);

base_image *pdf_load_inline_image(pdf_document *doc, pdf_obj *rdb, pdf_obj *dict, base_stream *file);
int pdf_is_jpx_image(base_context *ctx, pdf_obj *dict);

typedef struct pdf_pattern_s pdf_pattern;

struct pdf_pattern_s
{
	base_storable storable;
	int ismask;
	float xstep;
	float ystep;
	base_matrix matrix;
	base_rect bbox;
	pdf_obj *resources;
	base_buffer *contents;
};

pdf_pattern *pdf_load_pattern(pdf_document *doc, pdf_obj *obj);
pdf_pattern *pdf_keep_pattern(base_context *ctx, pdf_pattern *pat);
void pdf_drop_pattern(base_context *ctx, pdf_pattern *pat);

typedef struct pdf_xobject_s pdf_xobject;

struct pdf_xobject_s
{
	base_storable storable;
	base_matrix matrix;
	base_rect bbox;
	int isolated;
	int knockout;
	int transparency;
	base_colorspace *colorspace;
	pdf_obj *resources;
	base_buffer *contents;
	pdf_obj *me;
};

pdf_xobject *pdf_load_xobject(pdf_document *doc, pdf_obj *obj);
pdf_xobject *pdf_keep_xobject(base_context *ctx, pdf_xobject *xobj);
void pdf_drop_xobject(base_context *ctx, pdf_xobject *xobj);

typedef struct pdf_cmap_s pdf_cmap;
typedef struct pdf_range_s pdf_range;

enum { PDF_CMAP_SINGLE, PDF_CMAP_RANGE, PDF_CMAP_TABLE, PDF_CMAP_MULTI };

struct pdf_range_s
{
	unsigned short low;
	
	unsigned short extent_flags;
	unsigned short offset;	
};

struct pdf_cmap_s
{
	base_storable storable;
	char cmap_name[32];

	char usecmap_name[32];
	pdf_cmap *usecmap;

	int wmode;

	int codespace_len;
	struct
	{
		unsigned short n;
		unsigned short low;
		unsigned short high;
	} codespace[40];

	int rlen, rcap;
	pdf_range *ranges;

	int tlen, tcap;
	unsigned short *table;
};

typedef struct
{
	char *name;
	pdf_cmap *cmap;
} pdf_cmap_table;

extern pdf_cmap_table g_customCMapTable[1024];
extern int g_customCMapTableSize;

pdf_cmap *pdf_new_cmap(base_context *ctx);
pdf_cmap *pdf_keep_cmap(base_context *ctx, pdf_cmap *cmap);
void pdf_drop_cmap(base_context *ctx, pdf_cmap *cmap);
void pdf_free_cmap_imp(base_context *ctx, base_storable *cmap);
unsigned int pdf_cmap_size(base_context *ctx, pdf_cmap *cmap);

void pdf_print_cmap(base_context *ctx, pdf_cmap *cmap);
int pdf_cmap_wmode(base_context *ctx, pdf_cmap *cmap);
void pdf_set_cmap_wmode(base_context *ctx, pdf_cmap *cmap, int wmode);
void pdf_set_usecmap(base_context *ctx, pdf_cmap *cmap, pdf_cmap *usecmap);

void pdf_add_codespace(base_context *ctx, pdf_cmap *cmap, int low, int high, int n);
void pdf_map_range_to_table(base_context *ctx, pdf_cmap *cmap, int low, int *map, int len);
void pdf_map_range_to_range(base_context *ctx, pdf_cmap *cmap, int srclo, int srchi, int dstlo);
void pdf_map_one_to_many(base_context *ctx, pdf_cmap *cmap, int one, int *many, int len);
void pdf_sort_cmap(base_context *ctx, pdf_cmap *cmap);

int pdf_lookup_cmap(pdf_cmap *cmap, int cpt);
int pdf_lookup_cmap_full(pdf_cmap *cmap, int cpt, int *out);
int pdf_decode_cmap(pdf_cmap *cmap, unsigned char *s, int *cpt);

pdf_cmap *pdf_new_identity_cmap(base_context *ctx, int wmode, int bytes);
pdf_cmap *pdf_load_cmap(base_context *ctx, base_stream *file);
pdf_cmap *pdf_load_system_cmap(base_context *ctx, char *name);
pdf_cmap *pdf_load_builtin_cmap(base_context *ctx, char *name);
pdf_cmap *pdf_load_embedded_cmap(pdf_document *doc, pdf_obj *ref);

enum
{
	PDF_FD_FIXED_PITCH = 1 << 0,
	PDF_FD_SERIF = 1 << 1,
	PDF_FD_SYMBOLIC = 1 << 2,
	PDF_FD_SCRIPT = 1 << 3,
	PDF_FD_NONSYMBOLIC = 1 << 5,
	PDF_FD_ITALIC = 1 << 6,
	PDF_FD_ALL_CAP = 1 << 16,
	PDF_FD_SMALL_CAP = 1 << 17,
	PDF_FD_FORCE_BOLD = 1 << 18
};

enum { PDF_ROS_CNS, PDF_ROS_GB, PDF_ROS_JAPAN, PDF_ROS_KOREA };

void pdf_load_encoding(char **estrings, char *encoding);
int pdf_lookup_agl(char *name);
const char **pdf_lookup_agl_duplicates(int ucs);

extern const unsigned short pdf_doc_encoding[256];
extern const char * const pdf_mac_roman[256];
extern const char * const pdf_mac_expert[256];
extern const char * const pdf_win_ansi[256];
extern const char * const pdf_standard[256];

typedef struct pdf_font_desc_s pdf_font_desc;
typedef struct pdf_hmtx_s pdf_hmtx;
typedef struct pdf_vmtx_s pdf_vmtx;

struct pdf_hmtx_s
{
	unsigned short lo;
	unsigned short hi;
	int w;	
};

struct pdf_vmtx_s
{
	unsigned short lo;
	unsigned short hi;
	short x;
	short y;
	short w;
};

struct pdf_font_desc_s
{
	base_storable storable;
	unsigned int size;

	base_font *font;

	
	int flags;
	float italic_angle;
	float ascent;
	float descent;
	float cap_height;
	float x_height;
	float missing_width;

	
	pdf_cmap *encoding;
	pdf_cmap *to_ttf_cmap;
	int cid_to_gid_len;
	unsigned short *cid_to_gid;

	
	pdf_cmap *to_unicode;
	int cid_to_ucs_len;
	unsigned short *cid_to_ucs;

	
	int wmode;

	int hmtx_len, hmtx_cap;
	pdf_hmtx dhmtx;
	pdf_hmtx *hmtx;

	int vmtx_len, vmtx_cap;
	pdf_vmtx dvmtx;
	pdf_vmtx *vmtx;

	int is_embedded;
};

void pdf_set_font_wmode(base_context *ctx, pdf_font_desc *font, int wmode);
void pdf_set_default_hmtx(base_context *ctx, pdf_font_desc *font, int w);
void pdf_set_default_vmtx(base_context *ctx, pdf_font_desc *font, int y, int w);
void pdf_add_hmtx(base_context *ctx, pdf_font_desc *font, int lo, int hi, int w);
void pdf_add_vmtx(base_context *ctx, pdf_font_desc *font, int lo, int hi, int x, int y, int w);
void pdf_end_hmtx(base_context *ctx, pdf_font_desc *font);
void pdf_end_vmtx(base_context *ctx, pdf_font_desc *font);
pdf_hmtx pdf_lookup_hmtx(base_context *ctx, pdf_font_desc *font, int cid);
pdf_vmtx pdf_lookup_vmtx(base_context *ctx, pdf_font_desc *font, int cid);

void pdf_load_to_unicode(pdf_document *doc, pdf_font_desc *font, char **strings, char *collection, pdf_obj *cmapstm);

int pdf_font_cid_to_gid(base_context *ctx, pdf_font_desc *fontdesc, int cid);

unsigned char *pdf_lookup_builtin_font(char *name, unsigned int *len);
unsigned char *pdf_lookup_substitute_font(int mono, int serif, int bold, int italic, unsigned int *len);
unsigned char *pdf_lookup_substitute_cjk_font(int ros, int serif, unsigned int *len);

pdf_font_desc *pdf_load_type3_font(pdf_document *doc, pdf_obj *rdb, pdf_obj *obj);
pdf_font_desc *pdf_load_font(pdf_document *doc, pdf_obj *rdb, pdf_obj *obj);

pdf_font_desc *pdf_new_font_desc(base_context *ctx);
pdf_font_desc *pdf_keep_font(base_context *ctx, pdf_font_desc *fontdesc);
void pdf_drop_font(base_context *ctx, pdf_font_desc *font);

void pdf_print_font(base_context *ctx, pdf_font_desc *fontdesc);

typedef struct pdf_annot_s pdf_annot;

struct pdf_annot_s
{
	pdf_obj *obj;
	base_rect rect;
	pdf_xobject *ap;
	base_matrix matrix;
	pdf_annot *next;
};

base_link_dest pdf_parse_link_dest(pdf_document *doc, pdf_obj *dest);
base_link_dest pdf_parse_action(pdf_document *doc, pdf_obj *action);
pdf_obj *pdf_lookup_dest(pdf_document *doc, pdf_obj *needle);
pdf_obj *pdf_lookup_name(pdf_document *doc, char *which, pdf_obj *needle);
pdf_obj *pdf_load_name_tree(pdf_document *doc, char *which);

base_link *pdf_load_link_annots(pdf_document *, pdf_obj *annots, base_matrix page_ctm);

pdf_annot *pdf_load_annots(pdf_document *, pdf_obj *annots);
void pdf_free_annot(base_context *ctx, pdf_annot *link);

struct pdf_page_s
{
	base_matrix ctm; 
	base_rect mediabox;
	int rotate;
	int transparency;
	pdf_obj *resources;
	base_buffer *contents;
	base_link *links;
	pdf_annot *annots;
};

void pdf_run_glyph(pdf_document *doc, pdf_obj *resources, base_buffer *contents, base_device *dev, base_matrix ctm, void *gstate);

void pdf_store_item(base_context *ctx, pdf_obj *key, void *val, unsigned int itemsize);
void *pdf_find_item(base_context *ctx, base_store_free_fn *free, pdf_obj *key);
void pdf_remove_item(base_context *ctx, base_store_free_fn *free, pdf_obj *key);

#endif
