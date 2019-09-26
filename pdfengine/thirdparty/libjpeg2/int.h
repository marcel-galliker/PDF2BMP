
#ifndef __INT_H
#define __INT_H

static INLINE int int_min(int a, int b) {
	return a < b ? a : b;
}

static INLINE int int_max(int a, int b) {
	return (a > b) ? a : b;
}

static INLINE int int_clamp(int a, int min, int max) {
	if (a < min)
		return min;
	if (a > max)
		return max;
	return a;
}

static INLINE int int_abs(int a) {
	return a < 0 ? -a : a;
}

static INLINE int int_ceildiv(int a, int b) {
	return (a + b - 1) / b;
}

static INLINE int int_ceildivpow2(int a, int b) {
	return (a + (1 << b) - 1) >> b;
}

static INLINE int int_floordivpow2(int a, int b) {
	return a >> b;
}

static INLINE int int_floorlog2(int a) {
	int l;
	for (l = 0; a > 1; l++) {
		a >>= 1;
	}
	return l;
}

#endif
