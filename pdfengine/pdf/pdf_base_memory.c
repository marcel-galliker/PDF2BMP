#include "pdf-internal.h"

static void *
do_scavenging_malloc(base_context *ctx, unsigned int size)
{
	void *p;
	int phase = 0;

	base_lock(ctx, base_LOCK_ALLOC);
	do {
		p = ctx->alloc->malloc(ctx->alloc->user, size);
		if (p != NULL)
		{
			base_unlock(ctx, base_LOCK_ALLOC);
			return p;
		}
	} while (base_store_scavenge(ctx, size, &phase));
	base_unlock(ctx, base_LOCK_ALLOC);

	return NULL;
}

static void *
do_scavenging_realloc(base_context *ctx, void *p, unsigned int size)
{
	void *q;
	int phase = 0;

	base_lock(ctx, base_LOCK_ALLOC);
	do {
		q = ctx->alloc->realloc(ctx->alloc->user, p, size);
		if (q != NULL)
		{
			base_unlock(ctx, base_LOCK_ALLOC);
			return q;
		}
	} while (base_store_scavenge(ctx, size, &phase));
	base_unlock(ctx, base_LOCK_ALLOC);

	return NULL;
}

void *
base_malloc(base_context *ctx, unsigned int size)
{
	void *p;

	if (size == 0)
		return NULL;

	p = do_scavenging_malloc(ctx, size);
	if (!p)
		base_throw(ctx, "malloc of %d bytes failed", size);
	return p;
}

void *
base_malloc_no_throw(base_context *ctx, unsigned int size)
{
	return do_scavenging_malloc(ctx, size);
}

void *
base_malloc_array(base_context *ctx, unsigned int count, unsigned int size)
{
	void *p;

	if (count == 0 || size == 0)
		return 0;

	if (count > UINT_MAX / size)
		base_throw(ctx, "malloc of array (%d x %d bytes) failed (integer overflow)", count, size);

	p = do_scavenging_malloc(ctx, count * size);
	if (!p)
		base_throw(ctx, "malloc of array (%d x %d bytes) failed", count, size);
	return p;
}

void *
base_malloc_array_no_throw(base_context *ctx, unsigned int count, unsigned int size)
{
	if (count == 0 || size == 0)
		return 0;

	if (count > UINT_MAX / size)
	{
		fprintf(stderr, "error: malloc of array (%d x %d bytes) failed (integer overflow)", count, size);
		return NULL;
	}

	return do_scavenging_malloc(ctx, count * size);
}

void *
base_calloc(base_context *ctx, unsigned int count, unsigned int size)
{
	void *p;

	if (count == 0 || size == 0)
		return 0;

	if (count > UINT_MAX / size)
	{
		base_throw(ctx, "calloc (%d x %d bytes) failed (integer overflow)", count, size);
	}

	p = do_scavenging_malloc(ctx, count * size);
	if (!p)
	{
		base_throw(ctx, "calloc (%d x %d bytes) failed", count, size);
	}
	memset(p, 0, count*size);
	return p;
}

void *
base_calloc_no_throw(base_context *ctx, unsigned int count, unsigned int size)
{
	void *p;

	if (count == 0 || size == 0)
		return 0;

	if (count > UINT_MAX / size)
	{
		fprintf(stderr, "error: calloc (%d x %d bytes) failed (integer overflow)\n", count, size);
		return NULL;
	}

	p = do_scavenging_malloc(ctx, count * size);
	if (p)
	{
		memset(p, 0, count*size);
	}
	return p;
}

void *
base_resize_array(base_context *ctx, void *p, unsigned int count, unsigned int size)
{
	void *np;

	if (count == 0 || size == 0)
	{
		base_free(ctx, p);
		return 0;
	}

	if (count > UINT_MAX / size)
		base_throw(ctx, "resize array (%d x %d bytes) failed (integer overflow)", count, size);

	np = do_scavenging_realloc(ctx, p, count * size);
	if (!np)
		base_throw(ctx, "resize array (%d x %d bytes) failed", count, size);
	return np;
}

void *
base_resize_array_no_throw(base_context *ctx, void *p, unsigned int count, unsigned int size)
{
	if (count == 0 || size == 0)
	{
		base_free(ctx, p);
		return 0;
	}

	if (count > UINT_MAX / size)
	{
		fprintf(stderr, "error: resize array (%d x %d bytes) failed (integer overflow)\n", count, size);
		return NULL;
	}

	return do_scavenging_realloc(ctx, p, count * size);
}

void
base_free(base_context *ctx, void *p)
{
	base_lock(ctx, base_LOCK_ALLOC);
	ctx->alloc->free(ctx->alloc->user, p);
	base_unlock(ctx, base_LOCK_ALLOC);
}

char *
base_strdup(base_context *ctx, char *s)
{
	int len = strlen(s) + 1;
	char *ns = base_malloc(ctx, len);
	memcpy(ns, s, len);
	return ns;
}

char *
base_strdup_no_throw(base_context *ctx, char *s)
{
	int len = strlen(s) + 1;
	char *ns = base_malloc_no_throw(ctx, len);
	if (ns)
		memcpy(ns, s, len);
	return ns;
}

static void *
base_malloc_default(void *opaque, unsigned int size)
{
	return malloc(size);
}

static void *
base_realloc_default(void *opaque, void *old, unsigned int size)
{
	return realloc(old, size);
}

static void
base_free_default(void *opaque, void *ptr)
{
	free(ptr);
}

base_alloc_context base_alloc_default =
{
	NULL,
	base_malloc_default,
	base_realloc_default,
	base_free_default
};

static void
base_lock_default(void *user, int lock)
{
}

static void
base_unlock_default(void *user, int lock)
{
}

base_locks_context base_locks_default =
{
	NULL,
	base_lock_default,
	base_unlock_default
};

#ifdef PDF_DEBUG_LOCKING

enum
{
	base_LOCK_DEBUG_CONTEXT_MAX = 100
};

base_context *base_lock_debug_contexts[base_LOCK_DEBUG_CONTEXT_MAX];
int base_locks_debug[base_LOCK_DEBUG_CONTEXT_MAX][base_LOCK_MAX];

static int find_context(base_context *ctx)
{
	int i;

	for (i = 0; i < base_LOCK_DEBUG_CONTEXT_MAX; i++)
	{
		if (base_lock_debug_contexts[i] == ctx)
			return i;
		if (base_lock_debug_contexts[i] == NULL)
		{
			int gottit = 0;
			
			ctx->locks->lock(ctx->locks->user, base_LOCK_ALLOC);
			
			if (base_lock_debug_contexts[i] == NULL)
			{
				gottit = 1;
				base_lock_debug_contexts[i] = ctx;
			}
			ctx->locks->unlock(ctx->locks->user, base_LOCK_ALLOC);
			if (gottit)
				return i;
		}
	}
	return -1;
}

void
base_assert_lock_held(base_context *ctx, int lock)
{
	int idx = find_context(ctx);
	if (idx < 0)
		return;

	if (base_locks_debug[idx][lock] == 0)
		fprintf(stderr, "Lock %d not held when expected\n", lock);
}

void
base_assert_lock_not_held(base_context *ctx, int lock)
{
	int idx = find_context(ctx);
	if (idx < 0)
		return;

	if (base_locks_debug[idx][lock] != 0)
		fprintf(stderr, "Lock %d held when not expected\n", lock);
}

void base_lock_debug_lock(base_context *ctx, int lock)
{
	int i;
	int idx = find_context(ctx);
	if (idx < 0)
		return;

	if (base_locks_debug[idx][lock] != 0)
	{
		fprintf(stderr, "Attempt to take lock %d when held already!\n", lock);
	}
	for (i = lock-1; i >= 0; i--)
	{
		if (base_locks_debug[idx][i] != 0)
		{
			fprintf(stderr, "Lock ordering violation: Attempt to take lock %d when %d held already!\n", lock, i);
		}
	}
	base_locks_debug[idx][lock] = 1;
}

void base_lock_debug_unlock(base_context *ctx, int lock)
{
	int idx = find_context(ctx);
	if (idx < 0)
		return;

	if (base_locks_debug[idx][lock] == 0)
	{
		fprintf(stderr, "Attempt to release lock %d when not held!\n", lock);
	}
	base_locks_debug[idx][lock] = 0;
}

#endif
