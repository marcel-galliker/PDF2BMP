#include "pdf-internal.h"

enum { MAX_KEY_LEN = 48 };
typedef struct base_hash_entry_s base_hash_entry;

struct base_hash_entry_s
{
	unsigned char key[MAX_KEY_LEN];
	void *val;
};

struct base_hash_table_s
{
	int keylen;
	int size;
	int load;
	int lock; 
	base_hash_entry *ents;
};

static unsigned hash(unsigned char *s, int len)
{
	unsigned val = 0;
	int i;
	for (i = 0; i < len; i++)
	{
		val += s[i];
		val += (val << 10);
		val ^= (val >> 6);
	}
	val += (val << 3);
	val ^= (val >> 11);
	val += (val << 15);
	return val;
}

base_hash_table *
base_new_hash_table(base_context *ctx, int initialsize, int keylen, int lock)
{
	base_hash_table *table;

	assert(keylen <= MAX_KEY_LEN);

	table = base_malloc_struct(ctx, base_hash_table);
	table->keylen = keylen;
	table->size = initialsize;
	table->load = 0;
	table->lock = lock;
	base_try(ctx)
	{
		table->ents = base_malloc_array(ctx, table->size, sizeof(base_hash_entry));
		memset(table->ents, 0, sizeof(base_hash_entry) * table->size);
	}
	base_catch(ctx)
	{
		base_free(ctx, table);
		base_rethrow(ctx);
	}

	return table;
}

void
base_empty_hash(base_context *ctx, base_hash_table *table)
{
	table->load = 0;
	memset(table->ents, 0, sizeof(base_hash_entry) * table->size);
}

int
base_hash_len(base_context *ctx, base_hash_table *table)
{
	return table->size;
}

void *
base_hash_get_key(base_context *ctx, base_hash_table *table, int idx)
{
	return table->ents[idx].key;
}

void *
base_hash_get_val(base_context *ctx, base_hash_table *table, int idx)
{
	return table->ents[idx].val;
}

void
base_free_hash(base_context *ctx, base_hash_table *table)
{
	base_free(ctx, table->ents);
	base_free(ctx, table);
}

static void *
do_hash_insert(base_context *ctx, base_hash_table *table, void *key, void *val)
{
	base_hash_entry *ents;
	unsigned size;
	unsigned pos;

	ents = table->ents;
	size = table->size;
	pos = hash(key, table->keylen) % size;

	if (table->lock >= 0)
		base_assert_lock_held(ctx, table->lock);

	while (1)
	{
		if (!ents[pos].val)
		{
			memcpy(ents[pos].key, key, table->keylen);
			ents[pos].val = val;
			table->load ++;
			return NULL;
		}

		if (memcmp(key, ents[pos].key, table->keylen) == 0)
		{
			base_warn(ctx, "assert: overwrite hash slot");
			return ents[pos].val;
		}

		pos = (pos + 1) % size;
	}
}

static void
base_resize_hash(base_context *ctx, base_hash_table *table, int newsize)
{
	base_hash_entry *oldents = table->ents;
	base_hash_entry *newents = table->ents;
	int oldsize = table->size;
	int oldload = table->load;
	int i;

	if (newsize < oldload * 8 / 10)
	{
		base_warn(ctx, "assert: resize hash too small");
		return;
	}

	if (table->lock == base_LOCK_ALLOC)
		base_unlock(ctx, base_LOCK_ALLOC);
	newents = base_malloc_array(ctx, newsize, sizeof(base_hash_entry));
	if (table->lock == base_LOCK_ALLOC)
		base_lock(ctx, base_LOCK_ALLOC);
	if (table->lock >= 0)
	{
		if (table->size >= newsize)
		{
			
			base_unlock(ctx, table->lock);
			base_free(ctx, newents);
			return;
		}
	}
	table->ents = newents;
	memset(table->ents, 0, sizeof(base_hash_entry) * newsize);
	table->size = newsize;
	table->load = 0;

	for (i = 0; i < oldsize; i++)
	{
		if (oldents[i].val)
		{
			do_hash_insert(ctx, table, oldents[i].key, oldents[i].val);
		}
	}

	if (table->lock == base_LOCK_ALLOC)
		base_unlock(ctx, base_LOCK_ALLOC);
	base_free(ctx, oldents);
	if (table->lock == base_LOCK_ALLOC)
		base_lock(ctx, base_LOCK_ALLOC);
}

void *
base_hash_find(base_context *ctx, base_hash_table *table, void *key)
{
	base_hash_entry *ents = table->ents;
	unsigned size = table->size;
	unsigned pos = hash(key, table->keylen) % size;

	if (table->lock >= 0)
		base_assert_lock_held(ctx, table->lock);

	while (1)
	{
		if (!ents[pos].val)
			return NULL;

		if (memcmp(key, ents[pos].key, table->keylen) == 0)
			return ents[pos].val;

		pos = (pos + 1) % size;
	}
}

void *
base_hash_insert(base_context *ctx, base_hash_table *table, void *key, void *val)
{
	if (table->load > table->size * 8 / 10)
	{
		base_resize_hash(ctx, table, table->size * 2);
	}

	return do_hash_insert(ctx, table, key, val);
}

void
base_hash_remove(base_context *ctx, base_hash_table *table, void *key)
{
	base_hash_entry *ents = table->ents;
	unsigned size = table->size;
	unsigned pos = hash(key, table->keylen) % size;
	unsigned hole, look, code;

	if (table->lock >= 0)
		base_assert_lock_held(ctx, table->lock);

	while (1)
	{
		if (!ents[pos].val)
		{
			base_warn(ctx, "assert: remove non-existent hash entry");
			return;
		}

		if (memcmp(key, ents[pos].key, table->keylen) == 0)
		{
			ents[pos].val = NULL;

			hole = pos;
			look = (hole + 1) % size;

			while (ents[look].val)
			{
				code = hash(ents[look].key, table->keylen) % size;
				if ((code <= hole && hole < look) ||
					(look < code && code <= hole) ||
					(hole < look && look < code))
				{
					ents[hole] = ents[look];
					ents[look].val = NULL;
					hole = look;
				}

				look = (look + 1) % size;
			}

			table->load --;

			return;
		}

		pos = (pos + 1) % size;
	}
}

void
base_print_hash(base_context *ctx, FILE *out, base_hash_table *table)
{
	int i, k;

	fprintf(out, "cache load %d / %d\n", table->load, table->size);

	for (i = 0; i < table->size; i++)
	{
		if (!table->ents[i].val)
			fprintf(out, "table % 4d: empty\n", i);
		else
		{
			fprintf(out, "table % 4d: key=", i);
			for (k = 0; k < MAX_KEY_LEN; k++)
				fprintf(out, "%02x", ((char*)table->ents[i].key)[k]);
			fprintf(out, " val=$%p\n", table->ents[i].val);
		}
	}
}
