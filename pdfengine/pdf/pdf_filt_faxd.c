#include "pdf-internal.h"

typedef struct cfd_node_s cfd_node;

struct cfd_node_s
{
	short val;
	short nbits;
};

enum
{
	cfd_white_initial_bits = 8,
	cfd_black_initial_bits = 7,
	cfd_2d_initial_bits = 7,
	cfd_uncompressed_initial_bits = 6	
};

enum
{
	ERROR = -1,
	ZEROS = -2, 
	UNCOMPRESSED = -3
};

enum
{
	P = -4,
	H = -5,
	VR3 = 0,
	VR2 = 1,
	VR1 = 2,
	V0 = 3,
	VL1 = 4,
	VL2 = 5,
	VL3 = 6
};

const cfd_node cf_white_decode[] = {
	{256,12},{272,12},{29,8},{30,8},{45,8},{46,8},{22,7},{22,7},
	{23,7},{23,7},{47,8},{48,8},{13,6},{13,6},{13,6},{13,6},{20,7},
	{20,7},{33,8},{34,8},{35,8},{36,8},{37,8},{38,8},{19,7},{19,7},
	{31,8},{32,8},{1,6},{1,6},{1,6},{1,6},{12,6},{12,6},{12,6},{12,6},
	{53,8},{54,8},{26,7},{26,7},{39,8},{40,8},{41,8},{42,8},{43,8},
	{44,8},{21,7},{21,7},{28,7},{28,7},{61,8},{62,8},{63,8},{0,8},
	{320,8},{384,8},{10,5},{10,5},{10,5},{10,5},{10,5},{10,5},{10,5},
	{10,5},{11,5},{11,5},{11,5},{11,5},{11,5},{11,5},{11,5},{11,5},
	{27,7},{27,7},{59,8},{60,8},{288,9},{290,9},{18,7},{18,7},{24,7},
	{24,7},{49,8},{50,8},{51,8},{52,8},{25,7},{25,7},{55,8},{56,8},
	{57,8},{58,8},{192,6},{192,6},{192,6},{192,6},{1664,6},{1664,6},
	{1664,6},{1664,6},{448,8},{512,8},{292,9},{640,8},{576,8},{294,9},
	{296,9},{298,9},{300,9},{302,9},{256,7},{256,7},{2,4},{2,4},{2,4},
	{2,4},{2,4},{2,4},{2,4},{2,4},{2,4},{2,4},{2,4},{2,4},{2,4},{2,4},
	{2,4},{2,4},{3,4},{3,4},{3,4},{3,4},{3,4},{3,4},{3,4},{3,4},{3,4},
	{3,4},{3,4},{3,4},{3,4},{3,4},{3,4},{3,4},{128,5},{128,5},{128,5},
	{128,5},{128,5},{128,5},{128,5},{128,5},{8,5},{8,5},{8,5},{8,5},
	{8,5},{8,5},{8,5},{8,5},{9,5},{9,5},{9,5},{9,5},{9,5},{9,5},{9,5},
	{9,5},{16,6},{16,6},{16,6},{16,6},{17,6},{17,6},{17,6},{17,6},
	{4,4},{4,4},{4,4},{4,4},{4,4},{4,4},{4,4},{4,4},{4,4},{4,4},{4,4},
	{4,4},{4,4},{4,4},{4,4},{4,4},{5,4},{5,4},{5,4},{5,4},{5,4},{5,4},
	{5,4},{5,4},{5,4},{5,4},{5,4},{5,4},{5,4},{5,4},{5,4},{5,4},
	{14,6},{14,6},{14,6},{14,6},{15,6},{15,6},{15,6},{15,6},{64,5},
	{64,5},{64,5},{64,5},{64,5},{64,5},{64,5},{64,5},{6,4},{6,4},
	{6,4},{6,4},{6,4},{6,4},{6,4},{6,4},{6,4},{6,4},{6,4},{6,4},{6,4},
	{6,4},{6,4},{6,4},{7,4},{7,4},{7,4},{7,4},{7,4},{7,4},{7,4},{7,4},
	{7,4},{7,4},{7,4},{7,4},{7,4},{7,4},{7,4},{7,4},{-2,3},{-2,3},
	{-1,0},{-1,0},{-1,0},{-1,0},{-1,0},{-1,0},{-1,0},{-1,0},{-1,0},
	{-1,0},{-1,0},{-1,0},{-1,0},{-3,4},{1792,3},{1792,3},{1984,4},
	{2048,4},{2112,4},{2176,4},{2240,4},{2304,4},{1856,3},{1856,3},
	{1920,3},{1920,3},{2368,4},{2432,4},{2496,4},{2560,4},{1472,1},
	{1536,1},{1600,1},{1728,1},{704,1},{768,1},{832,1},{896,1},
	{960,1},{1024,1},{1088,1},{1152,1},{1216,1},{1280,1},{1344,1},
	{1408,1}
};

const cfd_node cf_black_decode[] = {
	{128,12},{160,13},{224,12},{256,12},{10,7},{11,7},{288,12},{12,7},
	{9,6},{9,6},{8,6},{8,6},{7,5},{7,5},{7,5},{7,5},{6,4},{6,4},{6,4},
	{6,4},{6,4},{6,4},{6,4},{6,4},{5,4},{5,4},{5,4},{5,4},{5,4},{5,4},
	{5,4},{5,4},{1,3},{1,3},{1,3},{1,3},{1,3},{1,3},{1,3},{1,3},{1,3},
	{1,3},{1,3},{1,3},{1,3},{1,3},{1,3},{1,3},{4,3},{4,3},{4,3},{4,3},
	{4,3},{4,3},{4,3},{4,3},{4,3},{4,3},{4,3},{4,3},{4,3},{4,3},{4,3},
	{4,3},{3,2},{3,2},{3,2},{3,2},{3,2},{3,2},{3,2},{3,2},{3,2},{3,2},
	{3,2},{3,2},{3,2},{3,2},{3,2},{3,2},{3,2},{3,2},{3,2},{3,2},{3,2},
	{3,2},{3,2},{3,2},{3,2},{3,2},{3,2},{3,2},{3,2},{3,2},{3,2},{3,2},
	{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},
	{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},
	{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},
	{-2,4},{-2,4},{-1,0},{-1,0},{-1,0},{-1,0},{-1,0},{-1,0},{-1,0},
	{-1,0},{-1,0},{-1,0},{-1,0},{-1,0},{-1,0},{-3,5},{1792,4},
	{1792,4},{1984,5},{2048,5},{2112,5},{2176,5},{2240,5},{2304,5},
	{1856,4},{1856,4},{1920,4},{1920,4},{2368,5},{2432,5},{2496,5},
	{2560,5},{18,3},{18,3},{18,3},{18,3},{18,3},{18,3},{18,3},{18,3},
	{52,5},{52,5},{640,6},{704,6},{768,6},{832,6},{55,5},{55,5},
	{56,5},{56,5},{1280,6},{1344,6},{1408,6},{1472,6},{59,5},{59,5},
	{60,5},{60,5},{1536,6},{1600,6},{24,4},{24,4},{24,4},{24,4},
	{25,4},{25,4},{25,4},{25,4},{1664,6},{1728,6},{320,5},{320,5},
	{384,5},{384,5},{448,5},{448,5},{512,6},{576,6},{53,5},{53,5},
	{54,5},{54,5},{896,6},{960,6},{1024,6},{1088,6},{1152,6},{1216,6},
	{64,3},{64,3},{64,3},{64,3},{64,3},{64,3},{64,3},{64,3},{13,1},
	{13,1},{13,1},{13,1},{13,1},{13,1},{13,1},{13,1},{13,1},{13,1},
	{13,1},{13,1},{13,1},{13,1},{13,1},{13,1},{23,4},{23,4},{50,5},
	{51,5},{44,5},{45,5},{46,5},{47,5},{57,5},{58,5},{61,5},{256,5},
	{16,3},{16,3},{16,3},{16,3},{17,3},{17,3},{17,3},{17,3},{48,5},
	{49,5},{62,5},{63,5},{30,5},{31,5},{32,5},{33,5},{40,5},{41,5},
	{22,4},{22,4},{14,1},{14,1},{14,1},{14,1},{14,1},{14,1},{14,1},
	{14,1},{14,1},{14,1},{14,1},{14,1},{14,1},{14,1},{14,1},{14,1},
	{15,2},{15,2},{15,2},{15,2},{15,2},{15,2},{15,2},{15,2},{128,5},
	{192,5},{26,5},{27,5},{28,5},{29,5},{19,4},{19,4},{20,4},{20,4},
	{34,5},{35,5},{36,5},{37,5},{38,5},{39,5},{21,4},{21,4},{42,5},
	{43,5},{0,3},{0,3},{0,3},{0,3}
};

const cfd_node cf_2d_decode[] = {
	{128,11},{144,10},{6,7},{0,7},{5,6},{5,6},{1,6},{1,6},{-4,4},
	{-4,4},{-4,4},{-4,4},{-4,4},{-4,4},{-4,4},{-4,4},{-5,3},{-5,3},
	{-5,3},{-5,3},{-5,3},{-5,3},{-5,3},{-5,3},{-5,3},{-5,3},{-5,3},
	{-5,3},{-5,3},{-5,3},{-5,3},{-5,3},{4,3},{4,3},{4,3},{4,3},{4,3},
	{4,3},{4,3},{4,3},{4,3},{4,3},{4,3},{4,3},{4,3},{4,3},{4,3},{4,3},
	{2,3},{2,3},{2,3},{2,3},{2,3},{2,3},{2,3},{2,3},{2,3},{2,3},{2,3},
	{2,3},{2,3},{2,3},{2,3},{2,3},{3,1},{3,1},{3,1},{3,1},{3,1},{3,1},
	{3,1},{3,1},{3,1},{3,1},{3,1},{3,1},{3,1},{3,1},{3,1},{3,1},{3,1},
	{3,1},{3,1},{3,1},{3,1},{3,1},{3,1},{3,1},{3,1},{3,1},{3,1},{3,1},
	{3,1},{3,1},{3,1},{3,1},{3,1},{3,1},{3,1},{3,1},{3,1},{3,1},{3,1},
	{3,1},{3,1},{3,1},{3,1},{3,1},{3,1},{3,1},{3,1},{3,1},{3,1},{3,1},
	{3,1},{3,1},{3,1},{3,1},{3,1},{3,1},{3,1},{3,1},{3,1},{3,1},{3,1},
	{3,1},{3,1},{3,1},{-2,4},{-1,0},{-1,0},{-1,0},{-1,0},{-1,0},
	{-1,0},{-1,0},{-1,0},{-1,0},{-1,0},{-1,0},{-1,0},{-1,0},{-1,0},
	{-1,0},{-1,0},{-1,0},{-1,0},{-1,0},{-1,0},{-1,0},{-1,0},{-3,3}
};

const cfd_node cf_uncompressed_decode[] = {
	{64,12},{5,6},{4,5},{4,5},{3,4},{3,4},{3,4},{3,4},{2,3},{2,3},
	{2,3},{2,3},{2,3},{2,3},{2,3},{2,3},{1,2},{1,2},{1,2},{1,2},{1,2},
	{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},
	{0,1},{0,1},{0,1},{0,1},{0,1},{0,1},{0,1},{0,1},{0,1},{0,1},{0,1},
	{0,1},{0,1},{0,1},{0,1},{0,1},{0,1},{0,1},{0,1},{0,1},{0,1},{0,1},
	{0,1},{0,1},{0,1},{0,1},{0,1},{0,1},{0,1},{0,1},{0,1},{0,1},
	{-1,0},{-1,0},{8,6},{9,6},{6,5},{6,5},{7,5},{7,5},{4,4},{4,4},
	{4,4},{4,4},{5,4},{5,4},{5,4},{5,4},{2,3},{2,3},{2,3},{2,3},{2,3},
	{2,3},{2,3},{2,3},{3,3},{3,3},{3,3},{3,3},{3,3},{3,3},{3,3},{3,3},
	{0,2},{0,2},{0,2},{0,2},{0,2},{0,2},{0,2},{0,2},{0,2},{0,2},{0,2},
	{0,2},{0,2},{0,2},{0,2},{0,2},{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},
	{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},{1,2}
};

static inline int getbit(const unsigned char *buf, int x)
{
	return ( buf[x >> 3] >> ( 7 - (x & 7) ) ) & 1;
}

static int
find_changing(const unsigned char *line, int x, int w)
{
	int a, b;

	if (!line)
		return w;

	if (x == -1)
	{
		a = 0;
		x = 0;
	}
	else
	{
		a = getbit(line, x);
		x++;
	}

	while (x < w)
	{
		b = getbit(line, x);
		if (a != b)
			break;
		x++;
	}

	return x;
}

static int
find_changing_color(const unsigned char *line, int x, int w, int color)
{
	if (!line)
		return w;

	x = find_changing(line, (x > 0 || !color) ? x : -1, w);

	if (x < w && getbit(line, x) != color)
		x = find_changing(line, x, w);

	return x;
}

static const unsigned char lm[8] = {
	0xFF, 0x7F, 0x3F, 0x1F, 0x0F, 0x07, 0x03, 0x01
};

static const unsigned char rm[8] = {
	0x00, 0x80, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC, 0xFE
};

static inline void setbits(unsigned char *line, int x0, int x1)
{
	int a0, a1, b0, b1, a;

	a0 = x0 >> 3;
	a1 = x1 >> 3;

	b0 = x0 & 7;
	b1 = x1 & 7;

	if (a0 == a1)
	{
		if (b1)
			line[a0] |= lm[b0] & rm[b1];
	}
	else
	{
		line[a0] |= lm[b0];
		for (a = a0 + 1; a < a1; a++)
			line[a] = 0xFF;
		if (b1)
			line[a1] |= rm[b1];
	}
}

typedef struct base_faxd_s base_faxd;

enum
{
	STATE_NORMAL,	
	STATE_MAKEUP,	
	STATE_EOL,		
	STATE_H1, STATE_H2,	
	STATE_DONE		
};

struct base_faxd_s
{
	base_context *ctx;
	base_stream *chain;

	int k;
	int end_of_line;
	int encoded_byte_align;
	int columns;
	int rows;
	int end_of_block;
	int black_is_1;

	int stride;
	int ridx;

	int bidx;
	unsigned int word;

	int stage;

	int a, c, dim, eolc;
	unsigned char *ref;
	unsigned char *dst;
	unsigned char *rp, *wp;
};

static inline void eat_bits(base_faxd *fax, int nbits)
{
	fax->word <<= nbits;
	fax->bidx += nbits;
}

static int
fill_bits(base_faxd *fax)
{
	while (fax->bidx >= 8)
	{
		int c = base_read_byte(fax->chain);
		if (c == EOF)
			return EOF;
		fax->bidx -= 8;
		fax->word |= c << fax->bidx;
	}
	return 0;
}

static int
get_code(base_faxd *fax, const cfd_node *table, int initialbits)
{
	unsigned int word = fax->word;
	int tidx = word >> (32 - initialbits);
	int val = table[tidx].val;
	int nbits = table[tidx].nbits;

	if (nbits > initialbits)
	{
		int mask = (1 << (32 - initialbits)) - 1;
		tidx = val + ((word & mask) >> (32 - nbits));
		val = table[tidx].val;
		nbits = initialbits + table[tidx].nbits;
	}

	eat_bits(fax, nbits);

	return val;
}

static void
dec1d(base_context *ctx, base_faxd *fax)
{
	int code;

	if (fax->a == -1)
		fax->a = 0;

	if (fax->c)
		code = get_code(fax, cf_black_decode, cfd_black_initial_bits);
	else
		code = get_code(fax, cf_white_decode, cfd_white_initial_bits);

	if (code == UNCOMPRESSED)
		base_throw(ctx, "uncompressed data in faxd");

	if (code < 0)
		base_throw(ctx, "negative code in 1d faxd");

	if (fax->a + code > fax->columns)
		base_throw(ctx, "overflow in 1d faxd");

	if (fax->c)
		setbits(fax->dst, fax->a, fax->a + code);

	fax->a += code;

	if (code < 64)
	{
		fax->c = !fax->c;
		fax->stage = STATE_NORMAL;
	}
	else
		fax->stage = STATE_MAKEUP;
}

static void
dec2d(base_context *ctx, base_faxd *fax)
{
	int code, b1, b2;

	if (fax->stage == STATE_H1 || fax->stage == STATE_H2)
	{
		if (fax->a == -1)
			fax->a = 0;

		if (fax->c)
			code = get_code(fax, cf_black_decode, cfd_black_initial_bits);
		else
			code = get_code(fax, cf_white_decode, cfd_white_initial_bits);

		if (code == UNCOMPRESSED)
			base_throw(ctx, "uncompressed data in faxd");

		if (code < 0)
			base_throw(ctx, "negative code in 2d faxd");

		if (fax->a + code > fax->columns)
			base_throw(ctx, "overflow in 2d faxd");

		if (fax->c)
			setbits(fax->dst, fax->a, fax->a + code);

		fax->a += code;

		if (code < 64)
		{
			fax->c = !fax->c;
			if (fax->stage == STATE_H1)
				fax->stage = STATE_H2;
			else if (fax->stage == STATE_H2)
				fax->stage = STATE_NORMAL;
		}

		return;
	}

	code = get_code(fax, cf_2d_decode, cfd_2d_initial_bits);

	switch (code)
	{
	case H:
		fax->stage = STATE_H1;
		break;

	case P:
		b1 = find_changing_color(fax->ref, fax->a, fax->columns, !fax->c);
		if (b1 >= fax->columns)
			b2 = fax->columns;
		else
			b2 = find_changing(fax->ref, b1, fax->columns);
		if (fax->c) setbits(fax->dst, fax->a, b2);
		fax->a = b2;
		break;

	case V0:
		b1 = find_changing_color(fax->ref, fax->a, fax->columns, !fax->c);
		if (fax->c) setbits(fax->dst, fax->a, b1);
		fax->a = b1;
		fax->c = !fax->c;
		break;

	case VR1:
		b1 = 1 + find_changing_color(fax->ref, fax->a, fax->columns, !fax->c);
		if (b1 >= fax->columns) b1 = fax->columns;
		if (fax->c) setbits(fax->dst, fax->a, b1);
		fax->a = b1;
		fax->c = !fax->c;
		break;

	case VR2:
		b1 = 2 + find_changing_color(fax->ref, fax->a, fax->columns, !fax->c);
		if (b1 >= fax->columns) b1 = fax->columns;
		if (fax->c) setbits(fax->dst, fax->a, b1);
		fax->a = b1;
		fax->c = !fax->c;
		break;

	case VR3:
		b1 = 3 + find_changing_color(fax->ref, fax->a, fax->columns, !fax->c);
		if (b1 >= fax->columns) b1 = fax->columns;
		if (fax->c) setbits(fax->dst, fax->a, b1);
		fax->a = b1;
		fax->c = !fax->c;
		break;

	case VL1:
		b1 = -1 + find_changing_color(fax->ref, fax->a, fax->columns, !fax->c);
		if (b1 < 0) b1 = 0;
		if (fax->c) setbits(fax->dst, fax->a, b1);
		fax->a = b1;
		fax->c = !fax->c;
		break;

	case VL2:
		b1 = -2 + find_changing_color(fax->ref, fax->a, fax->columns, !fax->c);
		if (b1 < 0) b1 = 0;
		if (fax->c) setbits(fax->dst, fax->a, b1);
		fax->a = b1;
		fax->c = !fax->c;
		break;

	case VL3:
		b1 = -3 + find_changing_color(fax->ref, fax->a, fax->columns, !fax->c);
		if (b1 < 0) b1 = 0;
		if (fax->c) setbits(fax->dst, fax->a, b1);
		fax->a = b1;
		fax->c = !fax->c;
		break;

	case UNCOMPRESSED:
		base_throw(ctx, "uncompressed data in faxd");

	case ERROR:
		base_throw(ctx, "invalid code in 2d faxd");

	default:
		base_throw(ctx, "invalid code in 2d faxd (%d)", code);
	}
}

static int
read_faxd(base_stream *stm, unsigned char *buf, int len)
{
	base_faxd *fax = stm->state;
	unsigned char *p = buf;
	unsigned char *ep = buf + len;
	unsigned char *tmp;

	if (fax->stage == STATE_DONE)
		return 0;

	if (fax->stage == STATE_EOL)
		goto eol;

loop:

	if (fill_bits(fax))
	{
		if (fax->bidx > 31)
		{
			if (fax->a > 0)
				goto eol;
			goto rtc;
		}
	}

	if ((fax->word >> (32 - 12)) == 0)
	{
		eat_bits(fax, 1);
		goto loop;
	}

	if ((fax->word >> (32 - 12)) == 1)
	{
		eat_bits(fax, 12);
		fax->eolc ++;

		if (fax->k > 0)
		{
			if (fax->a == -1)
				fax->a = 0;
			if ((fax->word >> (32 - 1)) == 1)
				fax->dim = 1;
			else
				fax->dim = 2;
			eat_bits(fax, 1);
		}
	}
	else if (fax->k > 0 && fax->a == -1)
	{
		fax->a = 0;
		if ((fax->word >> (32 - 1)) == 1)
			fax->dim = 1;
		else
			fax->dim = 2;
		eat_bits(fax, 1);
	}
	else if (fax->dim == 1)
	{
		fax->eolc = 0;
		dec1d(stm->ctx, fax);
	}
	else if (fax->dim == 2)
	{
		fax->eolc = 0;
		dec2d(stm->ctx, fax);
	}

	
	if (fax->stage == STATE_MAKEUP || fax->stage == STATE_H1 || fax->stage == STATE_H2)
		goto loop;

	
	if (fax->eolc || fax->a >= fax->columns)
	{
		if (fax->a > 0)
			goto eol;
		if (fax->eolc == (fax->k < 0 ? 2 : 6))
			goto rtc;
	}

	goto loop;

eol:
	fax->stage = STATE_EOL;

	if (fax->black_is_1)
	{
		while (fax->rp < fax->wp && p < ep)
			*p++ = *fax->rp++;
	}
	else
	{
		while (fax->rp < fax->wp && p < ep)
			*p++ = *fax->rp++ ^ 0xff;
	}

	if (fax->rp < fax->wp)
		return p - buf;

	tmp = fax->ref;
	fax->ref = fax->dst;
	fax->dst = tmp;
	memset(fax->dst, 0, fax->stride);

	fax->rp = fax->dst;
	fax->wp = fax->dst + fax->stride;

	fax->stage = STATE_NORMAL;
	fax->c = 0;
	fax->a = -1;
	fax->ridx ++;

	if (!fax->end_of_block && fax->rows)
	{
		if (fax->ridx >= fax->rows)
			goto rtc;
	}

	
	if (fax->k > 0 && !fax->eolc && fax->a == -1)
	{
		if (fax->ridx % fax->k == 0)
			fax->dim = 1;
		else
			fax->dim = 2;
	}

	
	if (fax->encoded_byte_align)
	{
		if (fax->end_of_line)
			eat_bits(fax, (12 - fax->bidx) & 7);
		else
			eat_bits(fax, (8 - fax->bidx) & 7);
	}

	
	if (p == buf + len)
		return p - buf;

	goto loop;

rtc:
	fax->stage = STATE_DONE;
	return p - buf;
}

static void
close_faxd(base_context *ctx, void *state_)
{
	base_faxd *fax = (base_faxd *)state_;
	int i;

	
	i = (32 - fax->bidx) / 8;
	while (i--)
		base_unread_byte(fax->chain);

	base_close(fax->chain);
	base_free(ctx, fax->ref);
	base_free(ctx, fax->dst);
	base_free(ctx, fax);
}

base_stream *
base_open_faxd(base_stream *chain,
	int k, int end_of_line, int encoded_byte_align,
	int columns, int rows, int end_of_block, int black_is_1)
{
	base_context *ctx = chain->ctx;
	base_faxd *fax = NULL;

	base_var(fax);

	base_try(ctx)
	{
		fax = base_malloc_struct(ctx, base_faxd);
		fax->chain = chain;

		fax->ref = NULL;
		fax->dst = NULL;

		fax->k = k;
		fax->end_of_line = end_of_line;
		fax->encoded_byte_align = encoded_byte_align;
		fax->columns = columns;
		fax->rows = rows;
		fax->end_of_block = end_of_block;
		fax->black_is_1 = black_is_1;

		fax->stride = ((fax->columns - 1) >> 3) + 1;
		fax->ridx = 0;
		fax->bidx = 32;
		fax->word = 0;

		fax->stage = STATE_NORMAL;
		fax->a = -1;
		fax->c = 0;
		fax->dim = fax->k < 0 ? 2 : 1;
		fax->eolc = 0;

		fax->ref = base_malloc(ctx, fax->stride);
		fax->dst = base_malloc(ctx, fax->stride);
		fax->rp = fax->dst;
		fax->wp = fax->dst + fax->stride;

		memset(fax->ref, 0, fax->stride);
		memset(fax->dst, 0, fax->stride);
	}
	base_catch(ctx)
	{
		if (fax)
		{
			base_free(ctx, fax->dst);
			base_free(ctx, fax->ref);
		}
		base_free(ctx, fax);
		base_close(chain);
		base_rethrow(ctx);
	}

	return base_new_stream(ctx, fax, read_faxd, close_faxd);
}
