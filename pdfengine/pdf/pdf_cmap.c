

#include "pdf-internal.h"
#include "pdfengine-internal.h"

#define pdf_range_high(r) ((r)->low + ((r)->extent_flags >> 2))
#define pdf_range_flags(r) ((r)->extent_flags & 3)
#define pdf_range_set_high(r, h) \
	((r)->extent_flags = (((r)->extent_flags & 3) | ((h - (r)->low) << 2)))
#define pdf_range_set_flags(r, f) \
	((r)->extent_flags = (((r)->extent_flags & ~3) | f))

void
pdf_free_cmap_imp(base_context *ctx, base_storable *cmap_)
{
	pdf_cmap *cmap = (pdf_cmap *)cmap_;
	if (cmap->usecmap)
		pdf_drop_cmap(ctx, cmap->usecmap);
	base_free(ctx, cmap->ranges);
	base_free(ctx, cmap->table);
	base_free(ctx, cmap);
}

pdf_cmap *
pdf_new_cmap(base_context *ctx)
{
	pdf_cmap *cmap;

	cmap = base_malloc_struct(ctx, pdf_cmap);
	base_INIT_STORABLE(cmap, 1, pdf_free_cmap_imp);

	strcpy(cmap->cmap_name, "");
	strcpy(cmap->usecmap_name, "");
	cmap->usecmap = NULL;
	cmap->wmode = 0;
	cmap->codespace_len = 0;

	cmap->rlen = 0;
	cmap->rcap = 0;
	cmap->ranges = NULL;

	cmap->tlen = 0;
	cmap->tcap = 0;
	cmap->table = NULL;

	return cmap;
}

pdf_cmap *
pdf_keep_cmap(base_context *ctx, pdf_cmap *cmap)
{
	return (pdf_cmap *)base_keep_storable(ctx, &cmap->storable);
}

void
pdf_drop_cmap(base_context *ctx, pdf_cmap *cmap)
{
	base_drop_storable(ctx, &cmap->storable);
}

void
pdf_set_usecmap(base_context *ctx, pdf_cmap *cmap, pdf_cmap *usecmap)
{
	int i;

	if (cmap->usecmap)
		pdf_drop_cmap(ctx, cmap->usecmap);
	cmap->usecmap = pdf_keep_cmap(ctx, usecmap);

	if (cmap->codespace_len == 0)
	{
		cmap->codespace_len = usecmap->codespace_len;
		for (i = 0; i < usecmap->codespace_len; i++)
			cmap->codespace[i] = usecmap->codespace[i];
	}
}

int
pdf_cmap_wmode(base_context *ctx, pdf_cmap *cmap)
{
	return cmap->wmode;
}

void
pdf_set_cmap_wmode(base_context *ctx, pdf_cmap *cmap, int wmode)
{
	cmap->wmode = wmode;
}

void
pdf_print_cmap(base_context *ctx, pdf_cmap *cmap)
{
	int i, k, n;

	printf("cmap $%p /%s {\n", (void *) cmap, cmap->cmap_name);

	if (cmap->usecmap_name[0])
		printf("\tusecmap /%s\n", cmap->usecmap_name);
	if (cmap->usecmap)
		printf("\tusecmap $%p\n", (void *) cmap->usecmap);

	printf("\twmode %d\n", cmap->wmode);

	printf("\tcodespaces {\n");
	for (i = 0; i < cmap->codespace_len; i++)
	{
		printf("\t\t<%x> <%x>\n", cmap->codespace[i].low, cmap->codespace[i].high);
	}
	printf("\t}\n");

	printf("\tranges (%d,%d) {\n", cmap->rlen, cmap->tlen);
	for (i = 0; i < cmap->rlen; i++)
	{
		pdf_range *r = &cmap->ranges[i];
		printf("\t\t<%04x> <%04x> ", r->low, pdf_range_high(r));
		if (pdf_range_flags(r) == PDF_CMAP_TABLE)
		{
			printf("[ ");
			for (k = 0; k < pdf_range_high(r) - r->low + 1; k++)
				printf("%d ", cmap->table[r->offset + k]);
			printf("]\n");
		}
		else if (pdf_range_flags(r) == PDF_CMAP_MULTI)
		{
			printf("< ");
			n = cmap->table[r->offset];
			for (k = 0; k < n; k++)
				printf("%04x ", cmap->table[r->offset + 1 + k]);
			printf(">\n");
		}
		else
			printf("%d\n", r->offset);
	}
	printf("\t}\n}\n");
}

void
pdf_add_codespace(base_context *ctx, pdf_cmap *cmap, int low, int high, int n)
{
	if (cmap->codespace_len + 1 == nelem(cmap->codespace))
	{
		base_warn(ctx, "assert: too many code space ranges");
		return;
	}

	cmap->codespace[cmap->codespace_len].n = n;
	cmap->codespace[cmap->codespace_len].low = low;
	cmap->codespace[cmap->codespace_len].high = high;
	cmap->codespace_len ++;
}

static void
add_table(base_context *ctx, pdf_cmap *cmap, int value)
{
	if (cmap->tlen == USHRT_MAX)
	{
		base_warn(ctx, "cmap table is full; ignoring additional entries");
		return;
	}
	if (cmap->tlen + 1 > cmap->tcap)
	{
		int new_cap = cmap->tcap > 1 ? (cmap->tcap * 3) / 2 : 256; 
		cmap->table = base_resize_array(ctx, cmap->table, new_cap, sizeof(unsigned short));
		cmap->tcap = new_cap;
	}
	cmap->table[cmap->tlen++] = value;
}

static void
add_range(base_context *ctx, pdf_cmap *cmap, int low, int high, int flag, int offset)
{
	
	if (high - low > 0x3fff)
	{
		add_range(ctx, cmap, low, low+0x3fff, flag, offset);
		add_range(ctx, cmap, low+0x3fff, high, flag, offset+0x3fff);
		return;
	}
	if (cmap->rlen + 1 > cmap->rcap)
	{
		int new_cap = cmap->rcap > 1 ? (cmap->rcap * 3) / 2 : 256;
		cmap->ranges = base_resize_array(ctx, cmap->ranges, new_cap, sizeof(pdf_range));
		cmap->rcap = new_cap;
	}
	cmap->ranges[cmap->rlen].low = low;
	pdf_range_set_high(&cmap->ranges[cmap->rlen], high);
	pdf_range_set_flags(&cmap->ranges[cmap->rlen], flag);
	cmap->ranges[cmap->rlen].offset = offset;
	cmap->rlen ++;
}

void
pdf_map_range_to_table(base_context *ctx, pdf_cmap *cmap, int low, int *table, int len)
{
	int i;
	int high = low + len;
	int offset = cmap->tlen;
	if (cmap->tlen + len >= USHRT_MAX)
		base_warn(ctx, "cannot map range to table; table is full");
	else
	{
		for (i = 0; i < len; i++)
			add_table(ctx, cmap, table[i]);
		add_range(ctx, cmap, low, high, PDF_CMAP_TABLE, offset);
	}
}

void
pdf_map_range_to_range(base_context *ctx, pdf_cmap *cmap, int low, int high, int offset)
{
	add_range(ctx, cmap, low, high, high - low == 0 ? PDF_CMAP_SINGLE : PDF_CMAP_RANGE, offset);
}

void
pdf_map_one_to_many(base_context *ctx, pdf_cmap *cmap, int low, int *values, int len)
{
	int offset, i;

	if (len == 1)
	{
		add_range(ctx, cmap, low, low, PDF_CMAP_SINGLE, values[0]);
		return;
	}

	if (len > 8)
	{
		base_warn(ctx, "one to many mapping is too large (%d); truncating", len);
		len = 8;
	}

	if (len == 2 &&
		values[0] >= 0xD800 && values[0] <= 0xDBFF &&
		values[1] >= 0xDC00 && values[1] <= 0xDFFF)
	{
		base_warn(ctx, "ignoring surrogate pair mapping in cmap");
		return;
	}

	if (cmap->tlen + len + 1 >= USHRT_MAX)
		base_warn(ctx, "cannot map one to many; table is full");
	else
	{
		offset = cmap->tlen;
		add_table(ctx, cmap, len);
		for (i = 0; i < len; i++)
			add_table(ctx, cmap, values[i]);
		add_range(ctx, cmap, low, low, PDF_CMAP_MULTI, offset);
	}
}

static int cmprange(const void *va, const void *vb)
{
	return ((const pdf_range*)va)->low - ((const pdf_range*)vb)->low;
}

void
pdf_sort_cmap(base_context *ctx, pdf_cmap *cmap)
{
	pdf_range *a;			
	pdf_range *b;			

	if (cmap->rlen == 0)
		return;

	qsort(cmap->ranges, cmap->rlen, sizeof(pdf_range), cmprange);

	if (cmap->tlen == USHRT_MAX)
	{
		base_warn(ctx, "cmap table is full; will not combine ranges");
		return;
	}

	a = cmap->ranges;
	b = cmap->ranges + 1;

	while (b < cmap->ranges + cmap->rlen)
	{
		
		if (pdf_range_flags(b) == PDF_CMAP_MULTI)
		{
			*(++a) = *b;
		}

		
		else if (pdf_range_high(a) + 1 == b->low)
		{
			
			if (pdf_range_high(a) - a->low + a->offset + 1 == b->offset)
			{
				
				if ((pdf_range_flags(a) == PDF_CMAP_SINGLE || pdf_range_flags(a) == PDF_CMAP_RANGE) && (pdf_range_high(b) - a->low <= 0x3fff))
				{
					pdf_range_set_flags(a, PDF_CMAP_RANGE);
					pdf_range_set_high(a, pdf_range_high(b));
				}

				
				else if (pdf_range_flags(a) == PDF_CMAP_TABLE && pdf_range_flags(b) == PDF_CMAP_SINGLE && (pdf_range_high(b) - a->low <= 0x3fff))
				{
					pdf_range_set_high(a, pdf_range_high(b));
					add_table(ctx, cmap, b->offset);
				}

				
				else if (pdf_range_flags(a) == PDF_CMAP_TABLE && pdf_range_flags(b) == PDF_CMAP_RANGE)
				{
					*(++a) = *b;
				}

				
				else
				{
					*(++a) = *b;
				}
			}

			
			else
			{
				
				if (pdf_range_flags(a) == PDF_CMAP_SINGLE && pdf_range_flags(b) == PDF_CMAP_SINGLE)
				{
					pdf_range_set_flags(a, PDF_CMAP_TABLE);
					pdf_range_set_high(a, pdf_range_high(b));
					add_table(ctx, cmap, a->offset);
					add_table(ctx, cmap, b->offset);
					a->offset = cmap->tlen - 2;
				}

				
				else if (pdf_range_flags(a) == PDF_CMAP_TABLE && pdf_range_flags(b) == PDF_CMAP_SINGLE && (pdf_range_high(b) - a->low <= 0x3fff))
				{
					pdf_range_set_high(a, pdf_range_high(b));
					add_table(ctx, cmap, b->offset);
				}

				
				else
				{
					*(++a) = *b;
				}
			}
		}

		
		else
		{
			*(++a) = *b;
		}

		b ++;
	}

	cmap->rlen = a - cmap->ranges + 1;
}

int
pdf_lookup_cmap(pdf_cmap *cmap, int cpt)
{
	int l = 0;
	int r = cmap->rlen - 1;
	int m;

	while (l <= r)
	{
		m = (l + r) >> 1;
		if (cpt < cmap->ranges[m].low)
			r = m - 1;
		else if (cpt > pdf_range_high(&cmap->ranges[m]))
			l = m + 1;
		else
		{
			int i = cpt - cmap->ranges[m].low + cmap->ranges[m].offset;
			if (pdf_range_flags(&cmap->ranges[m]) == PDF_CMAP_TABLE)
				return cmap->table[i];
			if (pdf_range_flags(&cmap->ranges[m]) == PDF_CMAP_MULTI)
				return -1; 
			return i;
		}
	}

	if (cmap->usecmap)
		return pdf_lookup_cmap(cmap->usecmap, cpt);

	return -1;
}

int
pdf_lookup_cmap_full(pdf_cmap *cmap, int cpt, int *out)
{
	int i, k, n;
	int l = 0;
	int r = cmap->rlen - 1;
	int m;

	while (l <= r)
	{
		m = (l + r) >> 1;
		if (cpt < cmap->ranges[m].low)
			r = m - 1;
		else if (cpt > pdf_range_high(&cmap->ranges[m]))
			l = m + 1;
		else
		{
			k = cpt - cmap->ranges[m].low + cmap->ranges[m].offset;
			if (pdf_range_flags(&cmap->ranges[m]) == PDF_CMAP_TABLE)
			{
				out[0] = cmap->table[k];
				return 1;
			}
			else if (pdf_range_flags(&cmap->ranges[m]) == PDF_CMAP_MULTI)
			{
				n = cmap->ranges[m].offset;
				for (i = 0; i < cmap->table[n]; i++)
					out[i] = cmap->table[n + i + 1];
				return cmap->table[n];
			}
			else
			{
				out[0] = k;
				return 1;
			}
		}
	}

	if (cmap->usecmap)
		return pdf_lookup_cmap_full(cmap->usecmap, cpt, out);

	return 0;
}

int
pdf_decode_cmap(pdf_cmap *cmap, unsigned char *buf, int *cpt)
{
	int k, n, c;

	c = 0;
	for (n = 0; n < 4; n++)
	{
		c = (c << 8) | buf[n];
		for (k = 0; k < cmap->codespace_len; k++)
		{
			if (cmap->codespace[k].n == n + 1)
			{
				if (c >= cmap->codespace[k].low && c <= cmap->codespace[k].high)
				{
					*cpt = c;
					return n + 1;
				}
			}
		}
	}

	*cpt = 0;
	return 1;
}
