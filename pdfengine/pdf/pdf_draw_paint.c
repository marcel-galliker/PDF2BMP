#include "pdf-internal.h"

typedef unsigned char byte;

void
base_paint_solid_alpha(byte * restrict dp, int w, int alpha)
{
	int t = base_EXPAND(255 - alpha);
	while (w--)
	{
		*dp = alpha + base_COMBINE(*dp, t);
		dp ++;
	}
}

void
base_paint_solid_color(byte * restrict dp, int n, int w, byte *color)
{
	int n1 = n - 1;
	int sa = base_EXPAND(color[n1]);
	int k;
	while (w--)
	{
		int ma = base_COMBINE(base_EXPAND(255), sa);
		for (k = 0; k < n1; k++)
			dp[k] = base_BLEND(color[k], dp[k], ma);
		dp[k] = base_BLEND(255, dp[k], ma);
		dp += n;
	}
}

static inline void
base_paint_span_with_color_2(byte * restrict dp, byte * restrict mp, int w, byte *color)
{
	int sa = base_EXPAND(color[1]);
	int g = color[0];
	while (w--)
	{
		int ma = *mp++;
		ma = base_COMBINE(base_EXPAND(ma), sa);
		dp[0] = base_BLEND(g, dp[0], ma);
		dp[1] = base_BLEND(255, dp[1], ma);
		dp += 2;
	}
}

static inline void
base_paint_span_with_color_4(byte * restrict dp, byte * restrict mp, int w, byte *color)
{
	int sa = base_EXPAND(color[3]);
	int r = color[0];
	int g = color[1];
	int b = color[2];
	while (w--)
	{
		int ma = *mp++;
		ma = base_COMBINE(base_EXPAND(ma), sa);
		dp[0] = base_BLEND(r, dp[0], ma);
		dp[1] = base_BLEND(g, dp[1], ma);
		dp[2] = base_BLEND(b, dp[2], ma);
		dp[3] = base_BLEND(255, dp[3], ma);
		dp += 4;
	}
}

static inline void
base_paint_span_with_color_N(byte * restrict dp, byte * restrict mp, int n, int w, byte *color)
{
	int n1 = n - 1;
	int sa = base_EXPAND(color[n1]);
	int k;
	while (w--)
	{
		int ma = *mp++;
		ma = base_COMBINE(base_EXPAND(ma), sa);
		for (k = 0; k < n1; k++)
			dp[k] = base_BLEND(color[k], dp[k], ma);
		dp[k] = base_BLEND(255, dp[k], ma);
		dp += n;
	}
}

void
base_paint_span_with_color(byte * restrict dp, byte * restrict mp, int n, int w, byte *color)
{
	switch (n)
	{
	case 2: base_paint_span_with_color_2(dp, mp, w, color); break;
	case 4: base_paint_span_with_color_4(dp, mp, w, color); break;
	default: base_paint_span_with_color_N(dp, mp, n, w, color); break;
	}
}

static inline void
base_paint_span_with_mask_2(byte * restrict dp, byte * restrict sp, byte * restrict mp, int w)
{
	while (w--)
	{
		int masa;
		int ma = *mp++;
		ma = base_EXPAND(ma);
		masa = base_COMBINE(sp[1], ma);
		masa = 255 - masa;
		masa = base_EXPAND(masa);
		*dp = base_COMBINE2(*sp, ma, *dp, masa);
		sp++; dp++;
		*dp = base_COMBINE2(*sp, ma, *dp, masa);
		sp++; dp++;
	}
}

static inline void
base_paint_span_with_mask_4(byte * restrict dp, byte * restrict sp, byte * restrict mp, int w)
{
	while (w--)
	{
		int masa;
		int ma = *mp++;
		ma = base_EXPAND(ma);
		masa = base_COMBINE(sp[3], ma);
		masa = 255 - masa;
		masa = base_EXPAND(masa);
		*dp = base_COMBINE2(*sp, ma, *dp, masa);
		sp++; dp++;
		*dp = base_COMBINE2(*sp, ma, *dp, masa);
		sp++; dp++;
		*dp = base_COMBINE2(*sp, ma, *dp, masa);
		sp++; dp++;
		*dp = base_COMBINE2(*sp, ma, *dp, masa);
		sp++; dp++;
	}
}

static inline void
base_paint_span_with_mask_N(byte * restrict dp, byte * restrict sp, byte * restrict mp, int n, int w)
{
	while (w--)
	{
		int k = n;
		int masa;
		int ma = *mp++;
		ma = base_EXPAND(ma);
		masa = base_COMBINE(sp[n-1], ma);
		masa = 255-masa;
		masa = base_EXPAND(masa);
		while (k--)
		{
			*dp = base_COMBINE2(*sp, ma, *dp, masa);
			sp++; dp++;
		}
	}
}

static void
base_paint_span_with_mask(byte * restrict dp, byte * restrict sp, byte * restrict mp, int n, int w)
{
	switch (n)
	{
	case 2: base_paint_span_with_mask_2(dp, sp, mp, w); break;
	case 4: base_paint_span_with_mask_4(dp, sp, mp, w); break;
	default: base_paint_span_with_mask_N(dp, sp, mp, n, w); break;
	}
}

static inline void
base_paint_span_2_with_alpha(byte * restrict dp, byte * restrict sp, int w, int alpha)
{
	alpha = base_EXPAND(alpha);
	while (w--)
	{
		int masa = base_COMBINE(sp[1], alpha);
		*dp = base_BLEND(*sp, *dp, masa);
		dp++; sp++;
		*dp = base_BLEND(*sp, *dp, masa);
		dp++; sp++;
	}
}

static inline void
base_paint_span_4_with_alpha(byte * restrict dp, byte * restrict sp, int w, int alpha)
{
	alpha = base_EXPAND(alpha);
	while (w--)
	{
		int masa = base_COMBINE(sp[3], alpha);
		*dp = base_BLEND(*sp, *dp, masa);
		sp++; dp++;
		*dp = base_BLEND(*sp, *dp, masa);
		sp++; dp++;
		*dp = base_BLEND(*sp, *dp, masa);
		sp++; dp++;
		*dp = base_BLEND(*sp, *dp, masa);
		sp++; dp++;
	}
}

static inline void
base_paint_span_N_with_alpha(byte * restrict dp, byte * restrict sp, int n, int w, int alpha)
{
	alpha = base_EXPAND(alpha);
	while (w--)
	{
		int masa = base_COMBINE(sp[n-1], alpha);
		int k = n;
		while (k--)
		{
			*dp = base_BLEND(*sp++, *dp, masa);
			dp++;
		}
	}
}

static inline void
base_paint_span_1(byte * restrict dp, byte * restrict sp, int w)
{
	while (w--)
	{
		int t = base_EXPAND(255 - sp[0]);
		*dp = *sp++ + base_COMBINE(*dp, t);
		dp ++;
	}
}

static inline void
base_paint_span_2(byte * restrict dp, byte * restrict sp, int w)
{
	while (w--)
	{
		int t = base_EXPAND(255 - sp[1]);
		*dp = *sp++ + base_COMBINE(*dp, t);
		dp++;
		*dp = *sp++ + base_COMBINE(*dp, t);
		dp++;
	}
}

static inline void
base_paint_span_4(byte * restrict dp, byte * restrict sp, int w)
{
	while (w--)
	{
		int t = base_EXPAND(255 - sp[3]);
		*dp = *sp++ + base_COMBINE(*dp, t);
		dp++;
		*dp = *sp++ + base_COMBINE(*dp, t);
		dp++;
		*dp = *sp++ + base_COMBINE(*dp, t);
		dp++;
		*dp = *sp++ + base_COMBINE(*dp, t);
		dp++;
	}
}

static inline void
base_paint_span_N(byte * restrict dp, byte * restrict sp, int n, int w)
{
	while (w--)
	{
		int k = n;
		int t = base_EXPAND(255 - sp[n-1]);
		while (k--)
		{
			*dp = *sp++ + base_COMBINE(*dp, t);
			dp++;
		}
	}
}

void
base_paint_span(byte * restrict dp, byte * restrict sp, int n, int w, int alpha)
{
	if (alpha == 255)
	{
		switch (n)
		{
		case 1: base_paint_span_1(dp, sp, w); break;
		case 2: base_paint_span_2(dp, sp, w); break;
		case 4: base_paint_span_4(dp, sp, w); break;
		default: base_paint_span_N(dp, sp, n, w); break;
		}
	}
	else if (alpha > 0)
	{
		switch (n)
		{
		case 2: base_paint_span_2_with_alpha(dp, sp, w, alpha); break;
		case 4: base_paint_span_4_with_alpha(dp, sp, w, alpha); break;
		default: base_paint_span_N_with_alpha(dp, sp, n, w, alpha); break;
		}
	}
}

void
base_paint_pixmap_with_rect(base_pixmap *dst, base_pixmap *src, int alpha, base_bbox bbox)
{
	unsigned char *sp, *dp;
	int x, y, w, h, n;

	assert(dst->n == src->n);

	bbox = base_intersect_bbox(bbox, base_pixmap_bbox_no_ctx(dst));
	bbox = base_intersect_bbox(bbox, base_pixmap_bbox_no_ctx(src));

	x = bbox.x0;
	y = bbox.y0;
	w = bbox.x1 - bbox.x0;
	h = bbox.y1 - bbox.y0;
	if ((w | h) == 0)
		return;

	n = src->n;
	sp = src->samples + ((y - src->y) * src->w + (x - src->x)) * src->n;
	dp = dst->samples + ((y - dst->y) * dst->w + (x - dst->x)) * dst->n;

	while (h--)
	{
		base_paint_span(dp, sp, n, w, alpha);
		sp += src->w * n;
		dp += dst->w * n;
	}
}

void
base_paint_pixmap(base_pixmap *dst, base_pixmap *src, int alpha)
{
	unsigned char *sp, *dp;
	base_bbox bbox;
	int x, y, w, h, n;

	assert(dst->n == src->n);

	bbox = base_pixmap_bbox_no_ctx(dst);
	bbox = base_intersect_bbox(bbox, base_pixmap_bbox_no_ctx(src));

	x = bbox.x0;
	y = bbox.y0;
	w = bbox.x1 - bbox.x0;
	h = bbox.y1 - bbox.y0;
	if ((w | h) == 0)
		return;

	n = src->n;
	sp = src->samples + ((y - src->y) * src->w + (x - src->x)) * src->n;
	dp = dst->samples + ((y - dst->y) * dst->w + (x - dst->x)) * dst->n;

	while (h--)
	{
		base_paint_span(dp, sp, n, w, alpha);
		sp += src->w * n;
		dp += dst->w * n;
	}
}

void
base_paint_pixmap_with_mask(base_pixmap *dst, base_pixmap *src, base_pixmap *msk)
{
	unsigned char *sp, *dp, *mp;
	base_bbox bbox;
	int x, y, w, h, n;

	assert(dst->n == src->n);
	assert(msk->n == 1);

	bbox = base_pixmap_bbox_no_ctx(dst);
	bbox = base_intersect_bbox(bbox, base_pixmap_bbox_no_ctx(src));
	bbox = base_intersect_bbox(bbox, base_pixmap_bbox_no_ctx(msk));

	x = bbox.x0;
	y = bbox.y0;
	w = bbox.x1 - bbox.x0;
	h = bbox.y1 - bbox.y0;
	if ((w | h) == 0)
		return;

	n = src->n;
	sp = src->samples + ((y - src->y) * src->w + (x - src->x)) * src->n;
	mp = msk->samples + ((y - msk->y) * msk->w + (x - msk->x)) * msk->n;
	dp = dst->samples + ((y - dst->y) * dst->w + (x - dst->x)) * dst->n;

	while (h--)
	{
		base_paint_span_with_mask(dp, sp, mp, n, w);
		sp += src->w * n;
		dp += dst->w * n;
		mp += msk->w;
	}
}
