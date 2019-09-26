

#define JPEG_INTERNALS
#include "jinclude.h"
#include "jpeglib.h"
#include "jdct.h"		

#ifdef DCT_FLOAT_SUPPORTED

#if DCTSIZE != 8
  Sorry, this code only copes with 8x8 DCTs. 
#endif

GLOBAL(void)
jpeg_fdct_float (FAST_FLOAT * data, JSAMPARRAY sample_data, JDIMENSION start_col)
{
  FAST_FLOAT tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7;
  FAST_FLOAT tmp10, tmp11, tmp12, tmp13;
  FAST_FLOAT z1, z2, z3, z4, z5, z11, z13;
  FAST_FLOAT *dataptr;
  JSAMPROW elemptr;
  int ctr;

  

  dataptr = data;
  for (ctr = 0; ctr < DCTSIZE; ctr++) {
    elemptr = sample_data[ctr] + start_col;

    
    tmp0 = (FAST_FLOAT) (GETJSAMPLE(elemptr[0]) + GETJSAMPLE(elemptr[7]));
    tmp7 = (FAST_FLOAT) (GETJSAMPLE(elemptr[0]) - GETJSAMPLE(elemptr[7]));
    tmp1 = (FAST_FLOAT) (GETJSAMPLE(elemptr[1]) + GETJSAMPLE(elemptr[6]));
    tmp6 = (FAST_FLOAT) (GETJSAMPLE(elemptr[1]) - GETJSAMPLE(elemptr[6]));
    tmp2 = (FAST_FLOAT) (GETJSAMPLE(elemptr[2]) + GETJSAMPLE(elemptr[5]));
    tmp5 = (FAST_FLOAT) (GETJSAMPLE(elemptr[2]) - GETJSAMPLE(elemptr[5]));
    tmp3 = (FAST_FLOAT) (GETJSAMPLE(elemptr[3]) + GETJSAMPLE(elemptr[4]));
    tmp4 = (FAST_FLOAT) (GETJSAMPLE(elemptr[3]) - GETJSAMPLE(elemptr[4]));

    

    tmp10 = tmp0 + tmp3;	
    tmp13 = tmp0 - tmp3;
    tmp11 = tmp1 + tmp2;
    tmp12 = tmp1 - tmp2;

    
    dataptr[0] = tmp10 + tmp11 - 8 * CENTERJSAMPLE; 
    dataptr[4] = tmp10 - tmp11;

    z1 = (tmp12 + tmp13) * ((FAST_FLOAT) 0.707106781); 
    dataptr[2] = tmp13 + z1;	
    dataptr[6] = tmp13 - z1;

    

    tmp10 = tmp4 + tmp5;	
    tmp11 = tmp5 + tmp6;
    tmp12 = tmp6 + tmp7;

    
    z5 = (tmp10 - tmp12) * ((FAST_FLOAT) 0.382683433); 
    z2 = ((FAST_FLOAT) 0.541196100) * tmp10 + z5; 
    z4 = ((FAST_FLOAT) 1.306562965) * tmp12 + z5; 
    z3 = tmp11 * ((FAST_FLOAT) 0.707106781); 

    z11 = tmp7 + z3;		
    z13 = tmp7 - z3;

    dataptr[5] = z13 + z2;	
    dataptr[3] = z13 - z2;
    dataptr[1] = z11 + z4;
    dataptr[7] = z11 - z4;

    dataptr += DCTSIZE;		
  }

  

  dataptr = data;
  for (ctr = DCTSIZE-1; ctr >= 0; ctr--) {
    tmp0 = dataptr[DCTSIZE*0] + dataptr[DCTSIZE*7];
    tmp7 = dataptr[DCTSIZE*0] - dataptr[DCTSIZE*7];
    tmp1 = dataptr[DCTSIZE*1] + dataptr[DCTSIZE*6];
    tmp6 = dataptr[DCTSIZE*1] - dataptr[DCTSIZE*6];
    tmp2 = dataptr[DCTSIZE*2] + dataptr[DCTSIZE*5];
    tmp5 = dataptr[DCTSIZE*2] - dataptr[DCTSIZE*5];
    tmp3 = dataptr[DCTSIZE*3] + dataptr[DCTSIZE*4];
    tmp4 = dataptr[DCTSIZE*3] - dataptr[DCTSIZE*4];

    

    tmp10 = tmp0 + tmp3;	
    tmp13 = tmp0 - tmp3;
    tmp11 = tmp1 + tmp2;
    tmp12 = tmp1 - tmp2;

    dataptr[DCTSIZE*0] = tmp10 + tmp11; 
    dataptr[DCTSIZE*4] = tmp10 - tmp11;

    z1 = (tmp12 + tmp13) * ((FAST_FLOAT) 0.707106781); 
    dataptr[DCTSIZE*2] = tmp13 + z1; 
    dataptr[DCTSIZE*6] = tmp13 - z1;

    

    tmp10 = tmp4 + tmp5;	
    tmp11 = tmp5 + tmp6;
    tmp12 = tmp6 + tmp7;

    
    z5 = (tmp10 - tmp12) * ((FAST_FLOAT) 0.382683433); 
    z2 = ((FAST_FLOAT) 0.541196100) * tmp10 + z5; 
    z4 = ((FAST_FLOAT) 1.306562965) * tmp12 + z5; 
    z3 = tmp11 * ((FAST_FLOAT) 0.707106781); 

    z11 = tmp7 + z3;		
    z13 = tmp7 - z3;

    dataptr[DCTSIZE*5] = z13 + z2; 
    dataptr[DCTSIZE*3] = z13 - z2;
    dataptr[DCTSIZE*1] = z11 + z4;
    dataptr[DCTSIZE*7] = z11 - z4;

    dataptr++;			
  }
}

#endif 
