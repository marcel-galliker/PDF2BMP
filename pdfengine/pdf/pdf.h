#ifndef PDF_H
#define PDF_H

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>

#include <assert.h>
#include <errno.h>
#include <limits.h>	
#include <float.h> 
#include <fcntl.h> 

#include <setjmp.h>

#ifdef __APPLE__
#define base_setjmp _setjmp
#define base_longjmp _longjmp
#else
#define base_setjmp setjmp
#define base_longjmp longjmp
#endif

#ifdef __ANDROID__
#include <android/log.h>
#define LOG_TAG "pdfengine"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#else
#define LOGI(...) do {} while(0)
#define LOGE(...) do {} while(0)
#endif

#define nelem(x) (sizeof(x)/sizeof((x)[0]))

#define ABS(x) ( (x) < 0 ? -(x) : (x) )
#define MIN(a,b) ( (a) < (b) ? (a) : (b) )
#define MAX(a,b) ( (a) > (b) ? (a) : (b) )
#define CLAMP(x,a,b) ( (x) > (b) ? (b) : ( (x) < (a) ? (a) : (x) ) )
#define DIV_BY_ZERO(a, b, min, max) (((a) < 0) ^ ((b) < 0) ? (min) : (max))

#if defined(_MSC_VER) && !defined(_QT_VER) 

#pragma warning( disable: 4244 ) 
#pragma warning( disable: 4996 ) 
#pragma warning( disable: 4996 ) 

#include <io.h>

int gettimeofday(struct timeval *tv, struct timezone *tz);

// #define snprintf _snprintf
// #define isnan _isnan
#define hypotf _hypotf

#else 

#include <unistd.h>

#ifndef O_BINARY
#define O_BINARY 0
#endif

#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef M_SQRT2
#define M_SQRT2 1.41421356237309504880
#endif

#if __STDC_VERSION__ == 199901L 
#elif _MSC_VER >= 1500 
#define inline __inline
#define restrict __restrict
#elif __GNUC__ >= 3 
#define inline __inline
#define restrict __restrict
#else 
#define inline
#define restrict
#endif

#ifndef __printflike
#if __GNUC__ > 2 || __GNUC__ == 2 && __GNUC_MINOR__ >= 7
#define __printflike(fmtarg, firstvararg) \
	__attribute__((__format__ (__printf__, fmtarg, firstvararg)))
#else
#define __printflike(fmtarg, firstvararg)
#endif
#endif

typedef struct base_alloc_context_s base_alloc_context;
typedef struct base_error_context_s base_error_context;
typedef struct base_warn_context_s base_warn_context;
typedef struct base_font_context_s base_font_context;
typedef struct base_aa_context_s base_aa_context;
typedef struct base_locks_context_s base_locks_context;
typedef struct base_store_s base_store;
typedef struct base_glyph_cache_s base_glyph_cache;
typedef struct base_context_s base_context;

struct base_alloc_context_s
{
	void *user;
	void *(*malloc)(void *, unsigned int);
	void *(*realloc)(void *, void *, unsigned int);
	void (*free)(void *, void *);
};

struct base_error_context_s
{
	int top;
	struct {
		int code;
		jmp_buf buffer;
	} stack[256];
	char message[256];
};

void base_var_imp(void *);
#define base_var(var) base_var_imp((void *)&(var))

#define base_try(ctx) \
	if (base_push_try(ctx->error), \
		(ctx->error->stack[ctx->error->top].code = base_setjmp(ctx->error->stack[ctx->error->top].buffer)) == 0) \
	{ do {

#define base_always(ctx) \
		} while (0); \
	} \
	{ do { \

#define base_catch(ctx) \
		} while(0); \
	} \
	if (ctx->error->stack[ctx->error->top--].code)

void base_push_try(base_error_context *ex);
void base_throw(base_context *, char *, ...) __printflike(2, 3);
void base_rethrow(base_context *);
void base_warn(base_context *ctx, char *fmt, ...) __printflike(2, 3);

void base_flush_warnings(base_context *ctx);

struct base_context_s
{
	base_alloc_context *alloc;
	base_locks_context *locks;
	base_error_context *error;
	base_warn_context *warn;
	base_font_context *font;
	base_aa_context *aa;
	base_store *store;
	base_glyph_cache *glyph_cache;
};

enum {
	base_STORE_UNLIMITED = 0,
	base_STORE_DEFAULT = 256 << 20,
};

base_context *base_new_context(base_alloc_context *alloc, base_locks_context *locks, unsigned int max_store);

base_context *base_clone_context(base_context *ctx);

void base_free_context(base_context *ctx);

int base_aa_level(base_context *ctx);

void base_set_aa_level(base_context *ctx, int bits);

struct base_locks_context_s
{
	void *user;
	void (*lock)(void *user, int lock);
	void (*unlock)(void *user, int lock);
};

enum {
	base_LOCK_ALLOC = 0,
	base_LOCK_FILE,
	base_LOCK_FREETYPE,
	base_LOCK_GLYPHCACHE,
	base_LOCK_MAX
};

void *base_malloc(base_context *ctx, unsigned int size);

void *base_calloc(base_context *ctx, unsigned int count, unsigned int size);

#define base_malloc_struct(CTX, STRUCT) \
	base_calloc(CTX,1,sizeof(STRUCT))

void *base_malloc_array(base_context *ctx, unsigned int count, unsigned int size);

void *base_resize_array(base_context *ctx, void *p, unsigned int count, unsigned int size);

char *base_strdup(base_context *ctx, char *s);

void base_free(base_context *ctx, void *p);

void *base_malloc_no_throw(base_context *ctx, unsigned int size);

void *base_calloc_no_throw(base_context *ctx, unsigned int count, unsigned int size);

void *base_malloc_array_no_throw(base_context *ctx, unsigned int count, unsigned int size);

void *base_resize_array_no_throw(base_context *ctx, void *p, unsigned int count, unsigned int size);

char *base_strdup_no_throw(base_context *ctx, char *s);

char *base_strsep(char **stringp, const char *delim);

int base_strlcpy(char *dst, const char *src, int n);

int base_strlcat(char *dst, const char *src, int n);

int base_chartorune(int *rune, char *str);

int base_runetochar(char *str, int rune);

int base_runelen(int rune);

extern int base_getopt(int nargc, char * const *nargv, const char *ostr);
extern int base_optind;
extern char *base_optarg;

typedef struct base_point_s base_point;
struct base_point_s
{
	float x, y;
};

typedef struct base_rect_s base_rect;
struct base_rect_s
{
	float x0, y0;
	float x1, y1;
};

typedef struct base_bbox_s base_bbox;
struct base_bbox_s
{
	int x0, y0;
	int x1, y1;
};

extern const base_rect base_unit_rect;

extern const base_bbox base_unit_bbox;

extern const base_rect base_empty_rect;

extern const base_bbox base_empty_bbox;

extern const base_rect base_infinite_rect;

extern const base_bbox base_infinite_bbox;

#define base_is_empty_rect(r) ((r).x0 == (r).x1 || (r).y0 == (r).y1)

#define base_is_empty_bbox(b) ((b).x0 == (b).x1 || (b).y0 == (b).y1)

#define base_is_infinite_rect(r) ((r).x0 > (r).x1 || (r).y0 > (r).y1)

#define base_is_infinite_bbox(b) ((b).x0 > (b).x1 || (b).y0 > (b).y1)

typedef struct base_matrix_s base_matrix;
struct base_matrix_s
{
	float a, b, c, d, e, f;
};

extern const base_matrix base_identity;

base_matrix base_concat(base_matrix left, base_matrix right);

base_matrix base_scale(float sx, float sy);

base_matrix base_shear(float sx, float sy);

base_matrix base_rotate(float degrees);

base_matrix base_translate(float tx, float ty);

base_matrix base_invert_matrix(base_matrix matrix);

int base_is_rectilinear(base_matrix m);

float base_matrix_expansion(base_matrix m); 

base_bbox base_bbox_covering_rect(base_rect rect);

base_bbox base_round_rect(base_rect rect);

base_rect base_intersect_rect(base_rect a, base_rect b);

base_bbox base_intersect_bbox(base_bbox a, base_bbox b);

base_rect base_union_rect(base_rect a, base_rect b);

base_bbox base_union_bbox(base_bbox a, base_bbox b);

base_point base_transform_point(base_matrix transform, base_point point);

base_point base_transform_vector(base_matrix transform, base_point vector);

base_rect base_transform_rect(base_matrix transform, base_rect rect);

base_bbox base_transform_bbox(base_matrix matrix, base_bbox bbox);

typedef struct base_buffer_s base_buffer;

base_buffer *base_keep_buffer(base_context *ctx, base_buffer *buf);

void base_drop_buffer(base_context *ctx, base_buffer *buf);

int base_buffer_storage(base_context *ctx, base_buffer *buf, unsigned char **data);

typedef struct base_stream_s base_stream;

base_stream *base_open_file(base_context *ctx, const wchar_t *filename);

base_stream *base_open_file_w(base_context *ctx, const wchar_t *filename);

base_stream *base_open_fd(base_context *ctx, int file);

base_stream *base_open_memory(base_context *ctx, unsigned char *data, int len);

base_stream *base_open_buffer(base_context *ctx, base_buffer *buf);

void base_close(base_stream *stm);

int base_tell(base_stream *stm);

void base_seek(base_stream *stm, int offset, int whence);

int base_read(base_stream *stm, unsigned char *data, int len);

base_buffer *base_read_all(base_stream *stm, int initial);

typedef struct base_bitmap_s base_bitmap;

base_bitmap *base_keep_bitmap(base_context *ctx, base_bitmap *bit);

void base_drop_bitmap(base_context *ctx, base_bitmap *bit);

typedef struct base_colorspace_s base_colorspace;

base_colorspace *base_find_device_colorspace(base_context *ctx, char *name);

extern base_colorspace *base_device_gray;

extern base_colorspace *base_device_rgb;

extern base_colorspace *base_device_bgr;

extern base_colorspace *base_device_cmyk;

typedef struct base_pixmap_s base_pixmap;

base_bbox base_pixmap_bbox(base_context *ctx, base_pixmap *pix);

int base_pixmap_width(base_context *ctx, base_pixmap *pix);

int base_pixmap_height(base_context *ctx, base_pixmap *pix);

base_pixmap *base_new_pixmap(base_context *ctx, base_colorspace *cs, int w, int h);

base_pixmap *base_new_pixmap_with_bbox(base_context *ctx, base_colorspace *colorspace, base_bbox bbox);

base_pixmap *base_new_pixmap_with_data(base_context *ctx, base_colorspace *colorspace, int w, int h, unsigned char *samples);

base_pixmap *base_new_pixmap_with_bbox_and_data(base_context *ctx, base_colorspace *colorspace, base_bbox bbox, unsigned char *samples);

base_pixmap *base_keep_pixmap(base_context *ctx, base_pixmap *pix);

void base_drop_pixmap(base_context *ctx, base_pixmap *pix);

base_colorspace *base_pixmap_colorspace(base_context *ctx, base_pixmap *pix);

int base_pixmap_components(base_context *ctx, base_pixmap *pix);

unsigned char *base_pixmap_samples(base_context *ctx, base_pixmap *pix);

void base_clear_pixmap_with_value(base_context *ctx, base_pixmap *pix, int value);

void base_clear_pixmap(base_context *ctx, base_pixmap *pix);

void base_invert_pixmap(base_context *ctx, base_pixmap *pix);

void base_invert_pixmap_rect(base_pixmap *image, base_bbox rect);

void base_gamma_pixmap(base_context *ctx, base_pixmap *pix, float gamma);

void base_unmultiply_pixmap(base_context *ctx, base_pixmap *pix);

void base_convert_pixmap(base_context *ctx, base_pixmap *dst, base_pixmap *src);

void base_write_pixmap(base_context *ctx, base_pixmap *img, char *name, int rgb);

void base_write_pnm(base_context *ctx, base_pixmap *pixmap, char *filename);

void base_write_pam(base_context *ctx, base_pixmap *pixmap, char *filename, int savealpha);

void base_write_png(base_context *ctx, base_pixmap *pixmap, char *filename, int savealpha);

void base_write_pbm(base_context *ctx, base_bitmap *bitmap, char *filename);

void base_write_jpeg(base_context *ctx, base_pixmap *pixmap, wchar_t *filename, int quality);

int base_write_bmp(base_context *ctx, base_pixmap *pixmap, wchar_t *filename, unsigned char** pBmpTotalBuffer, int* nTotalBufSize, unsigned char** pBmpLineBuffer, int* nLineBufSize, unsigned char* pLinearBuf, unsigned char* pNewPixelBuf, int* pErrorBuf, int convmode, int convsplit);

void base_write_tiff(base_context *ctx, base_pixmap *pixmap, wchar_t *filename);

void base_write_gif(base_context *ctx, base_pixmap *pixmap, wchar_t *filename);

void base_md5_pixmap(base_pixmap *pixmap, unsigned char digest[16]);

typedef struct base_image_s base_image;

base_pixmap *base_image_to_pixmap(base_context *ctx, base_image *image, int w, int h);

void base_drop_image(base_context *ctx, base_image *image);

base_image *base_keep_image(base_context *ctx, base_image *image);

typedef struct base_halftone_s base_halftone;

base_bitmap *base_halftone_pixmap(base_context *ctx, base_pixmap *pix, base_halftone *ht);

typedef struct base_font_s base_font;

typedef struct base_device_s base_device;

void base_free_device(base_device *dev);

void base_flush_device(base_device *dev);

base_device *base_new_trace_device(base_context *ctx);

base_device *base_new_bbox_device(base_context *ctx, base_bbox *bboxp);

base_device *base_new_draw_device(base_context *ctx, base_pixmap *dest);
base_device *base_new_draw_device_with_extract(base_context *ctx, base_pixmap *dest, wchar_t *dstpath, wchar_t *filename, wchar_t *format, int pageno);

base_device *base_new_draw_device_with_bbox(base_context *ctx, base_pixmap *dest, base_bbox clip);

typedef struct base_text_style_s base_text_style;
typedef struct base_text_char_s base_text_char;
typedef struct base_text_span_s base_text_span;
typedef struct base_text_line_s base_text_line;
typedef struct base_text_block_s base_text_block;

typedef struct base_text_sheet_s base_text_sheet;
typedef struct base_text_page_s base_text_page;

 
struct base_text_sheet_s
{
	int maxid;
	base_text_style *style;
};

 
struct base_text_style_s
{
	base_text_style *next;
	int id;
	base_font *font;
	float size;
	int wmode;
	int script;
	
	float color[4];
};

 
struct base_text_page_s
{
	base_rect mediabox;
	int len, cap;
	base_text_block *blocks;
};

 
struct base_text_block_s
{
	base_rect bbox;
	int len, cap;
	base_text_line *lines;
};

 
struct base_text_line_s
{
	base_rect bbox;
	int len, cap;
	base_text_span *spans;
};

 
struct base_text_span_s
{
	base_rect bbox;
	int len, cap;
	base_text_char *text;
	base_text_style *style;
};

 
struct base_text_char_s
{
	base_rect bbox;
	int c;
	float color[4];
};

base_device *base_new_text_device(base_context *ctx, base_text_sheet *sheet, base_text_page *page);

base_text_sheet *base_new_text_sheet(base_context *ctx);
void base_free_text_sheet(base_context *ctx, base_text_sheet *sheet);

base_text_page *base_new_text_page(base_context *ctx, base_rect mediabox);
void base_free_text_page(base_context *ctx, base_text_page *page);

 
void base_print_text_sheet(base_context *ctx, FILE *out, base_text_sheet *sheet);

 
void base_print_text_page_html(base_context *ctx, FILE *out, base_text_page *page);

 
void base_print_text_page_xml(base_context *ctx, FILE *out, base_text_page *page);

 
void base_print_text_page(base_context *ctx, FILE *out, base_text_page *page);

base_device *base_new_html_device(base_context *ctx, base_text_sheet *sheet, base_text_page *page, base_pixmap *dest);

typedef struct base_cookie_s base_cookie;

struct base_cookie_s
{
	int abort;
	int progress;
	int progress_max; 
};

typedef struct base_display_list_s base_display_list;

base_display_list *base_new_display_list(base_context *ctx);

void base_watermark_process(base_display_list *list, base_device *dev, base_bbox org_bbox, base_bbox area, wchar_t* strWatermark, double dAngle, double dAlpha);

base_device *base_new_list_device(base_context *ctx, base_display_list *list);

void base_run_display_list(base_display_list *list, base_device *dev, base_matrix ctm, base_bbox area, base_cookie *cookie, char bOnlyText);

void base_free_display_list(base_context *ctx, base_display_list *list);

base_display_list *base_new_display_rect_list(base_context *ctx);
base_device *base_new_rect_list_device(base_context *ctx, base_display_list *list);
void base_run_display_rect_list(base_display_list *list, base_device *dev, base_matrix ctm, base_bbox area, base_cookie *cookie);
void base_free_display_rect_list(base_context *ctx, base_display_list *list);

typedef struct base_link_s base_link;

typedef struct base_link_dest_s base_link_dest;

typedef enum base_link_kind_e
{
	base_LINK_NONE = 0,
	base_LINK_GOTO,
	base_LINK_URI,
	base_LINK_LAUNCH,
	base_LINK_NAMED,
	base_LINK_GOTOR
} base_link_kind;

enum {
	base_link_flag_l_valid = 1, 
	base_link_flag_t_valid = 2, 
	base_link_flag_r_valid = 4, 
	base_link_flag_b_valid = 8, 
	base_link_flag_fit_h = 16, 
	base_link_flag_fit_v = 32, 
	base_link_flag_r_is_zoom = 64 
};

struct base_link_dest_s
{
	base_link_kind kind;
	union
	{
		struct
		{
			int page;
			int flags;
			base_point lt;
			base_point rb;
			char *file_spec;
			int new_window;
		}
		gotor;
		struct
		{
			char *uri;
			int is_map;
		}
		uri;
		struct
		{
			char *file_spec;
			int new_window;
		}
		launch;
		struct
		{
			char *named;
		}
		named;
	}
	ld;
};

struct base_link_s
{
	int refs;
	base_rect rect;
	base_link_dest dest;
	base_link *next;
};

base_link *base_new_link(base_context *ctx, base_rect bbox, base_link_dest dest);
base_link *base_keep_link(base_context *ctx, base_link *link);

void base_drop_link(base_context *ctx, base_link *link);

void base_free_link_dest(base_context *ctx, base_link_dest *dest);

typedef struct base_outline_s base_outline;

struct base_outline_s
{
	char *title;
	base_link_dest dest;
	base_outline *next;
	base_outline *down;
};

void base_print_outline_xml(base_context *ctx, FILE *out, base_outline *outline);

void base_print_outline(base_context *ctx, FILE *out, base_outline *outline);

void base_free_outline(base_context *ctx, base_outline *outline);

typedef struct base_document_s base_document;
typedef struct base_page_s base_page;

base_document *base_open_document(base_context *ctx, wchar_t *filename);

void base_close_document(base_document *doc);

int base_needs_password(base_document *doc);

int base_authenticate_password(base_document *doc);

base_outline *base_load_outline(base_document *doc);

int base_count_pages(base_document *doc);

base_page *base_load_page(base_document *doc, int number);

base_link *base_load_links(base_document *doc, base_page *page);

base_rect base_bound_page(base_document *doc, base_page *page);

void base_run_page(base_document *doc, base_page *page, base_device *dev, base_matrix transform, base_cookie *cookie);

void base_free_page(base_document *doc, base_page *page);

int base_meta(base_document *doc, int key, void *ptr, int size);

enum
{
	base_META_UNKNOWN_KEY = -1,
	base_META_OK = 0,

	
	base_META_FORMAT_INFO = 1,

	
	base_META_CRYPT_INFO = 2,

	
	base_META_HAS_PERMISSION = 3,

	base_PERMISSION_PRINT = 0,
	base_PERMISSION_CHANGE = 1,
	base_PERMISSION_COPY = 2,
	base_PERMISSION_NOTES = 3,

	
	base_META_INFO = 4,
};

#endif
