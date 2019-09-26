#include "pdf-internal.h"
#include "pdfengine-internal.h"

static base_outline *
pdf_load_outline_imp(pdf_document *xref, pdf_obj *dict)
{
	base_context *ctx = xref->ctx;
	base_outline *node, **prev, *first;
	pdf_obj *obj;
	pdf_obj *odict = dict;

	base_var(dict);

	base_try(ctx)
	{
		first = NULL;
		prev = &first;
		while (dict && pdf_is_dict(dict))
		{
			if (pdf_dict_mark(dict))
				break;
			node = base_malloc_struct(ctx, base_outline);
			node->title = NULL;
			node->dest.kind = base_LINK_NONE;
			node->down = NULL;
			node->next = NULL;
			*prev = node;
			prev = &node->next;

			obj = pdf_dict_gets(dict, "Title");
			if (obj)
				node->title = pdf_to_utf8(ctx, obj);

			if ((obj = pdf_dict_gets(dict, "Dest")))
				node->dest = pdf_parse_link_dest(xref, obj);
			else if ((obj = pdf_dict_gets(dict, "A")))
				node->dest = pdf_parse_action(xref, obj);

			obj = pdf_dict_gets(dict, "First");
			if (obj)
				node->down = pdf_load_outline_imp(xref, obj);

			dict = pdf_dict_gets(dict, "Next");
		}
	}
	base_catch(ctx)
	{
		for (dict = odict; dict && pdf_dict_marked(dict); dict = pdf_dict_gets(dict, "Next"))
			pdf_dict_unmark(dict);
		base_rethrow(ctx);
	}

	for (dict = odict; dict && pdf_dict_marked(dict); dict = pdf_dict_gets(dict, "Next"))
		pdf_dict_unmark(dict);

	return first;
}

base_outline *
pdf_load_outline(pdf_document *xref)
{
	pdf_obj *root, *obj, *first;

	root = pdf_dict_gets(xref->trailer, "Root");
	obj = pdf_dict_gets(root, "Outlines");
	first = pdf_dict_gets(obj, "First");
	if (first)
		return pdf_load_outline_imp(xref, first);

	return NULL;
}
