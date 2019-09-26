#include "pdf-internal.h"
#include "pdfengine-internal.h"

struct info
{
	pdf_obj *resources;
	pdf_obj *mediabox;
	pdf_obj *cropbox;
	pdf_obj *rotate;
};

static void
put_marker_bool(base_context *ctx, pdf_obj *rdb, char *marker, int val)
{
	pdf_obj *tmp;

	tmp = pdf_new_bool(ctx, val);
	base_try(ctx)
	{
		pdf_dict_puts(rdb, marker, tmp);
	}
	base_catch(ctx)
	{
		pdf_drop_obj(tmp);
		base_rethrow(ctx);
	}
	pdf_drop_obj(tmp);
}

typedef struct pdf_page_load_s pdf_page_load;

struct pdf_page_load_s
{
	int max;
	int pos;
	pdf_obj *node;
	pdf_obj *kids;
	struct info info;
};

static void
pdf_load_page_tree_node(pdf_document *xref, pdf_obj *node, struct info info)
{
	pdf_obj *dict, *kids, *count;
	pdf_obj *obj;
	base_context *ctx = xref->ctx;
	pdf_page_load *stack = NULL;
	int stacklen = -1;
	int stackmax = 0;

	base_try(ctx)
	{
		do
		{
			if (!node || pdf_dict_mark(node))
			{
				
			}
			else
			{
				kids = pdf_dict_gets(node, "Kids");
				count = pdf_dict_gets(node, "Count");
				if (pdf_is_array(kids) && pdf_is_int(count))
				{
					
					obj = pdf_dict_gets(node, "Resources");
					if (obj)
						info.resources = obj;
					obj = pdf_dict_gets(node, "MediaBox");
					if (obj)
						info.mediabox = obj;
					obj = pdf_dict_gets(node, "CropBox");
					if (obj)
						info.cropbox = obj;
					obj = pdf_dict_gets(node, "Rotate");
					if (obj)
						info.rotate = obj;
					stacklen++;
					if (stacklen == stackmax)
					{
						stack = base_resize_array(ctx, stack, stackmax ? stackmax*2 : 10, sizeof(*stack));
						stackmax = stackmax ? stackmax*2 : 10;
					}
					stack[stacklen].kids = kids;
					stack[stacklen].node = node;
					stack[stacklen].pos = -1;
					stack[stacklen].max = pdf_array_len(kids);
					stack[stacklen].info = info;
				}
				else if ((dict = pdf_to_dict(node)) != NULL)
				{
					if (info.resources && !pdf_dict_gets(dict, "Resources"))
						pdf_dict_puts(dict, "Resources", info.resources);
					if (info.mediabox && !pdf_dict_gets(dict, "MediaBox"))
						pdf_dict_puts(dict, "MediaBox", info.mediabox);
					if (info.cropbox && !pdf_dict_gets(dict, "CropBox"))
						pdf_dict_puts(dict, "CropBox", info.cropbox);
					if (info.rotate && !pdf_dict_gets(dict, "Rotate"))
						pdf_dict_puts(dict, "Rotate", info.rotate);

					if (xref->page_len == xref->page_cap)
					{
						base_warn(ctx, "found more pages than expected");
						xref->page_refs = base_resize_array(ctx, xref->page_refs, xref->page_cap+1, sizeof(pdf_obj*));
						xref->page_objs = base_resize_array(ctx, xref->page_objs, xref->page_cap+1, sizeof(pdf_obj*));
						xref->page_cap ++;
					}

					xref->page_refs[xref->page_len] = pdf_keep_obj(node);
					xref->page_objs[xref->page_len] = pdf_keep_obj(dict);
					xref->page_len ++;
					pdf_dict_unmark(node);
				}
			}
			
			while (++stack[stacklen].pos == stack[stacklen].max)
			{
				pdf_dict_unmark(stack[stacklen].node);
				stacklen--;
				if (stacklen < 0) 
					break;
				node = stack[stacklen].node;
				info = stack[stacklen].info;
				pdf_dict_unmark(node); 
			}
			if (stacklen >= 0)
				node = pdf_array_get(stack[stacklen].kids, stack[stacklen].pos);
		}
		while (stacklen >= 0);
	}
	base_always(ctx)
	{
		while (stacklen >= 0)
			pdf_dict_unmark(stack[stacklen--].node);
		base_free(ctx, stack);
	}
	base_catch(ctx)
	{
		base_rethrow(ctx);
	}
}

static void
pdf_load_page_tree(pdf_document *xref)
{
	base_context *ctx = xref->ctx;
	pdf_obj *catalog;
	pdf_obj *pages;
	pdf_obj *count;
	struct info info;

	if (xref->page_len)
		return;

	catalog = pdf_dict_gets(xref->trailer, "Root");
	pages = pdf_dict_gets(catalog, "Pages");
	count = pdf_dict_gets(pages, "Count");

	if (!pdf_is_dict(pages))
		base_throw(ctx, "missing page tree");
	if (!pdf_is_int(count))
		base_throw(ctx, "missing page count");

	xref->page_cap = pdf_to_int(count);
	xref->page_len = 0;
	xref->page_refs = base_malloc_array(ctx, xref->page_cap, sizeof(pdf_obj*));
	xref->page_objs = base_malloc_array(ctx, xref->page_cap, sizeof(pdf_obj*));

	info.resources = NULL;
	info.mediabox = NULL;
	info.cropbox = NULL;
	info.rotate = NULL;

	pdf_load_page_tree_node(xref, pages, info);
}

int
pdf_count_pages(pdf_document *xref)
{
	pdf_load_page_tree(xref);
	return xref->page_len;
}

int
pdf_lookup_page_number(pdf_document *xref, pdf_obj *page)
{
	int i, num = pdf_to_num(page);

	pdf_load_page_tree(xref);
	for (i = 0; i < xref->page_len; i++)
		if (num == pdf_to_num(xref->page_refs[i]))
			return i;
	return -1;
}

static int pdf_resources_use_blending(base_context *ctx, pdf_obj *rdb);

static int
pdf_extgstate_uses_blending(base_context *ctx, pdf_obj *dict)
{
	pdf_obj *obj = pdf_dict_gets(dict, "BM");
	if (pdf_is_name(obj) && strcmp(pdf_to_name(obj), "Normal"))
		return 1;
	return 0;
}

static int
pdf_pattern_uses_blending(base_context *ctx, pdf_obj *dict)
{
	pdf_obj *obj;
	obj = pdf_dict_gets(dict, "Resources");
	if (pdf_resources_use_blending(ctx, obj))
		return 1;
	obj = pdf_dict_gets(dict, "ExtGState");
	return pdf_extgstate_uses_blending(ctx, obj);
}

static int
pdf_xobject_uses_blending(base_context *ctx, pdf_obj *dict)
{
	pdf_obj *obj = pdf_dict_gets(dict, "Resources");
	return pdf_resources_use_blending(ctx, obj);
}

static int
pdf_resources_use_blending(base_context *ctx, pdf_obj *rdb)
{
	pdf_obj *obj;
	int i, n, useBM = 0;

	if (!rdb)
		return 0;

	
	obj = pdf_dict_gets(rdb, ".useBM");
	if (obj)
		return pdf_to_bool(obj);

	
	if (pdf_dict_mark(rdb))
		return 0;

	base_try(ctx)
	{
		obj = pdf_dict_gets(rdb, "ExtGState");
		n = pdf_dict_len(obj);
		for (i = 0; i < n; i++)
			if (pdf_extgstate_uses_blending(ctx, pdf_dict_get_val(obj, i)))
				goto found;

		obj = pdf_dict_gets(rdb, "Pattern");
		n = pdf_dict_len(obj);
		for (i = 0; i < n; i++)
			if (pdf_pattern_uses_blending(ctx, pdf_dict_get_val(obj, i)))
				goto found;

		obj = pdf_dict_gets(rdb, "XObject");
		n = pdf_dict_len(obj);
		for (i = 0; i < n; i++)
			if (pdf_xobject_uses_blending(ctx, pdf_dict_get_val(obj, i)))
				goto found;
		if (0)
		{
found:
			useBM = 1;
		}
	}
	base_catch(ctx)
	{
		pdf_dict_unmark(rdb);
		base_rethrow(ctx);
	}
	pdf_dict_unmark(rdb);

	put_marker_bool(ctx, rdb, ".useBM", useBM);
	return useBM;
}

static base_buffer *
pdf_load_page_contents_array(pdf_document *xref, pdf_obj *list)
{
	base_buffer *big;
	base_buffer *one;
	int i, n;
	base_context *ctx = xref->ctx;

	big = base_new_buffer(ctx, 32 * 1024);

	n = pdf_array_len(list);
	base_var(i); 
	for (i = 0; i < n; i++)
	{
		pdf_obj *stm = pdf_array_get(list, i);
		base_try(ctx)
		{
			one = pdf_load_stream(xref, pdf_to_num(stm), pdf_to_gen(stm));
		}
		base_catch(ctx)
		{
			base_warn(ctx, "cannot load content stream part %d/%d", i + 1, n);
			continue;
		}

		if (big->len + one->len + 1 > big->cap)
			base_resize_buffer(ctx, big, big->len + one->len + 1);
		memcpy(big->data + big->len, one->data, one->len);
		big->data[big->len + one->len] = ' ';
		big->len += one->len + 1;

		base_drop_buffer(ctx, one);
	}

	if (n > 0 && big->len == 0)
	{
		base_drop_buffer(ctx, big);
		base_throw(ctx, "cannot load content stream");
	}
	base_trim_buffer(ctx, big);

	return big;
}

static base_buffer *
pdf_load_page_contents(pdf_document *xref, pdf_obj *obj)
{
	base_context *ctx = xref->ctx;

	if (pdf_is_array(obj))
	{
		return pdf_load_page_contents_array(xref, obj);
		
	}
	else if (pdf_is_stream(xref, pdf_to_num(obj), pdf_to_gen(obj)))
	{
		return pdf_load_stream(xref, pdf_to_num(obj), pdf_to_gen(obj));
		
	}

	base_warn(ctx, "page contents missing, leaving page blank");
	return base_new_buffer(ctx, 0);
}

pdf_page *
pdf_load_page(pdf_document *xref, int number)
{
	base_context *ctx = xref->ctx;
	pdf_page *page;
	pdf_annot *annot;
	pdf_obj *pageobj, *pageref, *obj;
	base_rect mediabox, cropbox, realbox;
	base_matrix ctm;

	pdf_load_page_tree(xref);
	if (number < 0 || number >= xref->page_len)
		base_throw(ctx, "cannot find page %d", number + 1);

	pageobj = xref->page_objs[number];
	pageref = xref->page_refs[number];

	page = base_malloc_struct(ctx, pdf_page);
	page->resources = NULL;
	page->contents = NULL;
	page->transparency = 0;
	page->links = NULL;
	page->annots = NULL;

	mediabox = pdf_to_rect(ctx, pdf_dict_gets(pageobj, "MediaBox"));
	if (base_is_empty_rect(mediabox))
	{
		base_warn(ctx, "cannot find page size for page %d", number + 1);
		mediabox.x0 = 0;
		mediabox.y0 = 0;
		mediabox.x1 = 612;
		mediabox.y1 = 792;
	}

	cropbox = pdf_to_rect(ctx, pdf_dict_gets(pageobj, "CropBox"));
	if (!base_is_empty_rect(cropbox))
		mediabox = base_intersect_rect(mediabox, cropbox);

	page->mediabox.x0 = MIN(mediabox.x0, mediabox.x1);
	page->mediabox.y0 = MIN(mediabox.y0, mediabox.y1);
	page->mediabox.x1 = MAX(mediabox.x0, mediabox.x1);
	page->mediabox.y1 = MAX(mediabox.y0, mediabox.y1);

	if (page->mediabox.x1 - page->mediabox.x0 < 1 || page->mediabox.y1 - page->mediabox.y0 < 1)
	{
		base_warn(ctx, "invalid page size in page %d", number + 1);
		page->mediabox = base_unit_rect;
	}

	page->rotate = pdf_to_int(pdf_dict_gets(pageobj, "Rotate"));
	
	if (page->rotate < 0)
		page->rotate = 360 - ((-page->rotate) % 360);
	if (page->rotate >= 360)
		page->rotate = page->rotate % 360;
	page->rotate = 90*((page->rotate + 45)/90);
	if (page->rotate > 360)
		page->rotate = 0;

	ctm = base_concat(base_rotate(-page->rotate), base_scale(1, -1));
	realbox = base_transform_rect(ctm, page->mediabox);
	page->ctm = base_concat(ctm, base_translate(-realbox.x0, -realbox.y0));

	obj = pdf_dict_gets(pageobj, "Annots");
	if (obj)
	{
		page->links = pdf_load_link_annots(xref, obj, page->ctm);
		page->annots = pdf_load_annots(xref, obj);
	}

	page->resources = pdf_dict_gets(pageobj, "Resources");
	if (page->resources)
		pdf_keep_obj(page->resources);

	obj = pdf_dict_gets(pageobj, "Contents");
	base_try(ctx)
	{
		page->contents = pdf_load_page_contents(xref, obj);

		if (pdf_resources_use_blending(ctx, page->resources))
			page->transparency = 1;

		for (annot = page->annots; annot && !page->transparency; annot = annot->next)
			if (pdf_resources_use_blending(ctx, annot->ap->resources))
				page->transparency = 1;
	}
	base_catch(ctx)
	{
		pdf_free_page(xref, page);
		base_throw(ctx, "cannot load page %d contents (%d 0 R)", number + 1, pdf_to_num(pageref));
	}

	return page;
}

base_rect
pdf_bound_page(pdf_document *xref, pdf_page *page)
{
	base_rect bounds, mediabox = base_transform_rect(base_rotate(page->rotate), page->mediabox);
	bounds.x0 = bounds.y0 = 0;
	bounds.x1 = mediabox.x1 - mediabox.x0;
	bounds.y1 = mediabox.y1 - mediabox.y0;
	return bounds;
}

base_link *
pdf_load_links(pdf_document *xref, pdf_page *page)
{
	return base_keep_link(xref->ctx, page->links);
}

void
pdf_free_page(pdf_document *xref, pdf_page *page)
{
	if (page->resources)
		pdf_drop_obj(page->resources);
	if (page->contents)
		base_drop_buffer(xref->ctx, page->contents);
	if (page->links)
		base_drop_link(xref->ctx, page->links);
	if (page->annots)
		pdf_free_annot(xref->ctx, page->annots);
	base_free(xref->ctx, page);
}
