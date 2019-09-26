#include "pdf-internal.h"

#define MIN_BOMB (100 << 20)

int
base_read(base_stream *stm, unsigned char *buf, int len)
{
	int count, n;

	count = MIN(len, stm->wp - stm->rp);
	if (count)
	{
		memcpy(buf, stm->rp, count);
		stm->rp += count;
	}

	if (count == len || stm->error || stm->eof)
		return count;

	assert(stm->rp == stm->wp);

	if (len - count < stm->ep - stm->bp)
	{
		n = stm->read(stm, stm->bp, stm->ep - stm->bp);
		if (n == 0)
		{
			stm->eof = 1;
		}
		else if (n > 0)
		{
			stm->rp = stm->bp;
			stm->wp = stm->bp + n;
			stm->pos += n;
		}

		n = MIN(len - count, stm->wp - stm->rp);
		if (n)
		{
			memcpy(buf + count, stm->rp, n);
			stm->rp += n;
			count += n;
		}
	}
	else
	{
		n = stm->read(stm, buf + count, len - count);
		if (n == 0)
		{
			stm->eof = 1;
		}
		else if (n > 0)
		{
			stm->pos += n;
			count += n;
		}
	}

	return count;
}

void
base_fill_buffer(base_stream *stm)
{
	int n;

	assert(stm->rp == stm->wp);

	if (stm->error || stm->eof)
		return;

	base_try(stm->ctx)
	{
		n = stm->read(stm, stm->bp, stm->ep - stm->bp);
		if (n == 0)
		{
			stm->eof = 1;
		}
		else if (n > 0)
		{
			stm->rp = stm->bp;
			stm->wp = stm->bp + n;
			stm->pos += n;
		}
	}
	base_catch(stm->ctx)
	{
		base_warn(stm->ctx, "read error; treating as end of file");
		stm->error = 1;
	}
}

base_buffer *
base_read_all(base_stream *stm, int initial)
{
	base_buffer *buf = NULL;
	int n;
	base_context *ctx = stm->ctx;

	base_var(buf);

	base_try(ctx)
	{
		if (initial < 1024)
			initial = 1024;

		buf = base_new_buffer(ctx, initial+1);

		while (1)
		{
			if (buf->len == buf->cap)
				base_grow_buffer(ctx, buf);

			if (buf->len >= MIN_BOMB && buf->len / 200 > initial)
			{
				base_throw(ctx, "compression bomb detected");
			}

			n = base_read(stm, buf->data + buf->len, buf->cap - buf->len);
			if (n == 0)
				break;

			buf->len += n;
		}
	}
	base_catch(ctx)
	{
		base_drop_buffer(ctx, buf);
		base_rethrow(ctx);
	}
	base_trim_buffer(ctx, buf);

	return buf;
}

void
base_read_line(base_stream *stm, char *mem, int n)
{
	char *s = mem;
	int c = EOF;
	while (n > 1)
	{
		c = base_read_byte(stm);
		if (c == EOF)
			break;
		if (c == '\r') {
			c = base_peek_byte(stm);
			if (c == '\n')
				base_read_byte(stm);
			break;
		}
		if (c == '\n')
			break;
		*s++ = c;
		n--;
	}
	if (n)
		*s = '\0';
}

int
base_tell(base_stream *stm)
{
	return stm->pos - (stm->wp - stm->rp);
}

void
base_seek(base_stream *stm, int offset, int whence)
{
	if (stm->seek)
	{
		if (whence == 1)
		{
			offset = base_tell(stm) + offset;
			whence = 0;
		}
		if (whence == 0)
		{
			int dist = stm->pos - offset;
			if (dist >= 0 && dist <= stm->wp - stm->bp)
			{
				stm->rp = stm->wp - dist;
				stm->eof = 0;
				return;
			}
		}
		stm->seek(stm, offset, whence);
		stm->eof = 0;
	}
	else if (whence != 2)
	{
		if (whence == 0)
			offset -= base_tell(stm);
		if (offset < 0)
			base_warn(stm->ctx, "cannot seek backwards");
		
		while (offset-- > 0)
			base_read_byte(stm);
	}
	else
		base_warn(stm->ctx, "cannot seek");
}
