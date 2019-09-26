#include "pdf-internal.h"

/***************************************************************************************/
/* function name:	base_free_context
/* description:		free the context variable, which is used in most functions of Pdfengine
/* param 1:			context to free
/* return:			none
/* creator & modifier : 
/* create:			2012.09.10 ~
/* working:			2012.09.10 ~
/***************************************************************************************/

void
base_free_context(base_context *ctx)
{
	if (!ctx)
		return;

	
	base_drop_glyph_cache_context(ctx);
	base_drop_store_context(ctx);
	base_free_aa_context(ctx);
	base_drop_font_context(ctx);

	if (ctx->warn)
	{
		base_flush_warnings(ctx);
		base_free(ctx, ctx->warn);
	}

	if (ctx->error)
	{
		assert(ctx->error->top == -1);
		base_free(ctx, ctx->error);
	}

	
	ctx->alloc->free(ctx->alloc->user, ctx);
}

static base_context *
new_context_phase1(base_alloc_context *alloc, base_locks_context *locks)
{
	base_context *ctx;

	ctx = alloc->malloc(alloc->user, sizeof(base_context));
	if (!ctx)
		return NULL;
	memset(ctx, 0, sizeof *ctx);
	ctx->alloc = alloc;
	ctx->locks = locks;

	ctx->glyph_cache = NULL;

	ctx->error = base_malloc_no_throw(ctx, sizeof(base_error_context));
	if (!ctx->error)
		goto cleanup;
	ctx->error->top = -1;
	ctx->error->message[0] = 0;

	ctx->warn = base_malloc_no_throw(ctx, sizeof(base_warn_context));
	if (!ctx->warn)
		goto cleanup;
	ctx->warn->message[0] = 0;
	ctx->warn->count = 0;

	
	base_try(ctx)
	{
		base_new_aa_context(ctx);
	}
	base_catch(ctx)
	{
		goto cleanup;
	}

	return ctx;

cleanup:
	fprintf(stderr, "cannot create context (phase 1)\n");
	base_free_context(ctx);
	return NULL;
}

/***************************************************************************************/
/* function name:	base_new_context
/* description:		create the context variable, which is used in most functions of Pdfengine
/* param 1:			pointer to alloc context
/* param 2:			pointer to lock context
/* param 3:			maximum store
/* return:			newly created context
/* creator & modifier : 
/* create:			2012.09.10 ~
/* working:			2012.09.10 ~
/***************************************************************************************/

base_context *
base_new_context(base_alloc_context *alloc, base_locks_context *locks, unsigned int max_store)
{
	base_context *ctx;

	if (!alloc)
		alloc = &base_alloc_default;

	if (!locks)
		locks = &base_locks_default;

	ctx = new_context_phase1(alloc, locks);

	
	base_try(ctx)
	{
		base_new_store_context(ctx, max_store);
		base_new_glyph_cache_context(ctx);
		base_new_font_context(ctx);
	}
	base_catch(ctx)
	{
		fprintf(stderr, "cannot create context (phase 2)\n");
		base_free_context(ctx);
		return NULL;
	}
	return ctx;
}

base_context *
base_clone_context(base_context *ctx)
{
	
	if (ctx == NULL || ctx->locks == &base_locks_default)
		return NULL;
	return base_clone_context_internal(ctx);
}

base_context *
base_clone_context_internal(base_context *ctx)
{
	base_context *new_ctx;

	if (ctx == NULL || ctx->alloc == NULL)
		return NULL;
	new_ctx = new_context_phase1(ctx->alloc, ctx->locks);
	
	base_copy_aa_context(new_ctx, ctx);
	
	new_ctx->store = ctx->store;
	new_ctx->store = base_keep_store_context(new_ctx);
	new_ctx->glyph_cache = ctx->glyph_cache;
	new_ctx->glyph_cache = base_keep_glyph_cache(new_ctx);
	new_ctx->font = ctx->font;
	new_ctx->font = base_keep_font_context(new_ctx);
	return new_ctx;
}
