#include "pdf-internal.h"

void base_var_imp(void *var)
{
	var = var; 
}

void base_flush_warnings(base_context *ctx)
{
	if (ctx->warn->count > 1)
	{
		fprintf(stderr, "warning: ... repeated %d times ...\n", ctx->warn->count);
		LOGE("warning: ... repeated %d times ...\n", ctx->warn->count);
	}
	ctx->warn->message[0] = 0;
	ctx->warn->count = 0;
}

void base_warn(base_context *ctx, char *fmt, ...)
{
	va_list ap;
	char buf[sizeof ctx->warn->message];

	va_start(ap, fmt);
	vsnprintf(buf, sizeof buf, fmt, ap);
	va_end(ap);

	if (!strcmp(buf, ctx->warn->message))
	{
		ctx->warn->count++;
	}
	else
	{
		base_flush_warnings(ctx);
		fprintf(stderr, "warning: %s\n", buf);
		LOGE("warning: %s\n", buf);
		base_strlcpy(ctx->warn->message, buf, sizeof ctx->warn->message);
		ctx->warn->count = 1;
	}
}

static void throw(base_error_context *ex)
{
	if (ex->top >= 0) {
		base_longjmp(ex->stack[ex->top].buffer, 1);
	} else {
		fprintf(stderr, "uncaught exception: %s\n", ex->message);
		LOGE("uncaught exception: %s\n", ex->message);
		exit(EXIT_FAILURE);
	}
}

void base_push_try(base_error_context *ex)
{
	assert(ex);
	if (ex->top + 1 >= nelem(ex->stack))
	{
		fprintf(stderr, "exception stack overflow!\n");
		exit(EXIT_FAILURE);
	}
	ex->top++;
}

char *base_caught(base_context *ctx)
{
	assert(ctx);
	assert(ctx->error);
	return ctx->error->message;
}

void base_throw(base_context *ctx, char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	vsnprintf(ctx->error->message, sizeof ctx->error->message, fmt, args);
	va_end(args);

	base_flush_warnings(ctx);
	fprintf(stderr, "error: %s\n", ctx->error->message);
	LOGE("error: %s\n", ctx->error->message);

	throw(ctx->error);
}

void base_rethrow(base_context *ctx)
{
	throw(ctx->error);
}
