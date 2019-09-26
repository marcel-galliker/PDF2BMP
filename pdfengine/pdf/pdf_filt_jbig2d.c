#include "pdf-internal.h"

#ifdef _WIN32 

typedef signed char int8_t;
typedef short int int16_t;
typedef int int32_t;
typedef __int64 int64_t;

typedef unsigned char uint8_t;
typedef unsigned short int uint16_t;
typedef unsigned int uint32_t;

#else
#include <inttypes.h>
#endif

#include <jbig2.h>

typedef struct base_jbig2d_s base_jbig2d;

struct base_jbig2d_s
{
	base_stream *chain;
	Jbig2Ctx *ctx;
	Jbig2GlobalCtx *gctx;
	Jbig2Image *page;
	int idx;
};

static void
close_jbig2d(base_context *ctx, void *state_)
{
	base_jbig2d *state = (base_jbig2d *)state_;
	if (state->page)
		jbig2_release_page(state->ctx, state->page);
	if (state->gctx)
		jbig2_global_ctx_free(state->gctx);
	jbig2_ctx_free(state->ctx);
	base_close(state->chain);
	base_free(ctx, state);
}

static int
read_jbig2d(base_stream *stm, unsigned char *buf, int len)
{
	base_jbig2d *state = stm->state;
	unsigned char tmp[4096];
	unsigned char *p = buf;
	unsigned char *ep = buf + len;
	unsigned char *s;
	int x, w, n;

	if (!state->page)
	{
		while (1)
		{
			n = base_read(state->chain, tmp, sizeof tmp);
			if (n == 0)
				break;
			jbig2_data_in(state->ctx, tmp, n);
		}

		jbig2_complete_page(state->ctx);

		state->page = jbig2_page_out(state->ctx);
		if (!state->page)
			base_throw(stm->ctx, "jbig2_page_out failed");
	}

	s = state->page->data;
	w = state->page->height * state->page->stride;
	x = state->idx;
	while (p < ep && x < w)
		*p++ = s[x++] ^ 0xff;
	state->idx = x;

	return p - buf;
}

base_stream *
base_open_jbig2d(base_stream *chain, base_buffer *globals)
{
	base_jbig2d *state = NULL;
	base_context *ctx = chain->ctx;

	base_var(state);

	base_try(ctx)
	{
		state = base_malloc_struct(chain->ctx, base_jbig2d);
		state->ctx = NULL;
		state->gctx = NULL;
		state->chain = chain;
		state->ctx = jbig2_ctx_new(NULL, JBIG2_OPTIONS_EMBEDDED, NULL, NULL, NULL);
		state->page = NULL;
		state->idx = 0;

		if (globals)
		{
			jbig2_data_in(state->ctx, globals->data, globals->len);
			state->gctx = jbig2_make_global_ctx(state->ctx);
			state->ctx = jbig2_ctx_new(NULL, JBIG2_OPTIONS_EMBEDDED, state->gctx, NULL, NULL);
		}
	}
	base_catch(ctx)
	{
		if (state)
		{
			if (state->gctx)
				jbig2_global_ctx_free(state->gctx);
			if (state->ctx)
				jbig2_ctx_free(state->ctx);
		}
		base_drop_buffer(ctx, globals);
		base_free(ctx, state);
		base_close(chain);
		base_rethrow(ctx);
	}
	base_drop_buffer(ctx, globals);

	return base_new_stream(ctx, state, read_jbig2d, close_jbig2d);
}
