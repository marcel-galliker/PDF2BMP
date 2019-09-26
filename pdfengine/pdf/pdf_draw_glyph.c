#include "pdf-internal.h"

#define MAX_FONT_SIZE 1000
#define MAX_GLYPH_SIZE 256
#define MAX_CACHE_SIZE (1024*1024)

typedef struct base_glyph_key_s base_glyph_key;

struct base_glyph_cache_s
{
	int refs;
	base_hash_table *hash;
	int total;
};

struct base_glyph_key_s
{
	base_font *font;
	int a, b;
	int c, d;
	unsigned short gid;
	unsigned char e, f;
	int aa;
};

void
base_new_glyph_cache_context(base_context *ctx)
{
	base_glyph_cache *cache;

	cache = base_malloc_struct(ctx, base_glyph_cache);
	base_try(ctx)
	{
		cache->hash = base_new_hash_table(ctx, 509, sizeof(base_glyph_key), base_LOCK_GLYPHCACHE);
	}
	base_catch(ctx)
	{
		base_free(ctx, cache);
		base_rethrow(ctx);
	}
	cache->total = 0;
	cache->refs = 1;

	ctx->glyph_cache = cache;
}

static void
base_evict_glyph_cache(base_context *ctx)
{
	base_glyph_cache *cache = ctx->glyph_cache;
	base_glyph_key *key;
	base_pixmap *pixmap;
	int i;

	for (i = 0; i < base_hash_len(ctx, cache->hash); i++)
	{
		key = base_hash_get_key(ctx, cache->hash, i);
		if (key->font)
			base_drop_font(ctx, key->font);
		pixmap = base_hash_get_val(ctx, cache->hash, i);
		if (pixmap)
			base_drop_pixmap(ctx, pixmap);
	}

	cache->total = 0;

	base_empty_hash(ctx, cache->hash);
}

void
base_drop_glyph_cache_context(base_context *ctx)
{
	if (!ctx->glyph_cache)
		return;

	base_lock(ctx, base_LOCK_GLYPHCACHE);
	ctx->glyph_cache->refs--;
	if (ctx->glyph_cache->refs == 0)
	{
		base_evict_glyph_cache(ctx);
		base_free_hash(ctx, ctx->glyph_cache->hash);
		base_free(ctx, ctx->glyph_cache);
		ctx->glyph_cache = NULL;
	}
	base_unlock(ctx, base_LOCK_GLYPHCACHE);
}

base_glyph_cache *
base_keep_glyph_cache(base_context *ctx)
{
	base_lock(ctx, base_LOCK_GLYPHCACHE);
	ctx->glyph_cache->refs++;
	base_unlock(ctx, base_LOCK_GLYPHCACHE);
	return ctx->glyph_cache;
}

base_pixmap *
base_render_stroked_glyph(base_context *ctx, base_font *font, int gid, base_matrix trm, base_matrix ctm, base_stroke_state *stroke)
{
	if (font->ft_face)
		return base_render_ft_stroked_glyph(ctx, font, gid, trm, ctm, stroke);
	return base_render_glyph(ctx, font, gid, trm, NULL);
}

base_pixmap *
base_render_glyph(base_context *ctx, base_font *font, int gid, base_matrix ctm, base_colorspace *model)
{
	base_glyph_cache *cache;
	base_glyph_key key;
	base_pixmap *val;
	float size = base_matrix_expansion(ctm);

	cache = ctx->glyph_cache;

	if (size > MAX_FONT_SIZE)
	{
		
		base_warn(ctx, "font size too large (%g), not rendering glyph", size);
		return NULL;
	}

	memset(&key, 0, sizeof key);
	key.font = font;
	key.gid = gid;
	key.a = ctm.a * 65536;
	key.b = ctm.b * 65536;
	key.c = ctm.c * 65536;
	key.d = ctm.d * 65536;
	key.e = (ctm.e - floorf(ctm.e)) * 256;
	key.f = (ctm.f - floorf(ctm.f)) * 256;
	key.aa = base_aa_level(ctx);

	base_lock(ctx, base_LOCK_GLYPHCACHE);
	val = base_hash_find(ctx, cache->hash, &key);
	if (val)
	{
		base_keep_pixmap(ctx, val);
		base_unlock(ctx, base_LOCK_GLYPHCACHE);
		return val;
	}

	ctm.e = floorf(ctm.e) + key.e / 256.0f;
	ctm.f = floorf(ctm.f) + key.f / 256.0f;

	base_try(ctx)
	{
		if (font->ft_face)
		{
			val = base_render_ft_glyph(ctx, font, gid, ctm, key.aa);
		}
		else if (font->t3procs)
		{
			
			base_unlock(ctx, base_LOCK_GLYPHCACHE);
			val = base_render_t3_glyph(ctx, font, gid, ctm, model);
			base_lock(ctx, base_LOCK_GLYPHCACHE);
		}
		else
		{
			base_warn(ctx, "assert: uninitialized font structure");
			val = NULL;
		}
	}
	base_catch(ctx)
	{
		base_unlock(ctx, base_LOCK_GLYPHCACHE);
		base_rethrow(ctx);
	}

	if (val)
	{
		if (val->w < MAX_GLYPH_SIZE && val->h < MAX_GLYPH_SIZE)
		{
			if (cache->total + val->w * val->h > MAX_CACHE_SIZE)
				base_evict_glyph_cache(ctx);
			base_try(ctx)
			{
				base_pixmap *pix = base_hash_insert(ctx, cache->hash, &key, val);
				if (pix)
				{
					base_drop_pixmap(ctx, val);
					val = pix;
				}
				else
					base_keep_font(ctx, key.font);
				val = base_keep_pixmap(ctx, val);
			}
			base_catch(ctx)
			{
				base_warn(ctx, "Failed to encache glyph - continuing");
			}
			cache->total += val->w * val->h;
		}
	}

	base_unlock(ctx, base_LOCK_GLYPHCACHE);
	return val;
}
