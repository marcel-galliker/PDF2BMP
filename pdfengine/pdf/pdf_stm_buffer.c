#include "pdf-internal.h"

base_buffer *
base_new_buffer(base_context *ctx, int size)
{
	base_buffer *b;

	size = size > 1 ? size : 16;

	b = base_malloc_struct(ctx, base_buffer);
	b->refs = 1;
	base_try(ctx)
	{
		b->data = base_malloc(ctx, size);
	}
	base_catch(ctx)
	{
		base_free(ctx, b);
		base_rethrow(ctx);
	}
	b->cap = size;
	b->len = 0;

	return b;
}

base_buffer *
base_keep_buffer(base_context *ctx, base_buffer *buf)
{
	if (buf)
	{
		if (buf->refs == 1 && buf->cap > buf->len+1)
			base_resize_buffer(ctx, buf, buf->len);
		buf->refs ++;
	}

	return buf;
}

void
base_drop_buffer(base_context *ctx, base_buffer *buf)
{
	if (!buf)
		return;
	if (--buf->refs == 0)
	{
		base_free(ctx, buf->data);
		base_free(ctx, buf);
	}
}

void
base_resize_buffer(base_context *ctx, base_buffer *buf, int size)
{
	buf->data = base_resize_array(ctx, buf->data, size, 1);
	buf->cap = size;
	if (buf->len > buf->cap)
		buf->len = buf->cap;
}

void
base_grow_buffer(base_context *ctx, base_buffer *buf)
{
	base_resize_buffer(ctx, buf, (buf->cap * 3) / 2);
}

void
base_trim_buffer(base_context *ctx, base_buffer *buf)
{
	if (buf->cap > buf->len+1)
		base_resize_buffer(ctx, buf, buf->len);
}

int
base_buffer_storage(base_context *ctx, base_buffer *buf, unsigned char **datap)
{
	if (datap)
		*datap = (buf ? buf->data : NULL);
	return (buf ? buf->len : 0);
}
