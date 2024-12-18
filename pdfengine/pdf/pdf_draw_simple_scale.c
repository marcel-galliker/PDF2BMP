

#include "pdf-internal.h"

#define SINGLE_PIXEL_SPECIALS

#ifdef ARCH_ARM
#ifdef ARCH_THUMB
#define ENTER_ARM ".balign 4\nmov r12,pc\nbx r12\n0:.arm\n"
#define ENTER_THUMB "9:.thumb\n"
#else
#define ENTER_ARM
#define ENTER_THUMB
#endif
#endif

#ifdef DEBUG_SCALING
#ifdef WIN32
#include <windows.h>
static void debug_print(const char *fmt, ...)
{
	va_list args;
	char text[256];
	va_start(args, fmt);
	vsprintf(text, fmt, args);
	va_end(args);
	OutputDebugStringA(text);
	printf(text);
}
#else
static void debug_print(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
}
#endif
#endif
#ifdef DEBUG_SCALING
#define DBUG(A) debug_print A
#else
#define DBUG(A) do {} while(0==1)
#endif

typedef struct base_scale_filter_s base_scale_filter;

struct base_scale_filter_s
{
	int width;
	float (*fn)(base_scale_filter *, float);
};

static float
triangle(base_scale_filter *filter, float f)
{
	if (f >= 1)
		return 0;
	return 1-f;
}

static float
box(base_scale_filter *filter, float f)
{
	if (f >= 0.5f)
		return 0;
	return 1;
}

static float
simple(base_scale_filter *filter, float x)
{
	if (x >= 1)
		return 0;
	return 1 + (2*x - 3)*x*x;
}

base_scale_filter base_scale_filter_box = { 1, box };
base_scale_filter base_scale_filter_triangle = { 1, triangle };
base_scale_filter base_scale_filter_simple = { 1, simple };

typedef struct base_weights_s base_weights;

struct base_weights_s
{
	int flip;	
	int count;	
	int max_len;	
	int n;		
	int new_line;	
	int patch_l;	
	int index[1];
};

static base_weights *
new_weights(base_context *ctx, base_scale_filter *filter, int src_w, float dst_w, int patch_w, int n, int flip, int patch_l)
{
	int max_len;
	base_weights *weights;

	if (src_w > dst_w)
	{
		
		max_len = (int)ceilf((2 * filter->width * src_w)/dst_w);
		if (max_len > src_w)
			max_len = src_w;
	}
	else
	{
		
		max_len = 2 * filter->width;
	}
	
	weights = base_malloc(ctx, sizeof(*weights)+(max_len+3)*(patch_w+1)*sizeof(int));
	if (!weights)
		return NULL;
	weights->count = -1;
	weights->max_len = max_len;
	weights->index[0] = patch_w;
	weights->n = n;
	weights->patch_l = patch_l;
	weights->flip = flip;
	return weights;
}

static void
init_weights(base_weights *weights, int j)
{
	int index;

	j -= weights->patch_l;
	assert(weights->count == j-1);
	weights->count++;
	weights->new_line = 1;
	if (j == 0)
		index = weights->index[0];
	else
	{
		index = weights->index[j-1];
		index += 2 + weights->index[index+1];
	}
	weights->index[j] = index; 
	weights->index[index] = 0; 
	weights->index[index+1] = 0; 
}

static void
add_weight(base_weights *weights, int j, int i, base_scale_filter *filter,
	float x, float F, float G, int src_w, float dst_w)
{
	float dist = j - x + 0.5f - ((i + 0.5f)*dst_w/src_w);
	float f;
	int min, len, index, weight;

	dist *= G;
	if (dist < 0)
		dist = -dist;
	f = filter->fn(filter, dist)*F;
	weight = (int)(256*f+0.5f);

	
	if (i < 0)
	{
		i = 0;
		return;
	}
	else if (i >= src_w)
	{
		i = src_w-1;
		return;
	}
	if (weight == 0)
	{
		
		if (weights->new_line && f > 0)
			weight = 1;
		else
			return;
	}

	DBUG(("add_weight[%d][%d] = %d(%g) dist=%g\n",j,i,weight,f,dist));

	
	j -= weights->patch_l;
	if (weights->new_line)
	{
		
		weights->new_line = 0;
		index = weights->index[j]; 
		weights->index[index] = i; 
		weights->index[index+1] = 0; 
	}
	index = weights->index[j];
	min = weights->index[index++];
	len = weights->index[index++];
	while (i < min)
	{
		
		int k;

		for (k = len; k > 0; k--)
		{
			weights->index[index+k] = weights->index[index+k-1];
		}
		weights->index[index] = 0;
		min--;
		len++;
		weights->index[index-2] = min;
		weights->index[index-1] = len;
	}
	if (i-min >= len)
	{
		
		while (i-min >= ++len)
		{
			weights->index[index+len-1] = 0;
		}
		assert(len-1 == i-min);
		weights->index[index+i-min] = weight;
		weights->index[index-1] = len;
		assert(len <= weights->max_len);
	}
	else
	{
		
		weights->index[index+i-min] += weight;
	}
}

static void
reorder_weights(base_weights *weights, int j, int src_w)
{
	int idx = weights->index[j - weights->patch_l];
	int min = weights->index[idx++];
	int len = weights->index[idx++];
	int max = weights->max_len;
	int tmp = idx+max;
	int i, off;

	
	memcpy(&weights->index[tmp], &weights->index[idx], sizeof(int)*len);

	
	assert(len <= max);
	assert(min+len <= src_w);
	off = 0;
	if (len < max)
	{
		memset(&weights->index[tmp+len], 0, sizeof(int)*(max-len));
		len = max;
		if (min + len > src_w)
		{
			off = min + len - src_w;
			min = src_w - len;
			weights->index[idx-2] = min;
		}
		weights->index[idx-1] = len;
	}

	
	for (i = 0; i < len; i++)
	{
		weights->index[idx+((min+i+off) % max)] = weights->index[tmp+i];
	}
}

static void
check_weights(base_weights *weights, int j, int w, float x, float wf)
{
	int idx, len;
	int sum = 0;
	int max = -256;
	int maxidx = 0;
	int i;

	idx = weights->index[j - weights->patch_l];
	idx++; 
	len = weights->index[idx++];

	for(i=0; i < len; i++)
	{
		int v = weights->index[idx++];
		sum += v;
		if (v > max)
		{
			max = v;
			maxidx = idx;
		}
	}
	
	if (((j != 0) && (j != w-1)) || (sum > 256))
		weights->index[maxidx-1] += 256-sum;
	
	else if ((j == 0) && (x < 0.0001F) && (sum != 256))
		weights->index[maxidx-1] += 256-sum;
	
	else if ((j == w-1) && ((float)w-wf < 0.0001F) && (sum != 256))
		weights->index[maxidx-1] += 256-sum;
	DBUG(("total weight %d = %d\n", j, sum));
}

static base_weights *
make_weights(base_context *ctx, int src_w, float x, float dst_w, base_scale_filter *filter, int vertical, int dst_w_int, int patch_l, int patch_r, int n, int flip)
{
	base_weights *weights;
	float F, G;
	float window;
	int j;

	if (dst_w < src_w)
	{
		
		F = dst_w / src_w;
		G = 1;
	}
	else
	{
		
		F = 1;
		G = src_w / dst_w;
	}
	window = filter->width / F;
	DBUG(("make_weights src_w=%d x=%g dst_w=%g patch_l=%d patch_r=%d F=%g window=%g\n", src_w, x, dst_w, patch_l, patch_r, F, window));
	weights	= new_weights(ctx, filter, src_w, dst_w, patch_r-patch_l, n, flip, patch_l);
	if (!weights)
		return NULL;
	for (j = patch_l; j < patch_r; j++)
	{
		
		float centre = (j - x + 0.5f)*src_w/dst_w - 0.5f;
		int l, r;
		l = ceilf(centre - window);
		r = floorf(centre + window);
		DBUG(("%d: centre=%g l=%d r=%d\n", j, centre, l, r));
		init_weights(weights, j);
		for (; l <= r; l++)
		{
			add_weight(weights, j, l, filter, x, F, G, src_w, dst_w);
		}
		check_weights(weights, j, dst_w_int, x, dst_w);
		if (vertical)
		{
			reorder_weights(weights, j, src_w);
		}
	}
	weights->count++; 
	return weights;
}

static void
scale_row_to_temp(unsigned char *dst, unsigned char *src, base_weights *weights)
{
	int *contrib = &weights->index[weights->index[0]];
	int len, i, j, n;
	unsigned char *min;
	int tmp[base_MAX_COLORS];
	int *t = tmp;

	n = weights->n;
	for (j = 0; j < n; j++)
		tmp[j] = 128;
	if (weights->flip)
	{
		dst += (weights->count-1)*n;
		for (i=weights->count; i > 0; i--)
		{
			min = &src[n * *contrib++];
			len = *contrib++;
			while (len-- > 0)
			{
				for (j = n; j > 0; j--)
					*t++ += *min++ * *contrib;
				t -= n;
				contrib++;
			}
			for (j = n; j > 0; j--)
			{
				*dst++ = (unsigned char)(*t>>8);
				*t++ = 128;
			}
			t -= n;
			dst -= n*2;
		}
	}
	else
	{
		for (i=weights->count; i > 0; i--)
		{
			min = &src[n * *contrib++];
			len = *contrib++;
			while (len-- > 0)
			{
				for (j = n; j > 0; j--)
					*t++ += *min++ * *contrib;
				t -= n;
				contrib++;
			}
			for (j = n; j > 0; j--)
			{
				*dst++ = (unsigned char)(*t>>8);
				*t++ = 128;
			}
			t -= n;
		}
	}
}

#ifdef ARCH_ARM

static void
scale_row_to_temp1(unsigned char *dst, unsigned char *src, base_weights *weights)
__attribute__((naked));

static void
scale_row_to_temp2(unsigned char *dst, unsigned char *src, base_weights *weights)
__attribute__((naked));

static void
scale_row_to_temp4(unsigned char *dst, unsigned char *src, base_weights *weights)
__attribute__((naked));

static void
scale_row_from_temp(unsigned char *dst, unsigned char *src, base_weights *weights, int width, int row)
__attribute__((naked));

static void
scale_row_to_temp1(unsigned char *dst, unsigned char *src, base_weights *weights)
{
	
	asm volatile(
	ENTER_ARM
	"stmfd	r13!,{r4-r5,r9,r14}				\n"
	"@ r0 = dst						\n"
	"@ r1 = src						\n"
	"@ r2 = weights						\n"
	"ldr	r12,[r2],#4		@ r12= flip		\n"
	"ldr	r3, [r2],#20		@ r3 = count r2 = &index\n"
	"ldr	r4, [r2]		@ r4 = index[0]		\n"
	"cmp	r12,#0			@ if (flip)		\n"
	"beq	4f			@ {			\n"
	"add	r2, r2, r4, LSL #2	@ r2 = &index[index[0]] \n"
	"add	r0, r0, r3		@ dst += count		\n"
	"1:							\n"
	"ldr	r4, [r2], #4		@ r4 = *contrib++	\n"
	"ldr	r9, [r2], #4		@ r9 = len = *contrib++	\n"
	"mov	r5, #128		@ r5 = a = 128		\n"
	"add	r4, r1, r4		@ r4 = min = &src[r4]	\n"
	"cmp	r9, #0			@ while (len-- > 0)	\n"
	"beq	3f			@ {			\n"
	"2:							\n"
	"ldr	r12,[r2], #4		@ r12 = *contrib++	\n"
	"ldrb	r14,[r4], #1		@ r14 = *min++		\n"
	"subs	r9, r9, #1		@ r9 = len--		\n"
	"@stall on r14						\n"
	"mla	r5, r12,r14,r5		@ g += r14 * r12	\n"
	"bgt	2b			@ }			\n"
	"3:							\n"
	"mov	r5, r5, lsr #8		@ g >>= 8		\n"
	"strb	r5,[r0, #-1]!		@ *--dst=a		\n"
	"subs	r3, r3, #1		@ i--			\n"
	"bgt	1b			@ 			\n"
	"ldmfd	r13!,{r4-r5,r9,PC}	@ pop, return to thumb	\n"
	"4:"
	"add	r2, r2, r4, LSL #2	@ r2 = &index[index[0]] \n"
	"5:"
	"ldr	r4, [r2], #4		@ r4 = *contrib++	\n"
	"ldr	r9, [r2], #4		@ r9 = len = *contrib++	\n"
	"mov	r5, #128		@ r5 = a = 128		\n"
	"add	r4, r1, r4		@ r4 = min = &src[r4]	\n"
	"cmp	r9, #0			@ while (len-- > 0)	\n"
	"beq	7f			@ {			\n"
	"6:							\n"
	"ldr	r12,[r2], #4		@ r12 = *contrib++	\n"
	"ldrb	r14,[r4], #1		@ r14 = *min++		\n"
	"subs	r9, r9, #1		@ r9 = len--		\n"
	"@stall on r14						\n"
	"mla	r5, r12,r14,r5		@ a += r14 * r12	\n"
	"bgt	6b			@ }			\n"
	"7:							\n"
	"mov	r5, r5, LSR #8		@ a >>= 8		\n"
	"strb	r5, [r0], #1		@ *dst++=a		\n"
	"subs	r3, r3, #1		@ i--			\n"
	"bgt	5b			@ 			\n"
	"ldmfd	r13!,{r4-r5,r9,PC}	@ pop, return to thumb	\n"
	ENTER_THUMB
	);
}

static void
scale_row_to_temp2(unsigned char *dst, unsigned char *src, base_weights *weights)
{
	asm volatile(
	ENTER_ARM
	"stmfd	r13!,{r4-r6,r9-r11,r14}				\n"
	"@ r0 = dst						\n"
	"@ r1 = src						\n"
	"@ r2 = weights						\n"
	"ldr	r12,[r2],#4		@ r12= flip		\n"
	"ldr	r3, [r2],#20		@ r3 = count r2 = &index\n"
	"ldr	r4, [r2]		@ r4 = index[0]		\n"
	"cmp	r12,#0			@ if (flip)		\n"
	"beq	4f			@ {			\n"
	"add	r2, r2, r4, LSL #2	@ r2 = &index[index[0]] \n"
	"add	r0, r0, r3, LSL #1	@ dst += 2*count	\n"
	"1:							\n"
	"ldr	r4, [r2], #4		@ r4 = *contrib++	\n"
	"ldr	r9, [r2], #4		@ r9 = len = *contrib++	\n"
	"mov	r5, #128		@ r5 = g = 128		\n"
	"mov	r6, #128		@ r6 = a = 128		\n"
	"add	r4, r1, r4, LSL #1	@ r4 = min = &src[2*r4]	\n"
	"cmp	r9, #0			@ while (len-- > 0)	\n"
	"beq	3f			@ {			\n"
	"2:							\n"
	"ldr	r14,[r2], #4		@ r14 = *contrib++	\n"
	"ldrb	r11,[r4], #1		@ r11 = *min++		\n"
	"ldrb	r12,[r4], #1		@ r12 = *min++		\n"
	"subs	r9, r9, #1		@ r9 = len--		\n"
	"mla	r5, r14,r11,r5		@ g += r11 * r14	\n"
	"mla	r6, r14,r12,r6		@ a += r12 * r14	\n"
	"bgt	2b			@ }			\n"
	"3:							\n"
	"mov	r5, r5, lsr #8		@ g >>= 8		\n"
	"mov	r6, r6, lsr #8		@ a >>= 8		\n"
	"strb	r5, [r0, #-2]!		@ *--dst=a		\n"
	"strb	r6, [r0, #1]		@ *--dst=g		\n"
	"subs	r3, r3, #1		@ i--			\n"
	"bgt	1b			@ 			\n"
	"ldmfd	r13!,{r4-r6,r9-r11,PC}	@ pop, return to thumb	\n"
	"4:"
	"add	r2, r2, r4, LSL #2	@ r2 = &index[index[0]] \n"
	"5:"
	"ldr	r4, [r2], #4		@ r4 = *contrib++	\n"
	"ldr	r9, [r2], #4		@ r9 = len = *contrib++	\n"
	"mov	r5, #128		@ r5 = g = 128		\n"
	"mov	r6, #128		@ r6 = a = 128		\n"
	"add	r4, r1, r4, LSL #1	@ r4 = min = &src[2*r4]	\n"
	"cmp	r9, #0			@ while (len-- > 0)	\n"
	"beq	7f			@ {			\n"
	"6:							\n"
	"ldr	r14,[r2], #4		@ r10 = *contrib++	\n"
	"ldrb	r11,[r4], #1		@ r11 = *min++		\n"
	"ldrb	r12,[r4], #1		@ r12 = *min++		\n"
	"subs	r9, r9, #1		@ r9 = len--		\n"
	"mla	r5, r14,r11,r5		@ g += r11 * r14	\n"
	"mla	r6, r14,r12,r6		@ a += r12 * r14	\n"
	"bgt	6b			@ }			\n"
	"7:							\n"
	"mov	r5, r5, lsr #8		@ g >>= 8		\n"
	"mov	r6, r6, lsr #8		@ a >>= 8		\n"
	"strb	r5, [r0], #1		@ *dst++=g		\n"
	"strb	r6, [r0], #1		@ *dst++=a		\n"
	"subs	r3, r3, #1		@ i--			\n"
	"bgt	5b			@ 			\n"
	"ldmfd	r13!,{r4-r6,r9-r11,PC}	@ pop, return to thumb	\n"
	ENTER_THUMB
	);
}

static void
scale_row_to_temp4(unsigned char *dst, unsigned char *src, base_weights *weights)
{
	asm volatile(
	ENTER_ARM
	"stmfd	r13!,{r4-r11,r14}				\n"
	"@ r0 = dst						\n"
	"@ r1 = src						\n"
	"@ r2 = weights						\n"
	"ldr	r12,[r2],#4		@ r12= flip		\n"
	"ldr	r3, [r2],#20		@ r3 = count r2 = &index\n"
	"ldr	r4, [r2]		@ r4 = index[0]		\n"
	"ldr	r5,=0x00800080		@ r5 = rounding		\n"
	"ldr	r6,=0x00FF00FF		@ r7 = 0x00FF00FF	\n"
	"cmp	r12,#0			@ if (flip)		\n"
	"beq	4f			@ {			\n"
	"add	r2, r2, r4, LSL #2	@ r2 = &index[index[0]] \n"
	"add	r0, r0, r3, LSL #2	@ dst += 4*count	\n"
	"1:							\n"
	"ldr	r4, [r2], #4		@ r4 = *contrib++	\n"
	"ldr	r9, [r2], #4		@ r9 = len = *contrib++	\n"
	"mov	r7, r5			@ r7 = b = rounding	\n"
	"mov	r8, r5			@ r8 = a = rounding	\n"
	"add	r4, r1, r4, LSL #2	@ r4 = min = &src[4*r4]	\n"
	"cmp	r9, #0			@ while (len-- > 0)	\n"
	"beq	3f			@ {			\n"
	"2:							\n"
	"ldr	r11,[r4], #4		@ r11 = *min++		\n"
	"ldr	r10,[r2], #4		@ r10 = *contrib++	\n"
	"subs	r9, r9, #1		@ r9 = len--		\n"
	"and	r12,r6, r11		@ r12 = __22__00	\n"
	"and	r11,r6, r11,LSR #8	@ r11 = __33__11	\n"
	"mla	r7, r10,r12,r7		@ b += r14 * r10	\n"
	"mla	r8, r10,r11,r8		@ a += r11 * r10	\n"
	"bgt	2b			@ }			\n"
	"3:							\n"
	"and	r7, r6, r7, lsr #8	@ r7 = __22__00		\n"
	"bic	r8, r8, r6		@ r8 = 33__11__		\n"
	"orr	r7, r7, r8		@ r7 = 33221100		\n"
	"str	r7, [r0, #-4]!		@ *--dst=r		\n"
	"subs	r3, r3, #1		@ i--			\n"
	"bgt	1b			@ 			\n"
	"ldmfd	r13!,{r4-r11,PC}	@ pop, return to thumb	\n"
	"4:							\n"
	"add	r2, r2, r4, LSL #2	@ r2 = &index[index[0]] \n"
	"5:							\n"
	"ldr	r4, [r2], #4		@ r4 = *contrib++	\n"
	"ldr	r9, [r2], #4		@ r9 = len = *contrib++	\n"
	"mov	r7, r5			@ r7 = b = rounding	\n"
	"mov	r8, r5			@ r8 = a = rounding	\n"
	"add	r4, r1, r4, LSL #2	@ r4 = min = &src[4*r4]	\n"
	"cmp	r9, #0			@ while (len-- > 0)	\n"
	"beq	7f			@ {			\n"
	"6:							\n"
	"ldr	r11,[r4], #4		@ r11 = *min++		\n"
	"ldr	r10,[r2], #4		@ r10 = *contrib++	\n"
	"subs	r9, r9, #1		@ r9 = len--		\n"
	"and	r12,r6, r11		@ r12 = __22__00	\n"
	"and	r11,r6, r11,LSR #8	@ r11 = __33__11	\n"
	"mla	r7, r10,r12,r7		@ b += r14 * r10	\n"
	"mla	r8, r10,r11,r8		@ a += r11 * r10	\n"
	"bgt	6b			@ }			\n"
	"7:							\n"
	"and	r7, r6, r7, lsr #8	@ r7 = __22__00		\n"
	"bic	r8, r8, r6		@ r8 = 33__11__		\n"
	"orr	r7, r7, r8		@ r7 = 33221100		\n"
	"str	r7, [r0], #4		@ *dst++=r		\n"
	"subs	r3, r3, #1		@ i--			\n"
	"bgt	5b			@ 			\n"
	"ldmfd	r13!,{r4-r11,PC}	@ pop, return to thumb	\n"
	ENTER_THUMB
	);
}

static void
scale_row_from_temp(unsigned char *dst, unsigned char *src, base_weights *weights, int width, int row)
{
	asm volatile(
	ENTER_ARM
	"ldr	r12,[r13]		@ r12= row		\n"
	"add	r2, r2, #24		@ r2 = weights->index	\n"
	"stmfd	r13!,{r4-r11,r14}				\n"
	"@ r0 = dst						\n"
	"@ r1 = src						\n"
	"@ r2 = &weights->index[0]				\n"
	"@ r3 = width						\n"
	"@ r12= row						\n"
	"ldr	r4, [r2, r12, LSL #2]	@ r4 = index[row]	\n"
	"add	r2, r2, #4		@ r2 = &index[1]	\n"
	"subs	r6, r3, #4		@ r6 = x = width-4	\n"
	"ldr	r14,[r2, r4, LSL #2]!	@ r2 = contrib = index[index[row]+1]\n"
	"				@ r14= len = *contrib	\n"
	"blt	4f			@ while (x >= 0) {	\n"
#ifndef ARCH_ARM_CAN_LOAD_UNALIGNED
	"tst	r3, #3			@ if (r3 & 3)		\n"
	"blt	4f			@ can't do fast code	\n"
#endif
	"ldr	r9, =0x00FF00FF		@ r9 = 0x00FF00FF	\n"
	"1:							\n"
	"ldr	r5, =0x00800080		@ r5 = val0 = round	\n"
	"stmfd	r13!,{r1,r2}		@ stash r1,r2,r14	\n"
	"				@ r1 = min = src	\n"
	"				@ r2 = contrib2-4	\n"
	"movs	r8, r14			@ r8 = len2 = len	\n"
	"mov	r7, r5			@ r7 = val1 = round	\n"
	"ble	3f			@ while (len2-- > 0) {	\n"
	"2:							\n"
	"ldr	r12,[r1], r3		@ r12 = *min	r5 = min += width\n"
	"ldr	r10,[r2, #4]!		@ r10 = *contrib2++	\n"
	"subs	r8, r8, #1		@ len2--		\n"
	"and	r11,r9, r12		@ r11= __22__00		\n"
	"and	r12,r9, r12,LSR #8	@ r12= __33__11		\n"
	"mla	r5, r10,r11,r5		@ r5 = val0 += r11 * r10\n"
	"mla	r7, r10,r12,r7		@ r7 = val1 += r12 * r10\n"
	"bgt	2b			@ }			\n"
	"3:							\n"
	"ldmfd	r13!,{r1,r2}		@ restore r1,r2,r14	\n"
	"and	r5, r9, r5, LSR #8	@ r5 = __22__00		\n"
	"and	r7, r7, r9, LSL #8	@ r7 = 33__11__		\n"
	"orr	r5, r5, r7		@ r5 = 33221100		\n"
	"subs	r6, r6, #4		@ x--			\n"
	"add	r1, r1, #4		@ src++			\n"
	"str	r5, [r0], #4		@ *dst++ = val		\n"
	"bge	1b			@ 			\n"
	"4:				@ } (Less than 4 to go)	\n"
	"adds	r6, r6, #4		@ r6 = x += 4		\n"
	"beq	8f			@ if (x == 0) done	\n"
	"5:							\n"
	"mov	r5, r1			@ r5 = min = src	\n"
	"mov	r7, #128		@ r7 = val = 128	\n"
	"movs	r8, r14			@ r8 = len2 = len	\n"
	"add	r9, r2, #4		@ r9 = contrib2		\n"
	"ble	7f			@ while (len2-- > 0) {	\n"
	"6:							\n"
	"ldr	r10,[r9], #4		@ r10 = *contrib2++	\n"
	"ldrb	r12,[r5], r3		@ r12 = *min	r5 = min += width\n"
	"subs	r8, r8, #1		@ len2--		\n"
	"@ stall r12						\n"
	"mla	r7, r10,r12,r7		@ val += r12 * r10	\n"
	"bgt	6b			@ }			\n"
	"7:							\n"
	"mov	r7, r7, asr #8		@ r7 = val >>= 8	\n"
	"subs	r6, r6, #1		@ x--			\n"
	"add	r1, r1, #1		@ src++			\n"
	"strb	r7, [r0], #1		@ *dst++ = val		\n"
	"bgt	5b			@ 			\n"
	"8:							\n"
	"ldmfd	r13!,{r4-r11,PC}	@ pop, return to thumb	\n"
	ENTER_THUMB
	);
}
#else

static void
scale_row_to_temp1(unsigned char *dst, unsigned char *src, base_weights *weights)
{
	int *contrib = &weights->index[weights->index[0]];
	int len, i;
	unsigned char *min;

	assert(weights->n == 1);
	if (weights->flip)
	{
		dst += weights->count;
		for (i=weights->count; i > 0; i--)
		{
			int val = 128;
			min = &src[*contrib++];
			len = *contrib++;
			while (len-- > 0)
			{
				val += *min++ * *contrib++;
			}
			*--dst = (unsigned char)(val>>8);
		}
	}
	else
	{
		for (i=weights->count; i > 0; i--)
		{
			int val = 128;
			min = &src[*contrib++];
			len = *contrib++;
			while (len-- > 0)
			{
				val += *min++ * *contrib++;
			}
			*dst++ = (unsigned char)(val>>8);
		}
	}
}

static void
scale_row_to_temp2(unsigned char *dst, unsigned char *src, base_weights *weights)
{
	int *contrib = &weights->index[weights->index[0]];
	int len, i;
	unsigned char *min;

	assert(weights->n == 2);
	if (weights->flip)
	{
		dst += 2*weights->count;
		for (i=weights->count; i > 0; i--)
		{
			int c1 = 128;
			int c2 = 128;
			min = &src[2 * *contrib++];
			len = *contrib++;
			while (len-- > 0)
			{
				c1 += *min++ * *contrib;
				c2 += *min++ * *contrib++;
			}
			*--dst = (unsigned char)(c2>>8);
			*--dst = (unsigned char)(c1>>8);
		}
	}
	else
	{
		for (i=weights->count; i > 0; i--)
		{
			int c1 = 128;
			int c2 = 128;
			min = &src[2 * *contrib++];
			len = *contrib++;
			while (len-- > 0)
			{
				c1 += *min++ * *contrib;
				c2 += *min++ * *contrib++;
			}
			*dst++ = (unsigned char)(c1>>8);
			*dst++ = (unsigned char)(c2>>8);
		}
	}
}

static void
scale_row_to_temp4(unsigned char *dst, unsigned char *src, base_weights *weights)
{
	int *contrib = &weights->index[weights->index[0]];
	int len, i;
	unsigned char *min;

	assert(weights->n == 4);
	if (weights->flip)
	{
		dst += 4*weights->count;
		for (i=weights->count; i > 0; i--)
		{
			int r = 128;
			int g = 128;
			int b = 128;
			int a = 128;
			min = &src[4 * *contrib++];
			len = *contrib++;
			while (len-- > 0)
			{
				r += *min++ * *contrib;
				g += *min++ * *contrib;
				b += *min++ * *contrib;
				a += *min++ * *contrib++;
			}
			*--dst = (unsigned char)(a>>8);
			*--dst = (unsigned char)(b>>8);
			*--dst = (unsigned char)(g>>8);
			*--dst = (unsigned char)(r>>8);
		}
	}
	else
	{
		for (i=weights->count; i > 0; i--)
		{
			int r = 128;
			int g = 128;
			int b = 128;
			int a = 128;
			min = &src[4 * *contrib++];
			len = *contrib++;
			while (len-- > 0)
			{
				r += *min++ * *contrib;
				g += *min++ * *contrib;
				b += *min++ * *contrib;
				a += *min++ * *contrib++;
			}
			*dst++ = (unsigned char)(r>>8);
			*dst++ = (unsigned char)(g>>8);
			*dst++ = (unsigned char)(b>>8);
			*dst++ = (unsigned char)(a>>8);
		}
	}
}

static void
scale_row_from_temp(unsigned char *dst, unsigned char *src, base_weights *weights, int width, int row)
{
	int *contrib = &weights->index[weights->index[row]];
	int len, x;

	contrib++; 
	len = *contrib++;
	for (x=width; x > 0; x--)
	{
		unsigned char *min = src;
		int val = 128;
		int len2 = len;
		int *contrib2 = contrib;

		while (len2-- > 0)
		{
			val += *min * *contrib2++;
			min += width;
		}
		*dst++ = (unsigned char)(val>>8);
		src++;
	}
}
#endif

#ifdef SINGLE_PIXEL_SPECIALS
static void
duplicate_single_pixel(unsigned char *dst, unsigned char *src, int n, int w, int h)
{
	int i;

	for (i = n; i > 0; i--)
		*dst++ = *src++;
	for (i = (w*h-1)*n; i > 0; i--)
	{
		*dst = dst[-n];
		dst++;
	}
}

static void
scale_single_row(unsigned char *dst, unsigned char *src, base_weights *weights, int src_w, int h)
{
	int *contrib = &weights->index[weights->index[0]];
	int min, len, i, j, n;
	int tmp[base_MAX_COLORS];

	n = weights->n;
	
	for (j = 0; j < n; j++)
		tmp[j] = 128;
	if (weights->flip)
	{
		dst += (weights->count-1)*n;
		for (i=weights->count; i > 0; i--)
		{
			min = *contrib++;
			len = *contrib++;
			min *= n;
			while (len-- > 0)
			{
				for (j = 0; j < n; j++)
					tmp[j] += src[min++] * *contrib;
				contrib++;
			}
			for (j = 0; j < n; j++)
			{
				*dst++ = (unsigned char)(tmp[j]>>8);
				tmp[j] = 128;
			}
			dst -= 2*n;
		}
		dst += n * (weights->count+1);
	}
	else
	{
		for (i=weights->count; i > 0; i--)
		{
			min = *contrib++;
			len = *contrib++;
			min *= n;
			while (len-- > 0)
			{
				for (j = 0; j < n; j++)
					tmp[j] += src[min++] * *contrib;
				contrib++;
			}
			for (j = 0; j < n; j++)
			{
				*dst++ = (unsigned char)(tmp[j]>>8);
				tmp[j] = 128;
			}
		}
	}
	
	n *= weights->count;
	while (--h > 0)
	{
		memcpy(dst, dst-n, n);
		dst += n;
	}
}

static void
scale_single_col(unsigned char *dst, unsigned char *src, base_weights *weights, int src_w, int n, int w, int flip_y)
{
	int *contrib = &weights->index[weights->index[0]];
	int min, len, i, j;
	int tmp[base_MAX_COLORS];

	for (j = 0; j < n; j++)
		tmp[j] = 128;
	if (flip_y)
	{
		src_w = (src_w-1)*n;
		w = (w-1)*n;
		for (i=weights->count; i > 0; i--)
		{
			
			min = *contrib++;
			len = *contrib++;
			min = src_w-min*n;
			while (len-- > 0)
			{
				for (j = 0; j < n; j++)
					tmp[j] += src[src_w-min+j] * *contrib;
				contrib++;
			}
			for (j = 0; j < n; j++)
			{
				*dst++ = (unsigned char)(tmp[j]>>8);
				tmp[j] = 128;
			}
			
			for (j = w; j > 0; j--)
			{
				*dst = dst[-n];
				dst++;
			}
		}
	}
	else
	{
		w = (w-1)*n;
		for (i=weights->count; i > 0; i--)
		{
			
			min = *contrib++;
			len = *contrib++;
			min *= n;
			while (len-- > 0)
			{
				for (j = 0; j < n; j++)
					tmp[j] += src[min++] * *contrib;
				contrib++;
			}
			for (j = 0; j < n; j++)
			{
				*dst++ = (unsigned char)(tmp[j]>>8);
				tmp[j] = 128;
			}
			
			for (j = w; j > 0; j--)
			{
				*dst = dst[-n];
				dst++;
			}
		}
	}
}
#endif 

base_pixmap *
base_scale_pixmap(base_context *ctx, base_pixmap *src, float x, float y, float w, float h, base_bbox *clip)
{
	base_scale_filter *filter = &base_scale_filter_simple;
	base_weights *contrib_rows = NULL;
	base_weights *contrib_cols = NULL;
	base_pixmap *output = NULL;
	unsigned char *temp = NULL;
	int max_row, temp_span, temp_rows, row;
	int dst_w_int, dst_h_int, dst_x_int, dst_y_int;
	int flip_x, flip_y;
	base_bbox patch;

	base_var(contrib_cols);
	base_var(contrib_rows);

	DBUG(("Scale: (%d,%d) to (%g,%g) at (%g,%g)\n",src->w,src->h,w,h,x,y));

	
	
	
	flip_x = (w < 0);
	if (flip_x)
	{
		float tmp;
		w = -w;
		dst_x_int = floorf(x-w);
		tmp = ceilf(x);
		dst_w_int = (int)tmp;
		x = tmp - x;
		dst_w_int -= dst_x_int;
	}
	else
	{
		dst_x_int = floorf(x);
		x -= (float)dst_x_int;
		dst_w_int = (int)ceilf(x + w);
	}
	
	flip_y = (h < 0);
	if (flip_y)
	{
		float tmp;
		h = -h;
		dst_y_int = floorf(y-h);
		tmp = ceilf(y);
		dst_h_int = (int)tmp;
		y = tmp - y;
		dst_h_int -= dst_y_int;
	}
	else
	{
		dst_y_int = floorf(y);
		y -= (float)dst_y_int;
		dst_h_int = (int)ceilf(y + h);
	}

	DBUG(("Result image: (%d,%d) at (%d,%d) (subpix=%g,%g)\n", dst_w_int, dst_h_int, dst_x_int, dst_y_int, x, y));

	
	patch.x0 = 0;
	patch.y0 = 0;
	patch.x1 = dst_w_int;
	patch.y1 = dst_h_int;
	if (clip)
	{
		if (flip_x)
		{
			if (dst_x_int + dst_w_int > clip->x1)
				patch.x0 = dst_x_int + dst_w_int - clip->x1;
			if (clip->x0 > dst_x_int)
			{
				patch.x1 = dst_w_int - (clip->x0 - dst_x_int);
				dst_x_int = clip->x0;
			}
		}
		else
		{
			if (dst_x_int + dst_w_int > clip->x1)
				patch.x1 = clip->x1 - dst_x_int;
			if (clip->x0 > dst_x_int)
			{
				patch.x0 = clip->x0 - dst_x_int;
				dst_x_int += patch.x0;
			}
		}

		if (flip_y)
		{
			if (dst_y_int + dst_h_int > clip->y1)
				patch.y1 = clip->y1 - dst_y_int;
			if (clip->y0 > dst_y_int)
			{
				patch.y0 = clip->y0 - dst_y_int;
				dst_y_int = clip->y0;
			}
		}
		else
		{
			if (dst_y_int + dst_h_int > clip->y1)
				patch.y1 = clip->y1 - dst_y_int;
			if (clip->y0 > dst_y_int)
			{
				patch.y0 = clip->y0 - dst_y_int;
				dst_y_int += patch.y0;
			}
		}
	}
	if (patch.x0 >= patch.x1 || patch.y0 >= patch.y1)
		return NULL;

	base_try(ctx)
	{
		
#ifdef SINGLE_PIXEL_SPECIALS
		if (src->w == 1)
			contrib_cols = NULL;
		else
#endif 
			contrib_cols = make_weights(ctx, src->w, x, w, filter, 0, dst_w_int, patch.x0, patch.x1, src->n, flip_x);
#ifdef SINGLE_PIXEL_SPECIALS
		if (src->h == 1)
			contrib_rows = NULL;
		else
#endif 
			contrib_rows = make_weights(ctx, src->h, y, h, filter, 1, dst_h_int, patch.y0, patch.y1, src->n, flip_y);

		output = base_new_pixmap(ctx, src->colorspace, patch.x1 - patch.x0, patch.y1 - patch.y0);
	}
	base_catch(ctx)
	{
		base_free(ctx, contrib_cols);
		base_free(ctx, contrib_rows);
		base_rethrow(ctx);
	}
	output->x = dst_x_int;
	output->y = dst_y_int;

	
#ifdef SINGLE_PIXEL_SPECIALS
	if (!contrib_rows)
	{
		
		if (!contrib_cols)
		{
			
			duplicate_single_pixel(output->samples, src->samples, src->n, patch.x1-patch.x0, patch.y1-patch.y0);
		}
		else
		{
			
			scale_single_row(output->samples, src->samples, contrib_cols, src->w, patch.y1-patch.y0);
		}
	}
	else if (!contrib_cols)
	{
		
		scale_single_col(output->samples, src->samples, contrib_rows, src->h, src->n, patch.x1-patch.x0, flip_y);
	}
	else
#endif 
	{
		void (*row_scale)(unsigned char *dst, unsigned char *src, base_weights *weights);

		temp_span = contrib_cols->count * src->n;
		temp_rows = contrib_rows->max_len;
		if (temp_span <= 0 || temp_rows > INT_MAX / temp_span)
			goto cleanup;
		base_try(ctx)
		{
			temp = base_calloc(ctx, temp_span*temp_rows, sizeof(unsigned char));
		}
		base_catch(ctx)
		{
			base_drop_pixmap(ctx, output);
			base_free(ctx, contrib_cols);
			base_free(ctx, contrib_rows);
			base_rethrow(ctx);
		}
		switch (src->n)
		{
		default:
			row_scale = scale_row_to_temp;
			break;
		case 1: 
			row_scale = scale_row_to_temp1;
			break;
		case 2: 
			row_scale = scale_row_to_temp2;
			break;
		case 4: 
			row_scale = scale_row_to_temp4;
			break;
		}
		max_row = contrib_rows->index[contrib_rows->index[0]];
		for (row = 0; row < contrib_rows->count; row++)
		{
			
			int row_index = contrib_rows->index[row];
			int row_min = contrib_rows->index[row_index++];
			int row_len = contrib_rows->index[row_index++];
			while (max_row < row_min+row_len)
			{
				
				assert(max_row < src->h);
				DBUG(("scaling row %d to temp\n", max_row));
				(*row_scale)(&temp[temp_span*(max_row % temp_rows)], &src->samples[(flip_y ? (src->h-1-max_row): max_row)*src->w*src->n], contrib_cols);
				max_row++;
			}

			DBUG(("scaling row %d from temp\n", row));
			scale_row_from_temp(&output->samples[row*output->w*output->n], temp, contrib_rows, temp_span, row);
		}
		base_free(ctx, temp);
	}

cleanup:
	base_free(ctx, contrib_rows);
	base_free(ctx, contrib_cols);
	return output;
}
