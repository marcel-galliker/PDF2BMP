

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _JBIG2_H
#define _JBIG2_H

typedef enum {
  JBIG2_SEVERITY_DEBUG,
  JBIG2_SEVERITY_INFO,
  JBIG2_SEVERITY_WARNING,
  JBIG2_SEVERITY_FATAL
} Jbig2Severity;

typedef enum {
  JBIG2_OPTIONS_EMBEDDED = 1
} Jbig2Options;

typedef struct _Jbig2Allocator Jbig2Allocator;
typedef struct _Jbig2Ctx Jbig2Ctx;
typedef struct _Jbig2GlobalCtx Jbig2GlobalCtx;
typedef struct _Jbig2Segment Jbig2Segment;
typedef struct _Jbig2Image Jbig2Image;

typedef struct _Jbig2Page Jbig2Page;
typedef struct _Jbig2SymbolDictionary Jbig2SymbolDictionary;

struct _Jbig2Image {
        int             width, height, stride;
        uint8_t        *data;
	int		refcount;
};

Jbig2Image*     jbig2_image_new(Jbig2Ctx *ctx, int width, int height);
Jbig2Image*	jbig2_image_clone(Jbig2Ctx *ctx, Jbig2Image *image);
void		jbig2_image_release(Jbig2Ctx *ctx, Jbig2Image *image);
void            jbig2_image_free(Jbig2Ctx *ctx, Jbig2Image *image);
void		jbig2_image_clear(Jbig2Ctx *ctx, Jbig2Image *image, int value);
Jbig2Image	*jbig2_image_resize(Jbig2Ctx *ctx, Jbig2Image *image,
                                int width, int height);

typedef int (*Jbig2ErrorCallback) (void *data,
				  const char *msg, Jbig2Severity severity,
				  int32_t seg_idx);

struct _Jbig2Allocator {
  void *(*alloc) (Jbig2Allocator *allocator, size_t size);
  void (*free) (Jbig2Allocator *allocator, void *p);
  void *(*realloc) (Jbig2Allocator *allocator, void *p, size_t size);
};

Jbig2Ctx *jbig2_ctx_new (Jbig2Allocator *allocator,
			 Jbig2Options options,
			 Jbig2GlobalCtx *global_ctx,
			 Jbig2ErrorCallback error_callback,
			 void *error_callback_data);
void jbig2_ctx_free (Jbig2Ctx *ctx);

Jbig2GlobalCtx *jbig2_make_global_ctx (Jbig2Ctx *ctx);
void jbig2_global_ctx_free (Jbig2GlobalCtx *global_ctx);

int jbig2_data_in (Jbig2Ctx *ctx, const unsigned char *data, size_t size);

Jbig2Image *jbig2_page_out (Jbig2Ctx *ctx);

int jbig2_release_page (Jbig2Ctx *ctx, Jbig2Image *image);

int jbig2_complete_page (Jbig2Ctx *ctx);

struct _Jbig2Segment {
  uint32_t number;
  uint8_t flags;
  uint32_t page_association;
  size_t data_length;
  int referred_to_segment_count;
  uint32_t *referred_to_segments;
  void *result;
};

Jbig2Segment *jbig2_parse_segment_header (Jbig2Ctx *ctx, uint8_t *buf, size_t buf_size,
			    size_t *p_header_size);
int jbig2_parse_segment (Jbig2Ctx *ctx, Jbig2Segment *segment,
			 const uint8_t *segment_data);
void jbig2_free_segment (Jbig2Ctx *ctx, Jbig2Segment *segment);

Jbig2Segment *jbig2_find_segment(Jbig2Ctx *ctx, uint32_t number);

#endif 

#ifdef __cplusplus
}
#endif
