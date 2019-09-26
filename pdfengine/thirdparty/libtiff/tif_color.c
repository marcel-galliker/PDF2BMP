

#include "tiffiop.h"
#include <math.h>

void
TIFFCIELabToXYZ(TIFFCIELabToRGB *cielab, uint32 l, int32 a, int32 b,
		float *X, float *Y, float *Z)
{
	float L = (float)l * 100.0F / 255.0F;
	float cby, tmp;

	if( L < 8.856F ) {
		*Y = (L * cielab->Y0) / 903.292F;
		cby = 7.787F * (*Y / cielab->Y0) + 16.0F / 116.0F;
	} else {
		cby = (L + 16.0F) / 116.0F;
		*Y = cielab->Y0 * cby * cby * cby;
	}

	tmp = (float)a / 500.0F + cby;
	if( tmp < 0.2069F )
		*X = cielab->X0 * (tmp - 0.13793F) / 7.787F;
	else    
		*X = cielab->X0 * tmp * tmp * tmp;

	tmp = cby - (float)b / 200.0F;
	if( tmp < 0.2069F )
		*Z = cielab->Z0 * (tmp - 0.13793F) / 7.787F;
	else    
		*Z = cielab->Z0 * tmp * tmp * tmp;
}

#define RINT(R) ((uint32)((R)>0?((R)+0.5):((R)-0.5)))

void
TIFFXYZToRGB(TIFFCIELabToRGB *cielab, float X, float Y, float Z,
	     uint32 *r, uint32 *g, uint32 *b)
{
	int i;
	float Yr, Yg, Yb;
	float *matrix = &cielab->display.d_mat[0][0];

	
	Yr =  matrix[0] * X + matrix[1] * Y + matrix[2] * Z;
	Yg =  matrix[3] * X + matrix[4] * Y + matrix[5] * Z;
	Yb =  matrix[6] * X + matrix[7] * Y + matrix[8] * Z;

	
	Yr = TIFFmax(Yr, cielab->display.d_Y0R);
	Yg = TIFFmax(Yg, cielab->display.d_Y0G);
	Yb = TIFFmax(Yb, cielab->display.d_Y0B);

	
	Yr = TIFFmin(Yr, cielab->display.d_YCR);
	Yg = TIFFmin(Yg, cielab->display.d_YCG);
	Yb = TIFFmin(Yb, cielab->display.d_YCB);

	
	i = (int)((Yr - cielab->display.d_Y0R) / cielab->rstep);
	i = TIFFmin(cielab->range, i);
	*r = RINT(cielab->Yr2r[i]);

	i = (int)((Yg - cielab->display.d_Y0G) / cielab->gstep);
	i = TIFFmin(cielab->range, i);
	*g = RINT(cielab->Yg2g[i]);

	i = (int)((Yb - cielab->display.d_Y0B) / cielab->bstep);
	i = TIFFmin(cielab->range, i);
	*b = RINT(cielab->Yb2b[i]);

	
	*r = TIFFmin(*r, cielab->display.d_Vrwr);
	*g = TIFFmin(*g, cielab->display.d_Vrwg);
	*b = TIFFmin(*b, cielab->display.d_Vrwb);
}
#undef RINT

int
TIFFCIELabToRGBInit(TIFFCIELabToRGB* cielab,
		    TIFFDisplay *display, float *refWhite)
{
	int i;
	double gamma;

	cielab->range = CIELABTORGB_TABLE_RANGE;

	_TIFFmemcpy(&cielab->display, display, sizeof(TIFFDisplay));

	
	gamma = 1.0 / cielab->display.d_gammaR ;
	cielab->rstep =
		(cielab->display.d_YCR - cielab->display.d_Y0R)	/ cielab->range;
	for(i = 0; i <= cielab->range; i++) {
		cielab->Yr2r[i] = cielab->display.d_Vrwr
		    * ((float)pow((double)i / cielab->range, gamma));
	}

	
	gamma = 1.0 / cielab->display.d_gammaG ;
	cielab->gstep =
	    (cielab->display.d_YCR - cielab->display.d_Y0R) / cielab->range;
	for(i = 0; i <= cielab->range; i++) {
		cielab->Yg2g[i] = cielab->display.d_Vrwg
		    * ((float)pow((double)i / cielab->range, gamma));
	}

	
	gamma = 1.0 / cielab->display.d_gammaB ;
	cielab->bstep =
	    (cielab->display.d_YCR - cielab->display.d_Y0R) / cielab->range;
	for(i = 0; i <= cielab->range; i++) {
		cielab->Yb2b[i] = cielab->display.d_Vrwb
		    * ((float)pow((double)i / cielab->range, gamma));
	}

	
	cielab->X0 = refWhite[0];
	cielab->Y0 = refWhite[1];
	cielab->Z0 = refWhite[2];

	return 0;
}

#define	SHIFT			16
#define	FIX(x)			((int32)((x) * (1L<<SHIFT) + 0.5))
#define	ONE_HALF		((int32)(1<<(SHIFT-1)))
#define	Code2V(c, RB, RW, CR)	((((c)-(int32)(RB))*(float)(CR))/(float)(((RW)-(RB)) ? ((RW)-(RB)) : 1))
#define	CLAMP(f,min,max)	((f)<(min)?(min):(f)>(max)?(max):(f))
#define HICLAMP(f,max)		((f)>(max)?(max):(f))

void
TIFFYCbCrtoRGB(TIFFYCbCrToRGB *ycbcr, uint32 Y, int32 Cb, int32 Cr,
	       uint32 *r, uint32 *g, uint32 *b)
{
	
	Y = HICLAMP(Y, 255), Cb = CLAMP(Cb, 0, 255), Cr = CLAMP(Cr, 0, 255);

	*r = ycbcr->clamptab[ycbcr->Y_tab[Y] + ycbcr->Cr_r_tab[Cr]];
	*g = ycbcr->clamptab[ycbcr->Y_tab[Y]
	    + (int)((ycbcr->Cb_g_tab[Cb] + ycbcr->Cr_g_tab[Cr]) >> SHIFT)];
	*b = ycbcr->clamptab[ycbcr->Y_tab[Y] + ycbcr->Cb_b_tab[Cb]];
}

int
TIFFYCbCrToRGBInit(TIFFYCbCrToRGB* ycbcr, float *luma, float *refBlackWhite)
{
    TIFFRGBValue* clamptab;
    int i;
    
#define LumaRed	    luma[0]
#define LumaGreen   luma[1]
#define LumaBlue    luma[2]

    clamptab = (TIFFRGBValue*)(
	(tidata_t) ycbcr+TIFFroundup(sizeof (TIFFYCbCrToRGB), sizeof (long)));
    _TIFFmemset(clamptab, 0, 256);		
    ycbcr->clamptab = (clamptab += 256);
    for (i = 0; i < 256; i++)
	clamptab[i] = (TIFFRGBValue) i;
    _TIFFmemset(clamptab+256, 255, 2*256);	
    ycbcr->Cr_r_tab = (int*) (clamptab + 3*256);
    ycbcr->Cb_b_tab = ycbcr->Cr_r_tab + 256;
    ycbcr->Cr_g_tab = (int32*) (ycbcr->Cb_b_tab + 256);
    ycbcr->Cb_g_tab = ycbcr->Cr_g_tab + 256;
    ycbcr->Y_tab = ycbcr->Cb_g_tab + 256;

    { float f1 = 2-2*LumaRed;		int32 D1 = FIX(f1);
      float f2 = LumaRed*f1/LumaGreen;	int32 D2 = -FIX(f2);
      float f3 = 2-2*LumaBlue;		int32 D3 = FIX(f3);
      float f4 = LumaBlue*f3/LumaGreen;	int32 D4 = -FIX(f4);
      int x;

#undef LumaBlue
#undef LumaGreen
#undef LumaRed
      
      
      for (i = 0, x = -128; i < 256; i++, x++) {
	    int32 Cr = (int32)Code2V(x, refBlackWhite[4] - 128.0F,
			    refBlackWhite[5] - 128.0F, 127);
	    int32 Cb = (int32)Code2V(x, refBlackWhite[2] - 128.0F,
			    refBlackWhite[3] - 128.0F, 127);

	    ycbcr->Cr_r_tab[i] = (int32)((D1*Cr + ONE_HALF)>>SHIFT);
	    ycbcr->Cb_b_tab[i] = (int32)((D3*Cb + ONE_HALF)>>SHIFT);
	    ycbcr->Cr_g_tab[i] = D2*Cr;
	    ycbcr->Cb_g_tab[i] = D4*Cb + ONE_HALF;
	    ycbcr->Y_tab[i] =
		    (int32)Code2V(x + 128, refBlackWhite[0], refBlackWhite[1], 255);
      }
    }

    return 0;
}
#undef	HICLAMP
#undef	CLAMP
#undef	Code2V
#undef	SHIFT
#undef	ONE_HALF
#undef	FIX

