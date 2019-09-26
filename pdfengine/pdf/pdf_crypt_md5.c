

#include "pdf-internal.h"

enum
{
	S11 = 7, S12 = 12, S13 = 17, S14 = 22,
	S21 = 5, S22 = 9, S23 = 14, S24 = 20,
	S31 = 4, S32 = 11, S33 = 16, S34 = 23,
	S41 = 6, S42 = 10, S43 = 15, S44 = 21
};

static void encode(unsigned char *, const unsigned int *, const unsigned);
static void decode(unsigned int *, const unsigned char *, const unsigned);
static void transform(unsigned int state[4], const unsigned char block[64]);

static unsigned char padding[64] =
{
	0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

#define F(x, y, z) (((x) & (y)) | ((~x) & (z)))
#define G(x, y, z) (((x) & (z)) | ((y) & (~z)))
#define H(x, y, z) ((x) ^ (y) ^ (z))
#define I(x, y, z) ((y) ^ ((x) | (~z)))

#define ROTATE(x, n) (((x) << (n)) | ((x) >> (32-(n))))

#define FF(a, b, c, d, x, s, ac) { \
	(a) += F ((b), (c), (d)) + (x) + (unsigned int)(ac); \
	(a) = ROTATE ((a), (s)); \
	(a) += (b); \
	}
#define GG(a, b, c, d, x, s, ac) { \
	(a) += G ((b), (c), (d)) + (x) + (unsigned int)(ac); \
	(a) = ROTATE ((a), (s)); \
	(a) += (b); \
	}
#define HH(a, b, c, d, x, s, ac) { \
	(a) += H ((b), (c), (d)) + (x) + (unsigned int)(ac); \
	(a) = ROTATE ((a), (s)); \
	(a) += (b); \
	}
#define II(a, b, c, d, x, s, ac) { \
	(a) += I ((b), (c), (d)) + (x) + (unsigned int)(ac); \
	(a) = ROTATE ((a), (s)); \
	(a) += (b); \
	}

static void encode(unsigned char *output, const unsigned int *input, const unsigned len)
{
	unsigned i, j;

	for (i = 0, j = 0; j < len; i++, j += 4)
	{
		output[j] = (unsigned char)(input[i] & 0xff);
		output[j+1] = (unsigned char)((input[i] >> 8) & 0xff);
		output[j+2] = (unsigned char)((input[i] >> 16) & 0xff);
		output[j+3] = (unsigned char)((input[i] >> 24) & 0xff);
	}
}

static void decode(unsigned int *output, const unsigned char *input, const unsigned len)
{
	unsigned i, j;

	for (i = 0, j = 0; j < len; i++, j += 4)
	{
		output[i] = ((unsigned int)input[j]) |
		(((unsigned int)input[j+1]) << 8) |
		(((unsigned int)input[j+2]) << 16) |
		(((unsigned int)input[j+3]) << 24);
	}
}

static void transform(unsigned int state[4], const unsigned char block[64])
{
	unsigned int a = state[0];
	unsigned int b = state[1];
	unsigned int c = state[2];
	unsigned int d = state[3];
	unsigned int x[16];

	decode(x, block, 64);

	
	FF (a, b, c, d, x[ 0], S11, 0xd76aa478); 
	FF (d, a, b, c, x[ 1], S12, 0xe8c7b756); 
	FF (c, d, a, b, x[ 2], S13, 0x242070db); 
	FF (b, c, d, a, x[ 3], S14, 0xc1bdceee); 
	FF (a, b, c, d, x[ 4], S11, 0xf57c0faf); 
	FF (d, a, b, c, x[ 5], S12, 0x4787c62a); 
	FF (c, d, a, b, x[ 6], S13, 0xa8304613); 
	FF (b, c, d, a, x[ 7], S14, 0xfd469501); 
	FF (a, b, c, d, x[ 8], S11, 0x698098d8); 
	FF (d, a, b, c, x[ 9], S12, 0x8b44f7af); 
	FF (c, d, a, b, x[10], S13, 0xffff5bb1); 
	FF (b, c, d, a, x[11], S14, 0x895cd7be); 
	FF (a, b, c, d, x[12], S11, 0x6b901122); 
	FF (d, a, b, c, x[13], S12, 0xfd987193); 
	FF (c, d, a, b, x[14], S13, 0xa679438e); 
	FF (b, c, d, a, x[15], S14, 0x49b40821); 

	
	GG (a, b, c, d, x[ 1], S21, 0xf61e2562); 
	GG (d, a, b, c, x[ 6], S22, 0xc040b340); 
	GG (c, d, a, b, x[11], S23, 0x265e5a51); 
	GG (b, c, d, a, x[ 0], S24, 0xe9b6c7aa); 
	GG (a, b, c, d, x[ 5], S21, 0xd62f105d); 
	GG (d, a, b, c, x[10], S22, 0x02441453); 
	GG (c, d, a, b, x[15], S23, 0xd8a1e681); 
	GG (b, c, d, a, x[ 4], S24, 0xe7d3fbc8); 
	GG (a, b, c, d, x[ 9], S21, 0x21e1cde6); 
	GG (d, a, b, c, x[14], S22, 0xc33707d6); 
	GG (c, d, a, b, x[ 3], S23, 0xf4d50d87); 
	GG (b, c, d, a, x[ 8], S24, 0x455a14ed); 
	GG (a, b, c, d, x[13], S21, 0xa9e3e905); 
	GG (d, a, b, c, x[ 2], S22, 0xfcefa3f8); 
	GG (c, d, a, b, x[ 7], S23, 0x676f02d9); 
	GG (b, c, d, a, x[12], S24, 0x8d2a4c8a); 

	
	HH (a, b, c, d, x[ 5], S31, 0xfffa3942); 
	HH (d, a, b, c, x[ 8], S32, 0x8771f681); 
	HH (c, d, a, b, x[11], S33, 0x6d9d6122); 
	HH (b, c, d, a, x[14], S34, 0xfde5380c); 
	HH (a, b, c, d, x[ 1], S31, 0xa4beea44); 
	HH (d, a, b, c, x[ 4], S32, 0x4bdecfa9); 
	HH (c, d, a, b, x[ 7], S33, 0xf6bb4b60); 
	HH (b, c, d, a, x[10], S34, 0xbebfbc70); 
	HH (a, b, c, d, x[13], S31, 0x289b7ec6); 
	HH (d, a, b, c, x[ 0], S32, 0xeaa127fa); 
	HH (c, d, a, b, x[ 3], S33, 0xd4ef3085); 
	HH (b, c, d, a, x[ 6], S34, 0x04881d05); 
	HH (a, b, c, d, x[ 9], S31, 0xd9d4d039); 
	HH (d, a, b, c, x[12], S32, 0xe6db99e5); 
	HH (c, d, a, b, x[15], S33, 0x1fa27cf8); 
	HH (b, c, d, a, x[ 2], S34, 0xc4ac5665); 

	
	II (a, b, c, d, x[ 0], S41, 0xf4292244); 
	II (d, a, b, c, x[ 7], S42, 0x432aff97); 
	II (c, d, a, b, x[14], S43, 0xab9423a7); 
	II (b, c, d, a, x[ 5], S44, 0xfc93a039); 
	II (a, b, c, d, x[12], S41, 0x655b59c3); 
	II (d, a, b, c, x[ 3], S42, 0x8f0ccc92); 
	II (c, d, a, b, x[10], S43, 0xffeff47d); 
	II (b, c, d, a, x[ 1], S44, 0x85845dd1); 
	II (a, b, c, d, x[ 8], S41, 0x6fa87e4f); 
	II (d, a, b, c, x[15], S42, 0xfe2ce6e0); 
	II (c, d, a, b, x[ 6], S43, 0xa3014314); 
	II (b, c, d, a, x[13], S44, 0x4e0811a1); 
	II (a, b, c, d, x[ 4], S41, 0xf7537e82); 
	II (d, a, b, c, x[11], S42, 0xbd3af235); 
	II (c, d, a, b, x[ 2], S43, 0x2ad7d2bb); 
	II (b, c, d, a, x[ 9], S44, 0xeb86d391); 

	state[0] += a;
	state[1] += b;
	state[2] += c;
	state[3] += d;

	
	memset(x, 0, sizeof (x));
}

void base_md5_init(base_md5 *context)
{
	context->count[0] = context->count[1] = 0;

	
	context->state[0] = 0x67452301;
	context->state[1] = 0xefcdab89;
	context->state[2] = 0x98badcfe;
	context->state[3] = 0x10325476;
}

void base_md5_update(base_md5 *context, const unsigned char *input, unsigned inlen)
{
	unsigned i, index, partlen;

	
	index = (unsigned)((context->count[0] >> 3) & 0x3F);

	
	context->count[0] += (unsigned int) inlen << 3;
	if (context->count[0] < (unsigned int) inlen << 3)
		context->count[1] ++;
	context->count[1] += (unsigned int) inlen >> 29;

	partlen = 64 - index;

	
	if (inlen >= partlen)
	{
		memcpy(context->buffer + index, input, partlen);
		transform(context->state, context->buffer);

		for (i = partlen; i + 63 < inlen; i += 64)
			transform(context->state, input + i);

		index = 0;
	}
	else
	{
		i = 0;
	}

	
	memcpy(context->buffer + index, input + i, inlen - i);
}

void base_md5_final(base_md5 *context, unsigned char digest[16])
{
	unsigned char bits[8];
	unsigned index, padlen;

	
	encode(bits, context->count, 8);

	
	index = (unsigned)((context->count[0] >> 3) & 0x3f);
	padlen = index < 56 ? 56 - index : 120 - index;
	base_md5_update(context, padding, padlen);

	
	base_md5_update(context, bits, 8);

	
	encode(digest, context->state, 16);

	
	memset(context, 0, sizeof(base_md5));
}
