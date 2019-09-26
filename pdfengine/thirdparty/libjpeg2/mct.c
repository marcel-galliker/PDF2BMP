

#ifdef __SSE__
#include <xmmintrin.h>
#endif

#include "opj_includes.h"

static const double mct_norms[3] = { 1.732, .8292, .8292 };

static const double mct_norms_real[3] = { 1.732, 1.805, 1.573 };

void mct_encode(
		int* restrict c0,
		int* restrict c1,
		int* restrict c2,
		int n)
{
	int i;
	for(i = 0; i < n; ++i) {
		int r = c0[i];
		int g = c1[i];
		int b = c2[i];
		int y = (r + (g * 2) + b) >> 2;
		int u = b - g;
		int v = r - g;
		c0[i] = y;
		c1[i] = u;
		c2[i] = v;
	}
}

void mct_decode(
		int* restrict c0,
		int* restrict c1, 
		int* restrict c2, 
		int n)
{
	int i;
	for (i = 0; i < n; ++i) {
		int y = c0[i];
		int u = c1[i];
		int v = c2[i];
		int g = y - ((u + v) >> 2);
		int r = v + g;
		int b = u + g;
		c0[i] = r;
		c1[i] = g;
		c2[i] = b;
	}
}

double mct_getnorm(int compno) {
	return mct_norms[compno];
}

void mct_encode_real(
		int* restrict c0,
		int* restrict c1,
		int* restrict c2,
		int n)
{
	int i;
	for(i = 0; i < n; ++i) {
		int r = c0[i];
		int g = c1[i];
		int b = c2[i];
		int y =  fix_mul(r, 2449) + fix_mul(g, 4809) + fix_mul(b, 934);
		int u = -fix_mul(r, 1382) - fix_mul(g, 2714) + fix_mul(b, 4096);
		int v =  fix_mul(r, 4096) - fix_mul(g, 3430) - fix_mul(b, 666);
		c0[i] = y;
		c1[i] = u;
		c2[i] = v;
	}
}

void mct_decode_real(
		float* restrict c0,
		float* restrict c1,
		float* restrict c2,
		int n)
{
	int i;
#ifdef __SSE__
	__m128 vrv, vgu, vgv, vbu;
	vrv = _mm_set1_ps(1.402f);
	vgu = _mm_set1_ps(0.34413f);
	vgv = _mm_set1_ps(0.71414f);
	vbu = _mm_set1_ps(1.772f);
	for (i = 0; i < (n >> 3); ++i) {
		__m128 vy, vu, vv;
		__m128 vr, vg, vb;

		vy = _mm_load_ps(c0);
		vu = _mm_load_ps(c1);
		vv = _mm_load_ps(c2);
		vr = _mm_add_ps(vy, _mm_mul_ps(vv, vrv));
		vg = _mm_sub_ps(_mm_sub_ps(vy, _mm_mul_ps(vu, vgu)), _mm_mul_ps(vv, vgv));
		vb = _mm_add_ps(vy, _mm_mul_ps(vu, vbu));
		_mm_store_ps(c0, vr);
		_mm_store_ps(c1, vg);
		_mm_store_ps(c2, vb);
		c0 += 4;
		c1 += 4;
		c2 += 4;

		vy = _mm_load_ps(c0);
		vu = _mm_load_ps(c1);
		vv = _mm_load_ps(c2);
		vr = _mm_add_ps(vy, _mm_mul_ps(vv, vrv));
		vg = _mm_sub_ps(_mm_sub_ps(vy, _mm_mul_ps(vu, vgu)), _mm_mul_ps(vv, vgv));
		vb = _mm_add_ps(vy, _mm_mul_ps(vu, vbu));
		_mm_store_ps(c0, vr);
		_mm_store_ps(c1, vg);
		_mm_store_ps(c2, vb);
		c0 += 4;
		c1 += 4;
		c2 += 4;
	}
	n &= 7;
#endif
	for(i = 0; i < n; ++i) {
		float y = c0[i];
		float u = c1[i];
		float v = c2[i];
		float r = y + (v * 1.402f);
		float g = y - (u * 0.34413f) - (v * (0.71414f));
		float b = y + (u * 1.772f);
		c0[i] = r;
		c1[i] = g;
		c2[i] = b;
	}
}

double mct_getnorm_real(int compno) {
	return mct_norms_real[compno];
}
