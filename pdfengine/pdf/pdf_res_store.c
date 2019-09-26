#include "pdf-internal.h"

typedef struct base_item_s base_item;

struct base_item_s
{
	void *key;
	base_storable *val;
	unsigned int size;
	base_item *next;
	base_item *prev;
	base_store *store;
	base_store_type *type;
};

struct base_store_s
{
	int refs;

	
	base_item *head;
	base_item *tail;

	
	base_hash_table *hash;

	
	unsigned int max;
	unsigned int size;
};

void
base_new_store_context(base_context *ctx, unsigned int max)
{
	base_store *store;
	store = base_malloc_struct(ctx, base_store);
	base_try(ctx)
	{
		store->hash = base_new_hash_table(ctx, 4096, sizeof(base_store_hash), base_LOCK_ALLOC);
	}
	base_catch(ctx)
	{
		base_free(ctx, store);
		base_rethrow(ctx);
	}
	store->refs = 1;
	store->head = NULL;
	store->tail = NULL;
	store->size = 0;
	store->max = max;
	ctx->store = store;
}

void *
base_keep_storable(base_context *ctx, base_storable *s)
{
	if (s == NULL)
		return NULL;
	base_lock(ctx, base_LOCK_ALLOC);
	if (s->refs > 0)
		s->refs++;
	base_unlock(ctx, base_LOCK_ALLOC);
	return s;
}

void
base_drop_storable(base_context *ctx, base_storable *s)
{
	int do_free = 0;

	if (s == NULL)
		return;
	base_lock(ctx, base_LOCK_ALLOC);
	if (s->refs < 0)
	{
		
	}
	else if (--s->refs == 0)
	{
		
		do_free = 1;
	}
	base_unlock(ctx, base_LOCK_ALLOC);
	if (do_free)
		s->free(ctx, s);
}

static void
evict(base_context *ctx, base_item *item)
{
	base_store *store = ctx->store;
	int drop;

	store->size -= item->size;
	
	if (item->next)
		item->next->prev = item->prev;
	else
		store->tail = item->prev;
	if (item->prev)
		item->prev->next = item->next;
	else
		store->head = item->next;
	
	drop = (item->val->refs > 0 && --item->val->refs == 0);
	
	if (item->type->make_hash_key)
	{
		base_store_hash hash = { NULL };
		hash.free = item->val->free;
		if (item->type->make_hash_key(&hash, item->key))
			base_hash_remove(ctx, store->hash, &hash);
	}
	base_unlock(ctx, base_LOCK_ALLOC);
	if (drop)
		item->val->free(ctx, item->val);
	
	item->type->drop_key(ctx, item->key);
	base_free(ctx, item);
	base_lock(ctx, base_LOCK_ALLOC);
}

static int
ensure_space(base_context *ctx, unsigned int tofree)
{
	base_item *item, *prev;
	unsigned int count;
	base_store *store = ctx->store;

	base_assert_lock_held(ctx, base_LOCK_ALLOC);

	
	count = 0;
	for (item = store->tail; item; item = item->prev)
	{
		if (item->val->refs == 1)
		{
			count += item->size;
			if (count >= tofree)
				break;
		}
	}

	
	if (item == NULL)
	{
		return 0;
	}

	
	count = 0;
	for (item = store->tail; item; item = prev)
	{
		prev = item->prev;
		if (item->val->refs == 1)
		{
			
			count += item->size;
			if (prev)
				prev->val->refs++;
			evict(ctx, item); 
			
			if (prev)
				--prev->val->refs;

			if (count >= tofree)
				return count;
		}
	}

	return count;
}

void *
base_store_item(base_context *ctx, void *key, void *val_, unsigned int itemsize, base_store_type *type)
{
	base_item *item = NULL;
	unsigned int size;
	base_storable *val = (base_storable *)val_;
	base_store *store = ctx->store;
	base_store_hash hash = { NULL };
	int use_hash = 0;

	if (!store)
		return NULL;

	base_var(item);

	
	base_try(ctx)
	{
		item = base_malloc_struct(ctx, base_item);
	}
	base_catch(ctx)
	{
		return NULL;
	}

	if (type->make_hash_key)
	{
		hash.free = val->free;
		use_hash = type->make_hash_key(&hash, key);
	}

	type->keep_key(ctx, key);
	base_lock(ctx, base_LOCK_ALLOC);
	if (store->max != base_STORE_UNLIMITED)
	{
		size = store->size + itemsize;
		while (size > store->max)
		{
			
			if (ensure_space(ctx, size - store->max) == 0)
			{
				
				base_unlock(ctx, base_LOCK_ALLOC);
				base_free(ctx, item);
				type->drop_key(ctx, key);
				return NULL;
			}
		}
	}
	store->size += itemsize;

	item->key = key;
	item->val = val;
	item->size = itemsize;
	item->next = NULL;
	item->type = type;

	
	if (use_hash)
	{
		base_item *existing;

		base_try(ctx)
		{
			
			existing = base_hash_insert(ctx, store->hash, &hash, item);
		}
		base_catch(ctx)
		{
			store->size -= itemsize;
			base_unlock(ctx, base_LOCK_ALLOC);
			base_free(ctx, item);
			return NULL;
		}
		if (existing)
		{
			
			existing->val->refs++;
			base_unlock(ctx, base_LOCK_ALLOC);
			base_free(ctx, item);
			return existing->val;
		}
	}
	
	if (val->refs > 0)
		val->refs++;
	
	item->next = store->head;
	if (item->next)
		item->next->prev = item;
	else
		store->tail = item;
	store->head = item;
	item->prev = NULL;
	base_unlock(ctx, base_LOCK_ALLOC);

	return NULL;
}

void *
base_find_item(base_context *ctx, base_store_free_fn *free, void *key, base_store_type *type)
{
	base_item *item;
	base_store *store = ctx->store;
	base_store_hash hash = { NULL };
	int use_hash = 0;

	if (!store)
		return NULL;

	if (!key)
		return NULL;

	if (type->make_hash_key)
	{
		hash.free = free;
		use_hash = type->make_hash_key(&hash, key);
	}

	base_lock(ctx, base_LOCK_ALLOC);
	if (use_hash)
	{
		
		item = base_hash_find(ctx, store->hash, &hash);
	}
	else
	{
		
		for (item = store->head; item; item = item->next)
		{
			if (item->val->free == free && !type->cmp_key(item->key, key))
				break;
		}
	}
	if (item)
	{
		
		
		if (item->next)
			item->next->prev = item->prev;
		else
			store->tail = item->prev;
		if (item->prev)
			item->prev->next = item->next;
		else
			store->head = item->next;
		
		item->next = store->head;
		if (item->next)
			item->next->prev = item;
		else
			store->tail = item;
		item->prev = NULL;
		store->head = item;
		
		if (item->val->refs > 0)
			item->val->refs++;
		base_unlock(ctx, base_LOCK_ALLOC);
		return (void *)item->val;
	}
	base_unlock(ctx, base_LOCK_ALLOC);

	return NULL;
}

void
base_remove_item(base_context *ctx, base_store_free_fn *free, void *key, base_store_type *type)
{
	base_item *item;
	base_store *store = ctx->store;
	int drop;
	base_store_hash hash;
	int use_hash = 0;

	if (type->make_hash_key)
	{
		hash.free = free;
		use_hash = type->make_hash_key(&hash, key);
	}

	base_lock(ctx, base_LOCK_ALLOC);
	if (use_hash)
	{
		
		item = base_hash_find(ctx, store->hash, &hash);
		if (item)
			base_hash_remove(ctx, store->hash, &hash);
	}
	else
	{
		
		for (item = store->head; item; item = item->next)
			if (item->val->free == free && !type->cmp_key(item->key, key))
				break;
	}
	if (item)
	{
		if (item->next)
			item->next->prev = item->prev;
		else
			store->tail = item->prev;
		if (item->prev)
			item->prev->next = item->next;
		else
			store->head = item->next;
		drop = (item->val->refs > 0 && --item->val->refs == 0);
		base_unlock(ctx, base_LOCK_ALLOC);
		if (drop)
			item->val->free(ctx, item->val);
		type->drop_key(ctx, item->key);
		base_free(ctx, item);
	}
	else
		base_unlock(ctx, base_LOCK_ALLOC);
}

void
base_empty_store(base_context *ctx)
{
	base_store *store = ctx->store;

	if (store == NULL)
		return;

	base_lock(ctx, base_LOCK_ALLOC);
	
	while (store->head)
	{
		evict(ctx, store->head); 
	}
	base_unlock(ctx, base_LOCK_ALLOC);
}

base_store *
base_keep_store_context(base_context *ctx)
{
	if (ctx == NULL || ctx->store == NULL)
		return NULL;
	base_lock(ctx, base_LOCK_ALLOC);
	ctx->store->refs++;
	base_unlock(ctx, base_LOCK_ALLOC);
	return ctx->store;
}

void
base_drop_store_context(base_context *ctx)
{
	int refs;
	if (ctx == NULL || ctx->store == NULL)
		return;
	base_lock(ctx, base_LOCK_ALLOC);
	refs = --ctx->store->refs;
	base_unlock(ctx, base_LOCK_ALLOC);
	if (refs != 0)
		return;

	base_empty_store(ctx);
	base_free_hash(ctx, ctx->store->hash);
	base_free(ctx, ctx->store);
	ctx->store = NULL;
}

void
base_print_store(base_context *ctx, FILE *out)
{
	base_item *item, *next;
	base_store *store = ctx->store;

	fprintf(out, "-- resource store contents --\n");

	base_lock(ctx, base_LOCK_ALLOC);
	for (item = store->head; item; item = next)
	{
		next = item->next;
		if (next)
			next->val->refs++;
		fprintf(out, "store[*][refs=%d][size=%d] ", item->val->refs, item->size);
		base_unlock(ctx, base_LOCK_ALLOC);
		item->type->debug(item->key);
		fprintf(out, " = %p\n", item->val);
		base_lock(ctx, base_LOCK_ALLOC);
		if (next)
			next->val->refs--;
	}
	base_unlock(ctx, base_LOCK_ALLOC);
}

static int
scavenge(base_context *ctx, unsigned int tofree)
{
	base_store *store = ctx->store;
	unsigned int count = 0;
	base_item *item, *prev;

	
	for (item = store->tail; item; item = prev)
	{
		prev = item->prev;
		if (item->val->refs == 1)
		{
			
			count += item->size;
			evict(ctx, item); 

			if (count >= tofree)
				break;

			
			prev = store->tail;
		}
	}
	
	return count != 0;
}

int base_store_scavenge(base_context *ctx, unsigned int size, int *phase)
{
	base_store *store;
	unsigned int max;

	if (ctx == NULL)
		return 0;
	store = ctx->store;
	if (store == NULL)
		return 0;

#ifdef DEBUG_SCAVENGING
	printf("Scavenging: store=%d size=%d phase=%d\n", store->size, size, *phase);
	base_print_store(ctx, stderr);
#endif
	do
	{
		unsigned int tofree;

		
		if (*phase >= 16)
			max = 0;
		else if (store->max != base_STORE_UNLIMITED)
			max = store->max / 16 * (16 - *phase);
		else
			max = store->size / (16 - *phase) * (15 - *phase);
		(*phase)++;

		
		if (size > UINT_MAX - store->size)
			tofree = UINT_MAX - max;
		else if (size + store->size > max)
			continue;
		else
			tofree = size + store->size - max;

		if (scavenge(ctx, tofree))
		{
#ifdef DEBUG_SCAVENGING
			printf("scavenged: store=%d\n", store->size);
			base_print_store(ctx, stderr);			
#endif
			return 1;
		}
	}
	while (max > 0);

#ifdef DEBUG_SCAVENGING
	printf("scavenging failed\n");
	base_print_store(ctx, stderr);
#endif
	return 0;
}
