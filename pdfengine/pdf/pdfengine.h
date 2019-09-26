#ifndef PDFENGINE_H
#define PDFENGINE_H

#include "pdf.h"

typedef struct pdf_document_s pdf_document;

typedef struct pdf_obj_s pdf_obj;

pdf_obj *pdf_new_null(base_context *ctx);
pdf_obj *pdf_new_bool(base_context *ctx, int b);
pdf_obj *pdf_new_int(base_context *ctx, int i);
pdf_obj *pdf_new_real(base_context *ctx, float f);
pdf_obj *base_new_name(base_context *ctx, char *str);
pdf_obj *pdf_new_string(base_context *ctx, char *str, int len);
pdf_obj *pdf_new_indirect(base_context *ctx, int num, int gen, void *doc);

pdf_obj *pdf_new_array(base_context *ctx, int initialcap);
pdf_obj *pdf_new_dict(base_context *ctx, int initialcap);
pdf_obj *pdf_copy_array(base_context *ctx, pdf_obj *array);
pdf_obj *pdf_copy_dict(base_context *ctx, pdf_obj *dict);

pdf_obj *pdf_keep_obj(pdf_obj *obj);
void pdf_drop_obj(pdf_obj *obj);

int pdf_is_null(pdf_obj *obj);
int pdf_is_bool(pdf_obj *obj);
int pdf_is_int(pdf_obj *obj);
int pdf_is_real(pdf_obj *obj);
int pdf_is_name(pdf_obj *obj);
int pdf_is_string(pdf_obj *obj);
int pdf_is_array(pdf_obj *obj);
int pdf_is_dict(pdf_obj *obj);
int pdf_is_indirect(pdf_obj *obj);
int pdf_is_stream(pdf_document *doc, int num, int gen);

int pdf_objcmp(pdf_obj *a, pdf_obj *b);

int pdf_dict_marked(pdf_obj *obj);
int pdf_dict_mark(pdf_obj *obj);
void pdf_dict_unmark(pdf_obj *obj);

int pdf_to_bool(pdf_obj *obj);
int pdf_to_int(pdf_obj *obj);
float pdf_to_real(pdf_obj *obj);
char *pdf_to_name(pdf_obj *obj);
char *pdf_to_str_buf(pdf_obj *obj);
pdf_obj *pdf_to_dict(pdf_obj *obj);
int pdf_to_str_len(pdf_obj *obj);
int pdf_to_num(pdf_obj *obj);
int pdf_to_gen(pdf_obj *obj);

int pdf_array_len(pdf_obj *array);
pdf_obj *pdf_array_get(pdf_obj *array, int i);
void pdf_array_put(pdf_obj *array, int i, pdf_obj *obj);
void pdf_array_push(pdf_obj *array, pdf_obj *obj);
void pdf_array_insert(pdf_obj *array, pdf_obj *obj);
int pdf_array_contains(pdf_obj *array, pdf_obj *obj);

int pdf_dict_len(pdf_obj *dict);
pdf_obj *pdf_dict_get_key(pdf_obj *dict, int idx);
pdf_obj *pdf_dict_get_val(pdf_obj *dict, int idx);
pdf_obj *pdf_dict_get(pdf_obj *dict, pdf_obj *key);
pdf_obj *pdf_dict_gets(pdf_obj *dict, char *key);
pdf_obj *pdf_dict_getsa(pdf_obj *dict, char *key, char *abbrev);
void base_dict_put(pdf_obj *dict, pdf_obj *key, pdf_obj *val);
void pdf_dict_puts(pdf_obj *dict, char *key, pdf_obj *val);
void pdf_dict_del(pdf_obj *dict, pdf_obj *key);
void pdf_dict_dels(pdf_obj *dict, char *key);
void pdf_sort_dict(pdf_obj *dict);

int pdf_fprint_obj(FILE *fp, pdf_obj *obj, int tight);
void pdf_print_obj(pdf_obj *obj);
void pdf_print_ref(pdf_obj *obj);

char *pdf_to_utf8(base_context *ctx, pdf_obj *src);
unsigned short *pdf_to_ucs2(base_context *ctx, pdf_obj *src); 
pdf_obj *pdf_to_utf8_name(base_context *ctx, pdf_obj *src);
char *pdf_from_ucs2(base_context *ctx, unsigned short *str);

base_rect pdf_to_rect(base_context *ctx, pdf_obj *array);
base_matrix pdf_to_matrix(base_context *ctx, pdf_obj *array);

int pdf_count_objects(pdf_document *doc);
pdf_obj *pdf_resolve_indirect(pdf_obj *ref);
pdf_obj *pdf_load_object(pdf_document *doc, int num, int gen);
void pdf_update_object(pdf_document *doc, int num, int gen, pdf_obj *newobj);

base_buffer *pdf_load_raw_stream(pdf_document *doc, int num, int gen);
base_buffer *pdf_load_stream(pdf_document *doc, int num, int gen);
base_stream *pdf_open_raw_stream(pdf_document *doc, int num, int gen);
base_stream *pdf_open_stream(pdf_document *doc, int num, int gen);

base_image *pdf_load_image(pdf_document *doc, pdf_obj *obj);

base_outline *pdf_load_outline(pdf_document *doc);

pdf_document *pdf_open_document(base_context *ctx, const wchar_t *filename);

pdf_document *pdf_open_document_with_stream(base_stream *file);

void pdf_close_document(pdf_document *doc);

int pdf_needs_password(pdf_document *doc);
int pdf_authenticate_password(pdf_document *doc, char *pw);

enum
{
	PDF_PERM_PRINT = 1 << 2,
	PDF_PERM_CHANGE = 1 << 3,
	PDF_PERM_COPY = 1 << 4,
	PDF_PERM_NOTES = 1 << 5,
	PDF_PERM_FILL_FORM = 1 << 8,
	PDF_PERM_ACCESSIBILITY = 1 << 9,
	PDF_PERM_ASSEMBLE = 1 << 10,
	PDF_PERM_HIGH_RES_PRINT = 1 << 11,
	PDF_DEFAULT_PERM_FLAGS = 0xfffc
};

int pdf_has_permission(pdf_document *doc, int p);

typedef struct pdf_page_s pdf_page;

int pdf_lookup_page_number(pdf_document *doc, pdf_obj *pageobj);
int pdf_count_pages(pdf_document *doc);

pdf_page *pdf_load_page(pdf_document *doc, int number);

base_link *pdf_load_links(pdf_document *doc, pdf_page *page);

base_rect pdf_bound_page(pdf_document *doc, pdf_page *page);

void pdf_free_page(pdf_document *doc, pdf_page *page);

void pdf_run_page(pdf_document *doc, pdf_page *page, base_device *dev, base_matrix ctm, base_cookie *cookie);

void pdf_run_page_with_usage(pdf_document *doc, pdf_page *page, base_device *dev, base_matrix ctm, char *event, base_cookie *cookie);

#endif
