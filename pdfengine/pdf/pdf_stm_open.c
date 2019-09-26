#include "pdf-internal.h"

base_stream *
base_new_stream(base_context *ctx, void *state,
	int(*read)(base_stream *stm, unsigned char *buf, int len),
	void(*close)(base_context *ctx, void *state))
{
	base_stream *stm;

	base_try(ctx)
	{
		stm = base_malloc_struct(ctx, base_stream);
	}
	base_catch(ctx)
	{
		close(ctx, state);
		base_rethrow(ctx);
	}

	stm->refs = 1;
	stm->error = 0;
	stm->eof = 0;
	stm->pos = 0;

	stm->bits = 0;
	stm->avail = 0;
	stm->locked = 0;

	stm->bp = stm->buf;
	stm->rp = stm->bp;
	stm->wp = stm->bp;
	stm->ep = stm->buf + sizeof stm->buf;

	stm->state = state;
	stm->read = read;
	stm->close = close;
	stm->seek = NULL;
	stm->ctx = ctx;

	return stm;
}

void
base_lock_stream(base_stream *stm)
{
	if (stm)
	{
		base_lock(stm->ctx, base_LOCK_FILE);
		stm->locked = 1;
	}
}

base_stream *
base_keep_stream(base_stream *stm)
{
	stm->refs ++;
	return stm;
}

void
base_close(base_stream *stm)
{
	if (!stm)
		return;
	stm->refs --;
	if (stm->refs == 0)
	{
		if (stm->close)
			stm->close(stm->ctx, stm->state);
		if (stm->locked)
			base_unlock(stm->ctx, base_LOCK_FILE);
		base_free(stm->ctx, stm);
	}
}

static int read_file(base_stream *stm, unsigned char *buf, int len)
{
	int n = read(*(int*)stm->state, buf, len);
	base_assert_lock_held(stm->ctx, base_LOCK_FILE);
	if (n < 0)
		base_throw(stm->ctx, "read error: %s", strerror(errno));
	return n;
}

static void seek_file(base_stream *stm, int offset, int whence)
{
	int n = lseek(*(int*)stm->state, offset, whence);
	base_assert_lock_held(stm->ctx, base_LOCK_FILE);
	if (n < 0)
		base_throw(stm->ctx, "cannot lseek: %s", strerror(errno));
	stm->pos = n;
	stm->rp = stm->bp;
	stm->wp = stm->bp;
}

static void close_file(base_context *ctx, void *state)
{
	int n = close(*(int*)state);
	if (n < 0)
		base_warn(ctx, "close error: %s", strerror(errno));
	base_free(ctx, state);
}

base_stream *
base_open_fd(base_context *ctx, int fd)
{
	base_stream *stm;
	int *state;

	state = base_malloc_struct(ctx, int);
	*state = fd;

	base_try(ctx)
	{
		stm = base_new_stream(ctx, state, read_file, close_file);
	}
	base_catch(ctx)
	{
		base_free(ctx, state);
		base_rethrow(ctx);
	}
	stm->seek = seek_file;

	return stm;
}

base_stream *
base_open_file(base_context *ctx, const wchar_t *name)
{
#ifdef _WIN32
	int fd = _wopen(name, O_BINARY | O_RDONLY, 0);
#else
	int fd = open(name, O_BINARY | O_RDONLY, 0);
#endif
	if (fd == -1)
		base_throw(ctx, "cannot open %s", name);
	return base_open_fd(ctx, fd);
}

#ifdef _WIN32
base_stream *
base_open_file_w(base_context *ctx, const wchar_t *name)
{
	int fd = _wopen(name, O_BINARY | O_RDONLY, 0);
	if (fd == -1)
		return NULL;
	return base_open_fd(ctx, fd);
}
#endif

static int read_buffer(base_stream *stm, unsigned char *buf, int len)
{
	return 0;
}

static void seek_buffer(base_stream *stm, int offset, int whence)
{
	if (whence == 0)
		stm->rp = stm->bp + offset;
	if (whence == 1)
		stm->rp += offset;
	if (whence == 2)
		stm->rp = stm->ep - offset;
	stm->rp = CLAMP(stm->rp, stm->bp, stm->ep);
	stm->wp = stm->ep;
}

static void close_buffer(base_context *ctx, void *state_)
{
	base_buffer *state = (base_buffer *)state_;
	if (state)
		base_drop_buffer(ctx, state);
}

base_stream *
base_open_buffer(base_context *ctx, base_buffer *buf)
{
	base_stream *stm;

	base_keep_buffer(ctx, buf);
	stm = base_new_stream(ctx, buf, read_buffer, close_buffer);
	stm->seek = seek_buffer;

	stm->bp = buf->data;
	stm->rp = buf->data;
	stm->wp = buf->data + buf->len;
	stm->ep = buf->data + buf->len;

	stm->pos = buf->len;

	return stm;
}

base_stream *
base_open_memory(base_context *ctx, unsigned char *data, int len)
{
	base_stream *stm;

	stm = base_new_stream(ctx, NULL, read_buffer, close_buffer);
	stm->seek = seek_buffer;

	stm->bp = data;
	stm->rp = data;
	stm->wp = data + len;
	stm->ep = data + len;

	stm->pos = len;

	return stm;
}
