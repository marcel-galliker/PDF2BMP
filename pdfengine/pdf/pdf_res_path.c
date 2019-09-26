#include <assert.h>
#include "pdf-internal.h"

base_path *
base_new_path(base_context *ctx)
{
	base_path *path;

	path = base_malloc_struct(ctx, base_path);
	path->len = 0;
	path->cap = 0;
	path->items = NULL;
	path->last = -1;

	return path;
}

base_path *
base_clone_path(base_context *ctx, base_path *old)
{
	base_path *path;

	assert(old);
	path = base_malloc_struct(ctx, base_path);
	base_try(ctx)
	{
		path->len = old->len;
		path->cap = old->len;
		path->items = base_malloc_array(ctx, path->cap, sizeof(base_path_item));
		memcpy(path->items, old->items, sizeof(base_path_item) * path->len);
	}
	base_catch(ctx)
	{
		base_free(ctx, path);
		base_rethrow(ctx);
	}

	return path;
}

void
base_free_path(base_context *ctx, base_path *path)
{
	if (path == NULL)
		return;
	base_free(ctx, path->items);
	base_free(ctx, path);
}

static void
grow_path(base_context *ctx, base_path *path, int n)
{
	int newcap = path->cap;
	if (path->len + n <= path->cap)
	{
		path->last = path->len;
		return;
	}
	while (path->len + n > newcap)
		newcap = newcap + 36;
	path->items = base_resize_array(ctx, path->items, newcap, sizeof(base_path_item));
	path->cap = newcap;
	path->last = path->len;
}

void
base_moveto(base_context *ctx, base_path *path, float x, float y)
{
	if (path->last >= 0 && path->items[path->last].k == base_MOVETO)
	{
		
		path->len = path->last;
	}
	grow_path(ctx, path, 3);
	path->items[path->len++].k = base_MOVETO;
	path->items[path->len++].v = x;
	path->items[path->len++].v = y;
}

void
base_lineto(base_context *ctx, base_path *path, float x, float y)
{
	float x0, y0;

	if (path->last < 0)
	{
		base_warn(ctx, "lineto with no current point");
		return;
	}
	if (path->items[path->last].k == base_CLOSE_PATH)
	{
		x0 = path->items[path->last-2].v;
		y0 = path->items[path->last-1].v;
	}
	else
	{
		x0 = path->items[path->len-2].v;
		y0 = path->items[path->len-1].v;
	}
	
	if (path->items[path->last].k != base_MOVETO && x0 == x && y0 == y)
		return;
	grow_path(ctx, path, 3);
	path->items[path->len++].k = base_LINETO;
	path->items[path->len++].v = x;
	path->items[path->len++].v = y;
}

void
base_curveto(base_context *ctx, base_path *path,
	float x1, float y1,
	float x2, float y2,
	float x3, float y3)
{
	float x0, y0;

	if (path->last < 0)
	{
		base_warn(ctx, "curveto with no current point");
		return;
	}
	if (path->items[path->last].k == base_CLOSE_PATH)
	{
		x0 = path->items[path->last-2].v;
		y0 = path->items[path->last-1].v;
	}
	else
	{
		x0 = path->items[path->len-2].v;
		y0 = path->items[path->len-1].v;
	}

	
	if (x0 == x1 && y0 == y1)
	{
		if (x2 == x3 && y2 == y3)
		{
			
			if (x1 == x2 && y1 == y2 && path->items[path->last].k != base_MOVETO)
				return;
			
			base_lineto(ctx, path, x3, y3);
			return;
		}
		if (x1 == x2 && y1 == y2)
		{
			
			base_lineto(ctx, path, x3, y3);
			return;
		}
	}
	else if (x1 == x2 && y1 == y2 && x2 == x3 && y2 == y3)
	{
		
		base_lineto(ctx, path, x3, y3);
		return;
	}

	grow_path(ctx, path, 7);
	path->items[path->len++].k = base_CURVETO;
	path->items[path->len++].v = x1;
	path->items[path->len++].v = y1;
	path->items[path->len++].v = x2;
	path->items[path->len++].v = y2;
	path->items[path->len++].v = x3;
	path->items[path->len++].v = y3;
}

void
base_curvetov(base_context *ctx, base_path *path, float x2, float y2, float x3, float y3)
{
	float x1, y1;
	if (path->last < 0)
	{
		base_warn(ctx, "curvetov with no current point");
		return;
	}
	if (path->items[path->last].k == base_CLOSE_PATH)
	{
		x1 = path->items[path->last-2].v;
		y1 = path->items[path->last-1].v;
	}
	else
	{
		x1 = path->items[path->len-2].v;
		y1 = path->items[path->len-1].v;
	}
	base_curveto(ctx, path, x1, y1, x2, y2, x3, y3);
}

void
base_curvetoy(base_context *ctx, base_path *path, float x1, float y1, float x3, float y3)
{
	base_curveto(ctx, path, x1, y1, x3, y3, x3, y3);
}

void
base_closepath(base_context *ctx, base_path *path)
{
	if (path->last < 0)
	{
		base_warn(ctx, "closepath with no current point");
		return;
	}
	
	if (path->items[path->last].k == base_CLOSE_PATH)
		return;
	grow_path(ctx, path, 1);
	path->items[path->len++].k = base_CLOSE_PATH;
}

static inline base_rect bound_expand(base_rect r, base_point p)
{
	if (p.x < r.x0) r.x0 = p.x;
	if (p.y < r.y0) r.y0 = p.y;
	if (p.x > r.x1) r.x1 = p.x;
	if (p.y > r.y1) r.y1 = p.y;
	return r;
}

base_rect
base_bound_path(base_context *ctx, base_path *path, base_stroke_state *stroke, base_matrix ctm)
{
	base_point p;
	base_rect r;
	int i = 0;

	
	if (path->len == 0)
		return base_empty_rect;
	
	if (path->len == 3)
		return base_empty_rect;

	p.x = path->items[1].v;
	p.y = path->items[2].v;
	p = base_transform_point(ctm, p);
	r.x0 = r.x1 = p.x;
	r.y0 = r.y1 = p.y;

	while (i < path->len)
	{
		switch (path->items[i++].k)
		{
		case base_CURVETO:
			p.x = path->items[i++].v;
			p.y = path->items[i++].v;
			r = bound_expand(r, base_transform_point(ctm, p));
			p.x = path->items[i++].v;
			p.y = path->items[i++].v;
			r = bound_expand(r, base_transform_point(ctm, p));
			p.x = path->items[i++].v;
			p.y = path->items[i++].v;
			r = bound_expand(r, base_transform_point(ctm, p));
			break;
		case base_MOVETO:
			if (i + 2 == path->len)
			{
				
				i += 2;
				break;
			}
			
		case base_LINETO:
			p.x = path->items[i++].v;
			p.y = path->items[i++].v;
			r = bound_expand(r, base_transform_point(ctm, p));
			break;
		case base_CLOSE_PATH:
			break;
		}
	}

	if (stroke)
	{
		float expand = stroke->linewidth;
		if (expand == 0)
			expand = 1.0f;
		expand *= base_matrix_max_expansion(ctm);
		if ((stroke->linejoin == base_LINEJOIN_MITER || stroke->linejoin == base_LINEJOIN_MITER_XPS) && stroke->miterlimit > 1)
			expand *= stroke->miterlimit;
		r.x0 -= expand;
		r.y0 -= expand;
		r.x1 += expand;
		r.y1 += expand;
	}

	return r;
}

void
base_transform_path(base_context *ctx, base_path *path, base_matrix ctm)
{
	base_point p;
	int k, i = 0;

	while (i < path->len)
	{
		switch (path->items[i++].k)
		{
		case base_CURVETO:
			for (k = 0; k < 3; k++)
			{
				p.x = path->items[i].v;
				p.y = path->items[i+1].v;
				p = base_transform_point(ctm, p);
				path->items[i].v = p.x;
				path->items[i+1].v = p.y;
				i += 2;
			}
			break;
		case base_MOVETO:
		case base_LINETO:
			p.x = path->items[i].v;
			p.y = path->items[i+1].v;
			p = base_transform_point(ctm, p);
			path->items[i].v = p.x;
			path->items[i+1].v = p.y;
			i += 2;
			break;
		case base_CLOSE_PATH:
			break;
		}
	}
}

void
base_print_path(base_context *ctx, FILE *out, base_path *path, int indent)
{
	float x, y;
	int i = 0;
	int n;
	while (i < path->len)
	{
		for (n = 0; n < indent; n++)
			fputc(' ', out);
		switch (path->items[i++].k)
		{
		case base_MOVETO:
			x = path->items[i++].v;
			y = path->items[i++].v;
			fprintf(out, "%g %g m\n", x, y);
			break;
		case base_LINETO:
			x = path->items[i++].v;
			y = path->items[i++].v;
			fprintf(out, "%g %g l\n", x, y);
			break;
		case base_CURVETO:
			x = path->items[i++].v;
			y = path->items[i++].v;
			fprintf(out, "%g %g ", x, y);
			x = path->items[i++].v;
			y = path->items[i++].v;
			fprintf(out, "%g %g ", x, y);
			x = path->items[i++].v;
			y = path->items[i++].v;
			fprintf(out, "%g %g c\n", x, y);
			break;
		case base_CLOSE_PATH:
			fprintf(out, "h\n");
			break;
		}
	}
}

base_stroke_state *
base_keep_stroke_state(base_context *ctx, base_stroke_state *stroke)
{
	base_lock(ctx, base_LOCK_ALLOC);

	if (!stroke)
		return NULL;

	if (stroke->refs > 0)
		stroke->refs++;
	base_unlock(ctx, base_LOCK_ALLOC);
	return stroke;
}

void
base_drop_stroke_state(base_context *ctx, base_stroke_state *stroke)
{
	int drop;

	if (!stroke)
		return;

	base_lock(ctx, base_LOCK_ALLOC);
	drop = (stroke->refs > 0 ? --stroke->refs == 0 : 0);
	base_unlock(ctx, base_LOCK_ALLOC);
	if (drop)
		base_free(ctx, stroke);
}

base_stroke_state *
base_new_stroke_state_with_len(base_context *ctx, int len)
{
	base_stroke_state *state;

	len -= nelem(state->dash_list);
	if (len < 0)
		len = 0;

	state = base_malloc(ctx, sizeof(*state) + sizeof(state->dash_list[0]) * len);
	state->refs = 1;
	state->start_cap = base_LINECAP_BUTT;
	state->dash_cap = base_LINECAP_BUTT;
	state->end_cap = base_LINECAP_BUTT;
	state->linejoin = base_LINEJOIN_MITER;
	state->linewidth = 1;
	state->miterlimit = 10;
	state->dash_phase = 0;
	state->dash_len = 0;
	memset(state->dash_list, 0, sizeof(state->dash_list[0]) * (len + nelem(state->dash_list)));

	return state;
}

base_stroke_state *
base_new_stroke_state(base_context *ctx)
{
	return base_new_stroke_state_with_len(ctx, 0);
}

base_stroke_state *
base_unshare_stroke_state_with_len(base_context *ctx, base_stroke_state *shared, int len)
{
	int single, unsize, shsize, shlen, drop;
	base_stroke_state *unshared;

	base_lock(ctx, base_LOCK_ALLOC);
	single = (shared->refs == 1);
	base_unlock(ctx, base_LOCK_ALLOC);

	shlen = shared->dash_len - nelem(shared->dash_list);
	if (shlen < 0)
		shlen = 0;
	shsize = sizeof(*shared) + sizeof(shared->dash_list[0]) * shlen;
	len -= nelem(shared->dash_list);
	if (len < 0)
		len = 0;
	if (single && shlen >= len)
		return shared;
	unsize = sizeof(*unshared) + sizeof(unshared->dash_list[0]) * len;
	unshared = base_malloc(ctx, unsize);
	memcpy(unshared, shared, (shsize > unsize ? unsize : shsize));
	unshared->refs = 1;
	base_lock(ctx, base_LOCK_ALLOC);
	drop = (shared->refs > 0 ? --shared->refs == 0 : 0);
	base_unlock(ctx, base_LOCK_ALLOC);
	if (drop)
		base_free(ctx, shared);
	return unshared;
}

base_stroke_state *
base_unshare_stroke_state(base_context *ctx, base_stroke_state *shared)
{
	return base_unshare_stroke_state_with_len(ctx, shared, shared->dash_len);
}
