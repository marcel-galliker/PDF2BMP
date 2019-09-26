#include "pdf-internal.h"

base_text *
base_new_text(base_context *ctx, base_font *font, base_matrix trm, int wmode)
{
	base_text *text;

	text = base_malloc_struct(ctx, base_text);
	text->font = base_keep_font(ctx, font);
	text->trm = trm;
	text->wmode = wmode;
	text->len = 0;
	text->cap = 0;
	text->items = NULL;

	return text;
}

void
base_free_text(base_context *ctx, base_text *text)
{
	if (text != NULL)
	{
		base_drop_font(ctx, text->font);
		base_free(ctx, text->items);
	}
	base_free(ctx, text);
}

base_text *
base_clone_text(base_context *ctx, base_text *old)
{
	base_text *text;

	text = base_malloc_struct(ctx, base_text);
	text->len = old->len;
	base_try(ctx)
	{
		text->items = base_malloc_array(ctx, text->len, sizeof(base_text_item));
	}
	base_catch(ctx)
	{
		base_free(ctx, text);
		base_rethrow(ctx);
	}
	memcpy(text->items, old->items, text->len * sizeof(base_text_item));
	text->font = base_keep_font(ctx, old->font);
	text->trm = old->trm;
	text->wmode = old->wmode;
	text->cap = text->len;

	return text;
}

base_rect
base_bound_text(base_context *ctx, base_text *text, base_matrix ctm)
{
	base_matrix tm, trm;
	base_rect bbox;
	base_rect gbox;
	int i;

	if (text->len == 0)
		return base_empty_rect;

	

	tm = text->trm;

	tm.e = text->items[0].x;
	tm.f = text->items[0].y;
	trm = base_concat(tm, ctm);
	bbox = base_bound_glyph(ctx, text->font, text->items[0].gid, trm);

	for (i = 1; i < text->len; i++)
	{
		if (text->items[i].gid >= 0)
		{
			tm.e = text->items[i].x;
			tm.f = text->items[i].y;
			trm = base_concat(tm, ctm);
			gbox = base_bound_glyph(ctx, text->font, text->items[i].gid, trm);

			bbox.x0 = MIN(bbox.x0, gbox.x0);
			bbox.y0 = MIN(bbox.y0, gbox.y0);
			bbox.x1 = MAX(bbox.x1, gbox.x1);
			bbox.y1 = MAX(bbox.y1, gbox.y1);
		}
	}

	
	bbox.x0 -= 1;
	bbox.y0 -= 1;
	bbox.x1 += 1;
	bbox.y1 += 1;

	return bbox;
}

static void
base_grow_text(base_context *ctx, base_text *text, int n)
{
	int new_cap = text->cap;
	if (text->len + n < new_cap)
		return;
	while (text->len + n > new_cap)
		new_cap = new_cap + 36;
	text->items = base_resize_array(ctx, text->items, new_cap, sizeof(base_text_item));
	text->cap = new_cap;
}

void
base_add_text(base_context *ctx, base_text *text, int gid, int ucs, float x, float y)
{
	base_grow_text(ctx, text, 1);
	text->items[text->len].ucs = ucs;
	text->items[text->len].gid = gid;
	text->items[text->len].x = x;
	text->items[text->len].y = y;
	text->len++;
}

static int
isxmlmeta(int c)
{
	return c < 32 || c >= 128 || c == '&' || c == '<' || c == '>' || c == '\'' || c == '"';
}

static void
do_print_text(FILE *out, base_text *text, int indent)
{
	int i, n;
	for (i = 0; i < text->len; i++)
	{
		for (n = 0; n < indent; n++)
			fputc(' ', out);
		if (!isxmlmeta(text->items[i].ucs))
			fprintf(out, "<g ucs=\"%c\" gid=\"%d\" x=\"%g\" y=\"%g\" />\n",
				text->items[i].ucs, text->items[i].gid, text->items[i].x, text->items[i].y);
		else
			fprintf(out, "<g ucs=\"U+%04X\" gid=\"%d\" x=\"%g\" y=\"%g\" />\n",
				text->items[i].ucs, text->items[i].gid, text->items[i].x, text->items[i].y);
	}
}

void base_print_text(base_context *ctx, FILE *out, base_text *text)
{
	do_print_text(out, text, 0);
}
