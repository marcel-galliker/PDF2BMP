#include "pdf-internal.h"

#include <zlib.h>

typedef struct base_flate_s base_flate;

struct base_flate_s
{
	base_stream *chain;
	z_stream z;
};

static void *zalloc(void *opaque, unsigned int items, unsigned int size)
{
	return base_malloc_array_no_throw(opaque, items, size);
}

static void zfree(void *opaque, void *ptr)
{
	base_free(opaque, ptr);
}

static int
read_flated(base_stream *stm, unsigned char *outbuf, int outlen)
{
	base_flate *state = stm->state;
	base_stream *chain = state->chain;
	z_streamp zp = &state->z;
	int code;

	zp->next_out = outbuf;
	zp->avail_out = outlen;

	while (zp->avail_out > 0)
	{
		if (chain->rp == chain->wp)
			base_fill_buffer(chain);

		zp->next_in = chain->rp;
		zp->avail_in = chain->wp - chain->rp;

		code = inflate(zp, Z_SYNC_FLUSH);

		chain->rp = chain->wp - zp->avail_in;

		if (code == Z_STREAM_END)
		{
			return outlen - zp->avail_out;
		}
		else if (code == Z_BUF_ERROR)
		{
			base_warn(stm->ctx, "premature end of data in flate filter");
			return outlen - zp->avail_out;
		}
		else if (code == Z_DATA_ERROR && zp->avail_in == 0)
		{
			base_warn(stm->ctx, "ignoring zlib error: %s", zp->msg);
			return outlen - zp->avail_out;
		}
		else if (code != Z_OK)
		{
			base_throw(stm->ctx, "zlib error: %s", zp->msg);
		}
	}

	return outlen - zp->avail_out;
}

static void
close_flated(base_context *ctx, void *state_)
{
	base_flate *state = (base_flate *)state_;
	int code;

	code = inflateEnd(&state->z);
	if (code != Z_OK)
		base_warn(ctx, "zlib error: inflateEnd: %s", state->z.msg);

	base_close(state->chain);
	base_free(ctx, state);
}

base_stream *
base_open_flated(base_stream *chain)
{
	base_flate *state = NULL;
	int code = Z_OK;
	base_context *ctx = chain->ctx;

	base_var(code);
	base_var(state);

	base_try(ctx)
	{
		state = base_malloc_struct(ctx, base_flate);
		state->chain = chain;

		state->z.zalloc = zalloc;
		state->z.zfree = zfree;
		state->z.opaque = ctx;
		state->z.next_in = NULL;
		state->z.avail_in = 0;

		code = inflateInit(&state->z);
		if (code != Z_OK)
			base_throw(ctx, "zlib error: inflateInit: %s", state->z.msg);
	}
	base_catch(ctx)
	{
		if (state && code == Z_OK)
			inflateEnd(&state->z);
		base_free(ctx, state);
		base_close(chain);
		base_rethrow(ctx);
	}
	return base_new_stream(ctx, state, read_flated, close_flated);
}
