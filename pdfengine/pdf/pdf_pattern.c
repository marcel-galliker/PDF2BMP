#include "pdf-internal.h"
#include "pdfengine-internal.h"

pdf_pattern *
pdf_keep_pattern(base_context *ctx, pdf_pattern *pat)
{
	return (pdf_pattern *)base_keep_storable(ctx, &pat->storable);
}

void
pdf_drop_pattern(base_context *ctx, pdf_pattern *pat)
{
	base_drop_storable(ctx, &pat->storable);
}

static void
pdf_free_pattern_imp(base_context *ctx, base_storable *pat_)
{
	pdf_pattern *pat = (pdf_pattern *)pat_;

	if (pat->resources)
		pdf_drop_obj(pat->resources);
	if (pat->contents)
		base_drop_buffer(ctx, pat->contents);
	base_free(ctx, pat);
}

static unsigned int
pdf_pattern_size(pdf_pattern *pat)
{
	if (pat == NULL)
		return 0;
	return sizeof(*pat) + (pat->contents ? pat->contents->cap : 0);
}

pdf_pattern *
pdf_load_pattern(pdf_document *xref, pdf_obj *dict)
{
	pdf_pattern *pat;
	pdf_obj *obj;
	base_context *ctx = xref->ctx;

	if ((pat = pdf_find_item(ctx, pdf_free_pattern_imp, dict)))
	{
		return pat;
	}

	pat = base_malloc_struct(ctx, pdf_pattern);
	base_INIT_STORABLE(pat, 1, pdf_free_pattern_imp);
	pat->resources = NULL;
	pat->contents = NULL;

	
	pdf_store_item(ctx, dict, pat, pdf_pattern_size(pat));

	pat->ismask = pdf_to_int(pdf_dict_gets(dict, "PaintType")) == 2;
	pat->xstep = pdf_to_real(pdf_dict_gets(dict, "XStep"));
	pat->ystep = pdf_to_real(pdf_dict_gets(dict, "YStep"));

	obj = pdf_dict_gets(dict, "BBox");
	pat->bbox = pdf_to_rect(ctx, obj);

	obj = pdf_dict_gets(dict, "Matrix");
	if (obj)
		pat->matrix = pdf_to_matrix(ctx, obj);
	else
		pat->matrix = base_identity;

	pat->resources = pdf_dict_gets(dict, "Resources");
	if (pat->resources)
		pdf_keep_obj(pat->resources);

	base_try(ctx)
	{
		pat->contents = pdf_load_stream(xref, pdf_to_num(dict), pdf_to_gen(dict));
	}
	base_catch(ctx)
	{
		pdf_remove_item(ctx, pdf_free_pattern_imp, dict);
		pdf_drop_pattern(ctx, pat);
		base_throw(ctx, "cannot load pattern stream (%d %d R)", pdf_to_num(dict), pdf_to_gen(dict));
	}
	return pat;
}
