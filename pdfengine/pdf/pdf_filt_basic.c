#include "pdf-internal.h"

base_stream *
base_open_copy(base_stream *chain)
{
	return base_keep_stream(chain);
}

struct null_filter
{
	base_stream *chain;
	int remain;
};

static int
read_null(base_stream *stm, unsigned char *buf, int len)
{
	struct null_filter *state = stm->state;
	int amount = MIN(len, state->remain);
	int n = base_read(state->chain, buf, amount);
	state->remain -= n;
	return n;
}

static void
close_null(base_context *ctx, void *state_)
{
	struct null_filter *state = (struct null_filter *)state_;
	base_stream *chain = state->chain;
	base_free(ctx, state);
	base_close(chain);
}

base_stream *
base_open_null(base_stream *chain, int len)
{
	struct null_filter *state;
	base_context *ctx = chain->ctx;

	base_try(ctx)
	{
		state = base_malloc_struct(ctx, struct null_filter);
		state->chain = chain;
		state->remain = len;
	}
	base_catch(ctx)
	{
		base_close(chain);
		base_rethrow(ctx);
	}

	return base_new_stream(ctx, state, read_null, close_null);
}

typedef struct base_ahxd_s base_ahxd;

struct base_ahxd_s
{
	base_stream *chain;
	int eod;
};

static inline int iswhite(int a)
{
	switch (a) {
	case '\n': case '\r': case '\t': case ' ':
	case '\0': case '\f': case '\b': case 0177:
		return 1;
	}
	return 0;
}

static inline int ishex(int a)
{
	return (a >= 'A' && a <= 'F') ||
		(a >= 'a' && a <= 'f') ||
		(a >= '0' && a <= '9');
}

static inline int unhex(int a)
{
	if (a >= 'A' && a <= 'F') return a - 'A' + 0xA;
	if (a >= 'a' && a <= 'f') return a - 'a' + 0xA;
	if (a >= '0' && a <= '9') return a - '0';
	return 0;
}

static int
read_ahxd(base_stream *stm, unsigned char *buf, int len)
{
	base_ahxd *state = stm->state;
	unsigned char *p = buf;
	unsigned char *ep = buf + len;
	int a, b, c, odd;

	odd = 0;

	while (p < ep)
	{
		if (state->eod)
			return p - buf;

		c = base_read_byte(state->chain);
		if (c < 0)
			return p - buf;

		if (ishex(c))
		{
			if (!odd)
			{
				a = unhex(c);
				odd = 1;
			}
			else
			{
				b = unhex(c);
				*p++ = (a << 4) | b;
				odd = 0;
			}
		}
		else if (c == '>')
		{
			if (odd)
				*p++ = (a << 4);
			state->eod = 1;
		}
		else if (!iswhite(c))
		{
			base_throw(stm->ctx, "bad data in ahxd: '%c'", c);
		}
	}

	return p - buf;
}

static void
close_ahxd(base_context *ctx, void *state_)
{
	base_ahxd *state = (base_ahxd *)state_;
	base_stream *chain = state->chain;
	base_free(ctx, state);
	base_close(chain);
}

base_stream *
base_open_ahxd(base_stream *chain)
{
	base_ahxd *state;
	base_context *ctx = chain->ctx;

	base_try(ctx)
	{
		state = base_malloc_struct(ctx, base_ahxd);
		state->chain = chain;
		state->eod = 0;
	}
	base_catch(ctx)
	{
		base_close(chain);
		base_rethrow(ctx);
	}

	return base_new_stream(ctx, state, read_ahxd, close_ahxd);
}

typedef struct base_a85d_s base_a85d;

struct base_a85d_s
{
	base_stream *chain;
	unsigned char bp[4];
	unsigned char *rp, *wp;
	int eod;
};

static int
read_a85d(base_stream *stm, unsigned char *buf, int len)
{
	base_a85d *state = stm->state;
	unsigned char *p = buf;
	unsigned char *ep = buf + len;
	int count = 0;
	int word = 0;
	int c;

	while (state->rp < state->wp && p < ep)
		*p++ = *state->rp++;

	while (p < ep)
	{
		if (state->eod)
			return p - buf;

		c = base_read_byte(state->chain);
		if (c < 0)
			return p - buf;

		if (c >= '!' && c <= 'u')
		{
			if (count == 4)
			{
				word = word * 85 + (c - '!');

				state->bp[0] = (word >> 24) & 0xff;
				state->bp[1] = (word >> 16) & 0xff;
				state->bp[2] = (word >> 8) & 0xff;
				state->bp[3] = (word) & 0xff;
				state->rp = state->bp;
				state->wp = state->bp + 4;

				word = 0;
				count = 0;
			}
			else
			{
				word = word * 85 + (c - '!');
				count ++;
			}
		}

		else if (c == 'z' && count == 0)
		{
			state->bp[0] = 0;
			state->bp[1] = 0;
			state->bp[2] = 0;
			state->bp[3] = 0;
			state->rp = state->bp;
			state->wp = state->bp + 4;
		}

		else if (c == '~')
		{
			c = base_read_byte(state->chain);
			if (c != '>')
				base_warn(stm->ctx, "bad eod marker in a85d");

			switch (count) {
			case 0:
				break;
			case 1:
				base_throw(stm->ctx, "partial final byte in a85d");
			case 2:
				word = word * (85 * 85 * 85) + 0xffffff;
				state->bp[0] = word >> 24;
				state->rp = state->bp;
				state->wp = state->bp + 1;
				break;
			case 3:
				word = word * (85 * 85) + 0xffff;
				state->bp[0] = word >> 24;
				state->bp[1] = word >> 16;
				state->rp = state->bp;
				state->wp = state->bp + 2;
				break;
			case 4:
				word = word * 85 + 0xff;
				state->bp[0] = word >> 24;
				state->bp[1] = word >> 16;
				state->bp[2] = word >> 8;
				state->rp = state->bp;
				state->wp = state->bp + 3;
				break;
			}
			state->eod = 1;
		}

		else if (!iswhite(c))
		{
			base_throw(stm->ctx, "bad data in a85d: '%c'", c);
		}

		while (state->rp < state->wp && p < ep)
			*p++ = *state->rp++;
	}

	return p - buf;
}

static void
close_a85d(base_context *ctx, void *state_)
{
	base_a85d *state = (base_a85d *)state_;
	base_stream *chain = state->chain;

	base_free(ctx, state);
	base_close(chain);
}

base_stream *
base_open_a85d(base_stream *chain)
{
	base_a85d *state;
	base_context *ctx = chain->ctx;

	base_try(ctx)
	{
		state = base_malloc_struct(ctx, base_a85d);
		state->chain = chain;
		state->rp = state->bp;
		state->wp = state->bp;
		state->eod = 0;
	}
	base_catch(ctx)
	{
		base_close(chain);
		base_rethrow(ctx);
	}

	return base_new_stream(ctx, state, read_a85d, close_a85d);
}

typedef struct base_rld_s base_rld;

struct base_rld_s
{
	base_stream *chain;
	int run, n, c;
};

static int
read_rld(base_stream *stm, unsigned char *buf, int len)
{
	base_rld *state = stm->state;
	unsigned char *p = buf;
	unsigned char *ep = buf + len;

	while (p < ep)
	{
		if (state->run == 128)
			return p - buf;

		if (state->n == 0)
		{
			state->run = base_read_byte(state->chain);
			if (state->run < 0)
				state->run = 128;
			if (state->run < 128)
				state->n = state->run + 1;
			if (state->run > 128)
			{
				state->n = 257 - state->run;
				state->c = base_read_byte(state->chain);
				if (state->c < 0)
					base_throw(stm->ctx, "premature end of data in run length decode");
			}
		}

		if (state->run < 128)
		{
			while (p < ep && state->n)
			{
				int c = base_read_byte(state->chain);
				if (c < 0)
					base_throw(stm->ctx, "premature end of data in run length decode");
				*p++ = c;
				state->n--;
			}
		}

		if (state->run > 128)
		{
			while (p < ep && state->n)
			{
				*p++ = state->c;
				state->n--;
			}
		}
	}

	return p - buf;
}

static void
close_rld(base_context *ctx, void *state_)
{
	base_rld *state = (base_rld *)state_;
	base_stream *chain = state->chain;

	base_free(ctx, state);
	base_close(chain);
}

base_stream *
base_open_rld(base_stream *chain)
{
	base_rld *state;
	base_context *ctx = chain->ctx;

	base_try(ctx)
	{
		state = base_malloc_struct(ctx, base_rld);
		state->chain = chain;
		state->run = 0;
		state->n = 0;
		state->c = 0;
	}
	base_catch(ctx)
	{
		base_close(chain);
		base_rethrow(ctx);
	}

	return base_new_stream(ctx, state, read_rld, close_rld);
}

typedef struct base_arc4c_s base_arc4c;

struct base_arc4c_s
{
	base_stream *chain;
	base_arc4 arc4;
};

static int
read_arc4(base_stream *stm, unsigned char *buf, int len)
{
	base_arc4c *state = stm->state;
	int n = base_read(state->chain, buf, len);
	base_arc4_encrypt(&state->arc4, buf, buf, n);
	return n;
}

static void
close_arc4(base_context *ctx, void *state_)
{
	base_arc4c *state = (base_arc4c *)state_;
	base_stream *chain = state->chain;

	base_free(ctx, state);
	base_close(chain);
}

base_stream *
base_open_arc4(base_stream *chain, unsigned char *key, unsigned keylen)
{
	base_arc4c *state;
	base_context *ctx = chain->ctx;

	base_try(ctx)
	{
		state = base_malloc_struct(ctx, base_arc4c);
		state->chain = chain;
		base_arc4_init(&state->arc4, key, keylen);
	}
	base_catch(ctx)
	{
		base_close(chain);
		base_rethrow(ctx);
	}

	return base_new_stream(ctx, state, read_arc4, close_arc4);
}

typedef struct base_aesd_s base_aesd;

struct base_aesd_s
{
	base_stream *chain;
	base_aes aes;
	unsigned char iv[16];
	int ivcount;
	unsigned char bp[16];
	unsigned char *rp, *wp;
};

static int
read_aesd(base_stream *stm, unsigned char *buf, int len)
{
	base_aesd *state = stm->state;
	unsigned char *p = buf;
	unsigned char *ep = buf + len;

	while (state->ivcount < 16)
	{
		int c = base_read_byte(state->chain);
		if (c < 0)
			base_throw(stm->ctx, "premature end in aes filter");
		state->iv[state->ivcount++] = c;
	}

	while (state->rp < state->wp && p < ep)
		*p++ = *state->rp++;

	while (p < ep)
	{
		int n = base_read(state->chain, state->bp, 16);
		if (n == 0)
			return p - buf;
		else if (n < 16)
			base_throw(stm->ctx, "partial block in aes filter");

		aes_crypt_cbc(&state->aes, AES_DECRYPT, 16, state->iv, state->bp, state->bp);
		state->rp = state->bp;
		state->wp = state->bp + 16;

		
		if (base_is_eof(state->chain))
		{
			int pad = state->bp[15];
			if (pad < 1 || pad > 16)
				base_throw(stm->ctx, "aes padding out of range: %d", pad);
			state->wp -= pad;
		}

		while (state->rp < state->wp && p < ep)
			*p++ = *state->rp++;
	}

	return p - buf;
}

static void
close_aesd(base_context *ctx, void *state_)
{
	base_aesd *state = (base_aesd *)state_;
	base_stream *chain = state->chain;

	base_free(ctx, state);
	base_close(chain);
}

base_stream *
base_open_aesd(base_stream *chain, unsigned char *key, unsigned keylen)
{
	base_aesd *state;
	base_context *ctx = chain->ctx;

	base_try(ctx)
	{
		state = base_malloc_struct(ctx, base_aesd);
		state->chain = chain;
		aes_setkey_dec(&state->aes, key, keylen * 8);
		state->ivcount = 0;
		state->rp = state->bp;
		state->wp = state->bp;
	}
	base_catch(ctx)
	{
		base_close(chain);
		base_rethrow(ctx);
	}

	return base_new_stream(ctx, state, read_aesd, close_aesd);
}
