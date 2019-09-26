#include "pdf-internal.h"

/***************************************************************************************/
/* function name:	base_strsep
/* description:		separate the string by the specified delimiter string
/* param 1:			string to separate
/* param 2:			delimiter string
/* return:			separated string
/* creator & modifier : 
/* create:			2012.09.10 ~
/* working:			2012.09.10 ~
/***************************************************************************************/

char *
base_strsep(char **stringp, const char *delim)
{
	char *ret = *stringp;
	if (!ret) return NULL;
	if ((*stringp = strpbrk(*stringp, delim)))
		*((*stringp)++) = '\0';
	return ret;
}

int
base_strlcpy(char *dst, const char *src, int siz)
{
	register char *d = dst;
	register const char *s = src;
	register int n = siz;

	
	if (n != 0 && --n != 0) {
		do {
			if ((*d++ = *s++) == 0)
				break;
		} while (--n != 0);
	}

	
	if (n == 0) {
		if (siz != 0)
			*d = '\0';		
		while (*s++)
			;
	}

	return(s - src - 1);	
}

int
base_strlcat(char *dst, const char *src, int siz)
{
	register char *d = dst;
	register const char *s = src;
	register int n = siz;
	int dlen;

	
	while (*d != '\0' && n-- != 0)
		d++;
	dlen = d - dst;
	n = siz - dlen;

	if (n == 0)
		return dlen + strlen(s);
	while (*s != '\0') {
		if (n != 1) {
			*d++ = *s;
			n--;
		}
		s++;
	}
	*d = '\0';

	return dlen + (s - src);	
}

enum
{
	UTFmax = 4, 
	Runesync = 0x80, 
	Runeself = 0x80, 
	Runeerror = 0xFFFD, 
	Runemax = 0x10FFFF, 
};

enum
{
	Bit1 = 7,
	Bitx = 6,
	Bit2 = 5,
	Bit3 = 4,
	Bit4 = 3,
	Bit5 = 2,

	T1 = ((1<<(Bit1+1))-1) ^ 0xFF, 
	Tx = ((1<<(Bitx+1))-1) ^ 0xFF, 
	T2 = ((1<<(Bit2+1))-1) ^ 0xFF, 
	T3 = ((1<<(Bit3+1))-1) ^ 0xFF, 
	T4 = ((1<<(Bit4+1))-1) ^ 0xFF, 
	T5 = ((1<<(Bit5+1))-1) ^ 0xFF, 

	Rune1 = (1<<(Bit1+0*Bitx))-1, 
	Rune2 = (1<<(Bit2+1*Bitx))-1, 
	Rune3 = (1<<(Bit3+2*Bitx))-1, 
	Rune4 = (1<<(Bit4+3*Bitx))-1, 

	Maskx = (1<<Bitx)-1,	
	Testx = Maskx ^ 0xFF,	

	Bad = Runeerror,
};

int
base_chartorune(int *rune, char *str)
{
	int c, c1, c2, c3;
	long l;

	
	c = *(unsigned char*)str;
	if(c < Tx) {
		*rune = c;
		return 1;
	}

	
	c1 = *(unsigned char*)(str+1) ^ Tx;
	if(c1 & Testx)
		goto bad;
	if(c < T3) {
		if(c < T2)
			goto bad;
		l = ((c << Bitx) | c1) & Rune2;
		if(l <= Rune1)
			goto bad;
		*rune = l;
		return 2;
	}

	
	c2 = *(unsigned char*)(str+2) ^ Tx;
	if(c2 & Testx)
		goto bad;
	if(c < T4) {
		l = ((((c << Bitx) | c1) << Bitx) | c2) & Rune3;
		if(l <= Rune2)
			goto bad;
		*rune = l;
		return 3;
	}

	
	c3 = *(unsigned char*)(str+3) ^ Tx;
	if (c3 & Testx)
		goto bad;
	if (c < T5) {
		l = ((((((c << Bitx) | c1) << Bitx) | c2) << Bitx) | c3) & Rune4;
		if (l <= Rune3)
			goto bad;
		*rune = l;
		return 4;
	}
	

	
bad:
	*rune = Bad;
	return 1;
}

int
base_runetochar(char *str, int rune)
{
	
	unsigned long c = (unsigned long)rune;

	
	if(c <= Rune1) {
		str[0] = c;
		return 1;
	}

	
	if(c <= Rune2) {
		str[0] = T2 | (c >> 1*Bitx);
		str[1] = Tx | (c & Maskx);
		return 2;
	}

	
	if (c > Runemax)
		c = Runeerror;

	
	if (c <= Rune3) {
		str[0] = T3 | (c >> 2*Bitx);
		str[1] = Tx | ((c >> 1*Bitx) & Maskx);
		str[2] = Tx | (c & Maskx);
		return 3;
	}

	
	str[0] = T4 | (c >> 3*Bitx);
	str[1] = Tx | ((c >> 2*Bitx) & Maskx);
	str[2] = Tx | ((c >> 1*Bitx) & Maskx);
	str[3] = Tx | (c & Maskx);
	return 4;
}

int
base_runelen(int c)
{
	char str[10];
	return base_runetochar(str, c);
}

float base_atof(const char *s)
{
	double d;

	
	errno = 0;
	d = strtod(s, NULL);
	if (errno == ERANGE || isnan(d)) {
		
		return 1.0;
	}
	d = CLAMP(d, -FLT_MAX, FLT_MAX);
	return (float)d;
}
