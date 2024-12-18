#include "pdf-internal.h"
#include "pdfengine-internal.h"

static pdf_obj *
pdf_lookup_name_imp(base_context *ctx, pdf_obj *node, pdf_obj *needle)
{
	pdf_obj *kids = pdf_dict_gets(node, "Kids");
	pdf_obj *names = pdf_dict_gets(node, "Names");

	if (pdf_is_array(kids))
	{
		int l = 0;
		int r = pdf_array_len(kids) - 1;

		while (l <= r)
		{
			int m = (l + r) >> 1;
			pdf_obj *kid = pdf_array_get(kids, m);
			pdf_obj *limits = pdf_dict_gets(kid, "Limits");
			pdf_obj *first = pdf_array_get(limits, 0);
			pdf_obj *last = pdf_array_get(limits, 1);

			if (pdf_objcmp(needle, first) < 0)
				r = m - 1;
			else if (pdf_objcmp(needle, last) > 0)
				l = m + 1;
			else
			{
				pdf_obj *obj;

				if (pdf_dict_mark(node))
					break;
				obj = pdf_lookup_name_imp(ctx, kid, needle);
				pdf_dict_unmark(node);
				return obj;
			}
		}
	}

	if (pdf_is_array(names))
	{
		int l = 0;
		int r = (pdf_array_len(names) / 2) - 1;

		while (l <= r)
		{
			int m = (l + r) >> 1;
			int c;
			pdf_obj *key = pdf_array_get(names, m * 2);
			pdf_obj *val = pdf_array_get(names, m * 2 + 1);

			c = pdf_objcmp(needle, key);
			if (c < 0)
				r = m - 1;
			else if (c > 0)
				l = m + 1;
			else
				return val;
		}

		
		r = pdf_array_len(names)/2;
		for (l = 0; l < r; l++)
			if (!pdf_objcmp(needle, pdf_array_get(names, l * 2)))
				return pdf_array_get(names, l * 2 + 1);
	}

	return NULL;
}

pdf_obj *
pdf_lookup_name(pdf_document *xref, char *which, pdf_obj *needle)
{
	base_context *ctx = xref->ctx;

	pdf_obj *root = pdf_dict_gets(xref->trailer, "Root");
	pdf_obj *names = pdf_dict_gets(root, "Names");
	pdf_obj *tree = pdf_dict_gets(names, which);
	return pdf_lookup_name_imp(ctx, tree, needle);
}

pdf_obj *
pdf_lookup_dest(pdf_document *xref, pdf_obj *needle)
{
	base_context *ctx = xref->ctx;

	pdf_obj *root = pdf_dict_gets(xref->trailer, "Root");
	pdf_obj *dests = pdf_dict_gets(root, "Dests");
	pdf_obj *names = pdf_dict_gets(root, "Names");
	pdf_obj *dest = NULL;

	
	if (dests)
	{
		if (pdf_is_name(needle))
			return pdf_dict_get(dests, needle);
		else
			return pdf_dict_gets(dests, pdf_to_str_buf(needle));
	}

	
	if (names && !dest)
	{
		pdf_obj *tree = pdf_dict_gets(names, "Dests");
		return pdf_lookup_name_imp(ctx, tree, needle);
	}

	return NULL;
}

static void
pdf_load_name_tree_imp(pdf_obj *dict, pdf_document *xref, pdf_obj *node)
{
	base_context *ctx = xref->ctx;
	pdf_obj *kids = pdf_dict_gets(node, "Kids");
	pdf_obj *names = pdf_dict_gets(node, "Names");
	int i;

	if (kids && !pdf_dict_mark(node))
	{
		for (i = 0; i < pdf_array_len(kids); i++)
			pdf_load_name_tree_imp(dict, xref, pdf_array_get(kids, i));
		pdf_dict_unmark(node);
	}

	if (names)
	{
		for (i = 0; i + 1 < pdf_array_len(names); i += 2)
		{
			pdf_obj *key = pdf_array_get(names, i);
			pdf_obj *val = pdf_array_get(names, i + 1);
			if (pdf_is_string(key))
			{
				key = pdf_to_utf8_name(ctx, key);
				base_dict_put(dict, key, val);
				pdf_drop_obj(key);
			}
			else if (pdf_is_name(key))
			{
				base_dict_put(dict, key, val);
			}
		}
	}
}

pdf_obj *
pdf_load_name_tree(pdf_document *xref, char *which)
{
	base_context *ctx = xref->ctx;

	pdf_obj *root = pdf_dict_gets(xref->trailer, "Root");
	pdf_obj *names = pdf_dict_gets(root, "Names");
	pdf_obj *tree = pdf_dict_gets(names, which);
	if (pdf_is_dict(tree))
	{
		pdf_obj *dict = pdf_new_dict(ctx, 100);
		pdf_load_name_tree_imp(dict, xref, tree);
		return dict;
	}
	return NULL;
}
