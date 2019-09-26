#include "pdf-internal.h"

typedef unsigned char byte;

static const char *base_blendmode_names[] =
{
	"Normal",
	"Multiply",
	"Screen",
	"Overlay",
	"Darken",
	"Lighten",
	"ColorDodge",
	"ColorBurn",
	"HardLight",
	"SoftLight",
	"Difference",
	"Exclusion",
	"Hue",
	"Saturation",
	"Color",
	"Luminosity",
};

int base_lookup_blendmode(char *name)
{
	int i;
	for (i = 0; i < nelem(base_blendmode_names); i++)
		if (!strcmp(name, base_blendmode_names[i]))
			return i;
	return base_BLEND_NORMAL;
}

char *base_blendmode_name(int blendmode)
{
	if (blendmode >= 0 && blendmode < nelem(base_blendmode_names))
		return (char*)base_blendmode_names[blendmode];
	return "Normal";
}

static inline int base_screen_byte(int b, int s)
{
	return b + s - base_mul255(b, s);
}

static inline int base_hard_light_byte(int b, int s)
{
	int s2 = s << 1;
	if (s <= 127)
		return base_mul255(b, s2);
	else
		return base_screen_byte(b, s2 - 255);
}

static inline int base_overlay_byte(int b, int s)
{
	return base_hard_light_byte(s, b); 
}

static inline int base_darken_byte(int b, int s)
{
	return MIN(b, s);
}

static inline int base_lighten_byte(int b, int s)
{
	return MAX(b, s);
}

static inline int base_color_dodge_byte(int b, int s)
{
	s = 255 - s;
	if (b == 0)
		return 0;
	else if (b >= s)
		return 255;
	else
		return (0x1fe * b + s) / (s << 1);
}

static inline int base_color_burn_byte(int b, int s)
{
	b = 255 - b;
	if (b == 0)
		return 255;
	else if (b >= s)
		return 0;
	else
		return 0xff - (0x1fe * b + s) / (s << 1);
}

static inline int base_soft_light_byte(int b, int s)
{
	
	if (s < 128) {
		return b - base_mul255(base_mul255((255 - (s<<1)), b), 255 - b);
	}
	else {
		int dbd;
		if (b < 64)
			dbd = base_mul255(base_mul255((b << 4) - 12, b) + 4, b);
		else
			dbd = (int)sqrtf(255.0f * b);
		return b + base_mul255(((s<<1) - 255), (dbd - b));
	}
}

static inline int base_difference_byte(int b, int s)
{
	return ABS(b - s);
}

static inline int base_exclusion_byte(int b, int s)
{
	return b + s - (base_mul255(b, s)<<1);
}

static void
base_luminosity_rgb(unsigned char *rd, unsigned char *gd, unsigned char *bd, int rb, int gb, int bb, int rs, int gs, int bs)
{
	int delta, scale;
	int r, g, b, y;

	
	delta = ((rs - rb) * 77 + (gs - gb) * 151 + (bs - bb) * 28 + 0x80) >> 8;
	r = rb + delta;
	g = gb + delta;
	b = bb + delta;

	if ((r | g | b) & 0x100)
	{
		y = (rs * 77 + gs * 151 + bs * 28 + 0x80) >> 8;
		if (delta > 0)
		{
			int max;
			max = MAX(r, MAX(g, b));
			scale = (max == y ? 0 : ((255 - y) << 16) / (max - y));
		}
		else
		{
			int min;
			min = MIN(r, MIN(g, b));
			scale = (y == min ? 0 : (y << 16) / (y - min));
		}
		r = y + (((r - y) * scale + 0x8000) >> 16);
		g = y + (((g - y) * scale + 0x8000) >> 16);
		b = y + (((b - y) * scale + 0x8000) >> 16);
	}

	*rd = CLAMP(r, 0, 255);
	*gd = CLAMP(g, 0, 255);
	*bd = CLAMP(b, 0, 255);
}

static void
base_saturation_rgb(unsigned char *rd, unsigned char *gd, unsigned char *bd, int rb, int gb, int bb, int rs, int gs, int bs)
{
	int minb, maxb;
	int mins, maxs;
	int y;
	int scale;
	int r, g, b;

	minb = MIN(rb, MIN(gb, bb));
	maxb = MAX(rb, MAX(gb, bb));
	if (minb == maxb)
	{
		
		gb = CLAMP(gb, 0, 255);
		*rd = gb;
		*gd = gb;
		*bd = gb;
		return;
	}

	mins = MIN(rs, MIN(gs, bs));
	maxs = MAX(rs, MAX(gs, bs));

	scale = ((maxs - mins) << 16) / (maxb - minb);
	y = (rb * 77 + gb * 151 + bb * 28 + 0x80) >> 8;
	r = y + ((((rb - y) * scale) + 0x8000) >> 16);
	g = y + ((((gb - y) * scale) + 0x8000) >> 16);
	b = y + ((((bb - y) * scale) + 0x8000) >> 16);

	if ((r | g | b) & 0x100)
	{
		int scalemin, scalemax;
		int min, max;

		min = MIN(r, MIN(g, b));
		max = MAX(r, MAX(g, b));

		if (min < 0)
			scalemin = (y << 16) / (y - min);
		else
			scalemin = 0x10000;

		if (max > 255)
			scalemax = ((255 - y) << 16) / (max - y);
		else
			scalemax = 0x10000;

		scale = MIN(scalemin, scalemax);
		r = y + (((r - y) * scale + 0x8000) >> 16);
		g = y + (((g - y) * scale + 0x8000) >> 16);
		b = y + (((b - y) * scale + 0x8000) >> 16);
	}

	*rd = CLAMP(r, 0, 255);
	*gd = CLAMP(g, 0, 255);
	*bd = CLAMP(b, 0, 255);
}

static void
base_color_rgb(unsigned char *rr, unsigned char *rg, unsigned char *rb, int br, int bg, int bb, int sr, int sg, int sb)
{
	base_luminosity_rgb(rr, rg, rb, sr, sg, sb, br, bg, bb);
}

static void
base_hue_rgb(unsigned char *rr, unsigned char *rg, unsigned char *rb, int br, int bg, int bb, int sr, int sg, int sb)
{
	unsigned char tr, tg, tb;
	base_luminosity_rgb(&tr, &tg, &tb, sr, sg, sb, br, bg, bb);
	base_saturation_rgb(rr, rg, rb, tr, tg, tb, br, bg, bb);
}

void
base_blend_pixel(unsigned char dp[3], unsigned char bp[3], unsigned char sp[3], int blendmode)
{
	int k;
	
	switch (blendmode)
	{
	case base_BLEND_HUE: base_hue_rgb(&dp[0], &dp[1], &dp[2], bp[0], bp[1], bp[2], sp[0], sp[1], sp[2]); return;
	case base_BLEND_SATURATION: base_saturation_rgb(&dp[0], &dp[1], &dp[2], bp[0], bp[1], bp[2], sp[0], sp[1], sp[2]); return;
	case base_BLEND_COLOR: base_color_rgb(&dp[0], &dp[1], &dp[2], bp[0], bp[1], bp[2], sp[0], sp[1], sp[2]); return;
	case base_BLEND_LUMINOSITY: base_luminosity_rgb(&dp[0], &dp[1], &dp[2], bp[0], bp[1], bp[2], sp[0], sp[1], sp[2]); return;
	}
	
	for (k = 0; k < 3; k++)
	{
		switch (blendmode)
		{
		default:
		case base_BLEND_NORMAL: dp[k] = sp[k]; break;
		case base_BLEND_MULTIPLY: dp[k] = base_mul255(bp[k], sp[k]); break;
		case base_BLEND_SCREEN: dp[k] = base_screen_byte(bp[k], sp[k]); break;
		case base_BLEND_OVERLAY: dp[k] = base_overlay_byte(bp[k], sp[k]); break;
		case base_BLEND_DARKEN: dp[k] = base_darken_byte(bp[k], sp[k]); break;
		case base_BLEND_LIGHTEN: dp[k] = base_lighten_byte(bp[k], sp[k]); break;
		case base_BLEND_COLOR_DODGE: dp[k] = base_color_dodge_byte(bp[k], sp[k]); break;
		case base_BLEND_COLOR_BURN: dp[k] = base_color_burn_byte(bp[k], sp[k]); break;
		case base_BLEND_HARD_LIGHT: dp[k] = base_hard_light_byte(bp[k], sp[k]); break;
		case base_BLEND_SOFT_LIGHT: dp[k] = base_soft_light_byte(bp[k], sp[k]); break;
		case base_BLEND_DIFFERENCE: dp[k] = base_difference_byte(bp[k], sp[k]); break;
		case base_BLEND_EXCLUSION: dp[k] = base_exclusion_byte(bp[k], sp[k]); break;
		}
	}
}

void
base_blend_separable(byte * restrict bp, byte * restrict sp, int n, int w, int blendmode)
{
	int k;
	int n1 = n - 1;
	while (w--)
	{
		int sa = sp[n1];
		int ba = bp[n1];
		int saba = base_mul255(sa, ba);

		
		int invsa = sa ? 255 * 256 / sa : 0;
		int invba = ba ? 255 * 256 / ba : 0;

		for (k = 0; k < n1; k++)
		{
			int sc = (sp[k] * invsa) >> 8;
			int bc = (bp[k] * invba) >> 8;
			int rc;

			switch (blendmode)
			{
			default:
			case base_BLEND_NORMAL: rc = sc; break;
			case base_BLEND_MULTIPLY: rc = base_mul255(bc, sc); break;
			case base_BLEND_SCREEN: rc = base_screen_byte(bc, sc); break;
			case base_BLEND_OVERLAY: rc = base_overlay_byte(bc, sc); break;
			case base_BLEND_DARKEN: rc = base_darken_byte(bc, sc); break;
			case base_BLEND_LIGHTEN: rc = base_lighten_byte(bc, sc); break;
			case base_BLEND_COLOR_DODGE: rc = base_color_dodge_byte(bc, sc); break;
			case base_BLEND_COLOR_BURN: rc = base_color_burn_byte(bc, sc); break;
			case base_BLEND_HARD_LIGHT: rc = base_hard_light_byte(bc, sc); break;
			case base_BLEND_SOFT_LIGHT: rc = base_soft_light_byte(bc, sc); break;
			case base_BLEND_DIFFERENCE: rc = base_difference_byte(bc, sc); break;
			case base_BLEND_EXCLUSION: rc = base_exclusion_byte(bc, sc); break;
			}

			bp[k] = base_mul255(255 - sa, bp[k]) + base_mul255(255 - ba, sp[k]) + base_mul255(saba, rc);
		}

		bp[k] = ba + sa - saba;

		sp += n;
		bp += n;
	}
}

void
base_blend_nonseparable(byte * restrict bp, byte * restrict sp, int w, int blendmode)
{
	while (w--)
	{
		unsigned char rr, rg, rb;

		int sa = sp[3];
		int ba = bp[3];
		int saba = base_mul255(sa, ba);

		
		int invsa = sa ? 255 * 256 / sa : 0;
		int invba = ba ? 255 * 256 / ba : 0;

		int sr = (sp[0] * invsa) >> 8;
		int sg = (sp[1] * invsa) >> 8;
		int sb = (sp[2] * invsa) >> 8;

		int br = (bp[0] * invba) >> 8;
		int bg = (bp[1] * invba) >> 8;
		int bb = (bp[2] * invba) >> 8;

		switch (blendmode)
		{
		default:
		case base_BLEND_HUE:
			base_hue_rgb(&rr, &rg, &rb, br, bg, bb, sr, sg, sb);
			break;
		case base_BLEND_SATURATION:
			base_saturation_rgb(&rr, &rg, &rb, br, bg, bb, sr, sg, sb);
			break;
		case base_BLEND_COLOR:
			base_color_rgb(&rr, &rg, &rb, br, bg, bb, sr, sg, sb);
			break;
		case base_BLEND_LUMINOSITY:
			base_luminosity_rgb(&rr, &rg, &rb, br, bg, bb, sr, sg, sb);
			break;
		}

		bp[0] = base_mul255(255 - sa, bp[0]) + base_mul255(255 - ba, sp[0]) + base_mul255(saba, rr);
		bp[1] = base_mul255(255 - sa, bp[1]) + base_mul255(255 - ba, sp[1]) + base_mul255(saba, rg);
		bp[2] = base_mul255(255 - sa, bp[2]) + base_mul255(255 - ba, sp[2]) + base_mul255(saba, rb);
		bp[3] = ba + sa - saba;

		sp += 4;
		bp += 4;
	}
}

static void
base_blend_separable_nonisolated(byte * restrict bp, byte * restrict sp, int n, int w, int blendmode, byte * restrict hp, int alpha)
{
	int k;
	int n1 = n - 1;

	if (alpha == 255 && blendmode == 0)
	{
		
		
		while (w--)
		{
			int ha = base_mul255(*hp++, alpha); 
			
			if (ha != 0)
			{
				for (k = 0; k < n; k++)
				{
					bp[k] = sp[k];
				}
			}

			sp += n;
			bp += n;
		}
		return;
	}
	while (w--)
	{
		int ha = *hp++;
		int haa = base_mul255(ha, alpha); 
		
		while (haa != 0) 
		{
			int sa, ba, bahaa, ra, invsa, invba, invha, invra;
			sa = sp[n1];
			if (sa == 0)
				break; 
			invsa = sa ? 255 * 256 / sa : 0;
			ba = bp[n1];
			if (ba == 0)
			{
				
				for (k = 0; k < n1; k++)
				{
					bp[k] = base_mul255((sp[k] * invsa) >> 8, haa);
				}
				bp[n1] = haa;
				break;
			}
			bahaa = base_mul255(ba, haa);

			
			invba = ba ? 255 * 256 / ba : 0;

			
			ra = bp[n1] = ba - bahaa + haa;
			if (ra == 0)
				break;
			
			invha = ha ? 255 * 256 / ha : 0;
			invra = ra ? 255 * 256 / ra : 0;

			
			sa = (haa*invra + 128)>>8;
			if (sa < 0) sa = 0;
			if (sa > 255) sa = 255;

			for (k = 0; k < n1; k++)
			{
				
				int sc = (sp[k] * invsa + 128) >> 8;
				int bc = (bp[k] * invba + 128) >> 8;
				int rc;

				
				sc = (((sc-bc) * invha + 128)>>8) + bc;
				if (sc < 0) sc = 0;
				if (sc > 255) sc = 255;

				switch (blendmode)
				{
				default:
				case base_BLEND_NORMAL: rc = sc; break;
				case base_BLEND_MULTIPLY: rc = base_mul255(bc, sc); break;
				case base_BLEND_SCREEN: rc = base_screen_byte(bc, sc); break;
				case base_BLEND_OVERLAY: rc = base_overlay_byte(bc, sc); break;
				case base_BLEND_DARKEN: rc = base_darken_byte(bc, sc); break;
				case base_BLEND_LIGHTEN: rc = base_lighten_byte(bc, sc); break;
				case base_BLEND_COLOR_DODGE: rc = base_color_dodge_byte(bc, sc); break;
				case base_BLEND_COLOR_BURN: rc = base_color_burn_byte(bc, sc); break;
				case base_BLEND_HARD_LIGHT: rc = base_hard_light_byte(bc, sc); break;
				case base_BLEND_SOFT_LIGHT: rc = base_soft_light_byte(bc, sc); break;
				case base_BLEND_DIFFERENCE: rc = base_difference_byte(bc, sc); break;
				case base_BLEND_EXCLUSION: rc = base_exclusion_byte(bc, sc); break;
				}
				
				rc = bc + base_mul255(sa, base_mul255(255 - ba, sc) + base_mul255(ba, rc) - bc);
				if (rc < 0) rc = 0;
				if (rc > 255) rc = 255;
				bp[k] = base_mul255(rc, ra);
			}
			break;
		}

		sp += n;
		bp += n;
	}
}

static void
base_blend_nonseparable_nonisolated(byte * restrict bp, byte * restrict sp, int w, int blendmode, byte * restrict hp, int alpha)
{
	while (w--)
	{
		int ha = *hp++;
		int haa = base_mul255(ha, alpha);
		if (haa != 0)
		{
			int sa = sp[3];
			int ba = bp[3];
			int baha = base_mul255(ba, haa);

			
			int ra = bp[3] = ba - baha + haa;
			if (ra != 0)
			{
				
				int invha = ha ? 255 * 256 / ha : 0;

				unsigned char rr, rg, rb;

				
				int invsa = sa ? 255 * 256 / sa : 0;
				int invba = ba ? 255 * 256 / ba : 0;

				int sr = (sp[0] * invsa) >> 8;
				int sg = (sp[1] * invsa) >> 8;
				int sb = (sp[2] * invsa) >> 8;

				int br = (bp[0] * invba) >> 8;
				int bg = (bp[1] * invba) >> 8;
				int bb = (bp[2] * invba) >> 8;

				
				sr = (((sr-br)*invha)>>8) + br;
				sg = (((sg-bg)*invha)>>8) + bg;
				sb = (((sb-bb)*invha)>>8) + bb;

				switch (blendmode)
				{
				default:
				case base_BLEND_HUE:
					base_hue_rgb(&rr, &rg, &rb, br, bg, bb, sr, sg, sb);
					break;
				case base_BLEND_SATURATION:
					base_saturation_rgb(&rr, &rg, &rb, br, bg, bb, sr, sg, sb);
					break;
				case base_BLEND_COLOR:
					base_color_rgb(&rr, &rg, &rb, br, bg, bb, sr, sg, sb);
					break;
				case base_BLEND_LUMINOSITY:
					base_luminosity_rgb(&rr, &rg, &rb, br, bg, bb, sr, sg, sb);
					break;
				}

				rr = base_mul255(255 - haa, bp[0]) + base_mul255(base_mul255(255 - ba, sr), haa) + base_mul255(baha, rr);
				rg = base_mul255(255 - haa, bp[1]) + base_mul255(base_mul255(255 - ba, sg), haa) + base_mul255(baha, rg);
				rb = base_mul255(255 - haa, bp[2]) + base_mul255(base_mul255(255 - ba, sb), haa) + base_mul255(baha, rb);
				bp[0] = base_mul255(ra, rr);
				bp[1] = base_mul255(ra, rg);
				bp[2] = base_mul255(ra, rb);
			}
		}

		sp += 4;
		bp += 4;
	}
}

void
base_blend_pixmap(base_pixmap *dst, base_pixmap *src, int alpha, int blendmode, int isolated, base_pixmap *shape)
{
	unsigned char *sp, *dp;
	base_bbox bbox;
	int x, y, w, h, n;

	
	if (isolated && alpha < 255)
	{
		sp = src->samples;
		n = src->w * src->h * src->n;
		while (n--)
		{
			*sp = base_mul255(*sp, alpha);
			sp++;
		}
	}

	bbox = base_pixmap_bbox_no_ctx(dst);
	bbox = base_intersect_bbox(bbox, base_pixmap_bbox_no_ctx(src));

	x = bbox.x0;
	y = bbox.y0;
	w = bbox.x1 - bbox.x0;
	h = bbox.y1 - bbox.y0;

	n = src->n;
	sp = src->samples + ((y - src->y) * src->w + (x - src->x)) * n;
	dp = dst->samples + ((y - dst->y) * dst->w + (x - dst->x)) * n;

	assert(src->n == dst->n);

	if (!isolated)
	{
		unsigned char *hp = shape->samples + (y - shape->y) * shape->w + (x - shape->x);

		while (h--)
		{
			if (n == 4 && blendmode >= base_BLEND_HUE)
				base_blend_nonseparable_nonisolated(dp, sp, w, blendmode, hp, alpha);
			else
				base_blend_separable_nonisolated(dp, sp, n, w, blendmode, hp, alpha);
			sp += src->w * n;
			dp += dst->w * n;
			hp += shape->w;
		}
	}
	else
	{
		while (h--)
		{
			if (n == 4 && blendmode >= base_BLEND_HUE)
				base_blend_nonseparable(dp, sp, w, blendmode);
			else
				base_blend_separable(dp, sp, n, w, blendmode);
			sp += src->w * n;
			dp += dst->w * n;
		}
	}
}
