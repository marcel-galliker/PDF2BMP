#include "pdf-internal.h"

#define LINE_DIST 0.9f
#define SPACE_DIST 0.2f
//#define SPACE_DIST (1/6)
#define PARAGRAPH_DIST 0.5f

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_ADVANCES_H

typedef struct base_text_device_s base_text_device;

struct base_text_device_s
{
	base_text_sheet *sheet;
	base_text_page *page;
	base_text_line cur_line;
	base_text_span cur_span;
	base_point point;
	int lastchar;
};

/***************************************************************************************/
/* function name:	base_new_text_sheet
/* description:		create base_text_sheet variable
/* param 1:			pointer to the context
/* return:			newly created base_text_sheet variable
/* creator & modifier : 
/* create:			2012.09.10 ~
/* working:			2012.09.10 ~
/***************************************************************************************/

base_text_sheet *
base_new_text_sheet(base_context *ctx)
{
	base_text_sheet *sheet = base_malloc(ctx, sizeof *sheet);
	sheet->maxid = 0;
	sheet->style = NULL;
	return sheet;
}

/***************************************************************************************/
/* function name:	base_free_text_sheet
/* description:		free base_text_sheet variable
/* param 1:			pointer to the context
/* param 2:			base_text_sheet to free
/* return:			none
/* creator & modifier : 
/* create:			2012.09.10 ~
/* working:			2012.09.10 ~
/***************************************************************************************/

void
base_free_text_sheet(base_context *ctx, base_text_sheet *sheet)
{
	base_text_style *style = sheet->style;
	while (style)
	{
		base_text_style *next = style->next;
		base_drop_font(ctx, style->font);
		base_free(ctx, style);
		style = next;
	}
	base_free(ctx, sheet);
}

static base_text_style *
base_lookup_text_style_imp(base_context *ctx, base_text_sheet *sheet,
	float size, base_font *font, int wmode, int script, float *color)
{
	base_text_style *style;

	for (style = sheet->style; style; style = style->next)
	{
		if (color != NULL &&
			style->font == font &&
			style->size == size &&
			style->wmode == wmode &&
			style->script == script && 
			style->color[0] == color[0] && style->color[1] == color[1] && style->color[2] == color[2] && style->color[3] == color[3])
		{
			return style;
		}
		
		if (color == NULL && 
			style->font == font &&
			style->size == size &&
			style->wmode == wmode &&
			style->script == script) 
		{
			return style;
		}
	}

	
	style = base_malloc(ctx, sizeof *style);
	style->id = sheet->maxid++;
	style->font = base_keep_font(ctx, font);
	style->size = size;
	style->wmode = wmode;
	style->script = script;
	style->next = sheet->style;
	sheet->style = style;
	if (color != NULL)
	{
		sheet->style->color[0] = color[0];
		sheet->style->color[1] = color[1];
		sheet->style->color[2] = color[2];
		sheet->style->color[3] = color[3];
	}
	return style;
}

static base_text_style *
base_lookup_text_style(base_context *ctx, base_text_sheet *sheet, base_text *text, base_matrix *ctm,
	base_colorspace *colorspace, float *color, float alpha, base_stroke_state *stroke)
{
	base_text_style *style;
	float colorfv[base_MAX_COLORS];
	float size = 1.0f;
	base_font *font = text ? text->font : NULL;
	int wmode = text ? text->wmode : 0;
	if (ctm && text)
	{
		base_matrix tm = text->trm;
		base_matrix trm;
		tm.e = 0;
		tm.f = 0;
		trm = base_concat(tm, *ctm);
		size = base_matrix_expansion(trm);
	}

	if (colorspace != NULL || color != NULL)
	{
		memset(colorfv, 0, sizeof(float) * base_MAX_COLORS);
		base_convert_color(ctx, base_device_rgb, colorfv, colorspace, color);
		colorfv[3] = alpha;

		style = base_lookup_text_style_imp(ctx, sheet, size, font, wmode, 0, colorfv);
	}
	else
	{
		style = base_lookup_text_style_imp(ctx, sheet, size, font, wmode, 0, color);
	}

	return style;
}

/***************************************************************************************/
/* function name:	base_new_text_page
/* description:		create base_text_page variable
/* param 1:			pointer to the context
/* param 2:			mediabox
/* return:			newly created base_text_page variable
/* creator & modifier : 
/* create:			2012.09.10 ~
/* working:			2012.09.10 ~
/***************************************************************************************/

base_text_page *
base_new_text_page(base_context *ctx, base_rect mediabox)
{
	base_text_page *page = base_malloc(ctx, sizeof(*page));
	page->mediabox = mediabox;
	page->len = 0;
	page->cap = 0;
	page->blocks = NULL;
	return page;
}

/***************************************************************************************/
/* function name:	base_free_text_page
/* description:		free base_text_page variable
/* param 1:			pointer to the context
/* param 2:			base_text_page to free
/* return:			none
/* creator & modifier : 
/* create:			2012.09.10 ~
/* working:			2012.09.10 ~
/***************************************************************************************/

void
base_free_text_page(base_context *ctx, base_text_page *page)
{
	base_text_block *block;
	base_text_line *line;
	base_text_span *span;
	for (block = page->blocks; block < page->blocks + page->len; block++)
	{
		for (line = block->lines; line < block->lines + block->len; line++)
		{
			for (span = line->spans; span < line->spans + line->len; span++)
			{
				base_free(ctx, span->text);
			}
			base_free(ctx, line->spans);
		}
		base_free(ctx, block->lines);
	}
	base_free(ctx, page->blocks);
	base_free(ctx, page);
}

static void
append_char(base_context *ctx, base_text_span *span, int c, base_rect bbox, base_text_style *style)
{
	if (span->len == span->cap)
	{
		int new_cap = MAX(64, span->cap * 2);
		span->text = base_resize_array(ctx, span->text, new_cap, sizeof(*span->text));
		span->cap = new_cap;
	}
	span->bbox = base_union_rect(span->bbox, bbox);
	span->text[span->len].c = c;
	span->text[span->len].bbox = bbox;
	span->text[span->len].color[0] = style->color[0];
	span->text[span->len].color[1] = style->color[1];
	span->text[span->len].color[2] = style->color[2];
	span->text[span->len].color[3] = style->color[3];
	span->len++;
}

static void
init_span(base_context *ctx, base_text_span *span, base_text_style *style)
{
	span->style = style;
	span->bbox = base_empty_rect;
	span->len = span->cap = 0;
	span->text = NULL;
}

static void
append_span(base_context *ctx, base_text_line *line, base_text_span *span)
{
	if (span->len == 0)
		return;
	if (line->len == line->cap)
	{
		int new_cap = MAX(8, line->cap * 2);
		line->spans = base_resize_array(ctx, line->spans, new_cap, sizeof(*line->spans));
		line->cap = new_cap;
	}
	line->bbox = base_union_rect(line->bbox, span->bbox);
	line->spans[line->len++] = *span;
}

static void
init_line(base_context *ctx, base_text_line *line)
{
	line->bbox = base_empty_rect;
	line->len = line->cap = 0;
	line->spans = NULL;
}

static void
append_line(base_context *ctx, base_text_block *block, base_text_line *line)
{
	if (block->len == block->cap)
	{
		int new_cap = MAX(16, block->cap * 2);
		block->lines = base_resize_array(ctx, block->lines, new_cap, sizeof *block->lines);
		block->cap = new_cap;
	}
	block->bbox = base_union_rect(block->bbox, line->bbox);
	block->lines[block->len++] = *line;
}

static base_text_block *
lookup_block_for_line(base_context *ctx, base_text_page *page, base_text_line *line)
{
	float size = line->len > 0 && line->spans[0].len > 0 ? line->spans[0].style->size : 1;
	int i;

	for (i = 0; i < page->len; i++)
	{
		base_text_block *block = page->blocks + i;
		float w = block->bbox.x1 - block->bbox.x0;
		float dx = line->bbox.x0 - block->bbox.x0;
		float dy = line->bbox.y0 - block->bbox.y1;
		if (dy > -size * 1.5f && dy < size * PARAGRAPH_DIST)
			if (line->bbox.x0 <= block->bbox.x1 && line->bbox.x1 >= block->bbox.x0)
				if (ABS(dx) < w / 2)
					return block;
	}

	if (page->len == page->cap)
	{
		int new_cap = MAX(16, page->cap * 2);
		page->blocks = base_resize_array(ctx, page->blocks, new_cap, sizeof(*page->blocks));
		page->cap = new_cap;
	}

	page->blocks[page->len].bbox = base_empty_rect;
	page->blocks[page->len].len = 0;
	page->blocks[page->len].cap = 0;
	page->blocks[page->len].lines = NULL;

	return &page->blocks[page->len++];
}

static void
insert_line(base_context *ctx, base_text_page *page, base_text_line *line)
{
	if (line->len == 0)
		return;
	append_line(ctx, lookup_block_for_line(ctx, page, line), line);
}

static base_rect
base_split_bbox(base_rect bbox, int i, int n)
{
	float w = (bbox.x1 - bbox.x0) / n;
	float x0 = bbox.x0;
	bbox.x0 = x0 + i * w;
	bbox.x1 = x0 + (i + 1) * w;
	return bbox;
}

static void
base_flush_text_line(base_context *ctx, base_text_device *dev, base_text_style *style)
{
	append_span(ctx, &dev->cur_line, &dev->cur_span);
	insert_line(ctx, dev->page, &dev->cur_line);
	init_span(ctx, &dev->cur_span, style);
	init_line(ctx, &dev->cur_line);
}

static void
base_add_text_char_imp(base_context *ctx, base_text_device *dev, base_text_style *style, int c, base_rect bbox)
{
	if (!dev->cur_span.style)
		dev->cur_span.style = style;
	if (style != dev->cur_span.style)
	{
		append_span(ctx, &dev->cur_line, &dev->cur_span);
		init_span(ctx, &dev->cur_span, style);
	}
	append_char(ctx, &dev->cur_span, c, bbox, style);
}

static void
base_add_text_char(base_context *ctx, base_text_device *dev, base_text_style *style, int c, base_rect bbox)
{
	switch (c)
	{
	case -1: 
		break;
	case 0xFB00: 
		base_add_text_char_imp(ctx, dev, style, 'f', base_split_bbox(bbox, 0, 2));
		base_add_text_char_imp(ctx, dev, style, 'f', base_split_bbox(bbox, 1, 2));
		break;
	case 0xFB01: 
		base_add_text_char_imp(ctx, dev, style, 'f', base_split_bbox(bbox, 0, 2));
		base_add_text_char_imp(ctx, dev, style, 'i', base_split_bbox(bbox, 1, 2));
		break;
	case 0xFB02: 
		base_add_text_char_imp(ctx, dev, style, 'f', base_split_bbox(bbox, 0, 2));
		base_add_text_char_imp(ctx, dev, style, 'l', base_split_bbox(bbox, 1, 2));
		break;
	case 0xFB03: 
		base_add_text_char_imp(ctx, dev, style, 'f', base_split_bbox(bbox, 0, 3));
		base_add_text_char_imp(ctx, dev, style, 'f', base_split_bbox(bbox, 1, 3));
		base_add_text_char_imp(ctx, dev, style, 'i', base_split_bbox(bbox, 2, 3));
		break;
	case 0xFB04: 
		base_add_text_char_imp(ctx, dev, style, 'f', base_split_bbox(bbox, 0, 3));
		base_add_text_char_imp(ctx, dev, style, 'f', base_split_bbox(bbox, 1, 3));
		base_add_text_char_imp(ctx, dev, style, 'l', base_split_bbox(bbox, 2, 3));
		break;
	case 0xFB05: 
	case 0xFB06: 
		base_add_text_char_imp(ctx, dev, style, 's', base_split_bbox(bbox, 0, 2));
		base_add_text_char_imp(ctx, dev, style, 't', base_split_bbox(bbox, 1, 2));
		break;
	default:
		base_add_text_char_imp(ctx, dev, style, c, bbox);
		break;
	}
}

static void
base_text_extract(base_context *ctx, base_text_device *dev, base_text *text, base_matrix ctm, base_text_style *style)
{
	base_point *pen = &dev->point;
	base_font *font = text->font;
	FT_Face face = font->ft_face;
	base_matrix tm = text->trm;
	base_matrix trm;
	float size;
	float adv;
	base_rect rect;
	base_point dir, ndir;
	base_point delta, ndelta;
	float dist, dot;
	float ascender = 1;
	float descender = 0;
	int multi;
	int i, j, err;

	if (text->len == 0)
		return;

	if (font->ft_face)
	{
		base_lock(ctx, base_LOCK_FREETYPE);
		err = FT_Set_Char_Size(font->ft_face, 64, 64, 72, 72);
		if (err)
			base_warn(ctx, "freetype set character size: %s", ft_error_string(err));
		ascender = (float)face->ascender / face->units_per_EM;
		descender = (float)face->descender / face->units_per_EM;
		base_unlock(ctx, base_LOCK_FREETYPE);
	}
	else if (font->t3procs && !base_is_empty_rect(font->bbox))
	{
		ascender = font->bbox.y1;
		descender = font->bbox.y0;
	}

	rect = base_empty_rect;

	if (text->wmode == 0)
	{
		dir.x = 1;
		dir.y = 0;
	}
	else
	{
		dir.x = 0;
		dir.y = 1;
	}

	tm.e = 0;
	tm.f = 0;
	trm = base_concat(tm, ctm);

	dir = base_transform_vector(trm, dir);
	dist = sqrtf(dir.x * dir.x + dir.y * dir.y);
	ndir.x = dir.x / dist;
	ndir.y = dir.y / dist;

	size = base_matrix_expansion(trm);

	for (i = 0; i < text->len; i++)
	{
		
		tm.e = text->items[i].x;
		tm.f = text->items[i].y;
		trm = base_concat(tm, ctm);

		delta.x = pen->x - trm.e;
		delta.y = pen->y - trm.f;
		if (pen->x == -1 && pen->y == -1)
			delta.x = delta.y = 0;

		dist = sqrtf(delta.x * delta.x + delta.y * delta.y);

		
		if (dist > 0)
		{
			ndelta.x = delta.x / dist;
			ndelta.y = delta.y / dist;
			dot = ndelta.x * ndir.x + ndelta.y * ndir.y;

			if (dist > size * LINE_DIST)
			{
				base_flush_text_line(ctx, dev, style);
				dev->lastchar = ' ';
			}
/*			else if (dev->lastchar == text->items[i].ucs && text->items[i].x < pen->x)
			{
				continue;
			}
*/			else if (fabsf(dot) > 0.95f && dist > size * SPACE_DIST && dev->lastchar != ' ')
			{
				base_rect spacerect;
				spacerect.x0 = -0.2f;
				spacerect.y0 = descender;
				spacerect.x1 = 0;
				spacerect.y1 = ascender;
				spacerect = base_transform_rect(trm, spacerect);
				base_add_text_char(ctx, dev, style, ' ', spacerect);
				dev->lastchar = ' ';
			}
		}

		
		if (font->ft_face)
		{
			FT_Fixed ftadv = 0;
			int mask = FT_LOAD_NO_BITMAP | FT_LOAD_NO_HINTING | FT_LOAD_IGNORE_TRANSFORM;

			
			

			base_lock(ctx, base_LOCK_FREETYPE);
			err = FT_Set_Char_Size(font->ft_face, 64, 64, 72, 72);
			if (err)
				base_warn(ctx, "freetype set character size: %s", ft_error_string(err));
			FT_Get_Advance(font->ft_face, text->items[i].gid, mask, &ftadv);
			adv = ftadv / 65536.0f;
			base_unlock(ctx, base_LOCK_FREETYPE);

			rect.x0 = 0;
			rect.y0 = descender;
			rect.x1 = adv;
			rect.y1 = ascender;
		}
		else
		{
			adv = font->t3widths[text->items[i].gid];
			rect.x0 = 0;
			rect.y0 = descender;
			rect.x1 = adv;
			rect.y1 = ascender;
		}

		rect = base_transform_rect(trm, rect);
		pen->x = trm.e + dir.x * adv;
		pen->y = trm.f + dir.y * adv;

		
		for (j = i + 1; j < text->len; j++)
			if (text->items[j].gid >= 0)
				break;
		multi = j - i;

		if (multi == 1)
		{
			base_add_text_char(ctx, dev, style, text->items[i].ucs, rect);
		}
		else
		{
			for (j = 0; j < multi; j++)
			{
				base_rect part = base_split_bbox(rect, j, multi);
				base_add_text_char(ctx, dev, style, text->items[i + j].ucs, part);
			}
			i += j - 1;
		}

		dev->lastchar = text->items[i].ucs;
	}
}

static void
base_text_fill_text(base_device *dev, base_text *text, base_matrix ctm,
	base_colorspace *colorspace, float *color, float alpha)
{
	base_text_device *tdev = dev->user;
	base_text_style *style;
	if (tdev->sheet == NULL)
		return;

	style = base_lookup_text_style(dev->ctx, tdev->sheet, text, &ctm, colorspace, color, alpha, NULL);
	base_text_extract(dev->ctx, tdev, text, ctm, style);
}

static void
base_text_stroke_text(base_device *dev, base_text *text, base_stroke_state *stroke, base_matrix ctm,
	base_colorspace *colorspace, float *color, float alpha)
{
	base_text_device *tdev = dev->user;
	base_text_style *style;
	if (tdev->sheet == NULL)
		return;

	style = base_lookup_text_style(dev->ctx, tdev->sheet, text, &ctm, colorspace, color, alpha, stroke);
	base_text_extract(dev->ctx, tdev, text, ctm, style);
}

static void
base_text_clip_text(base_device *dev, base_text *text, base_matrix ctm, int accumulate)
{
	base_text_device *tdev = dev->user;
	base_text_style *style;
	style = base_lookup_text_style(dev->ctx, tdev->sheet, text, &ctm, NULL, NULL, 0, NULL);
	base_text_extract(dev->ctx, tdev, text, ctm, style);
}

static void
base_text_clip_stroke_text(base_device *dev, base_text *text, base_stroke_state *stroke, base_matrix ctm)
{
	base_text_device *tdev = dev->user;
	base_text_style *style;
	style = base_lookup_text_style(dev->ctx, tdev->sheet, text, &ctm, NULL, NULL, 0, stroke);
	base_text_extract(dev->ctx, tdev, text, ctm, style);
}

static void
base_text_ignore_text(base_device *dev, base_text *text, base_matrix ctm)
{
	base_text_device *tdev = dev->user;
	base_text_style *style;
	style = base_lookup_text_style(dev->ctx, tdev->sheet, text, &ctm, NULL, NULL, 0, NULL);
	base_text_extract(dev->ctx, tdev, text, ctm, style);
}

static void
base_text_free_user(base_device *dev)
{
	base_context *ctx = dev->ctx;
	base_text_device *tdev = dev->user;

	append_span(ctx, &tdev->cur_line, &tdev->cur_span);
	insert_line(ctx, tdev->page, &tdev->cur_line);

	
	
	

	base_free(dev->ctx, tdev);
}

static void
base_text_flush_user(base_device *dev)
{
	base_context *ctx = dev->ctx;
	base_text_device *tdev = dev->user;

	append_span(ctx, &tdev->cur_line, &tdev->cur_span);
	insert_line(ctx, tdev->page, &tdev->cur_line);

	init_line(ctx, &tdev->cur_line);
	init_span(ctx, &tdev->cur_span, NULL);
}

/***************************************************************************************/
/* function name:	base_new_text_device
/* description:		create base_device variable
/* param 1:			pointer to the context
/* param 2:			pointer to the base_text_sheet
/* param 3:			pointer to the base_text_page
/* return:			newly created base_device variable
/* creator & modifier : 
/* create:			2012.09.10 ~
/* working:			2012.09.10 ~
/***************************************************************************************/

base_device *
base_new_text_device(base_context *ctx, base_text_sheet *sheet, base_text_page *page)
{
	base_device *dev;

	base_text_device *tdev = base_malloc_struct(ctx, base_text_device);
	tdev->sheet = sheet;
	tdev->page = page;
	tdev->point.x = -1;
	tdev->point.y = -1;
	tdev->lastchar = ' ';

	init_line(ctx, &tdev->cur_line);
	init_span(ctx, &tdev->cur_span, NULL);

	dev = base_new_device(ctx, tdev);
	dev->hints = base_IGNORE_IMAGE | base_IGNORE_SHADE;
	dev->free_user = base_text_free_user;
	dev->flush_user = base_text_flush_user;
	dev->fill_text = base_text_fill_text;
	dev->stroke_text = base_text_stroke_text;
	dev->clip_text = base_text_clip_text;
	dev->clip_stroke_text = base_text_clip_stroke_text;
	dev->ignore_text = base_text_ignore_text;
	return dev;
}

static int font_is_bold(base_font *font)
{
	FT_Face face = font->ft_face;
	if (face && (face->style_flags & FT_STYLE_FLAG_BOLD))
		return 1;
	if (strstr(font->name, "Bold"))
		return 1;
	return 0;
}

static int font_is_italic(base_font *font)
{
	FT_Face face = font->ft_face;
	if (face && (face->style_flags & FT_STYLE_FLAG_ITALIC))
		return 1;
	if (strstr(font->name, "Italic") || strstr(font->name, "Oblique"))
		return 1;
	return 0;
}

static void
base_print_style_begin(FILE *out, base_text_style *style)
{
	int script = style->script;
	fprintf(out, "<span class=\"s%d\">", style->id);
	while (script-- > 0)
		fprintf(out, "<sup>");
	while (++script < 0)
		fprintf(out, "<sub>");
}

static void
base_print_style_end(FILE *out, base_text_style *style)
{
	int script = style->script;
	while (script-- > 0)
		fprintf(out, "</sup>");
	while (++script < 0)
		fprintf(out, "</sub>");
	fprintf(out, "</span>");
}

static void
base_print_style(FILE *out, base_text_style *style)
{
	char *s = strchr(style->font->name, '+');
	s = s ? s + 1 : style->font->name;
	fprintf(out, "span.s%d{font-family:\"%s\";font-size:%gpt;",
		style->id, s, style->size);
	if (font_is_italic(style->font))
		fprintf(out, "font-style:italic;");
	if (font_is_bold(style->font))
		fprintf(out, "font-weight:bold;");
	fprintf(out, "}\n");
}

void
base_print_text_sheet(base_context *ctx, FILE *out, base_text_sheet *sheet)
{
	base_text_style *style;
	for (style = sheet->style; style; style = style->next)
		base_print_style(out, style);
}

void
base_print_text_page_html(base_context *ctx, FILE *out, base_text_page *page)
{
	int block_n, line_n, span_n, ch_n;
	base_text_style *style = NULL;
	base_text_block *block;
	base_text_line *line;
	base_text_span *span;

	fprintf(out, "<div class=\"page\">\n");

	for (block_n = 0; block_n < page->len; block_n++)
	{
		block = &page->blocks[block_n];
		fprintf(out, "<div class=\"block\">\n");
		for (line_n = 0; line_n < block->len; line_n++)
		{
			line = &block->lines[line_n];
			fprintf(out, "<p>");
			style = NULL;

			for (span_n = 0; span_n < line->len; span_n++)
			{
				span = &line->spans[span_n];
				if (style != span->style)
				{
					if (style)
						base_print_style_end(out, style);
					base_print_style_begin(out, span->style);
					style = span->style;
				}

				for (ch_n = 0; ch_n < span->len; ch_n++)
				{
					base_text_char *ch = &span->text[ch_n];
					if (ch->c == '<')
						fprintf(out, "&lt;");
					else if (ch->c == '>')
						fprintf(out, "&gt;");
					else if (ch->c == '&')
						fprintf(out, "&amp;");
					else if (ch->c >= 32 && ch->c <= 127)
						fprintf(out, "%c", ch->c);
					else
						fprintf(out, "&#x%x;", ch->c);
				}
			}
			if (style)
				base_print_style_end(out, style);
			fprintf(out, "</p>\n");
		}
		fprintf(out, "</div>\n");
	}

	fprintf(out, "</div>\n");
}

void
base_print_text_page_xml(base_context *ctx, FILE *out, base_text_page *page)
{
	base_text_block *block;
	base_text_line *line;
	base_text_span *span;
	base_text_char *ch;
	char *s;

	fprintf(out, "<page>\n");
	for (block = page->blocks; block < page->blocks + page->len; block++)
	{
		fprintf(out, "<block bbox=\"%g %g %g %g\">\n",
			block->bbox.x0, block->bbox.y0, block->bbox.x1, block->bbox.y1);
		for (line = block->lines; line < block->lines + block->len; line++)
		{
			fprintf(out, "<line bbox=\"%g %g %g %g\">\n",
				line->bbox.x0, line->bbox.y0, line->bbox.x1, line->bbox.y1);
			for (span = line->spans; span < line->spans + line->len; span++)
			{
				base_text_style *style = span->style;
				s = strchr(style->font->name, '+');
				s = s ? s + 1 : style->font->name;
				fprintf(out, "<span bbox=\"%g %g %g %g\" font=\"%s\" size=\"%g\">\n",
					span->bbox.x0, span->bbox.y0, span->bbox.x1, span->bbox.y1,
					s, style->size);
				for (ch = span->text; ch < span->text + span->len; ch++)
				{
					fprintf(out, "<char bbox=\"%g %g %g %g\" c=\"",
						ch->bbox.x0, ch->bbox.y0, ch->bbox.x1, ch->bbox.y1);
					switch (ch->c)
					{
					case '<': fprintf(out, "&lt;"); break;
					case '>': fprintf(out, "&gt;"); break;
					case '&': fprintf(out, "&amp;"); break;
					case '"': fprintf(out, "&quot;"); break;
					case '\'': fprintf(out, "&apos;"); break;
					default:
						if (ch->c >= 32 && ch->c <= 127)
							fprintf(out, "%c", ch->c);
						else
							fprintf(out, "&#x%x;", ch->c);
						break;
					}
					fprintf(out, "\"/>\n");
			}
				fprintf(out, "</span>\n");
			}
			fprintf(out, "</line>\n");
		}
		fprintf(out, "</block>\n");
	}
	fprintf(out, "</page>\n");
}

void
base_print_text_page(base_context *ctx, FILE *out, base_text_page *page)
{
	base_text_block *block;
	base_text_line *line;
	base_text_span *span;
	base_text_char *ch;
	char utf[10];
	int i, n;

	for (block = page->blocks; block < page->blocks + page->len; block++)
	{
		for (line = block->lines; line < block->lines + block->len; line++)
		{
			for (span = line->spans; span < line->spans + line->len; span++)
			{
				for (ch = span->text; ch < span->text + span->len; ch++)
				{
					n = base_runetochar(utf, ch->c);
					for (i = 0; i < n; i++)
						putc(utf[i], out);
				}
			}
			fprintf(out, "\n");
		}
		fprintf(out, "\n");
	}
}
