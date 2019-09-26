

#include "pdf-internal.h"

void
base_arc4_init(base_arc4 *arc4, const unsigned char *key, unsigned keylen)
{
	unsigned int t, u;
	unsigned int keyindex;
	unsigned int stateindex;
	unsigned char *state;
	unsigned int counter;

	state = arc4->state;

	arc4->x = 0;
	arc4->y = 0;

	for (counter = 0; counter < 256; counter++)
	{
		state[counter] = counter;
	}

	keyindex = 0;
	stateindex = 0;

	for (counter = 0; counter < 256; counter++)
	{
		t = state[counter];
		stateindex = (stateindex + key[keyindex] + t) & 0xff;
		u = state[stateindex];

		state[stateindex] = t;
		state[counter] = u;

		if (++keyindex >= keylen)
		{
			keyindex = 0;
		}
	}
}

static unsigned char
base_arc4_next(base_arc4 *arc4)
{
	unsigned int x;
	unsigned int y;
	unsigned int sx, sy;
	unsigned char *state;

	state = arc4->state;

	x = (arc4->x + 1) & 0xff;
	sx = state[x];
	y = (sx + arc4->y) & 0xff;
	sy = state[y];

	arc4->x = x;
	arc4->y = y;

	state[y] = sx;
	state[x] = sy;

	return state[(sx + sy) & 0xff];
}

void
base_arc4_encrypt(base_arc4 *arc4, unsigned char *dest, const unsigned char *src, unsigned len)
{
	unsigned int i;
	for (i = 0; i < len; i++)
	{
		unsigned char x;
		x = base_arc4_next(arc4);
		dest[i] = src[i] ^ x;
	}
}
