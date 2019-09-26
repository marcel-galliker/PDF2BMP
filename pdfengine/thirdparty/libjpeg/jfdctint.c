

#define JPEG_INTERNALS
#include "jinclude.h"
#include "jpeglib.h"
#include "jdct.h"		

#ifdef DCT_ISLOW_SUPPORTED

#if DCTSIZE != 8
  Sorry, this code only copes with 8x8 DCT blocks. 
#endif

#if BITS_IN_JSAMPLE == 8
#define CONST_BITS  13
#define PASS1_BITS  2
#else
#define CONST_BITS  13
#define PASS1_BITS  1		
#endif

#if CONST_BITS == 13
#define FIX_0_298631336  ((INT32)  2446)	
#define FIX_0_390180644  ((INT32)  3196)	
#define FIX_0_541196100  ((INT32)  4433)	
#define FIX_0_765366865  ((INT32)  6270)	
#define FIX_0_899976223  ((INT32)  7373)	
#define FIX_1_175875602  ((INT32)  9633)	
#define FIX_1_501321110  ((INT32)  12299)	
#define FIX_1_847759065  ((INT32)  15137)	
#define FIX_1_961570560  ((INT32)  16069)	
#define FIX_2_053119869  ((INT32)  16819)	
#define FIX_2_562915447  ((INT32)  20995)	
#define FIX_3_072711026  ((INT32)  25172)	
#else
#define FIX_0_298631336  FIX(0.298631336)
#define FIX_0_390180644  FIX(0.390180644)
#define FIX_0_541196100  FIX(0.541196100)
#define FIX_0_765366865  FIX(0.765366865)
#define FIX_0_899976223  FIX(0.899976223)
#define FIX_1_175875602  FIX(1.175875602)
#define FIX_1_501321110  FIX(1.501321110)
#define FIX_1_847759065  FIX(1.847759065)
#define FIX_1_961570560  FIX(1.961570560)
#define FIX_2_053119869  FIX(2.053119869)
#define FIX_2_562915447  FIX(2.562915447)
#define FIX_3_072711026  FIX(3.072711026)
#endif

#if BITS_IN_JSAMPLE == 8
#define MULTIPLY(var,const)  MULTIPLY16C16(var,const)
#else
#define MULTIPLY(var,const)  ((var) * (const))
#endif

GLOBAL(void)
jpeg_fdct_islow (DCTELEM * data, JSAMPARRAY sample_data, JDIMENSION start_col)
{
  INT32 tmp0, tmp1, tmp2, tmp3;
  INT32 tmp10, tmp11, tmp12, tmp13;
  INT32 z1;
  DCTELEM *dataptr;
  JSAMPROW elemptr;
  int ctr;
  SHIFT_TEMPS

  
  
  

  dataptr = data;
  for (ctr = 0; ctr < DCTSIZE; ctr++) {
    elemptr = sample_data[ctr] + start_col;

    

    tmp0 = GETJSAMPLE(elemptr[0]) + GETJSAMPLE(elemptr[7]);
    tmp1 = GETJSAMPLE(elemptr[1]) + GETJSAMPLE(elemptr[6]);
    tmp2 = GETJSAMPLE(elemptr[2]) + GETJSAMPLE(elemptr[5]);
    tmp3 = GETJSAMPLE(elemptr[3]) + GETJSAMPLE(elemptr[4]);

    tmp10 = tmp0 + tmp3;
    tmp12 = tmp0 - tmp3;
    tmp11 = tmp1 + tmp2;
    tmp13 = tmp1 - tmp2;

    tmp0 = GETJSAMPLE(elemptr[0]) - GETJSAMPLE(elemptr[7]);
    tmp1 = GETJSAMPLE(elemptr[1]) - GETJSAMPLE(elemptr[6]);
    tmp2 = GETJSAMPLE(elemptr[2]) - GETJSAMPLE(elemptr[5]);
    tmp3 = GETJSAMPLE(elemptr[3]) - GETJSAMPLE(elemptr[4]);

    
    dataptr[0] = (DCTELEM) ((tmp10 + tmp11 - 8 * CENTERJSAMPLE) << PASS1_BITS);
    dataptr[4] = (DCTELEM) ((tmp10 - tmp11) << PASS1_BITS);

    z1 = MULTIPLY(tmp12 + tmp13, FIX_0_541196100);
    
    z1 += ONE << (CONST_BITS-PASS1_BITS-1);
    dataptr[2] = (DCTELEM) RIGHT_SHIFT(z1 + MULTIPLY(tmp12, FIX_0_765366865),
				       CONST_BITS-PASS1_BITS);
    dataptr[6] = (DCTELEM) RIGHT_SHIFT(z1 - MULTIPLY(tmp13, FIX_1_847759065),
				       CONST_BITS-PASS1_BITS);

    

    tmp10 = tmp0 + tmp3;
    tmp11 = tmp1 + tmp2;
    tmp12 = tmp0 + tmp2;
    tmp13 = tmp1 + tmp3;
    z1 = MULTIPLY(tmp12 + tmp13, FIX_1_175875602); 
    
    z1 += ONE << (CONST_BITS-PASS1_BITS-1);

    tmp0  = MULTIPLY(tmp0,    FIX_1_501321110);    
    tmp1  = MULTIPLY(tmp1,    FIX_3_072711026);    
    tmp2  = MULTIPLY(tmp2,    FIX_2_053119869);    
    tmp3  = MULTIPLY(tmp3,    FIX_0_298631336);    
    tmp10 = MULTIPLY(tmp10, - FIX_0_899976223);    
    tmp11 = MULTIPLY(tmp11, - FIX_2_562915447);    
    tmp12 = MULTIPLY(tmp12, - FIX_0_390180644);    
    tmp13 = MULTIPLY(tmp13, - FIX_1_961570560);    

    tmp12 += z1;
    tmp13 += z1;

    dataptr[1] = (DCTELEM)
      RIGHT_SHIFT(tmp0 + tmp10 + tmp12, CONST_BITS-PASS1_BITS);
    dataptr[3] = (DCTELEM)
      RIGHT_SHIFT(tmp1 + tmp11 + tmp13, CONST_BITS-PASS1_BITS);
    dataptr[5] = (DCTELEM)
      RIGHT_SHIFT(tmp2 + tmp11 + tmp12, CONST_BITS-PASS1_BITS);
    dataptr[7] = (DCTELEM)
      RIGHT_SHIFT(tmp3 + tmp10 + tmp13, CONST_BITS-PASS1_BITS);

    dataptr += DCTSIZE;		
  }

  

  dataptr = data;
  for (ctr = DCTSIZE-1; ctr >= 0; ctr--) {
    

    tmp0 = dataptr[DCTSIZE*0] + dataptr[DCTSIZE*7];
    tmp1 = dataptr[DCTSIZE*1] + dataptr[DCTSIZE*6];
    tmp2 = dataptr[DCTSIZE*2] + dataptr[DCTSIZE*5];
    tmp3 = dataptr[DCTSIZE*3] + dataptr[DCTSIZE*4];

    
    tmp10 = tmp0 + tmp3 + (ONE << (PASS1_BITS-1));
    tmp12 = tmp0 - tmp3;
    tmp11 = tmp1 + tmp2;
    tmp13 = tmp1 - tmp2;

    tmp0 = dataptr[DCTSIZE*0] - dataptr[DCTSIZE*7];
    tmp1 = dataptr[DCTSIZE*1] - dataptr[DCTSIZE*6];
    tmp2 = dataptr[DCTSIZE*2] - dataptr[DCTSIZE*5];
    tmp3 = dataptr[DCTSIZE*3] - dataptr[DCTSIZE*4];

    dataptr[DCTSIZE*0] = (DCTELEM) RIGHT_SHIFT(tmp10 + tmp11, PASS1_BITS);
    dataptr[DCTSIZE*4] = (DCTELEM) RIGHT_SHIFT(tmp10 - tmp11, PASS1_BITS);

    z1 = MULTIPLY(tmp12 + tmp13, FIX_0_541196100);
    
    z1 += ONE << (CONST_BITS+PASS1_BITS-1);
    dataptr[DCTSIZE*2] = (DCTELEM)
      RIGHT_SHIFT(z1 + MULTIPLY(tmp12, FIX_0_765366865), CONST_BITS+PASS1_BITS);
    dataptr[DCTSIZE*6] = (DCTELEM)
      RIGHT_SHIFT(z1 - MULTIPLY(tmp13, FIX_1_847759065), CONST_BITS+PASS1_BITS);

    

    tmp10 = tmp0 + tmp3;
    tmp11 = tmp1 + tmp2;
    tmp12 = tmp0 + tmp2;
    tmp13 = tmp1 + tmp3;
    z1 = MULTIPLY(tmp12 + tmp13, FIX_1_175875602); 
    
    z1 += ONE << (CONST_BITS+PASS1_BITS-1);

    tmp0  = MULTIPLY(tmp0,    FIX_1_501321110);    
    tmp1  = MULTIPLY(tmp1,    FIX_3_072711026);    
    tmp2  = MULTIPLY(tmp2,    FIX_2_053119869);    
    tmp3  = MULTIPLY(tmp3,    FIX_0_298631336);    
    tmp10 = MULTIPLY(tmp10, - FIX_0_899976223);    
    tmp11 = MULTIPLY(tmp11, - FIX_2_562915447);    
    tmp12 = MULTIPLY(tmp12, - FIX_0_390180644);    
    tmp13 = MULTIPLY(tmp13, - FIX_1_961570560);    

    tmp12 += z1;
    tmp13 += z1;

    dataptr[DCTSIZE*1] = (DCTELEM)
      RIGHT_SHIFT(tmp0 + tmp10 + tmp12, CONST_BITS+PASS1_BITS);
    dataptr[DCTSIZE*3] = (DCTELEM)
      RIGHT_SHIFT(tmp1 + tmp11 + tmp13, CONST_BITS+PASS1_BITS);
    dataptr[DCTSIZE*5] = (DCTELEM)
      RIGHT_SHIFT(tmp2 + tmp11 + tmp12, CONST_BITS+PASS1_BITS);
    dataptr[DCTSIZE*7] = (DCTELEM)
      RIGHT_SHIFT(tmp3 + tmp10 + tmp13, CONST_BITS+PASS1_BITS);

    dataptr++;			
  }
}

#ifdef DCT_SCALING_SUPPORTED

GLOBAL(void)
jpeg_fdct_7x7 (DCTELEM * data, JSAMPARRAY sample_data, JDIMENSION start_col)
{
  INT32 tmp0, tmp1, tmp2, tmp3;
  INT32 tmp10, tmp11, tmp12;
  INT32 z1, z2, z3;
  DCTELEM *dataptr;
  JSAMPROW elemptr;
  int ctr;
  SHIFT_TEMPS

  
  MEMZERO(data, SIZEOF(DCTELEM) * DCTSIZE2);

  
  
  
  

  dataptr = data;
  for (ctr = 0; ctr < 7; ctr++) {
    elemptr = sample_data[ctr] + start_col;

    

    tmp0 = GETJSAMPLE(elemptr[0]) + GETJSAMPLE(elemptr[6]);
    tmp1 = GETJSAMPLE(elemptr[1]) + GETJSAMPLE(elemptr[5]);
    tmp2 = GETJSAMPLE(elemptr[2]) + GETJSAMPLE(elemptr[4]);
    tmp3 = GETJSAMPLE(elemptr[3]);

    tmp10 = GETJSAMPLE(elemptr[0]) - GETJSAMPLE(elemptr[6]);
    tmp11 = GETJSAMPLE(elemptr[1]) - GETJSAMPLE(elemptr[5]);
    tmp12 = GETJSAMPLE(elemptr[2]) - GETJSAMPLE(elemptr[4]);

    z1 = tmp0 + tmp2;
    
    dataptr[0] = (DCTELEM)
      ((z1 + tmp1 + tmp3 - 7 * CENTERJSAMPLE) << PASS1_BITS);
    tmp3 += tmp3;
    z1 -= tmp3;
    z1 -= tmp3;
    z1 = MULTIPLY(z1, FIX(0.353553391));                
    z2 = MULTIPLY(tmp0 - tmp2, FIX(0.920609002));       
    z3 = MULTIPLY(tmp1 - tmp2, FIX(0.314692123));       
    dataptr[2] = (DCTELEM) DESCALE(z1 + z2 + z3, CONST_BITS-PASS1_BITS);
    z1 -= z2;
    z2 = MULTIPLY(tmp0 - tmp1, FIX(0.881747734));       
    dataptr[4] = (DCTELEM)
      DESCALE(z2 + z3 - MULTIPLY(tmp1 - tmp3, FIX(0.707106781)), 
	      CONST_BITS-PASS1_BITS);
    dataptr[6] = (DCTELEM) DESCALE(z1 + z2, CONST_BITS-PASS1_BITS);

    

    tmp1 = MULTIPLY(tmp10 + tmp11, FIX(0.935414347));   
    tmp2 = MULTIPLY(tmp10 - tmp11, FIX(0.170262339));   
    tmp0 = tmp1 - tmp2;
    tmp1 += tmp2;
    tmp2 = MULTIPLY(tmp11 + tmp12, - FIX(1.378756276)); 
    tmp1 += tmp2;
    tmp3 = MULTIPLY(tmp10 + tmp12, FIX(0.613604268));   
    tmp0 += tmp3;
    tmp2 += tmp3 + MULTIPLY(tmp12, FIX(1.870828693));   

    dataptr[1] = (DCTELEM) DESCALE(tmp0, CONST_BITS-PASS1_BITS);
    dataptr[3] = (DCTELEM) DESCALE(tmp1, CONST_BITS-PASS1_BITS);
    dataptr[5] = (DCTELEM) DESCALE(tmp2, CONST_BITS-PASS1_BITS);

    dataptr += DCTSIZE;		
  }

  

  dataptr = data;
  for (ctr = 0; ctr < 7; ctr++) {
    

    tmp0 = dataptr[DCTSIZE*0] + dataptr[DCTSIZE*6];
    tmp1 = dataptr[DCTSIZE*1] + dataptr[DCTSIZE*5];
    tmp2 = dataptr[DCTSIZE*2] + dataptr[DCTSIZE*4];
    tmp3 = dataptr[DCTSIZE*3];

    tmp10 = dataptr[DCTSIZE*0] - dataptr[DCTSIZE*6];
    tmp11 = dataptr[DCTSIZE*1] - dataptr[DCTSIZE*5];
    tmp12 = dataptr[DCTSIZE*2] - dataptr[DCTSIZE*4];

    z1 = tmp0 + tmp2;
    dataptr[DCTSIZE*0] = (DCTELEM)
      DESCALE(MULTIPLY(z1 + tmp1 + tmp3, FIX(1.306122449)), 
	      CONST_BITS+PASS1_BITS);
    tmp3 += tmp3;
    z1 -= tmp3;
    z1 -= tmp3;
    z1 = MULTIPLY(z1, FIX(0.461784020));                
    z2 = MULTIPLY(tmp0 - tmp2, FIX(1.202428084));       
    z3 = MULTIPLY(tmp1 - tmp2, FIX(0.411026446));       
    dataptr[DCTSIZE*2] = (DCTELEM) DESCALE(z1 + z2 + z3, CONST_BITS+PASS1_BITS);
    z1 -= z2;
    z2 = MULTIPLY(tmp0 - tmp1, FIX(1.151670509));       
    dataptr[DCTSIZE*4] = (DCTELEM)
      DESCALE(z2 + z3 - MULTIPLY(tmp1 - tmp3, FIX(0.923568041)), 
	      CONST_BITS+PASS1_BITS);
    dataptr[DCTSIZE*6] = (DCTELEM) DESCALE(z1 + z2, CONST_BITS+PASS1_BITS);

    

    tmp1 = MULTIPLY(tmp10 + tmp11, FIX(1.221765677));   
    tmp2 = MULTIPLY(tmp10 - tmp11, FIX(0.222383464));   
    tmp0 = tmp1 - tmp2;
    tmp1 += tmp2;
    tmp2 = MULTIPLY(tmp11 + tmp12, - FIX(1.800824523)); 
    tmp1 += tmp2;
    tmp3 = MULTIPLY(tmp10 + tmp12, FIX(0.801442310));   
    tmp0 += tmp3;
    tmp2 += tmp3 + MULTIPLY(tmp12, FIX(2.443531355));   

    dataptr[DCTSIZE*1] = (DCTELEM) DESCALE(tmp0, CONST_BITS+PASS1_BITS);
    dataptr[DCTSIZE*3] = (DCTELEM) DESCALE(tmp1, CONST_BITS+PASS1_BITS);
    dataptr[DCTSIZE*5] = (DCTELEM) DESCALE(tmp2, CONST_BITS+PASS1_BITS);

    dataptr++;			
  }
}

GLOBAL(void)
jpeg_fdct_6x6 (DCTELEM * data, JSAMPARRAY sample_data, JDIMENSION start_col)
{
  INT32 tmp0, tmp1, tmp2;
  INT32 tmp10, tmp11, tmp12;
  DCTELEM *dataptr;
  JSAMPROW elemptr;
  int ctr;
  SHIFT_TEMPS

  
  MEMZERO(data, SIZEOF(DCTELEM) * DCTSIZE2);

  
  
  
  

  dataptr = data;
  for (ctr = 0; ctr < 6; ctr++) {
    elemptr = sample_data[ctr] + start_col;

    

    tmp0 = GETJSAMPLE(elemptr[0]) + GETJSAMPLE(elemptr[5]);
    tmp11 = GETJSAMPLE(elemptr[1]) + GETJSAMPLE(elemptr[4]);
    tmp2 = GETJSAMPLE(elemptr[2]) + GETJSAMPLE(elemptr[3]);

    tmp10 = tmp0 + tmp2;
    tmp12 = tmp0 - tmp2;

    tmp0 = GETJSAMPLE(elemptr[0]) - GETJSAMPLE(elemptr[5]);
    tmp1 = GETJSAMPLE(elemptr[1]) - GETJSAMPLE(elemptr[4]);
    tmp2 = GETJSAMPLE(elemptr[2]) - GETJSAMPLE(elemptr[3]);

    
    dataptr[0] = (DCTELEM)
      ((tmp10 + tmp11 - 6 * CENTERJSAMPLE) << PASS1_BITS);
    dataptr[2] = (DCTELEM)
      DESCALE(MULTIPLY(tmp12, FIX(1.224744871)),                 
	      CONST_BITS-PASS1_BITS);
    dataptr[4] = (DCTELEM)
      DESCALE(MULTIPLY(tmp10 - tmp11 - tmp11, FIX(0.707106781)), 
	      CONST_BITS-PASS1_BITS);

    

    tmp10 = DESCALE(MULTIPLY(tmp0 + tmp2, FIX(0.366025404)),     
		    CONST_BITS-PASS1_BITS);

    dataptr[1] = (DCTELEM) (tmp10 + ((tmp0 + tmp1) << PASS1_BITS));
    dataptr[3] = (DCTELEM) ((tmp0 - tmp1 - tmp2) << PASS1_BITS);
    dataptr[5] = (DCTELEM) (tmp10 + ((tmp2 - tmp1) << PASS1_BITS));

    dataptr += DCTSIZE;		
  }

  

  dataptr = data;
  for (ctr = 0; ctr < 6; ctr++) {
    

    tmp0 = dataptr[DCTSIZE*0] + dataptr[DCTSIZE*5];
    tmp11 = dataptr[DCTSIZE*1] + dataptr[DCTSIZE*4];
    tmp2 = dataptr[DCTSIZE*2] + dataptr[DCTSIZE*3];

    tmp10 = tmp0 + tmp2;
    tmp12 = tmp0 - tmp2;

    tmp0 = dataptr[DCTSIZE*0] - dataptr[DCTSIZE*5];
    tmp1 = dataptr[DCTSIZE*1] - dataptr[DCTSIZE*4];
    tmp2 = dataptr[DCTSIZE*2] - dataptr[DCTSIZE*3];

    dataptr[DCTSIZE*0] = (DCTELEM)
      DESCALE(MULTIPLY(tmp10 + tmp11, FIX(1.777777778)),         
	      CONST_BITS+PASS1_BITS);
    dataptr[DCTSIZE*2] = (DCTELEM)
      DESCALE(MULTIPLY(tmp12, FIX(2.177324216)),                 
	      CONST_BITS+PASS1_BITS);
    dataptr[DCTSIZE*4] = (DCTELEM)
      DESCALE(MULTIPLY(tmp10 - tmp11 - tmp11, FIX(1.257078722)), 
	      CONST_BITS+PASS1_BITS);

    

    tmp10 = MULTIPLY(tmp0 + tmp2, FIX(0.650711829));             

    dataptr[DCTSIZE*1] = (DCTELEM)
      DESCALE(tmp10 + MULTIPLY(tmp0 + tmp1, FIX(1.777777778)),   
	      CONST_BITS+PASS1_BITS);
    dataptr[DCTSIZE*3] = (DCTELEM)
      DESCALE(MULTIPLY(tmp0 - tmp1 - tmp2, FIX(1.777777778)),    
	      CONST_BITS+PASS1_BITS);
    dataptr[DCTSIZE*5] = (DCTELEM)
      DESCALE(tmp10 + MULTIPLY(tmp2 - tmp1, FIX(1.777777778)),   
	      CONST_BITS+PASS1_BITS);

    dataptr++;			
  }
}

GLOBAL(void)
jpeg_fdct_5x5 (DCTELEM * data, JSAMPARRAY sample_data, JDIMENSION start_col)
{
  INT32 tmp0, tmp1, tmp2;
  INT32 tmp10, tmp11;
  DCTELEM *dataptr;
  JSAMPROW elemptr;
  int ctr;
  SHIFT_TEMPS

  
  MEMZERO(data, SIZEOF(DCTELEM) * DCTSIZE2);

  
  
  
  
  
  

  dataptr = data;
  for (ctr = 0; ctr < 5; ctr++) {
    elemptr = sample_data[ctr] + start_col;

    

    tmp0 = GETJSAMPLE(elemptr[0]) + GETJSAMPLE(elemptr[4]);
    tmp1 = GETJSAMPLE(elemptr[1]) + GETJSAMPLE(elemptr[3]);
    tmp2 = GETJSAMPLE(elemptr[2]);

    tmp10 = tmp0 + tmp1;
    tmp11 = tmp0 - tmp1;

    tmp0 = GETJSAMPLE(elemptr[0]) - GETJSAMPLE(elemptr[4]);
    tmp1 = GETJSAMPLE(elemptr[1]) - GETJSAMPLE(elemptr[3]);

    
    dataptr[0] = (DCTELEM)
      ((tmp10 + tmp2 - 5 * CENTERJSAMPLE) << (PASS1_BITS+1));
    tmp11 = MULTIPLY(tmp11, FIX(0.790569415));          
    tmp10 -= tmp2 << 2;
    tmp10 = MULTIPLY(tmp10, FIX(0.353553391));          
    dataptr[2] = (DCTELEM) DESCALE(tmp11 + tmp10, CONST_BITS-PASS1_BITS-1);
    dataptr[4] = (DCTELEM) DESCALE(tmp11 - tmp10, CONST_BITS-PASS1_BITS-1);

    

    tmp10 = MULTIPLY(tmp0 + tmp1, FIX(0.831253876));    

    dataptr[1] = (DCTELEM)
      DESCALE(tmp10 + MULTIPLY(tmp0, FIX(0.513743148)), 
	      CONST_BITS-PASS1_BITS-1);
    dataptr[3] = (DCTELEM)
      DESCALE(tmp10 - MULTIPLY(tmp1, FIX(2.176250899)), 
	      CONST_BITS-PASS1_BITS-1);

    dataptr += DCTSIZE;		
  }

  

  dataptr = data;
  for (ctr = 0; ctr < 5; ctr++) {
    

    tmp0 = dataptr[DCTSIZE*0] + dataptr[DCTSIZE*4];
    tmp1 = dataptr[DCTSIZE*1] + dataptr[DCTSIZE*3];
    tmp2 = dataptr[DCTSIZE*2];

    tmp10 = tmp0 + tmp1;
    tmp11 = tmp0 - tmp1;

    tmp0 = dataptr[DCTSIZE*0] - dataptr[DCTSIZE*4];
    tmp1 = dataptr[DCTSIZE*1] - dataptr[DCTSIZE*3];

    dataptr[DCTSIZE*0] = (DCTELEM)
      DESCALE(MULTIPLY(tmp10 + tmp2, FIX(1.28)),        
	      CONST_BITS+PASS1_BITS);
    tmp11 = MULTIPLY(tmp11, FIX(1.011928851));          
    tmp10 -= tmp2 << 2;
    tmp10 = MULTIPLY(tmp10, FIX(0.452548340));          
    dataptr[DCTSIZE*2] = (DCTELEM) DESCALE(tmp11 + tmp10, CONST_BITS+PASS1_BITS);
    dataptr[DCTSIZE*4] = (DCTELEM) DESCALE(tmp11 - tmp10, CONST_BITS+PASS1_BITS);

    

    tmp10 = MULTIPLY(tmp0 + tmp1, FIX(1.064004961));    

    dataptr[DCTSIZE*1] = (DCTELEM)
      DESCALE(tmp10 + MULTIPLY(tmp0, FIX(0.657591230)), 
	      CONST_BITS+PASS1_BITS);
    dataptr[DCTSIZE*3] = (DCTELEM)
      DESCALE(tmp10 - MULTIPLY(tmp1, FIX(2.785601151)), 
	      CONST_BITS+PASS1_BITS);

    dataptr++;			
  }
}

GLOBAL(void)
jpeg_fdct_4x4 (DCTELEM * data, JSAMPARRAY sample_data, JDIMENSION start_col)
{
  INT32 tmp0, tmp1;
  INT32 tmp10, tmp11;
  DCTELEM *dataptr;
  JSAMPROW elemptr;
  int ctr;
  SHIFT_TEMPS

  
  MEMZERO(data, SIZEOF(DCTELEM) * DCTSIZE2);

  
  
  
  
  

  dataptr = data;
  for (ctr = 0; ctr < 4; ctr++) {
    elemptr = sample_data[ctr] + start_col;

    

    tmp0 = GETJSAMPLE(elemptr[0]) + GETJSAMPLE(elemptr[3]);
    tmp1 = GETJSAMPLE(elemptr[1]) + GETJSAMPLE(elemptr[2]);

    tmp10 = GETJSAMPLE(elemptr[0]) - GETJSAMPLE(elemptr[3]);
    tmp11 = GETJSAMPLE(elemptr[1]) - GETJSAMPLE(elemptr[2]);

    
    dataptr[0] = (DCTELEM)
      ((tmp0 + tmp1 - 4 * CENTERJSAMPLE) << (PASS1_BITS+2));
    dataptr[2] = (DCTELEM) ((tmp0 - tmp1) << (PASS1_BITS+2));

    

    tmp0 = MULTIPLY(tmp10 + tmp11, FIX_0_541196100);       
    
    tmp0 += ONE << (CONST_BITS-PASS1_BITS-3);

    dataptr[1] = (DCTELEM)
      RIGHT_SHIFT(tmp0 + MULTIPLY(tmp10, FIX_0_765366865), 
		  CONST_BITS-PASS1_BITS-2);
    dataptr[3] = (DCTELEM)
      RIGHT_SHIFT(tmp0 - MULTIPLY(tmp11, FIX_1_847759065), 
		  CONST_BITS-PASS1_BITS-2);

    dataptr += DCTSIZE;		
  }

  

  dataptr = data;
  for (ctr = 0; ctr < 4; ctr++) {
    

    
    tmp0 = dataptr[DCTSIZE*0] + dataptr[DCTSIZE*3] + (ONE << (PASS1_BITS-1));
    tmp1 = dataptr[DCTSIZE*1] + dataptr[DCTSIZE*2];

    tmp10 = dataptr[DCTSIZE*0] - dataptr[DCTSIZE*3];
    tmp11 = dataptr[DCTSIZE*1] - dataptr[DCTSIZE*2];

    dataptr[DCTSIZE*0] = (DCTELEM) RIGHT_SHIFT(tmp0 + tmp1, PASS1_BITS);
    dataptr[DCTSIZE*2] = (DCTELEM) RIGHT_SHIFT(tmp0 - tmp1, PASS1_BITS);

    

    tmp0 = MULTIPLY(tmp10 + tmp11, FIX_0_541196100);       
    
    tmp0 += ONE << (CONST_BITS+PASS1_BITS-1);

    dataptr[DCTSIZE*1] = (DCTELEM)
      RIGHT_SHIFT(tmp0 + MULTIPLY(tmp10, FIX_0_765366865), 
		  CONST_BITS+PASS1_BITS);
    dataptr[DCTSIZE*3] = (DCTELEM)
      RIGHT_SHIFT(tmp0 - MULTIPLY(tmp11, FIX_1_847759065), 
		  CONST_BITS+PASS1_BITS);

    dataptr++;			
  }
}

GLOBAL(void)
jpeg_fdct_3x3 (DCTELEM * data, JSAMPARRAY sample_data, JDIMENSION start_col)
{
  INT32 tmp0, tmp1, tmp2;
  DCTELEM *dataptr;
  JSAMPROW elemptr;
  int ctr;
  SHIFT_TEMPS

  
  MEMZERO(data, SIZEOF(DCTELEM) * DCTSIZE2);

  
  
  
  
  
  

  dataptr = data;
  for (ctr = 0; ctr < 3; ctr++) {
    elemptr = sample_data[ctr] + start_col;

    

    tmp0 = GETJSAMPLE(elemptr[0]) + GETJSAMPLE(elemptr[2]);
    tmp1 = GETJSAMPLE(elemptr[1]);

    tmp2 = GETJSAMPLE(elemptr[0]) - GETJSAMPLE(elemptr[2]);

    
    dataptr[0] = (DCTELEM)
      ((tmp0 + tmp1 - 3 * CENTERJSAMPLE) << (PASS1_BITS+2));
    dataptr[2] = (DCTELEM)
      DESCALE(MULTIPLY(tmp0 - tmp1 - tmp1, FIX(0.707106781)), 
	      CONST_BITS-PASS1_BITS-2);

    

    dataptr[1] = (DCTELEM)
      DESCALE(MULTIPLY(tmp2, FIX(1.224744871)),               
	      CONST_BITS-PASS1_BITS-2);

    dataptr += DCTSIZE;		
  }

  

  dataptr = data;
  for (ctr = 0; ctr < 3; ctr++) {
    

    tmp0 = dataptr[DCTSIZE*0] + dataptr[DCTSIZE*2];
    tmp1 = dataptr[DCTSIZE*1];

    tmp2 = dataptr[DCTSIZE*0] - dataptr[DCTSIZE*2];

    dataptr[DCTSIZE*0] = (DCTELEM)
      DESCALE(MULTIPLY(tmp0 + tmp1, FIX(1.777777778)),        
	      CONST_BITS+PASS1_BITS);
    dataptr[DCTSIZE*2] = (DCTELEM)
      DESCALE(MULTIPLY(tmp0 - tmp1 - tmp1, FIX(1.257078722)), 
	      CONST_BITS+PASS1_BITS);

    

    dataptr[DCTSIZE*1] = (DCTELEM)
      DESCALE(MULTIPLY(tmp2, FIX(2.177324216)),               
	      CONST_BITS+PASS1_BITS);

    dataptr++;			
  }
}

GLOBAL(void)
jpeg_fdct_2x2 (DCTELEM * data, JSAMPARRAY sample_data, JDIMENSION start_col)
{
  INT32 tmp0, tmp1, tmp2, tmp3;
  JSAMPROW elemptr;

  
  MEMZERO(data, SIZEOF(DCTELEM) * DCTSIZE2);

  
  

  
  elemptr = sample_data[0] + start_col;

  tmp0 = GETJSAMPLE(elemptr[0]) + GETJSAMPLE(elemptr[1]);
  tmp1 = GETJSAMPLE(elemptr[0]) - GETJSAMPLE(elemptr[1]);

  
  elemptr = sample_data[1] + start_col;

  tmp2 = GETJSAMPLE(elemptr[0]) + GETJSAMPLE(elemptr[1]);
  tmp3 = GETJSAMPLE(elemptr[0]) - GETJSAMPLE(elemptr[1]);

  

  
  
  data[DCTSIZE*0] = (DCTELEM) ((tmp0 + tmp2 - 4 * CENTERJSAMPLE) << 4);
  data[DCTSIZE*1] = (DCTELEM) ((tmp0 - tmp2) << 4);

  
  data[DCTSIZE*0+1] = (DCTELEM) ((tmp1 + tmp3) << 4);
  data[DCTSIZE*1+1] = (DCTELEM) ((tmp1 - tmp3) << 4);
}

GLOBAL(void)
jpeg_fdct_1x1 (DCTELEM * data, JSAMPARRAY sample_data, JDIMENSION start_col)
{
  
  MEMZERO(data, SIZEOF(DCTELEM) * DCTSIZE2);

  
  
  
  data[0] = (DCTELEM)
    ((GETJSAMPLE(sample_data[0][start_col]) - CENTERJSAMPLE) << 6);
}

GLOBAL(void)
jpeg_fdct_9x9 (DCTELEM * data, JSAMPARRAY sample_data, JDIMENSION start_col)
{
  INT32 tmp0, tmp1, tmp2, tmp3, tmp4;
  INT32 tmp10, tmp11, tmp12, tmp13;
  INT32 z1, z2;
  DCTELEM workspace[8];
  DCTELEM *dataptr;
  DCTELEM *wsptr;
  JSAMPROW elemptr;
  int ctr;
  SHIFT_TEMPS

  
  
  
  
  

  dataptr = data;
  ctr = 0;
  for (;;) {
    elemptr = sample_data[ctr] + start_col;

    

    tmp0 = GETJSAMPLE(elemptr[0]) + GETJSAMPLE(elemptr[8]);
    tmp1 = GETJSAMPLE(elemptr[1]) + GETJSAMPLE(elemptr[7]);
    tmp2 = GETJSAMPLE(elemptr[2]) + GETJSAMPLE(elemptr[6]);
    tmp3 = GETJSAMPLE(elemptr[3]) + GETJSAMPLE(elemptr[5]);
    tmp4 = GETJSAMPLE(elemptr[4]);

    tmp10 = GETJSAMPLE(elemptr[0]) - GETJSAMPLE(elemptr[8]);
    tmp11 = GETJSAMPLE(elemptr[1]) - GETJSAMPLE(elemptr[7]);
    tmp12 = GETJSAMPLE(elemptr[2]) - GETJSAMPLE(elemptr[6]);
    tmp13 = GETJSAMPLE(elemptr[3]) - GETJSAMPLE(elemptr[5]);

    z1 = tmp0 + tmp2 + tmp3;
    z2 = tmp1 + tmp4;
    
    dataptr[0] = (DCTELEM) ((z1 + z2 - 9 * CENTERJSAMPLE) << 1);
    dataptr[6] = (DCTELEM)
      DESCALE(MULTIPLY(z1 - z2 - z2, FIX(0.707106781)),  
	      CONST_BITS-1);
    z1 = MULTIPLY(tmp0 - tmp2, FIX(1.328926049));        
    z2 = MULTIPLY(tmp1 - tmp4 - tmp4, FIX(0.707106781)); 
    dataptr[2] = (DCTELEM)
      DESCALE(MULTIPLY(tmp2 - tmp3, FIX(1.083350441))    
	      + z1 + z2, CONST_BITS-1);
    dataptr[4] = (DCTELEM)
      DESCALE(MULTIPLY(tmp3 - tmp0, FIX(0.245575608))    
	      + z1 - z2, CONST_BITS-1);

    

    dataptr[3] = (DCTELEM)
      DESCALE(MULTIPLY(tmp10 - tmp12 - tmp13, FIX(1.224744871)), 
	      CONST_BITS-1);

    tmp11 = MULTIPLY(tmp11, FIX(1.224744871));        
    tmp0 = MULTIPLY(tmp10 + tmp12, FIX(0.909038955)); 
    tmp1 = MULTIPLY(tmp10 + tmp13, FIX(0.483689525)); 

    dataptr[1] = (DCTELEM) DESCALE(tmp11 + tmp0 + tmp1, CONST_BITS-1);

    tmp2 = MULTIPLY(tmp12 - tmp13, FIX(1.392728481)); 

    dataptr[5] = (DCTELEM) DESCALE(tmp0 - tmp11 - tmp2, CONST_BITS-1);
    dataptr[7] = (DCTELEM) DESCALE(tmp1 - tmp11 + tmp2, CONST_BITS-1);

    ctr++;

    if (ctr != DCTSIZE) {
      if (ctr == 9)
	break;			
      dataptr += DCTSIZE;	
    } else
      dataptr = workspace;	
  }

  

  dataptr = data;
  wsptr = workspace;
  for (ctr = DCTSIZE-1; ctr >= 0; ctr--) {
    

    tmp0 = dataptr[DCTSIZE*0] + wsptr[DCTSIZE*0];
    tmp1 = dataptr[DCTSIZE*1] + dataptr[DCTSIZE*7];
    tmp2 = dataptr[DCTSIZE*2] + dataptr[DCTSIZE*6];
    tmp3 = dataptr[DCTSIZE*3] + dataptr[DCTSIZE*5];
    tmp4 = dataptr[DCTSIZE*4];

    tmp10 = dataptr[DCTSIZE*0] - wsptr[DCTSIZE*0];
    tmp11 = dataptr[DCTSIZE*1] - dataptr[DCTSIZE*7];
    tmp12 = dataptr[DCTSIZE*2] - dataptr[DCTSIZE*6];
    tmp13 = dataptr[DCTSIZE*3] - dataptr[DCTSIZE*5];

    z1 = tmp0 + tmp2 + tmp3;
    z2 = tmp1 + tmp4;
    dataptr[DCTSIZE*0] = (DCTELEM)
      DESCALE(MULTIPLY(z1 + z2, FIX(1.580246914)),       
	      CONST_BITS+2);
    dataptr[DCTSIZE*6] = (DCTELEM)
      DESCALE(MULTIPLY(z1 - z2 - z2, FIX(1.117403309)),  
	      CONST_BITS+2);
    z1 = MULTIPLY(tmp0 - tmp2, FIX(2.100031287));        
    z2 = MULTIPLY(tmp1 - tmp4 - tmp4, FIX(1.117403309)); 
    dataptr[DCTSIZE*2] = (DCTELEM)
      DESCALE(MULTIPLY(tmp2 - tmp3, FIX(1.711961190))    
	      + z1 + z2, CONST_BITS+2);
    dataptr[DCTSIZE*4] = (DCTELEM)
      DESCALE(MULTIPLY(tmp3 - tmp0, FIX(0.388070096))    
	      + z1 - z2, CONST_BITS+2);

    

    dataptr[DCTSIZE*3] = (DCTELEM)
      DESCALE(MULTIPLY(tmp10 - tmp12 - tmp13, FIX(1.935399303)), 
	      CONST_BITS+2);

    tmp11 = MULTIPLY(tmp11, FIX(1.935399303));        
    tmp0 = MULTIPLY(tmp10 + tmp12, FIX(1.436506004)); 
    tmp1 = MULTIPLY(tmp10 + tmp13, FIX(0.764348879)); 

    dataptr[DCTSIZE*1] = (DCTELEM)
      DESCALE(tmp11 + tmp0 + tmp1, CONST_BITS+2);

    tmp2 = MULTIPLY(tmp12 - tmp13, FIX(2.200854883)); 

    dataptr[DCTSIZE*5] = (DCTELEM)
      DESCALE(tmp0 - tmp11 - tmp2, CONST_BITS+2);
    dataptr[DCTSIZE*7] = (DCTELEM)
      DESCALE(tmp1 - tmp11 + tmp2, CONST_BITS+2);

    dataptr++;			
    wsptr++;			
  }
}

GLOBAL(void)
jpeg_fdct_10x10 (DCTELEM * data, JSAMPARRAY sample_data, JDIMENSION start_col)
{
  INT32 tmp0, tmp1, tmp2, tmp3, tmp4;
  INT32 tmp10, tmp11, tmp12, tmp13, tmp14;
  DCTELEM workspace[8*2];
  DCTELEM *dataptr;
  DCTELEM *wsptr;
  JSAMPROW elemptr;
  int ctr;
  SHIFT_TEMPS

  
  
  
  
  

  dataptr = data;
  ctr = 0;
  for (;;) {
    elemptr = sample_data[ctr] + start_col;

    

    tmp0 = GETJSAMPLE(elemptr[0]) + GETJSAMPLE(elemptr[9]);
    tmp1 = GETJSAMPLE(elemptr[1]) + GETJSAMPLE(elemptr[8]);
    tmp12 = GETJSAMPLE(elemptr[2]) + GETJSAMPLE(elemptr[7]);
    tmp3 = GETJSAMPLE(elemptr[3]) + GETJSAMPLE(elemptr[6]);
    tmp4 = GETJSAMPLE(elemptr[4]) + GETJSAMPLE(elemptr[5]);

    tmp10 = tmp0 + tmp4;
    tmp13 = tmp0 - tmp4;
    tmp11 = tmp1 + tmp3;
    tmp14 = tmp1 - tmp3;

    tmp0 = GETJSAMPLE(elemptr[0]) - GETJSAMPLE(elemptr[9]);
    tmp1 = GETJSAMPLE(elemptr[1]) - GETJSAMPLE(elemptr[8]);
    tmp2 = GETJSAMPLE(elemptr[2]) - GETJSAMPLE(elemptr[7]);
    tmp3 = GETJSAMPLE(elemptr[3]) - GETJSAMPLE(elemptr[6]);
    tmp4 = GETJSAMPLE(elemptr[4]) - GETJSAMPLE(elemptr[5]);

    
    dataptr[0] = (DCTELEM)
      ((tmp10 + tmp11 + tmp12 - 10 * CENTERJSAMPLE) << 1);
    tmp12 += tmp12;
    dataptr[4] = (DCTELEM)
      DESCALE(MULTIPLY(tmp10 - tmp12, FIX(1.144122806)) - 
	      MULTIPLY(tmp11 - tmp12, FIX(0.437016024)),  
	      CONST_BITS-1);
    tmp10 = MULTIPLY(tmp13 + tmp14, FIX(0.831253876));    
    dataptr[2] = (DCTELEM)
      DESCALE(tmp10 + MULTIPLY(tmp13, FIX(0.513743148)),  
	      CONST_BITS-1);
    dataptr[6] = (DCTELEM)
      DESCALE(tmp10 - MULTIPLY(tmp14, FIX(2.176250899)),  
	      CONST_BITS-1);

    

    tmp10 = tmp0 + tmp4;
    tmp11 = tmp1 - tmp3;
    dataptr[5] = (DCTELEM) ((tmp10 - tmp11 - tmp2) << 1);
    tmp2 <<= CONST_BITS;
    dataptr[1] = (DCTELEM)
      DESCALE(MULTIPLY(tmp0, FIX(1.396802247)) +          
	      MULTIPLY(tmp1, FIX(1.260073511)) + tmp2 +   
	      MULTIPLY(tmp3, FIX(0.642039522)) +          
	      MULTIPLY(tmp4, FIX(0.221231742)),           
	      CONST_BITS-1);
    tmp12 = MULTIPLY(tmp0 - tmp4, FIX(0.951056516)) -     
	    MULTIPLY(tmp1 + tmp3, FIX(0.587785252));      
    tmp13 = MULTIPLY(tmp10 + tmp11, FIX(0.309016994)) +   
	    (tmp11 << (CONST_BITS - 1)) - tmp2;
    dataptr[3] = (DCTELEM) DESCALE(tmp12 + tmp13, CONST_BITS-1);
    dataptr[7] = (DCTELEM) DESCALE(tmp12 - tmp13, CONST_BITS-1);

    ctr++;

    if (ctr != DCTSIZE) {
      if (ctr == 10)
	break;			
      dataptr += DCTSIZE;	
    } else
      dataptr = workspace;	
  }

  

  dataptr = data;
  wsptr = workspace;
  for (ctr = DCTSIZE-1; ctr >= 0; ctr--) {
    

    tmp0 = dataptr[DCTSIZE*0] + wsptr[DCTSIZE*1];
    tmp1 = dataptr[DCTSIZE*1] + wsptr[DCTSIZE*0];
    tmp12 = dataptr[DCTSIZE*2] + dataptr[DCTSIZE*7];
    tmp3 = dataptr[DCTSIZE*3] + dataptr[DCTSIZE*6];
    tmp4 = dataptr[DCTSIZE*4] + dataptr[DCTSIZE*5];

    tmp10 = tmp0 + tmp4;
    tmp13 = tmp0 - tmp4;
    tmp11 = tmp1 + tmp3;
    tmp14 = tmp1 - tmp3;

    tmp0 = dataptr[DCTSIZE*0] - wsptr[DCTSIZE*1];
    tmp1 = dataptr[DCTSIZE*1] - wsptr[DCTSIZE*0];
    tmp2 = dataptr[DCTSIZE*2] - dataptr[DCTSIZE*7];
    tmp3 = dataptr[DCTSIZE*3] - dataptr[DCTSIZE*6];
    tmp4 = dataptr[DCTSIZE*4] - dataptr[DCTSIZE*5];

    dataptr[DCTSIZE*0] = (DCTELEM)
      DESCALE(MULTIPLY(tmp10 + tmp11 + tmp12, FIX(1.28)), 
	      CONST_BITS+2);
    tmp12 += tmp12;
    dataptr[DCTSIZE*4] = (DCTELEM)
      DESCALE(MULTIPLY(tmp10 - tmp12, FIX(1.464477191)) - 
	      MULTIPLY(tmp11 - tmp12, FIX(0.559380511)),  
	      CONST_BITS+2);
    tmp10 = MULTIPLY(tmp13 + tmp14, FIX(1.064004961));    
    dataptr[DCTSIZE*2] = (DCTELEM)
      DESCALE(tmp10 + MULTIPLY(tmp13, FIX(0.657591230)),  
	      CONST_BITS+2);
    dataptr[DCTSIZE*6] = (DCTELEM)
      DESCALE(tmp10 - MULTIPLY(tmp14, FIX(2.785601151)),  
	      CONST_BITS+2);

    

    tmp10 = tmp0 + tmp4;
    tmp11 = tmp1 - tmp3;
    dataptr[DCTSIZE*5] = (DCTELEM)
      DESCALE(MULTIPLY(tmp10 - tmp11 - tmp2, FIX(1.28)),  
	      CONST_BITS+2);
    tmp2 = MULTIPLY(tmp2, FIX(1.28));                     
    dataptr[DCTSIZE*1] = (DCTELEM)
      DESCALE(MULTIPLY(tmp0, FIX(1.787906876)) +          
	      MULTIPLY(tmp1, FIX(1.612894094)) + tmp2 +   
	      MULTIPLY(tmp3, FIX(0.821810588)) +          
	      MULTIPLY(tmp4, FIX(0.283176630)),           
	      CONST_BITS+2);
    tmp12 = MULTIPLY(tmp0 - tmp4, FIX(1.217352341)) -     
	    MULTIPLY(tmp1 + tmp3, FIX(0.752365123));      
    tmp13 = MULTIPLY(tmp10 + tmp11, FIX(0.395541753)) +   
	    MULTIPLY(tmp11, FIX(0.64)) - tmp2;            
    dataptr[DCTSIZE*3] = (DCTELEM) DESCALE(tmp12 + tmp13, CONST_BITS+2);
    dataptr[DCTSIZE*7] = (DCTELEM) DESCALE(tmp12 - tmp13, CONST_BITS+2);

    dataptr++;			
    wsptr++;			
  }
}

GLOBAL(void)
jpeg_fdct_11x11 (DCTELEM * data, JSAMPARRAY sample_data, JDIMENSION start_col)
{
  INT32 tmp0, tmp1, tmp2, tmp3, tmp4, tmp5;
  INT32 tmp10, tmp11, tmp12, tmp13, tmp14;
  INT32 z1, z2, z3;
  DCTELEM workspace[8*3];
  DCTELEM *dataptr;
  DCTELEM *wsptr;
  JSAMPROW elemptr;
  int ctr;
  SHIFT_TEMPS

  
  
  
  
  

  dataptr = data;
  ctr = 0;
  for (;;) {
    elemptr = sample_data[ctr] + start_col;

    

    tmp0 = GETJSAMPLE(elemptr[0]) + GETJSAMPLE(elemptr[10]);
    tmp1 = GETJSAMPLE(elemptr[1]) + GETJSAMPLE(elemptr[9]);
    tmp2 = GETJSAMPLE(elemptr[2]) + GETJSAMPLE(elemptr[8]);
    tmp3 = GETJSAMPLE(elemptr[3]) + GETJSAMPLE(elemptr[7]);
    tmp4 = GETJSAMPLE(elemptr[4]) + GETJSAMPLE(elemptr[6]);
    tmp5 = GETJSAMPLE(elemptr[5]);

    tmp10 = GETJSAMPLE(elemptr[0]) - GETJSAMPLE(elemptr[10]);
    tmp11 = GETJSAMPLE(elemptr[1]) - GETJSAMPLE(elemptr[9]);
    tmp12 = GETJSAMPLE(elemptr[2]) - GETJSAMPLE(elemptr[8]);
    tmp13 = GETJSAMPLE(elemptr[3]) - GETJSAMPLE(elemptr[7]);
    tmp14 = GETJSAMPLE(elemptr[4]) - GETJSAMPLE(elemptr[6]);

    
    dataptr[0] = (DCTELEM)
      ((tmp0 + tmp1 + tmp2 + tmp3 + tmp4 + tmp5 - 11 * CENTERJSAMPLE) << 1);
    tmp5 += tmp5;
    tmp0 -= tmp5;
    tmp1 -= tmp5;
    tmp2 -= tmp5;
    tmp3 -= tmp5;
    tmp4 -= tmp5;
    z1 = MULTIPLY(tmp0 + tmp3, FIX(1.356927976)) +       
	 MULTIPLY(tmp2 + tmp4, FIX(0.201263574));        
    z2 = MULTIPLY(tmp1 - tmp3, FIX(0.926112931));        
    z3 = MULTIPLY(tmp0 - tmp1, FIX(1.189712156));        
    dataptr[2] = (DCTELEM)
      DESCALE(z1 + z2 - MULTIPLY(tmp3, FIX(1.018300590)) 
	      - MULTIPLY(tmp4, FIX(1.390975730)),        
	      CONST_BITS-1);
    dataptr[4] = (DCTELEM)
      DESCALE(z2 + z3 + MULTIPLY(tmp1, FIX(0.062335650)) 
	      - MULTIPLY(tmp2, FIX(1.356927976))         
	      + MULTIPLY(tmp4, FIX(0.587485545)),        
	      CONST_BITS-1);
    dataptr[6] = (DCTELEM)
      DESCALE(z1 + z3 - MULTIPLY(tmp0, FIX(1.620527200)) 
	      - MULTIPLY(tmp2, FIX(0.788749120)),        
	      CONST_BITS-1);

    

    tmp1 = MULTIPLY(tmp10 + tmp11, FIX(1.286413905));    
    tmp2 = MULTIPLY(tmp10 + tmp12, FIX(1.068791298));    
    tmp3 = MULTIPLY(tmp10 + tmp13, FIX(0.764581576));    
    tmp0 = tmp1 + tmp2 + tmp3 - MULTIPLY(tmp10, FIX(1.719967871)) 
	   + MULTIPLY(tmp14, FIX(0.398430003));          
    tmp4 = MULTIPLY(tmp11 + tmp12, - FIX(0.764581576));  
    tmp5 = MULTIPLY(tmp11 + tmp13, - FIX(1.399818907));  
    tmp1 += tmp4 + tmp5 + MULTIPLY(tmp11, FIX(1.276416582)) 
	    - MULTIPLY(tmp14, FIX(1.068791298));         
    tmp10 = MULTIPLY(tmp12 + tmp13, FIX(0.398430003));   
    tmp2 += tmp4 + tmp10 - MULTIPLY(tmp12, FIX(1.989053629)) 
	    + MULTIPLY(tmp14, FIX(1.399818907));         
    tmp3 += tmp5 + tmp10 + MULTIPLY(tmp13, FIX(1.305598626)) 
	    - MULTIPLY(tmp14, FIX(1.286413905));         

    dataptr[1] = (DCTELEM) DESCALE(tmp0, CONST_BITS-1);
    dataptr[3] = (DCTELEM) DESCALE(tmp1, CONST_BITS-1);
    dataptr[5] = (DCTELEM) DESCALE(tmp2, CONST_BITS-1);
    dataptr[7] = (DCTELEM) DESCALE(tmp3, CONST_BITS-1);

    ctr++;

    if (ctr != DCTSIZE) {
      if (ctr == 11)
	break;			
      dataptr += DCTSIZE;	
    } else
      dataptr = workspace;	
  }

  

  dataptr = data;
  wsptr = workspace;
  for (ctr = DCTSIZE-1; ctr >= 0; ctr--) {
    

    tmp0 = dataptr[DCTSIZE*0] + wsptr[DCTSIZE*2];
    tmp1 = dataptr[DCTSIZE*1] + wsptr[DCTSIZE*1];
    tmp2 = dataptr[DCTSIZE*2] + wsptr[DCTSIZE*0];
    tmp3 = dataptr[DCTSIZE*3] + dataptr[DCTSIZE*7];
    tmp4 = dataptr[DCTSIZE*4] + dataptr[DCTSIZE*6];
    tmp5 = dataptr[DCTSIZE*5];

    tmp10 = dataptr[DCTSIZE*0] - wsptr[DCTSIZE*2];
    tmp11 = dataptr[DCTSIZE*1] - wsptr[DCTSIZE*1];
    tmp12 = dataptr[DCTSIZE*2] - wsptr[DCTSIZE*0];
    tmp13 = dataptr[DCTSIZE*3] - dataptr[DCTSIZE*7];
    tmp14 = dataptr[DCTSIZE*4] - dataptr[DCTSIZE*6];

    dataptr[DCTSIZE*0] = (DCTELEM)
      DESCALE(MULTIPLY(tmp0 + tmp1 + tmp2 + tmp3 + tmp4 + tmp5,
		       FIX(1.057851240)),                
	      CONST_BITS+2);
    tmp5 += tmp5;
    tmp0 -= tmp5;
    tmp1 -= tmp5;
    tmp2 -= tmp5;
    tmp3 -= tmp5;
    tmp4 -= tmp5;
    z1 = MULTIPLY(tmp0 + tmp3, FIX(1.435427942)) +       
	 MULTIPLY(tmp2 + tmp4, FIX(0.212906922));        
    z2 = MULTIPLY(tmp1 - tmp3, FIX(0.979689713));        
    z3 = MULTIPLY(tmp0 - tmp1, FIX(1.258538479));        
    dataptr[DCTSIZE*2] = (DCTELEM)
      DESCALE(z1 + z2 - MULTIPLY(tmp3, FIX(1.077210542)) 
	      - MULTIPLY(tmp4, FIX(1.471445400)),        
	      CONST_BITS+2);
    dataptr[DCTSIZE*4] = (DCTELEM)
      DESCALE(z2 + z3 + MULTIPLY(tmp1, FIX(0.065941844)) 
	      - MULTIPLY(tmp2, FIX(1.435427942))         
	      + MULTIPLY(tmp4, FIX(0.621472312)),        
	      CONST_BITS+2);
    dataptr[DCTSIZE*6] = (DCTELEM)
      DESCALE(z1 + z3 - MULTIPLY(tmp0, FIX(1.714276708)) 
	      - MULTIPLY(tmp2, FIX(0.834379234)),        
	      CONST_BITS+2);

    

    tmp1 = MULTIPLY(tmp10 + tmp11, FIX(1.360834544));    
    tmp2 = MULTIPLY(tmp10 + tmp12, FIX(1.130622199));    
    tmp3 = MULTIPLY(tmp10 + tmp13, FIX(0.808813568));    
    tmp0 = tmp1 + tmp2 + tmp3 - MULTIPLY(tmp10, FIX(1.819470145)) 
	   + MULTIPLY(tmp14, FIX(0.421479672));          
    tmp4 = MULTIPLY(tmp11 + tmp12, - FIX(0.808813568));  
    tmp5 = MULTIPLY(tmp11 + tmp13, - FIX(1.480800167));  
    tmp1 += tmp4 + tmp5 + MULTIPLY(tmp11, FIX(1.350258864)) 
	    - MULTIPLY(tmp14, FIX(1.130622199));         
    tmp10 = MULTIPLY(tmp12 + tmp13, FIX(0.421479672));   
    tmp2 += tmp4 + tmp10 - MULTIPLY(tmp12, FIX(2.104122847)) 
	    + MULTIPLY(tmp14, FIX(1.480800167));         
    tmp3 += tmp5 + tmp10 + MULTIPLY(tmp13, FIX(1.381129125)) 
	    - MULTIPLY(tmp14, FIX(1.360834544));         

    dataptr[DCTSIZE*1] = (DCTELEM) DESCALE(tmp0, CONST_BITS+2);
    dataptr[DCTSIZE*3] = (DCTELEM) DESCALE(tmp1, CONST_BITS+2);
    dataptr[DCTSIZE*5] = (DCTELEM) DESCALE(tmp2, CONST_BITS+2);
    dataptr[DCTSIZE*7] = (DCTELEM) DESCALE(tmp3, CONST_BITS+2);

    dataptr++;			
    wsptr++;			
  }
}

GLOBAL(void)
jpeg_fdct_12x12 (DCTELEM * data, JSAMPARRAY sample_data, JDIMENSION start_col)
{
  INT32 tmp0, tmp1, tmp2, tmp3, tmp4, tmp5;
  INT32 tmp10, tmp11, tmp12, tmp13, tmp14, tmp15;
  DCTELEM workspace[8*4];
  DCTELEM *dataptr;
  DCTELEM *wsptr;
  JSAMPROW elemptr;
  int ctr;
  SHIFT_TEMPS

  
  
  

  dataptr = data;
  ctr = 0;
  for (;;) {
    elemptr = sample_data[ctr] + start_col;

    

    tmp0 = GETJSAMPLE(elemptr[0]) + GETJSAMPLE(elemptr[11]);
    tmp1 = GETJSAMPLE(elemptr[1]) + GETJSAMPLE(elemptr[10]);
    tmp2 = GETJSAMPLE(elemptr[2]) + GETJSAMPLE(elemptr[9]);
    tmp3 = GETJSAMPLE(elemptr[3]) + GETJSAMPLE(elemptr[8]);
    tmp4 = GETJSAMPLE(elemptr[4]) + GETJSAMPLE(elemptr[7]);
    tmp5 = GETJSAMPLE(elemptr[5]) + GETJSAMPLE(elemptr[6]);

    tmp10 = tmp0 + tmp5;
    tmp13 = tmp0 - tmp5;
    tmp11 = tmp1 + tmp4;
    tmp14 = tmp1 - tmp4;
    tmp12 = tmp2 + tmp3;
    tmp15 = tmp2 - tmp3;

    tmp0 = GETJSAMPLE(elemptr[0]) - GETJSAMPLE(elemptr[11]);
    tmp1 = GETJSAMPLE(elemptr[1]) - GETJSAMPLE(elemptr[10]);
    tmp2 = GETJSAMPLE(elemptr[2]) - GETJSAMPLE(elemptr[9]);
    tmp3 = GETJSAMPLE(elemptr[3]) - GETJSAMPLE(elemptr[8]);
    tmp4 = GETJSAMPLE(elemptr[4]) - GETJSAMPLE(elemptr[7]);
    tmp5 = GETJSAMPLE(elemptr[5]) - GETJSAMPLE(elemptr[6]);

    
    dataptr[0] = (DCTELEM) (tmp10 + tmp11 + tmp12 - 12 * CENTERJSAMPLE);
    dataptr[6] = (DCTELEM) (tmp13 - tmp14 - tmp15);
    dataptr[4] = (DCTELEM)
      DESCALE(MULTIPLY(tmp10 - tmp12, FIX(1.224744871)), 
	      CONST_BITS);
    dataptr[2] = (DCTELEM)
      DESCALE(tmp14 - tmp15 + MULTIPLY(tmp13 + tmp15, FIX(1.366025404)), 
	      CONST_BITS);

    

    tmp10 = MULTIPLY(tmp1 + tmp4, FIX_0_541196100);    
    tmp14 = tmp10 + MULTIPLY(tmp1, FIX_0_765366865);   
    tmp15 = tmp10 - MULTIPLY(tmp4, FIX_1_847759065);   
    tmp12 = MULTIPLY(tmp0 + tmp2, FIX(1.121971054));   
    tmp13 = MULTIPLY(tmp0 + tmp3, FIX(0.860918669));   
    tmp10 = tmp12 + tmp13 + tmp14 - MULTIPLY(tmp0, FIX(0.580774953)) 
	    + MULTIPLY(tmp5, FIX(0.184591911));        
    tmp11 = MULTIPLY(tmp2 + tmp3, - FIX(0.184591911)); 
    tmp12 += tmp11 - tmp15 - MULTIPLY(tmp2, FIX(2.339493912)) 
	    + MULTIPLY(tmp5, FIX(0.860918669));        
    tmp13 += tmp11 - tmp14 + MULTIPLY(tmp3, FIX(0.725788011)) 
	    - MULTIPLY(tmp5, FIX(1.121971054));        
    tmp11 = tmp15 + MULTIPLY(tmp0 - tmp3, FIX(1.306562965)) 
	    - MULTIPLY(tmp2 + tmp5, FIX_0_541196100);  

    dataptr[1] = (DCTELEM) DESCALE(tmp10, CONST_BITS);
    dataptr[3] = (DCTELEM) DESCALE(tmp11, CONST_BITS);
    dataptr[5] = (DCTELEM) DESCALE(tmp12, CONST_BITS);
    dataptr[7] = (DCTELEM) DESCALE(tmp13, CONST_BITS);

    ctr++;

    if (ctr != DCTSIZE) {
      if (ctr == 12)
	break;			
      dataptr += DCTSIZE;	
    } else
      dataptr = workspace;	
  }

  

  dataptr = data;
  wsptr = workspace;
  for (ctr = DCTSIZE-1; ctr >= 0; ctr--) {
    

    tmp0 = dataptr[DCTSIZE*0] + wsptr[DCTSIZE*3];
    tmp1 = dataptr[DCTSIZE*1] + wsptr[DCTSIZE*2];
    tmp2 = dataptr[DCTSIZE*2] + wsptr[DCTSIZE*1];
    tmp3 = dataptr[DCTSIZE*3] + wsptr[DCTSIZE*0];
    tmp4 = dataptr[DCTSIZE*4] + dataptr[DCTSIZE*7];
    tmp5 = dataptr[DCTSIZE*5] + dataptr[DCTSIZE*6];

    tmp10 = tmp0 + tmp5;
    tmp13 = tmp0 - tmp5;
    tmp11 = tmp1 + tmp4;
    tmp14 = tmp1 - tmp4;
    tmp12 = tmp2 + tmp3;
    tmp15 = tmp2 - tmp3;

    tmp0 = dataptr[DCTSIZE*0] - wsptr[DCTSIZE*3];
    tmp1 = dataptr[DCTSIZE*1] - wsptr[DCTSIZE*2];
    tmp2 = dataptr[DCTSIZE*2] - wsptr[DCTSIZE*1];
    tmp3 = dataptr[DCTSIZE*3] - wsptr[DCTSIZE*0];
    tmp4 = dataptr[DCTSIZE*4] - dataptr[DCTSIZE*7];
    tmp5 = dataptr[DCTSIZE*5] - dataptr[DCTSIZE*6];

    dataptr[DCTSIZE*0] = (DCTELEM)
      DESCALE(MULTIPLY(tmp10 + tmp11 + tmp12, FIX(0.888888889)), 
	      CONST_BITS+1);
    dataptr[DCTSIZE*6] = (DCTELEM)
      DESCALE(MULTIPLY(tmp13 - tmp14 - tmp15, FIX(0.888888889)), 
	      CONST_BITS+1);
    dataptr[DCTSIZE*4] = (DCTELEM)
      DESCALE(MULTIPLY(tmp10 - tmp12, FIX(1.088662108)),         
	      CONST_BITS+1);
    dataptr[DCTSIZE*2] = (DCTELEM)
      DESCALE(MULTIPLY(tmp14 - tmp15, FIX(0.888888889)) +        
	      MULTIPLY(tmp13 + tmp15, FIX(1.214244803)),         
	      CONST_BITS+1);

    

    tmp10 = MULTIPLY(tmp1 + tmp4, FIX(0.481063200));   
    tmp14 = tmp10 + MULTIPLY(tmp1, FIX(0.680326102));  
    tmp15 = tmp10 - MULTIPLY(tmp4, FIX(1.642452502));  
    tmp12 = MULTIPLY(tmp0 + tmp2, FIX(0.997307603));   
    tmp13 = MULTIPLY(tmp0 + tmp3, FIX(0.765261039));   
    tmp10 = tmp12 + tmp13 + tmp14 - MULTIPLY(tmp0, FIX(0.516244403)) 
	    + MULTIPLY(tmp5, FIX(0.164081699));        
    tmp11 = MULTIPLY(tmp2 + tmp3, - FIX(0.164081699)); 
    tmp12 += tmp11 - tmp15 - MULTIPLY(tmp2, FIX(2.079550144)) 
	    + MULTIPLY(tmp5, FIX(0.765261039));        
    tmp13 += tmp11 - tmp14 + MULTIPLY(tmp3, FIX(0.645144899)) 
	    - MULTIPLY(tmp5, FIX(0.997307603));        
    tmp11 = tmp15 + MULTIPLY(tmp0 - tmp3, FIX(1.161389302)) 
	    - MULTIPLY(tmp2 + tmp5, FIX(0.481063200)); 

    dataptr[DCTSIZE*1] = (DCTELEM) DESCALE(tmp10, CONST_BITS+1);
    dataptr[DCTSIZE*3] = (DCTELEM) DESCALE(tmp11, CONST_BITS+1);
    dataptr[DCTSIZE*5] = (DCTELEM) DESCALE(tmp12, CONST_BITS+1);
    dataptr[DCTSIZE*7] = (DCTELEM) DESCALE(tmp13, CONST_BITS+1);

    dataptr++;			
    wsptr++;			
  }
}

GLOBAL(void)
jpeg_fdct_13x13 (DCTELEM * data, JSAMPARRAY sample_data, JDIMENSION start_col)
{
  INT32 tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6;
  INT32 tmp10, tmp11, tmp12, tmp13, tmp14, tmp15;
  INT32 z1, z2;
  DCTELEM workspace[8*5];
  DCTELEM *dataptr;
  DCTELEM *wsptr;
  JSAMPROW elemptr;
  int ctr;
  SHIFT_TEMPS

  
  
  

  dataptr = data;
  ctr = 0;
  for (;;) {
    elemptr = sample_data[ctr] + start_col;

    

    tmp0 = GETJSAMPLE(elemptr[0]) + GETJSAMPLE(elemptr[12]);
    tmp1 = GETJSAMPLE(elemptr[1]) + GETJSAMPLE(elemptr[11]);
    tmp2 = GETJSAMPLE(elemptr[2]) + GETJSAMPLE(elemptr[10]);
    tmp3 = GETJSAMPLE(elemptr[3]) + GETJSAMPLE(elemptr[9]);
    tmp4 = GETJSAMPLE(elemptr[4]) + GETJSAMPLE(elemptr[8]);
    tmp5 = GETJSAMPLE(elemptr[5]) + GETJSAMPLE(elemptr[7]);
    tmp6 = GETJSAMPLE(elemptr[6]);

    tmp10 = GETJSAMPLE(elemptr[0]) - GETJSAMPLE(elemptr[12]);
    tmp11 = GETJSAMPLE(elemptr[1]) - GETJSAMPLE(elemptr[11]);
    tmp12 = GETJSAMPLE(elemptr[2]) - GETJSAMPLE(elemptr[10]);
    tmp13 = GETJSAMPLE(elemptr[3]) - GETJSAMPLE(elemptr[9]);
    tmp14 = GETJSAMPLE(elemptr[4]) - GETJSAMPLE(elemptr[8]);
    tmp15 = GETJSAMPLE(elemptr[5]) - GETJSAMPLE(elemptr[7]);

    
    dataptr[0] = (DCTELEM)
      (tmp0 + tmp1 + tmp2 + tmp3 + tmp4 + tmp5 + tmp6 - 13 * CENTERJSAMPLE);
    tmp6 += tmp6;
    tmp0 -= tmp6;
    tmp1 -= tmp6;
    tmp2 -= tmp6;
    tmp3 -= tmp6;
    tmp4 -= tmp6;
    tmp5 -= tmp6;
    dataptr[2] = (DCTELEM)
      DESCALE(MULTIPLY(tmp0, FIX(1.373119086)) +   
	      MULTIPLY(tmp1, FIX(1.058554052)) +   
	      MULTIPLY(tmp2, FIX(0.501487041)) -   
	      MULTIPLY(tmp3, FIX(0.170464608)) -   
	      MULTIPLY(tmp4, FIX(0.803364869)) -   
	      MULTIPLY(tmp5, FIX(1.252223920)),    
	      CONST_BITS);
    z1 = MULTIPLY(tmp0 - tmp2, FIX(1.155388986)) - 
	 MULTIPLY(tmp3 - tmp4, FIX(0.435816023)) - 
	 MULTIPLY(tmp1 - tmp5, FIX(0.316450131));  
    z2 = MULTIPLY(tmp0 + tmp2, FIX(0.096834934)) - 
	 MULTIPLY(tmp3 + tmp4, FIX(0.937303064)) + 
	 MULTIPLY(tmp1 + tmp5, FIX(0.486914739));  

    dataptr[4] = (DCTELEM) DESCALE(z1 + z2, CONST_BITS);
    dataptr[6] = (DCTELEM) DESCALE(z1 - z2, CONST_BITS);

    

    tmp1 = MULTIPLY(tmp10 + tmp11, FIX(1.322312651));   
    tmp2 = MULTIPLY(tmp10 + tmp12, FIX(1.163874945));   
    tmp3 = MULTIPLY(tmp10 + tmp13, FIX(0.937797057)) +  
	   MULTIPLY(tmp14 + tmp15, FIX(0.338443458));   
    tmp0 = tmp1 + tmp2 + tmp3 -
	   MULTIPLY(tmp10, FIX(2.020082300)) +          
	   MULTIPLY(tmp14, FIX(0.318774355));           
    tmp4 = MULTIPLY(tmp14 - tmp15, FIX(0.937797057)) -  
	   MULTIPLY(tmp11 + tmp12, FIX(0.338443458));   
    tmp5 = MULTIPLY(tmp11 + tmp13, - FIX(1.163874945)); 
    tmp1 += tmp4 + tmp5 +
	    MULTIPLY(tmp11, FIX(0.837223564)) -         
	    MULTIPLY(tmp14, FIX(2.341699410));          
    tmp6 = MULTIPLY(tmp12 + tmp13, - FIX(0.657217813)); 
    tmp2 += tmp4 + tmp6 -
	    MULTIPLY(tmp12, FIX(1.572116027)) +         
	    MULTIPLY(tmp15, FIX(2.260109708));          
    tmp3 += tmp5 + tmp6 +
	    MULTIPLY(tmp13, FIX(2.205608352)) -         
	    MULTIPLY(tmp15, FIX(1.742345811));          

    dataptr[1] = (DCTELEM) DESCALE(tmp0, CONST_BITS);
    dataptr[3] = (DCTELEM) DESCALE(tmp1, CONST_BITS);
    dataptr[5] = (DCTELEM) DESCALE(tmp2, CONST_BITS);
    dataptr[7] = (DCTELEM) DESCALE(tmp3, CONST_BITS);

    ctr++;

    if (ctr != DCTSIZE) {
      if (ctr == 13)
	break;			
      dataptr += DCTSIZE;	
    } else
      dataptr = workspace;	
  }

  

  dataptr = data;
  wsptr = workspace;
  for (ctr = DCTSIZE-1; ctr >= 0; ctr--) {
    

    tmp0 = dataptr[DCTSIZE*0] + wsptr[DCTSIZE*4];
    tmp1 = dataptr[DCTSIZE*1] + wsptr[DCTSIZE*3];
    tmp2 = dataptr[DCTSIZE*2] + wsptr[DCTSIZE*2];
    tmp3 = dataptr[DCTSIZE*3] + wsptr[DCTSIZE*1];
    tmp4 = dataptr[DCTSIZE*4] + wsptr[DCTSIZE*0];
    tmp5 = dataptr[DCTSIZE*5] + dataptr[DCTSIZE*7];
    tmp6 = dataptr[DCTSIZE*6];

    tmp10 = dataptr[DCTSIZE*0] - wsptr[DCTSIZE*4];
    tmp11 = dataptr[DCTSIZE*1] - wsptr[DCTSIZE*3];
    tmp12 = dataptr[DCTSIZE*2] - wsptr[DCTSIZE*2];
    tmp13 = dataptr[DCTSIZE*3] - wsptr[DCTSIZE*1];
    tmp14 = dataptr[DCTSIZE*4] - wsptr[DCTSIZE*0];
    tmp15 = dataptr[DCTSIZE*5] - dataptr[DCTSIZE*7];

    dataptr[DCTSIZE*0] = (DCTELEM)
      DESCALE(MULTIPLY(tmp0 + tmp1 + tmp2 + tmp3 + tmp4 + tmp5 + tmp6,
		       FIX(0.757396450)),          
	      CONST_BITS+1);
    tmp6 += tmp6;
    tmp0 -= tmp6;
    tmp1 -= tmp6;
    tmp2 -= tmp6;
    tmp3 -= tmp6;
    tmp4 -= tmp6;
    tmp5 -= tmp6;
    dataptr[DCTSIZE*2] = (DCTELEM)
      DESCALE(MULTIPLY(tmp0, FIX(1.039995521)) +   
	      MULTIPLY(tmp1, FIX(0.801745081)) +   
	      MULTIPLY(tmp2, FIX(0.379824504)) -   
	      MULTIPLY(tmp3, FIX(0.129109289)) -   
	      MULTIPLY(tmp4, FIX(0.608465700)) -   
	      MULTIPLY(tmp5, FIX(0.948429952)),    
	      CONST_BITS+1);
    z1 = MULTIPLY(tmp0 - tmp2, FIX(0.875087516)) - 
	 MULTIPLY(tmp3 - tmp4, FIX(0.330085509)) - 
	 MULTIPLY(tmp1 - tmp5, FIX(0.239678205));  
    z2 = MULTIPLY(tmp0 + tmp2, FIX(0.073342435)) - 
	 MULTIPLY(tmp3 + tmp4, FIX(0.709910013)) + 
	 MULTIPLY(tmp1 + tmp5, FIX(0.368787494));  

    dataptr[DCTSIZE*4] = (DCTELEM) DESCALE(z1 + z2, CONST_BITS+1);
    dataptr[DCTSIZE*6] = (DCTELEM) DESCALE(z1 - z2, CONST_BITS+1);

    

    tmp1 = MULTIPLY(tmp10 + tmp11, FIX(1.001514908));   
    tmp2 = MULTIPLY(tmp10 + tmp12, FIX(0.881514751));   
    tmp3 = MULTIPLY(tmp10 + tmp13, FIX(0.710284161)) +  
	   MULTIPLY(tmp14 + tmp15, FIX(0.256335874));   
    tmp0 = tmp1 + tmp2 + tmp3 -
	   MULTIPLY(tmp10, FIX(1.530003162)) +          
	   MULTIPLY(tmp14, FIX(0.241438564));           
    tmp4 = MULTIPLY(tmp14 - tmp15, FIX(0.710284161)) -  
	   MULTIPLY(tmp11 + tmp12, FIX(0.256335874));   
    tmp5 = MULTIPLY(tmp11 + tmp13, - FIX(0.881514751)); 
    tmp1 += tmp4 + tmp5 +
	    MULTIPLY(tmp11, FIX(0.634110155)) -         
	    MULTIPLY(tmp14, FIX(1.773594819));          
    tmp6 = MULTIPLY(tmp12 + tmp13, - FIX(0.497774438)); 
    tmp2 += tmp4 + tmp6 -
	    MULTIPLY(tmp12, FIX(1.190715098)) +         
	    MULTIPLY(tmp15, FIX(1.711799069));          
    tmp3 += tmp5 + tmp6 +
	    MULTIPLY(tmp13, FIX(1.670519935)) -         
	    MULTIPLY(tmp15, FIX(1.319646532));          

    dataptr[DCTSIZE*1] = (DCTELEM) DESCALE(tmp0, CONST_BITS+1);
    dataptr[DCTSIZE*3] = (DCTELEM) DESCALE(tmp1, CONST_BITS+1);
    dataptr[DCTSIZE*5] = (DCTELEM) DESCALE(tmp2, CONST_BITS+1);
    dataptr[DCTSIZE*7] = (DCTELEM) DESCALE(tmp3, CONST_BITS+1);

    dataptr++;			
    wsptr++;			
  }
}

GLOBAL(void)
jpeg_fdct_14x14 (DCTELEM * data, JSAMPARRAY sample_data, JDIMENSION start_col)
{
  INT32 tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6;
  INT32 tmp10, tmp11, tmp12, tmp13, tmp14, tmp15, tmp16;
  DCTELEM workspace[8*6];
  DCTELEM *dataptr;
  DCTELEM *wsptr;
  JSAMPROW elemptr;
  int ctr;
  SHIFT_TEMPS

  
  
  

  dataptr = data;
  ctr = 0;
  for (;;) {
    elemptr = sample_data[ctr] + start_col;

    

    tmp0 = GETJSAMPLE(elemptr[0]) + GETJSAMPLE(elemptr[13]);
    tmp1 = GETJSAMPLE(elemptr[1]) + GETJSAMPLE(elemptr[12]);
    tmp2 = GETJSAMPLE(elemptr[2]) + GETJSAMPLE(elemptr[11]);
    tmp13 = GETJSAMPLE(elemptr[3]) + GETJSAMPLE(elemptr[10]);
    tmp4 = GETJSAMPLE(elemptr[4]) + GETJSAMPLE(elemptr[9]);
    tmp5 = GETJSAMPLE(elemptr[5]) + GETJSAMPLE(elemptr[8]);
    tmp6 = GETJSAMPLE(elemptr[6]) + GETJSAMPLE(elemptr[7]);

    tmp10 = tmp0 + tmp6;
    tmp14 = tmp0 - tmp6;
    tmp11 = tmp1 + tmp5;
    tmp15 = tmp1 - tmp5;
    tmp12 = tmp2 + tmp4;
    tmp16 = tmp2 - tmp4;

    tmp0 = GETJSAMPLE(elemptr[0]) - GETJSAMPLE(elemptr[13]);
    tmp1 = GETJSAMPLE(elemptr[1]) - GETJSAMPLE(elemptr[12]);
    tmp2 = GETJSAMPLE(elemptr[2]) - GETJSAMPLE(elemptr[11]);
    tmp3 = GETJSAMPLE(elemptr[3]) - GETJSAMPLE(elemptr[10]);
    tmp4 = GETJSAMPLE(elemptr[4]) - GETJSAMPLE(elemptr[9]);
    tmp5 = GETJSAMPLE(elemptr[5]) - GETJSAMPLE(elemptr[8]);
    tmp6 = GETJSAMPLE(elemptr[6]) - GETJSAMPLE(elemptr[7]);

    
    dataptr[0] = (DCTELEM)
      (tmp10 + tmp11 + tmp12 + tmp13 - 14 * CENTERJSAMPLE);
    tmp13 += tmp13;
    dataptr[4] = (DCTELEM)
      DESCALE(MULTIPLY(tmp10 - tmp13, FIX(1.274162392)) + 
	      MULTIPLY(tmp11 - tmp13, FIX(0.314692123)) - 
	      MULTIPLY(tmp12 - tmp13, FIX(0.881747734)),  
	      CONST_BITS);

    tmp10 = MULTIPLY(tmp14 + tmp15, FIX(1.105676686));    

    dataptr[2] = (DCTELEM)
      DESCALE(tmp10 + MULTIPLY(tmp14, FIX(0.273079590))   
	      + MULTIPLY(tmp16, FIX(0.613604268)),        
	      CONST_BITS);
    dataptr[6] = (DCTELEM)
      DESCALE(tmp10 - MULTIPLY(tmp15, FIX(1.719280954))   
	      - MULTIPLY(tmp16, FIX(1.378756276)),        
	      CONST_BITS);

    

    tmp10 = tmp1 + tmp2;
    tmp11 = tmp5 - tmp4;
    dataptr[7] = (DCTELEM) (tmp0 - tmp10 + tmp3 - tmp11 - tmp6);
    tmp3 <<= CONST_BITS;
    tmp10 = MULTIPLY(tmp10, - FIX(0.158341681));          
    tmp11 = MULTIPLY(tmp11, FIX(1.405321284));            
    tmp10 += tmp11 - tmp3;
    tmp11 = MULTIPLY(tmp0 + tmp2, FIX(1.197448846)) +     
	    MULTIPLY(tmp4 + tmp6, FIX(0.752406978));      
    dataptr[5] = (DCTELEM)
      DESCALE(tmp10 + tmp11 - MULTIPLY(tmp2, FIX(2.373959773)) 
	      + MULTIPLY(tmp4, FIX(1.119999435)),         
	      CONST_BITS);
    tmp12 = MULTIPLY(tmp0 + tmp1, FIX(1.334852607)) +     
	    MULTIPLY(tmp5 - tmp6, FIX(0.467085129));      
    dataptr[3] = (DCTELEM)
      DESCALE(tmp10 + tmp12 - MULTIPLY(tmp1, FIX(0.424103948)) 
	      - MULTIPLY(tmp5, FIX(3.069855259)),         
	      CONST_BITS);
    dataptr[1] = (DCTELEM)
      DESCALE(tmp11 + tmp12 + tmp3 + tmp6 -
	      MULTIPLY(tmp0 + tmp6, FIX(1.126980169)),    
	      CONST_BITS);

    ctr++;

    if (ctr != DCTSIZE) {
      if (ctr == 14)
	break;			
      dataptr += DCTSIZE;	
    } else
      dataptr = workspace;	
  }

  

  dataptr = data;
  wsptr = workspace;
  for (ctr = DCTSIZE-1; ctr >= 0; ctr--) {
    

    tmp0 = dataptr[DCTSIZE*0] + wsptr[DCTSIZE*5];
    tmp1 = dataptr[DCTSIZE*1] + wsptr[DCTSIZE*4];
    tmp2 = dataptr[DCTSIZE*2] + wsptr[DCTSIZE*3];
    tmp13 = dataptr[DCTSIZE*3] + wsptr[DCTSIZE*2];
    tmp4 = dataptr[DCTSIZE*4] + wsptr[DCTSIZE*1];
    tmp5 = dataptr[DCTSIZE*5] + wsptr[DCTSIZE*0];
    tmp6 = dataptr[DCTSIZE*6] + dataptr[DCTSIZE*7];

    tmp10 = tmp0 + tmp6;
    tmp14 = tmp0 - tmp6;
    tmp11 = tmp1 + tmp5;
    tmp15 = tmp1 - tmp5;
    tmp12 = tmp2 + tmp4;
    tmp16 = tmp2 - tmp4;

    tmp0 = dataptr[DCTSIZE*0] - wsptr[DCTSIZE*5];
    tmp1 = dataptr[DCTSIZE*1] - wsptr[DCTSIZE*4];
    tmp2 = dataptr[DCTSIZE*2] - wsptr[DCTSIZE*3];
    tmp3 = dataptr[DCTSIZE*3] - wsptr[DCTSIZE*2];
    tmp4 = dataptr[DCTSIZE*4] - wsptr[DCTSIZE*1];
    tmp5 = dataptr[DCTSIZE*5] - wsptr[DCTSIZE*0];
    tmp6 = dataptr[DCTSIZE*6] - dataptr[DCTSIZE*7];

    dataptr[DCTSIZE*0] = (DCTELEM)
      DESCALE(MULTIPLY(tmp10 + tmp11 + tmp12 + tmp13,
		       FIX(0.653061224)),                 
	      CONST_BITS+1);
    tmp13 += tmp13;
    dataptr[DCTSIZE*4] = (DCTELEM)
      DESCALE(MULTIPLY(tmp10 - tmp13, FIX(0.832106052)) + 
	      MULTIPLY(tmp11 - tmp13, FIX(0.205513223)) - 
	      MULTIPLY(tmp12 - tmp13, FIX(0.575835255)),  
	      CONST_BITS+1);

    tmp10 = MULTIPLY(tmp14 + tmp15, FIX(0.722074570));    

    dataptr[DCTSIZE*2] = (DCTELEM)
      DESCALE(tmp10 + MULTIPLY(tmp14, FIX(0.178337691))   
	      + MULTIPLY(tmp16, FIX(0.400721155)),        
	      CONST_BITS+1);
    dataptr[DCTSIZE*6] = (DCTELEM)
      DESCALE(tmp10 - MULTIPLY(tmp15, FIX(1.122795725))   
	      - MULTIPLY(tmp16, FIX(0.900412262)),        
	      CONST_BITS+1);

    

    tmp10 = tmp1 + tmp2;
    tmp11 = tmp5 - tmp4;
    dataptr[DCTSIZE*7] = (DCTELEM)
      DESCALE(MULTIPLY(tmp0 - tmp10 + tmp3 - tmp11 - tmp6,
		       FIX(0.653061224)),                 
	      CONST_BITS+1);
    tmp3  = MULTIPLY(tmp3 , FIX(0.653061224));            
    tmp10 = MULTIPLY(tmp10, - FIX(0.103406812));          
    tmp11 = MULTIPLY(tmp11, FIX(0.917760839));            
    tmp10 += tmp11 - tmp3;
    tmp11 = MULTIPLY(tmp0 + tmp2, FIX(0.782007410)) +     
	    MULTIPLY(tmp4 + tmp6, FIX(0.491367823));      
    dataptr[DCTSIZE*5] = (DCTELEM)
      DESCALE(tmp10 + tmp11 - MULTIPLY(tmp2, FIX(1.550341076)) 
	      + MULTIPLY(tmp4, FIX(0.731428202)),         
	      CONST_BITS+1);
    tmp12 = MULTIPLY(tmp0 + tmp1, FIX(0.871740478)) +     
	    MULTIPLY(tmp5 - tmp6, FIX(0.305035186));      
    dataptr[DCTSIZE*3] = (DCTELEM)
      DESCALE(tmp10 + tmp12 - MULTIPLY(tmp1, FIX(0.276965844)) 
	      - MULTIPLY(tmp5, FIX(2.004803435)),         
	      CONST_BITS+1);
    dataptr[DCTSIZE*1] = (DCTELEM)
      DESCALE(tmp11 + tmp12 + tmp3
	      - MULTIPLY(tmp0, FIX(0.735987049))          
	      - MULTIPLY(tmp6, FIX(0.082925825)),         
	      CONST_BITS+1);

    dataptr++;			
    wsptr++;			
  }
}

GLOBAL(void)
jpeg_fdct_15x15 (DCTELEM * data, JSAMPARRAY sample_data, JDIMENSION start_col)
{
  INT32 tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7;
  INT32 tmp10, tmp11, tmp12, tmp13, tmp14, tmp15, tmp16;
  INT32 z1, z2, z3;
  DCTELEM workspace[8*7];
  DCTELEM *dataptr;
  DCTELEM *wsptr;
  JSAMPROW elemptr;
  int ctr;
  SHIFT_TEMPS

  
  
  

  dataptr = data;
  ctr = 0;
  for (;;) {
    elemptr = sample_data[ctr] + start_col;

    

    tmp0 = GETJSAMPLE(elemptr[0]) + GETJSAMPLE(elemptr[14]);
    tmp1 = GETJSAMPLE(elemptr[1]) + GETJSAMPLE(elemptr[13]);
    tmp2 = GETJSAMPLE(elemptr[2]) + GETJSAMPLE(elemptr[12]);
    tmp3 = GETJSAMPLE(elemptr[3]) + GETJSAMPLE(elemptr[11]);
    tmp4 = GETJSAMPLE(elemptr[4]) + GETJSAMPLE(elemptr[10]);
    tmp5 = GETJSAMPLE(elemptr[5]) + GETJSAMPLE(elemptr[9]);
    tmp6 = GETJSAMPLE(elemptr[6]) + GETJSAMPLE(elemptr[8]);
    tmp7 = GETJSAMPLE(elemptr[7]);

    tmp10 = GETJSAMPLE(elemptr[0]) - GETJSAMPLE(elemptr[14]);
    tmp11 = GETJSAMPLE(elemptr[1]) - GETJSAMPLE(elemptr[13]);
    tmp12 = GETJSAMPLE(elemptr[2]) - GETJSAMPLE(elemptr[12]);
    tmp13 = GETJSAMPLE(elemptr[3]) - GETJSAMPLE(elemptr[11]);
    tmp14 = GETJSAMPLE(elemptr[4]) - GETJSAMPLE(elemptr[10]);
    tmp15 = GETJSAMPLE(elemptr[5]) - GETJSAMPLE(elemptr[9]);
    tmp16 = GETJSAMPLE(elemptr[6]) - GETJSAMPLE(elemptr[8]);

    z1 = tmp0 + tmp4 + tmp5;
    z2 = tmp1 + tmp3 + tmp6;
    z3 = tmp2 + tmp7;
    
    dataptr[0] = (DCTELEM) (z1 + z2 + z3 - 15 * CENTERJSAMPLE);
    z3 += z3;
    dataptr[6] = (DCTELEM)
      DESCALE(MULTIPLY(z1 - z3, FIX(1.144122806)) - 
	      MULTIPLY(z2 - z3, FIX(0.437016024)),  
	      CONST_BITS);
    tmp2 += ((tmp1 + tmp4) >> 1) - tmp7 - tmp7;
    z1 = MULTIPLY(tmp3 - tmp2, FIX(1.531135173)) -  
         MULTIPLY(tmp6 - tmp2, FIX(2.238241955));   
    z2 = MULTIPLY(tmp5 - tmp2, FIX(0.798468008)) -  
	 MULTIPLY(tmp0 - tmp2, FIX(0.091361227));   
    z3 = MULTIPLY(tmp0 - tmp3, FIX(1.383309603)) +  
	 MULTIPLY(tmp6 - tmp5, FIX(0.946293579)) +  
	 MULTIPLY(tmp1 - tmp4, FIX(0.790569415));   

    dataptr[2] = (DCTELEM) DESCALE(z1 + z3, CONST_BITS);
    dataptr[4] = (DCTELEM) DESCALE(z2 + z3, CONST_BITS);

    

    tmp2 = MULTIPLY(tmp10 - tmp12 - tmp13 + tmp15 + tmp16,
		    FIX(1.224744871));                         
    tmp1 = MULTIPLY(tmp10 - tmp14 - tmp15, FIX(1.344997024)) + 
	   MULTIPLY(tmp11 - tmp13 - tmp16, FIX(0.831253876));  
    tmp12 = MULTIPLY(tmp12, FIX(1.224744871));                 
    tmp4 = MULTIPLY(tmp10 - tmp16, FIX(1.406466353)) +         
	   MULTIPLY(tmp11 + tmp14, FIX(1.344997024)) +         
	   MULTIPLY(tmp13 + tmp15, FIX(0.575212477));          
    tmp0 = MULTIPLY(tmp13, FIX(0.475753014)) -                 
	   MULTIPLY(tmp14, FIX(0.513743148)) +                 
	   MULTIPLY(tmp16, FIX(1.700497885)) + tmp4 + tmp12;   
    tmp3 = MULTIPLY(tmp10, - FIX(0.355500862)) -               
	   MULTIPLY(tmp11, FIX(2.176250899)) -                 
	   MULTIPLY(tmp15, FIX(0.869244010)) + tmp4 - tmp12;   

    dataptr[1] = (DCTELEM) DESCALE(tmp0, CONST_BITS);
    dataptr[3] = (DCTELEM) DESCALE(tmp1, CONST_BITS);
    dataptr[5] = (DCTELEM) DESCALE(tmp2, CONST_BITS);
    dataptr[7] = (DCTELEM) DESCALE(tmp3, CONST_BITS);

    ctr++;

    if (ctr != DCTSIZE) {
      if (ctr == 15)
	break;			
      dataptr += DCTSIZE;	
    } else
      dataptr = workspace;	
  }

  

  dataptr = data;
  wsptr = workspace;
  for (ctr = DCTSIZE-1; ctr >= 0; ctr--) {
    

    tmp0 = dataptr[DCTSIZE*0] + wsptr[DCTSIZE*6];
    tmp1 = dataptr[DCTSIZE*1] + wsptr[DCTSIZE*5];
    tmp2 = dataptr[DCTSIZE*2] + wsptr[DCTSIZE*4];
    tmp3 = dataptr[DCTSIZE*3] + wsptr[DCTSIZE*3];
    tmp4 = dataptr[DCTSIZE*4] + wsptr[DCTSIZE*2];
    tmp5 = dataptr[DCTSIZE*5] + wsptr[DCTSIZE*1];
    tmp6 = dataptr[DCTSIZE*6] + wsptr[DCTSIZE*0];
    tmp7 = dataptr[DCTSIZE*7];

    tmp10 = dataptr[DCTSIZE*0] - wsptr[DCTSIZE*6];
    tmp11 = dataptr[DCTSIZE*1] - wsptr[DCTSIZE*5];
    tmp12 = dataptr[DCTSIZE*2] - wsptr[DCTSIZE*4];
    tmp13 = dataptr[DCTSIZE*3] - wsptr[DCTSIZE*3];
    tmp14 = dataptr[DCTSIZE*4] - wsptr[DCTSIZE*2];
    tmp15 = dataptr[DCTSIZE*5] - wsptr[DCTSIZE*1];
    tmp16 = dataptr[DCTSIZE*6] - wsptr[DCTSIZE*0];

    z1 = tmp0 + tmp4 + tmp5;
    z2 = tmp1 + tmp3 + tmp6;
    z3 = tmp2 + tmp7;
    dataptr[DCTSIZE*0] = (DCTELEM)
      DESCALE(MULTIPLY(z1 + z2 + z3, FIX(1.137777778)), 
	      CONST_BITS+2);
    z3 += z3;
    dataptr[DCTSIZE*6] = (DCTELEM)
      DESCALE(MULTIPLY(z1 - z3, FIX(1.301757503)) - 
	      MULTIPLY(z2 - z3, FIX(0.497227121)),  
	      CONST_BITS+2);
    tmp2 += ((tmp1 + tmp4) >> 1) - tmp7 - tmp7;
    z1 = MULTIPLY(tmp3 - tmp2, FIX(1.742091575)) -  
         MULTIPLY(tmp6 - tmp2, FIX(2.546621957));   
    z2 = MULTIPLY(tmp5 - tmp2, FIX(0.908479156)) -  
	 MULTIPLY(tmp0 - tmp2, FIX(0.103948774));   
    z3 = MULTIPLY(tmp0 - tmp3, FIX(1.573898926)) +  
	 MULTIPLY(tmp6 - tmp5, FIX(1.076671805)) +  
	 MULTIPLY(tmp1 - tmp4, FIX(0.899492312));   

    dataptr[DCTSIZE*2] = (DCTELEM) DESCALE(z1 + z3, CONST_BITS+2);
    dataptr[DCTSIZE*4] = (DCTELEM) DESCALE(z2 + z3, CONST_BITS+2);

    

    tmp2 = MULTIPLY(tmp10 - tmp12 - tmp13 + tmp15 + tmp16,
		    FIX(1.393487498));                         
    tmp1 = MULTIPLY(tmp10 - tmp14 - tmp15, FIX(1.530307725)) + 
	   MULTIPLY(tmp11 - tmp13 - tmp16, FIX(0.945782187));  
    tmp12 = MULTIPLY(tmp12, FIX(1.393487498));                 
    tmp4 = MULTIPLY(tmp10 - tmp16, FIX(1.600246161)) +         
	   MULTIPLY(tmp11 + tmp14, FIX(1.530307725)) +         
	   MULTIPLY(tmp13 + tmp15, FIX(0.654463974));          
    tmp0 = MULTIPLY(tmp13, FIX(0.541301207)) -                 
	   MULTIPLY(tmp14, FIX(0.584525538)) +                 
	   MULTIPLY(tmp16, FIX(1.934788705)) + tmp4 + tmp12;   
    tmp3 = MULTIPLY(tmp10, - FIX(0.404480980)) -               
	   MULTIPLY(tmp11, FIX(2.476089912)) -                 
	   MULTIPLY(tmp15, FIX(0.989006518)) + tmp4 - tmp12;   

    dataptr[DCTSIZE*1] = (DCTELEM) DESCALE(tmp0, CONST_BITS+2);
    dataptr[DCTSIZE*3] = (DCTELEM) DESCALE(tmp1, CONST_BITS+2);
    dataptr[DCTSIZE*5] = (DCTELEM) DESCALE(tmp2, CONST_BITS+2);
    dataptr[DCTSIZE*7] = (DCTELEM) DESCALE(tmp3, CONST_BITS+2);

    dataptr++;			
    wsptr++;			
  }
}

GLOBAL(void)
jpeg_fdct_16x16 (DCTELEM * data, JSAMPARRAY sample_data, JDIMENSION start_col)
{
  INT32 tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7;
  INT32 tmp10, tmp11, tmp12, tmp13, tmp14, tmp15, tmp16, tmp17;
  DCTELEM workspace[DCTSIZE2];
  DCTELEM *dataptr;
  DCTELEM *wsptr;
  JSAMPROW elemptr;
  int ctr;
  SHIFT_TEMPS

  
  
  
  

  dataptr = data;
  ctr = 0;
  for (;;) {
    elemptr = sample_data[ctr] + start_col;

    

    tmp0 = GETJSAMPLE(elemptr[0]) + GETJSAMPLE(elemptr[15]);
    tmp1 = GETJSAMPLE(elemptr[1]) + GETJSAMPLE(elemptr[14]);
    tmp2 = GETJSAMPLE(elemptr[2]) + GETJSAMPLE(elemptr[13]);
    tmp3 = GETJSAMPLE(elemptr[3]) + GETJSAMPLE(elemptr[12]);
    tmp4 = GETJSAMPLE(elemptr[4]) + GETJSAMPLE(elemptr[11]);
    tmp5 = GETJSAMPLE(elemptr[5]) + GETJSAMPLE(elemptr[10]);
    tmp6 = GETJSAMPLE(elemptr[6]) + GETJSAMPLE(elemptr[9]);
    tmp7 = GETJSAMPLE(elemptr[7]) + GETJSAMPLE(elemptr[8]);

    tmp10 = tmp0 + tmp7;
    tmp14 = tmp0 - tmp7;
    tmp11 = tmp1 + tmp6;
    tmp15 = tmp1 - tmp6;
    tmp12 = tmp2 + tmp5;
    tmp16 = tmp2 - tmp5;
    tmp13 = tmp3 + tmp4;
    tmp17 = tmp3 - tmp4;

    tmp0 = GETJSAMPLE(elemptr[0]) - GETJSAMPLE(elemptr[15]);
    tmp1 = GETJSAMPLE(elemptr[1]) - GETJSAMPLE(elemptr[14]);
    tmp2 = GETJSAMPLE(elemptr[2]) - GETJSAMPLE(elemptr[13]);
    tmp3 = GETJSAMPLE(elemptr[3]) - GETJSAMPLE(elemptr[12]);
    tmp4 = GETJSAMPLE(elemptr[4]) - GETJSAMPLE(elemptr[11]);
    tmp5 = GETJSAMPLE(elemptr[5]) - GETJSAMPLE(elemptr[10]);
    tmp6 = GETJSAMPLE(elemptr[6]) - GETJSAMPLE(elemptr[9]);
    tmp7 = GETJSAMPLE(elemptr[7]) - GETJSAMPLE(elemptr[8]);

    
    dataptr[0] = (DCTELEM)
      ((tmp10 + tmp11 + tmp12 + tmp13 - 16 * CENTERJSAMPLE) << PASS1_BITS);
    dataptr[4] = (DCTELEM)
      DESCALE(MULTIPLY(tmp10 - tmp13, FIX(1.306562965)) + 
	      MULTIPLY(tmp11 - tmp12, FIX_0_541196100),   
	      CONST_BITS-PASS1_BITS);

    tmp10 = MULTIPLY(tmp17 - tmp15, FIX(0.275899379)) +   
	    MULTIPLY(tmp14 - tmp16, FIX(1.387039845));    

    dataptr[2] = (DCTELEM)
      DESCALE(tmp10 + MULTIPLY(tmp15, FIX(1.451774982))   
	      + MULTIPLY(tmp16, FIX(2.172734804)),        
	      CONST_BITS-PASS1_BITS);
    dataptr[6] = (DCTELEM)
      DESCALE(tmp10 - MULTIPLY(tmp14, FIX(0.211164243))   
	      - MULTIPLY(tmp17, FIX(1.061594338)),        
	      CONST_BITS-PASS1_BITS);

    

    tmp11 = MULTIPLY(tmp0 + tmp1, FIX(1.353318001)) +         
	    MULTIPLY(tmp6 - tmp7, FIX(0.410524528));          
    tmp12 = MULTIPLY(tmp0 + tmp2, FIX(1.247225013)) +         
	    MULTIPLY(tmp5 + tmp7, FIX(0.666655658));          
    tmp13 = MULTIPLY(tmp0 + tmp3, FIX(1.093201867)) +         
	    MULTIPLY(tmp4 - tmp7, FIX(0.897167586));          
    tmp14 = MULTIPLY(tmp1 + tmp2, FIX(0.138617169)) +         
	    MULTIPLY(tmp6 - tmp5, FIX(1.407403738));          
    tmp15 = MULTIPLY(tmp1 + tmp3, - FIX(0.666655658)) +       
	    MULTIPLY(tmp4 + tmp6, - FIX(1.247225013));        
    tmp16 = MULTIPLY(tmp2 + tmp3, - FIX(1.353318001)) +       
	    MULTIPLY(tmp5 - tmp4, FIX(0.410524528));          
    tmp10 = tmp11 + tmp12 + tmp13 -
	    MULTIPLY(tmp0, FIX(2.286341144)) +                
	    MULTIPLY(tmp7, FIX(0.779653625));                 
    tmp11 += tmp14 + tmp15 + MULTIPLY(tmp1, FIX(0.071888074)) 
	     - MULTIPLY(tmp6, FIX(1.663905119));              
    tmp12 += tmp14 + tmp16 - MULTIPLY(tmp2, FIX(1.125726048)) 
	     + MULTIPLY(tmp5, FIX(1.227391138));              
    tmp13 += tmp15 + tmp16 + MULTIPLY(tmp3, FIX(1.065388962)) 
	     + MULTIPLY(tmp4, FIX(2.167985692));              

    dataptr[1] = (DCTELEM) DESCALE(tmp10, CONST_BITS-PASS1_BITS);
    dataptr[3] = (DCTELEM) DESCALE(tmp11, CONST_BITS-PASS1_BITS);
    dataptr[5] = (DCTELEM) DESCALE(tmp12, CONST_BITS-PASS1_BITS);
    dataptr[7] = (DCTELEM) DESCALE(tmp13, CONST_BITS-PASS1_BITS);

    ctr++;

    if (ctr != DCTSIZE) {
      if (ctr == DCTSIZE * 2)
	break;			
      dataptr += DCTSIZE;	
    } else
      dataptr = workspace;	
  }

  

  dataptr = data;
  wsptr = workspace;
  for (ctr = DCTSIZE-1; ctr >= 0; ctr--) {
    

    tmp0 = dataptr[DCTSIZE*0] + wsptr[DCTSIZE*7];
    tmp1 = dataptr[DCTSIZE*1] + wsptr[DCTSIZE*6];
    tmp2 = dataptr[DCTSIZE*2] + wsptr[DCTSIZE*5];
    tmp3 = dataptr[DCTSIZE*3] + wsptr[DCTSIZE*4];
    tmp4 = dataptr[DCTSIZE*4] + wsptr[DCTSIZE*3];
    tmp5 = dataptr[DCTSIZE*5] + wsptr[DCTSIZE*2];
    tmp6 = dataptr[DCTSIZE*6] + wsptr[DCTSIZE*1];
    tmp7 = dataptr[DCTSIZE*7] + wsptr[DCTSIZE*0];

    tmp10 = tmp0 + tmp7;
    tmp14 = tmp0 - tmp7;
    tmp11 = tmp1 + tmp6;
    tmp15 = tmp1 - tmp6;
    tmp12 = tmp2 + tmp5;
    tmp16 = tmp2 - tmp5;
    tmp13 = tmp3 + tmp4;
    tmp17 = tmp3 - tmp4;

    tmp0 = dataptr[DCTSIZE*0] - wsptr[DCTSIZE*7];
    tmp1 = dataptr[DCTSIZE*1] - wsptr[DCTSIZE*6];
    tmp2 = dataptr[DCTSIZE*2] - wsptr[DCTSIZE*5];
    tmp3 = dataptr[DCTSIZE*3] - wsptr[DCTSIZE*4];
    tmp4 = dataptr[DCTSIZE*4] - wsptr[DCTSIZE*3];
    tmp5 = dataptr[DCTSIZE*5] - wsptr[DCTSIZE*2];
    tmp6 = dataptr[DCTSIZE*6] - wsptr[DCTSIZE*1];
    tmp7 = dataptr[DCTSIZE*7] - wsptr[DCTSIZE*0];

    dataptr[DCTSIZE*0] = (DCTELEM)
      DESCALE(tmp10 + tmp11 + tmp12 + tmp13, PASS1_BITS+2);
    dataptr[DCTSIZE*4] = (DCTELEM)
      DESCALE(MULTIPLY(tmp10 - tmp13, FIX(1.306562965)) + 
	      MULTIPLY(tmp11 - tmp12, FIX_0_541196100),   
	      CONST_BITS+PASS1_BITS+2);

    tmp10 = MULTIPLY(tmp17 - tmp15, FIX(0.275899379)) +   
	    MULTIPLY(tmp14 - tmp16, FIX(1.387039845));    

    dataptr[DCTSIZE*2] = (DCTELEM)
      DESCALE(tmp10 + MULTIPLY(tmp15, FIX(1.451774982))   
	      + MULTIPLY(tmp16, FIX(2.172734804)),        
	      CONST_BITS+PASS1_BITS+2);
    dataptr[DCTSIZE*6] = (DCTELEM)
      DESCALE(tmp10 - MULTIPLY(tmp14, FIX(0.211164243))   
	      - MULTIPLY(tmp17, FIX(1.061594338)),        
	      CONST_BITS+PASS1_BITS+2);

    

    tmp11 = MULTIPLY(tmp0 + tmp1, FIX(1.353318001)) +         
	    MULTIPLY(tmp6 - tmp7, FIX(0.410524528));          
    tmp12 = MULTIPLY(tmp0 + tmp2, FIX(1.247225013)) +         
	    MULTIPLY(tmp5 + tmp7, FIX(0.666655658));          
    tmp13 = MULTIPLY(tmp0 + tmp3, FIX(1.093201867)) +         
	    MULTIPLY(tmp4 - tmp7, FIX(0.897167586));          
    tmp14 = MULTIPLY(tmp1 + tmp2, FIX(0.138617169)) +         
	    MULTIPLY(tmp6 - tmp5, FIX(1.407403738));          
    tmp15 = MULTIPLY(tmp1 + tmp3, - FIX(0.666655658)) +       
	    MULTIPLY(tmp4 + tmp6, - FIX(1.247225013));        
    tmp16 = MULTIPLY(tmp2 + tmp3, - FIX(1.353318001)) +       
	    MULTIPLY(tmp5 - tmp4, FIX(0.410524528));          
    tmp10 = tmp11 + tmp12 + tmp13 -
	    MULTIPLY(tmp0, FIX(2.286341144)) +                
	    MULTIPLY(tmp7, FIX(0.779653625));                 
    tmp11 += tmp14 + tmp15 + MULTIPLY(tmp1, FIX(0.071888074)) 
	     - MULTIPLY(tmp6, FIX(1.663905119));              
    tmp12 += tmp14 + tmp16 - MULTIPLY(tmp2, FIX(1.125726048)) 
	     + MULTIPLY(tmp5, FIX(1.227391138));              
    tmp13 += tmp15 + tmp16 + MULTIPLY(tmp3, FIX(1.065388962)) 
	     + MULTIPLY(tmp4, FIX(2.167985692));              

    dataptr[DCTSIZE*1] = (DCTELEM) DESCALE(tmp10, CONST_BITS+PASS1_BITS+2);
    dataptr[DCTSIZE*3] = (DCTELEM) DESCALE(tmp11, CONST_BITS+PASS1_BITS+2);
    dataptr[DCTSIZE*5] = (DCTELEM) DESCALE(tmp12, CONST_BITS+PASS1_BITS+2);
    dataptr[DCTSIZE*7] = (DCTELEM) DESCALE(tmp13, CONST_BITS+PASS1_BITS+2);

    dataptr++;			
    wsptr++;			
  }
}

GLOBAL(void)
jpeg_fdct_16x8 (DCTELEM * data, JSAMPARRAY sample_data, JDIMENSION start_col)
{
  INT32 tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7;
  INT32 tmp10, tmp11, tmp12, tmp13, tmp14, tmp15, tmp16, tmp17;
  INT32 z1;
  DCTELEM *dataptr;
  JSAMPROW elemptr;
  int ctr;
  SHIFT_TEMPS

  
  
  
  

  dataptr = data;
  ctr = 0;
  for (ctr = 0; ctr < DCTSIZE; ctr++) {
    elemptr = sample_data[ctr] + start_col;

    

    tmp0 = GETJSAMPLE(elemptr[0]) + GETJSAMPLE(elemptr[15]);
    tmp1 = GETJSAMPLE(elemptr[1]) + GETJSAMPLE(elemptr[14]);
    tmp2 = GETJSAMPLE(elemptr[2]) + GETJSAMPLE(elemptr[13]);
    tmp3 = GETJSAMPLE(elemptr[3]) + GETJSAMPLE(elemptr[12]);
    tmp4 = GETJSAMPLE(elemptr[4]) + GETJSAMPLE(elemptr[11]);
    tmp5 = GETJSAMPLE(elemptr[5]) + GETJSAMPLE(elemptr[10]);
    tmp6 = GETJSAMPLE(elemptr[6]) + GETJSAMPLE(elemptr[9]);
    tmp7 = GETJSAMPLE(elemptr[7]) + GETJSAMPLE(elemptr[8]);

    tmp10 = tmp0 + tmp7;
    tmp14 = tmp0 - tmp7;
    tmp11 = tmp1 + tmp6;
    tmp15 = tmp1 - tmp6;
    tmp12 = tmp2 + tmp5;
    tmp16 = tmp2 - tmp5;
    tmp13 = tmp3 + tmp4;
    tmp17 = tmp3 - tmp4;

    tmp0 = GETJSAMPLE(elemptr[0]) - GETJSAMPLE(elemptr[15]);
    tmp1 = GETJSAMPLE(elemptr[1]) - GETJSAMPLE(elemptr[14]);
    tmp2 = GETJSAMPLE(elemptr[2]) - GETJSAMPLE(elemptr[13]);
    tmp3 = GETJSAMPLE(elemptr[3]) - GETJSAMPLE(elemptr[12]);
    tmp4 = GETJSAMPLE(elemptr[4]) - GETJSAMPLE(elemptr[11]);
    tmp5 = GETJSAMPLE(elemptr[5]) - GETJSAMPLE(elemptr[10]);
    tmp6 = GETJSAMPLE(elemptr[6]) - GETJSAMPLE(elemptr[9]);
    tmp7 = GETJSAMPLE(elemptr[7]) - GETJSAMPLE(elemptr[8]);

    
    dataptr[0] = (DCTELEM)
      ((tmp10 + tmp11 + tmp12 + tmp13 - 16 * CENTERJSAMPLE) << PASS1_BITS);
    dataptr[4] = (DCTELEM)
      DESCALE(MULTIPLY(tmp10 - tmp13, FIX(1.306562965)) + 
	      MULTIPLY(tmp11 - tmp12, FIX_0_541196100),   
	      CONST_BITS-PASS1_BITS);

    tmp10 = MULTIPLY(tmp17 - tmp15, FIX(0.275899379)) +   
	    MULTIPLY(tmp14 - tmp16, FIX(1.387039845));    

    dataptr[2] = (DCTELEM)
      DESCALE(tmp10 + MULTIPLY(tmp15, FIX(1.451774982))   
	      + MULTIPLY(tmp16, FIX(2.172734804)),        
	      CONST_BITS-PASS1_BITS);
    dataptr[6] = (DCTELEM)
      DESCALE(tmp10 - MULTIPLY(tmp14, FIX(0.211164243))   
	      - MULTIPLY(tmp17, FIX(1.061594338)),        
	      CONST_BITS-PASS1_BITS);

    

    tmp11 = MULTIPLY(tmp0 + tmp1, FIX(1.353318001)) +         
	    MULTIPLY(tmp6 - tmp7, FIX(0.410524528));          
    tmp12 = MULTIPLY(tmp0 + tmp2, FIX(1.247225013)) +         
	    MULTIPLY(tmp5 + tmp7, FIX(0.666655658));          
    tmp13 = MULTIPLY(tmp0 + tmp3, FIX(1.093201867)) +         
	    MULTIPLY(tmp4 - tmp7, FIX(0.897167586));          
    tmp14 = MULTIPLY(tmp1 + tmp2, FIX(0.138617169)) +         
	    MULTIPLY(tmp6 - tmp5, FIX(1.407403738));          
    tmp15 = MULTIPLY(tmp1 + tmp3, - FIX(0.666655658)) +       
	    MULTIPLY(tmp4 + tmp6, - FIX(1.247225013));        
    tmp16 = MULTIPLY(tmp2 + tmp3, - FIX(1.353318001)) +       
	    MULTIPLY(tmp5 - tmp4, FIX(0.410524528));          
    tmp10 = tmp11 + tmp12 + tmp13 -
	    MULTIPLY(tmp0, FIX(2.286341144)) +                
	    MULTIPLY(tmp7, FIX(0.779653625));                 
    tmp11 += tmp14 + tmp15 + MULTIPLY(tmp1, FIX(0.071888074)) 
	     - MULTIPLY(tmp6, FIX(1.663905119));              
    tmp12 += tmp14 + tmp16 - MULTIPLY(tmp2, FIX(1.125726048)) 
	     + MULTIPLY(tmp5, FIX(1.227391138));              
    tmp13 += tmp15 + tmp16 + MULTIPLY(tmp3, FIX(1.065388962)) 
	     + MULTIPLY(tmp4, FIX(2.167985692));              

    dataptr[1] = (DCTELEM) DESCALE(tmp10, CONST_BITS-PASS1_BITS);
    dataptr[3] = (DCTELEM) DESCALE(tmp11, CONST_BITS-PASS1_BITS);
    dataptr[5] = (DCTELEM) DESCALE(tmp12, CONST_BITS-PASS1_BITS);
    dataptr[7] = (DCTELEM) DESCALE(tmp13, CONST_BITS-PASS1_BITS);

    dataptr += DCTSIZE;		
  }

  

  dataptr = data;
  for (ctr = DCTSIZE-1; ctr >= 0; ctr--) {
    

    tmp0 = dataptr[DCTSIZE*0] + dataptr[DCTSIZE*7];
    tmp1 = dataptr[DCTSIZE*1] + dataptr[DCTSIZE*6];
    tmp2 = dataptr[DCTSIZE*2] + dataptr[DCTSIZE*5];
    tmp3 = dataptr[DCTSIZE*3] + dataptr[DCTSIZE*4];

    tmp10 = tmp0 + tmp3;
    tmp12 = tmp0 - tmp3;
    tmp11 = tmp1 + tmp2;
    tmp13 = tmp1 - tmp2;

    tmp0 = dataptr[DCTSIZE*0] - dataptr[DCTSIZE*7];
    tmp1 = dataptr[DCTSIZE*1] - dataptr[DCTSIZE*6];
    tmp2 = dataptr[DCTSIZE*2] - dataptr[DCTSIZE*5];
    tmp3 = dataptr[DCTSIZE*3] - dataptr[DCTSIZE*4];

    dataptr[DCTSIZE*0] = (DCTELEM) DESCALE(tmp10 + tmp11, PASS1_BITS+1);
    dataptr[DCTSIZE*4] = (DCTELEM) DESCALE(tmp10 - tmp11, PASS1_BITS+1);

    z1 = MULTIPLY(tmp12 + tmp13, FIX_0_541196100);
    dataptr[DCTSIZE*2] = (DCTELEM) DESCALE(z1 + MULTIPLY(tmp12, FIX_0_765366865),
					   CONST_BITS+PASS1_BITS+1);
    dataptr[DCTSIZE*6] = (DCTELEM) DESCALE(z1 - MULTIPLY(tmp13, FIX_1_847759065),
					   CONST_BITS+PASS1_BITS+1);

    

    tmp10 = tmp0 + tmp3;
    tmp11 = tmp1 + tmp2;
    tmp12 = tmp0 + tmp2;
    tmp13 = tmp1 + tmp3;
    z1 = MULTIPLY(tmp12 + tmp13, FIX_1_175875602); 

    tmp0  = MULTIPLY(tmp0,    FIX_1_501321110);    
    tmp1  = MULTIPLY(tmp1,    FIX_3_072711026);    
    tmp2  = MULTIPLY(tmp2,    FIX_2_053119869);    
    tmp3  = MULTIPLY(tmp3,    FIX_0_298631336);    
    tmp10 = MULTIPLY(tmp10, - FIX_0_899976223);    
    tmp11 = MULTIPLY(tmp11, - FIX_2_562915447);    
    tmp12 = MULTIPLY(tmp12, - FIX_0_390180644);    
    tmp13 = MULTIPLY(tmp13, - FIX_1_961570560);    

    tmp12 += z1;
    tmp13 += z1;

    dataptr[DCTSIZE*1] = (DCTELEM) DESCALE(tmp0 + tmp10 + tmp12,
					   CONST_BITS+PASS1_BITS+1);
    dataptr[DCTSIZE*3] = (DCTELEM) DESCALE(tmp1 + tmp11 + tmp13,
					   CONST_BITS+PASS1_BITS+1);
    dataptr[DCTSIZE*5] = (DCTELEM) DESCALE(tmp2 + tmp11 + tmp12,
					   CONST_BITS+PASS1_BITS+1);
    dataptr[DCTSIZE*7] = (DCTELEM) DESCALE(tmp3 + tmp10 + tmp13,
					   CONST_BITS+PASS1_BITS+1);

    dataptr++;			
  }
}

GLOBAL(void)
jpeg_fdct_14x7 (DCTELEM * data, JSAMPARRAY sample_data, JDIMENSION start_col)
{
  INT32 tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6;
  INT32 tmp10, tmp11, tmp12, tmp13, tmp14, tmp15, tmp16;
  INT32 z1, z2, z3;
  DCTELEM *dataptr;
  JSAMPROW elemptr;
  int ctr;
  SHIFT_TEMPS

  
  MEMZERO(&data[DCTSIZE*7], SIZEOF(DCTELEM) * DCTSIZE);

  
  
  
  

  dataptr = data;
  for (ctr = 0; ctr < 7; ctr++) {
    elemptr = sample_data[ctr] + start_col;

    

    tmp0 = GETJSAMPLE(elemptr[0]) + GETJSAMPLE(elemptr[13]);
    tmp1 = GETJSAMPLE(elemptr[1]) + GETJSAMPLE(elemptr[12]);
    tmp2 = GETJSAMPLE(elemptr[2]) + GETJSAMPLE(elemptr[11]);
    tmp13 = GETJSAMPLE(elemptr[3]) + GETJSAMPLE(elemptr[10]);
    tmp4 = GETJSAMPLE(elemptr[4]) + GETJSAMPLE(elemptr[9]);
    tmp5 = GETJSAMPLE(elemptr[5]) + GETJSAMPLE(elemptr[8]);
    tmp6 = GETJSAMPLE(elemptr[6]) + GETJSAMPLE(elemptr[7]);

    tmp10 = tmp0 + tmp6;
    tmp14 = tmp0 - tmp6;
    tmp11 = tmp1 + tmp5;
    tmp15 = tmp1 - tmp5;
    tmp12 = tmp2 + tmp4;
    tmp16 = tmp2 - tmp4;

    tmp0 = GETJSAMPLE(elemptr[0]) - GETJSAMPLE(elemptr[13]);
    tmp1 = GETJSAMPLE(elemptr[1]) - GETJSAMPLE(elemptr[12]);
    tmp2 = GETJSAMPLE(elemptr[2]) - GETJSAMPLE(elemptr[11]);
    tmp3 = GETJSAMPLE(elemptr[3]) - GETJSAMPLE(elemptr[10]);
    tmp4 = GETJSAMPLE(elemptr[4]) - GETJSAMPLE(elemptr[9]);
    tmp5 = GETJSAMPLE(elemptr[5]) - GETJSAMPLE(elemptr[8]);
    tmp6 = GETJSAMPLE(elemptr[6]) - GETJSAMPLE(elemptr[7]);

    
    dataptr[0] = (DCTELEM)
      ((tmp10 + tmp11 + tmp12 + tmp13 - 14 * CENTERJSAMPLE) << PASS1_BITS);
    tmp13 += tmp13;
    dataptr[4] = (DCTELEM)
      DESCALE(MULTIPLY(tmp10 - tmp13, FIX(1.274162392)) + 
	      MULTIPLY(tmp11 - tmp13, FIX(0.314692123)) - 
	      MULTIPLY(tmp12 - tmp13, FIX(0.881747734)),  
	      CONST_BITS-PASS1_BITS);

    tmp10 = MULTIPLY(tmp14 + tmp15, FIX(1.105676686));    

    dataptr[2] = (DCTELEM)
      DESCALE(tmp10 + MULTIPLY(tmp14, FIX(0.273079590))   
	      + MULTIPLY(tmp16, FIX(0.613604268)),        
	      CONST_BITS-PASS1_BITS);
    dataptr[6] = (DCTELEM)
      DESCALE(tmp10 - MULTIPLY(tmp15, FIX(1.719280954))   
	      - MULTIPLY(tmp16, FIX(1.378756276)),        
	      CONST_BITS-PASS1_BITS);

    

    tmp10 = tmp1 + tmp2;
    tmp11 = tmp5 - tmp4;
    dataptr[7] = (DCTELEM) ((tmp0 - tmp10 + tmp3 - tmp11 - tmp6) << PASS1_BITS);
    tmp3 <<= CONST_BITS;
    tmp10 = MULTIPLY(tmp10, - FIX(0.158341681));          
    tmp11 = MULTIPLY(tmp11, FIX(1.405321284));            
    tmp10 += tmp11 - tmp3;
    tmp11 = MULTIPLY(tmp0 + tmp2, FIX(1.197448846)) +     
	    MULTIPLY(tmp4 + tmp6, FIX(0.752406978));      
    dataptr[5] = (DCTELEM)
      DESCALE(tmp10 + tmp11 - MULTIPLY(tmp2, FIX(2.373959773)) 
	      + MULTIPLY(tmp4, FIX(1.119999435)),         
	      CONST_BITS-PASS1_BITS);
    tmp12 = MULTIPLY(tmp0 + tmp1, FIX(1.334852607)) +     
	    MULTIPLY(tmp5 - tmp6, FIX(0.467085129));      
    dataptr[3] = (DCTELEM)
      DESCALE(tmp10 + tmp12 - MULTIPLY(tmp1, FIX(0.424103948)) 
	      - MULTIPLY(tmp5, FIX(3.069855259)),         
	      CONST_BITS-PASS1_BITS);
    dataptr[1] = (DCTELEM)
      DESCALE(tmp11 + tmp12 + tmp3 + tmp6 -
	      MULTIPLY(tmp0 + tmp6, FIX(1.126980169)),    
	      CONST_BITS-PASS1_BITS);

    dataptr += DCTSIZE;		
  }

  

  dataptr = data;
  for (ctr = DCTSIZE-1; ctr >= 0; ctr--) {
    

    tmp0 = dataptr[DCTSIZE*0] + dataptr[DCTSIZE*6];
    tmp1 = dataptr[DCTSIZE*1] + dataptr[DCTSIZE*5];
    tmp2 = dataptr[DCTSIZE*2] + dataptr[DCTSIZE*4];
    tmp3 = dataptr[DCTSIZE*3];

    tmp10 = dataptr[DCTSIZE*0] - dataptr[DCTSIZE*6];
    tmp11 = dataptr[DCTSIZE*1] - dataptr[DCTSIZE*5];
    tmp12 = dataptr[DCTSIZE*2] - dataptr[DCTSIZE*4];

    z1 = tmp0 + tmp2;
    dataptr[DCTSIZE*0] = (DCTELEM)
      DESCALE(MULTIPLY(z1 + tmp1 + tmp3, FIX(1.306122449)), 
	      CONST_BITS+PASS1_BITS+1);
    tmp3 += tmp3;
    z1 -= tmp3;
    z1 -= tmp3;
    z1 = MULTIPLY(z1, FIX(0.461784020));                
    z2 = MULTIPLY(tmp0 - tmp2, FIX(1.202428084));       
    z3 = MULTIPLY(tmp1 - tmp2, FIX(0.411026446));       
    dataptr[DCTSIZE*2] = (DCTELEM) DESCALE(z1 + z2 + z3, CONST_BITS+PASS1_BITS+1);
    z1 -= z2;
    z2 = MULTIPLY(tmp0 - tmp1, FIX(1.151670509));       
    dataptr[DCTSIZE*4] = (DCTELEM)
      DESCALE(z2 + z3 - MULTIPLY(tmp1 - tmp3, FIX(0.923568041)), 
	      CONST_BITS+PASS1_BITS+1);
    dataptr[DCTSIZE*6] = (DCTELEM) DESCALE(z1 + z2, CONST_BITS+PASS1_BITS+1);

    

    tmp1 = MULTIPLY(tmp10 + tmp11, FIX(1.221765677));   
    tmp2 = MULTIPLY(tmp10 - tmp11, FIX(0.222383464));   
    tmp0 = tmp1 - tmp2;
    tmp1 += tmp2;
    tmp2 = MULTIPLY(tmp11 + tmp12, - FIX(1.800824523)); 
    tmp1 += tmp2;
    tmp3 = MULTIPLY(tmp10 + tmp12, FIX(0.801442310));   
    tmp0 += tmp3;
    tmp2 += tmp3 + MULTIPLY(tmp12, FIX(2.443531355));   

    dataptr[DCTSIZE*1] = (DCTELEM) DESCALE(tmp0, CONST_BITS+PASS1_BITS+1);
    dataptr[DCTSIZE*3] = (DCTELEM) DESCALE(tmp1, CONST_BITS+PASS1_BITS+1);
    dataptr[DCTSIZE*5] = (DCTELEM) DESCALE(tmp2, CONST_BITS+PASS1_BITS+1);

    dataptr++;			
  }
}

GLOBAL(void)
jpeg_fdct_12x6 (DCTELEM * data, JSAMPARRAY sample_data, JDIMENSION start_col)
{
  INT32 tmp0, tmp1, tmp2, tmp3, tmp4, tmp5;
  INT32 tmp10, tmp11, tmp12, tmp13, tmp14, tmp15;
  DCTELEM *dataptr;
  JSAMPROW elemptr;
  int ctr;
  SHIFT_TEMPS

  
  MEMZERO(&data[DCTSIZE*6], SIZEOF(DCTELEM) * DCTSIZE * 2);

  
  
  
  

  dataptr = data;
  for (ctr = 0; ctr < 6; ctr++) {
    elemptr = sample_data[ctr] + start_col;

    

    tmp0 = GETJSAMPLE(elemptr[0]) + GETJSAMPLE(elemptr[11]);
    tmp1 = GETJSAMPLE(elemptr[1]) + GETJSAMPLE(elemptr[10]);
    tmp2 = GETJSAMPLE(elemptr[2]) + GETJSAMPLE(elemptr[9]);
    tmp3 = GETJSAMPLE(elemptr[3]) + GETJSAMPLE(elemptr[8]);
    tmp4 = GETJSAMPLE(elemptr[4]) + GETJSAMPLE(elemptr[7]);
    tmp5 = GETJSAMPLE(elemptr[5]) + GETJSAMPLE(elemptr[6]);

    tmp10 = tmp0 + tmp5;
    tmp13 = tmp0 - tmp5;
    tmp11 = tmp1 + tmp4;
    tmp14 = tmp1 - tmp4;
    tmp12 = tmp2 + tmp3;
    tmp15 = tmp2 - tmp3;

    tmp0 = GETJSAMPLE(elemptr[0]) - GETJSAMPLE(elemptr[11]);
    tmp1 = GETJSAMPLE(elemptr[1]) - GETJSAMPLE(elemptr[10]);
    tmp2 = GETJSAMPLE(elemptr[2]) - GETJSAMPLE(elemptr[9]);
    tmp3 = GETJSAMPLE(elemptr[3]) - GETJSAMPLE(elemptr[8]);
    tmp4 = GETJSAMPLE(elemptr[4]) - GETJSAMPLE(elemptr[7]);
    tmp5 = GETJSAMPLE(elemptr[5]) - GETJSAMPLE(elemptr[6]);

    
    dataptr[0] = (DCTELEM)
      ((tmp10 + tmp11 + tmp12 - 12 * CENTERJSAMPLE) << PASS1_BITS);
    dataptr[6] = (DCTELEM) ((tmp13 - tmp14 - tmp15) << PASS1_BITS);
    dataptr[4] = (DCTELEM)
      DESCALE(MULTIPLY(tmp10 - tmp12, FIX(1.224744871)), 
	      CONST_BITS-PASS1_BITS);
    dataptr[2] = (DCTELEM)
      DESCALE(tmp14 - tmp15 + MULTIPLY(tmp13 + tmp15, FIX(1.366025404)), 
	      CONST_BITS-PASS1_BITS);

    

    tmp10 = MULTIPLY(tmp1 + tmp4, FIX_0_541196100);    
    tmp14 = tmp10 + MULTIPLY(tmp1, FIX_0_765366865);   
    tmp15 = tmp10 - MULTIPLY(tmp4, FIX_1_847759065);   
    tmp12 = MULTIPLY(tmp0 + tmp2, FIX(1.121971054));   
    tmp13 = MULTIPLY(tmp0 + tmp3, FIX(0.860918669));   
    tmp10 = tmp12 + tmp13 + tmp14 - MULTIPLY(tmp0, FIX(0.580774953)) 
	    + MULTIPLY(tmp5, FIX(0.184591911));        
    tmp11 = MULTIPLY(tmp2 + tmp3, - FIX(0.184591911)); 
    tmp12 += tmp11 - tmp15 - MULTIPLY(tmp2, FIX(2.339493912)) 
	    + MULTIPLY(tmp5, FIX(0.860918669));        
    tmp13 += tmp11 - tmp14 + MULTIPLY(tmp3, FIX(0.725788011)) 
	    - MULTIPLY(tmp5, FIX(1.121971054));        
    tmp11 = tmp15 + MULTIPLY(tmp0 - tmp3, FIX(1.306562965)) 
	    - MULTIPLY(tmp2 + tmp5, FIX_0_541196100);  

    dataptr[1] = (DCTELEM) DESCALE(tmp10, CONST_BITS-PASS1_BITS);
    dataptr[3] = (DCTELEM) DESCALE(tmp11, CONST_BITS-PASS1_BITS);
    dataptr[5] = (DCTELEM) DESCALE(tmp12, CONST_BITS-PASS1_BITS);
    dataptr[7] = (DCTELEM) DESCALE(tmp13, CONST_BITS-PASS1_BITS);

    dataptr += DCTSIZE;		
  }

  

  dataptr = data;
  for (ctr = DCTSIZE-1; ctr >= 0; ctr--) {
    

    tmp0 = dataptr[DCTSIZE*0] + dataptr[DCTSIZE*5];
    tmp11 = dataptr[DCTSIZE*1] + dataptr[DCTSIZE*4];
    tmp2 = dataptr[DCTSIZE*2] + dataptr[DCTSIZE*3];

    tmp10 = tmp0 + tmp2;
    tmp12 = tmp0 - tmp2;

    tmp0 = dataptr[DCTSIZE*0] - dataptr[DCTSIZE*5];
    tmp1 = dataptr[DCTSIZE*1] - dataptr[DCTSIZE*4];
    tmp2 = dataptr[DCTSIZE*2] - dataptr[DCTSIZE*3];

    dataptr[DCTSIZE*0] = (DCTELEM)
      DESCALE(MULTIPLY(tmp10 + tmp11, FIX(1.777777778)),         
	      CONST_BITS+PASS1_BITS+1);
    dataptr[DCTSIZE*2] = (DCTELEM)
      DESCALE(MULTIPLY(tmp12, FIX(2.177324216)),                 
	      CONST_BITS+PASS1_BITS+1);
    dataptr[DCTSIZE*4] = (DCTELEM)
      DESCALE(MULTIPLY(tmp10 - tmp11 - tmp11, FIX(1.257078722)), 
	      CONST_BITS+PASS1_BITS+1);

    

    tmp10 = MULTIPLY(tmp0 + tmp2, FIX(0.650711829));             

    dataptr[DCTSIZE*1] = (DCTELEM)
      DESCALE(tmp10 + MULTIPLY(tmp0 + tmp1, FIX(1.777777778)),   
	      CONST_BITS+PASS1_BITS+1);
    dataptr[DCTSIZE*3] = (DCTELEM)
      DESCALE(MULTIPLY(tmp0 - tmp1 - tmp2, FIX(1.777777778)),    
	      CONST_BITS+PASS1_BITS+1);
    dataptr[DCTSIZE*5] = (DCTELEM)
      DESCALE(tmp10 + MULTIPLY(tmp2 - tmp1, FIX(1.777777778)),   
	      CONST_BITS+PASS1_BITS+1);

    dataptr++;			
  }
}

GLOBAL(void)
jpeg_fdct_10x5 (DCTELEM * data, JSAMPARRAY sample_data, JDIMENSION start_col)
{
  INT32 tmp0, tmp1, tmp2, tmp3, tmp4;
  INT32 tmp10, tmp11, tmp12, tmp13, tmp14;
  DCTELEM *dataptr;
  JSAMPROW elemptr;
  int ctr;
  SHIFT_TEMPS

  
  MEMZERO(&data[DCTSIZE*5], SIZEOF(DCTELEM) * DCTSIZE * 3);

  
  
  
  

  dataptr = data;
  for (ctr = 0; ctr < 5; ctr++) {
    elemptr = sample_data[ctr] + start_col;

    

    tmp0 = GETJSAMPLE(elemptr[0]) + GETJSAMPLE(elemptr[9]);
    tmp1 = GETJSAMPLE(elemptr[1]) + GETJSAMPLE(elemptr[8]);
    tmp12 = GETJSAMPLE(elemptr[2]) + GETJSAMPLE(elemptr[7]);
    tmp3 = GETJSAMPLE(elemptr[3]) + GETJSAMPLE(elemptr[6]);
    tmp4 = GETJSAMPLE(elemptr[4]) + GETJSAMPLE(elemptr[5]);

    tmp10 = tmp0 + tmp4;
    tmp13 = tmp0 - tmp4;
    tmp11 = tmp1 + tmp3;
    tmp14 = tmp1 - tmp3;

    tmp0 = GETJSAMPLE(elemptr[0]) - GETJSAMPLE(elemptr[9]);
    tmp1 = GETJSAMPLE(elemptr[1]) - GETJSAMPLE(elemptr[8]);
    tmp2 = GETJSAMPLE(elemptr[2]) - GETJSAMPLE(elemptr[7]);
    tmp3 = GETJSAMPLE(elemptr[3]) - GETJSAMPLE(elemptr[6]);
    tmp4 = GETJSAMPLE(elemptr[4]) - GETJSAMPLE(elemptr[5]);

    
    dataptr[0] = (DCTELEM)
      ((tmp10 + tmp11 + tmp12 - 10 * CENTERJSAMPLE) << PASS1_BITS);
    tmp12 += tmp12;
    dataptr[4] = (DCTELEM)
      DESCALE(MULTIPLY(tmp10 - tmp12, FIX(1.144122806)) - 
	      MULTIPLY(tmp11 - tmp12, FIX(0.437016024)),  
	      CONST_BITS-PASS1_BITS);
    tmp10 = MULTIPLY(tmp13 + tmp14, FIX(0.831253876));    
    dataptr[2] = (DCTELEM)
      DESCALE(tmp10 + MULTIPLY(tmp13, FIX(0.513743148)),  
	      CONST_BITS-PASS1_BITS);
    dataptr[6] = (DCTELEM)
      DESCALE(tmp10 - MULTIPLY(tmp14, FIX(2.176250899)),  
	      CONST_BITS-PASS1_BITS);

    

    tmp10 = tmp0 + tmp4;
    tmp11 = tmp1 - tmp3;
    dataptr[5] = (DCTELEM) ((tmp10 - tmp11 - tmp2) << PASS1_BITS);
    tmp2 <<= CONST_BITS;
    dataptr[1] = (DCTELEM)
      DESCALE(MULTIPLY(tmp0, FIX(1.396802247)) +          
	      MULTIPLY(tmp1, FIX(1.260073511)) + tmp2 +   
	      MULTIPLY(tmp3, FIX(0.642039522)) +          
	      MULTIPLY(tmp4, FIX(0.221231742)),           
	      CONST_BITS-PASS1_BITS);
    tmp12 = MULTIPLY(tmp0 - tmp4, FIX(0.951056516)) -     
	    MULTIPLY(tmp1 + tmp3, FIX(0.587785252));      
    tmp13 = MULTIPLY(tmp10 + tmp11, FIX(0.309016994)) +   
	    (tmp11 << (CONST_BITS - 1)) - tmp2;
    dataptr[3] = (DCTELEM) DESCALE(tmp12 + tmp13, CONST_BITS-PASS1_BITS);
    dataptr[7] = (DCTELEM) DESCALE(tmp12 - tmp13, CONST_BITS-PASS1_BITS);

    dataptr += DCTSIZE;		
  }

  

  dataptr = data;
  for (ctr = DCTSIZE-1; ctr >= 0; ctr--) {
    

    tmp0 = dataptr[DCTSIZE*0] + dataptr[DCTSIZE*4];
    tmp1 = dataptr[DCTSIZE*1] + dataptr[DCTSIZE*3];
    tmp2 = dataptr[DCTSIZE*2];

    tmp10 = tmp0 + tmp1;
    tmp11 = tmp0 - tmp1;

    tmp0 = dataptr[DCTSIZE*0] - dataptr[DCTSIZE*4];
    tmp1 = dataptr[DCTSIZE*1] - dataptr[DCTSIZE*3];

    dataptr[DCTSIZE*0] = (DCTELEM)
      DESCALE(MULTIPLY(tmp10 + tmp2, FIX(1.28)),        
	      CONST_BITS+PASS1_BITS);
    tmp11 = MULTIPLY(tmp11, FIX(1.011928851));          
    tmp10 -= tmp2 << 2;
    tmp10 = MULTIPLY(tmp10, FIX(0.452548340));          
    dataptr[DCTSIZE*2] = (DCTELEM) DESCALE(tmp11 + tmp10, CONST_BITS+PASS1_BITS);
    dataptr[DCTSIZE*4] = (DCTELEM) DESCALE(tmp11 - tmp10, CONST_BITS+PASS1_BITS);

    

    tmp10 = MULTIPLY(tmp0 + tmp1, FIX(1.064004961));    

    dataptr[DCTSIZE*1] = (DCTELEM)
      DESCALE(tmp10 + MULTIPLY(tmp0, FIX(0.657591230)), 
	      CONST_BITS+PASS1_BITS);
    dataptr[DCTSIZE*3] = (DCTELEM)
      DESCALE(tmp10 - MULTIPLY(tmp1, FIX(2.785601151)), 
	      CONST_BITS+PASS1_BITS);

    dataptr++;			
  }
}

GLOBAL(void)
jpeg_fdct_8x4 (DCTELEM * data, JSAMPARRAY sample_data, JDIMENSION start_col)
{
  INT32 tmp0, tmp1, tmp2, tmp3;
  INT32 tmp10, tmp11, tmp12, tmp13;
  INT32 z1;
  DCTELEM *dataptr;
  JSAMPROW elemptr;
  int ctr;
  SHIFT_TEMPS

  
  MEMZERO(&data[DCTSIZE*4], SIZEOF(DCTELEM) * DCTSIZE * 4);

  
  
  
  

  dataptr = data;
  for (ctr = 0; ctr < 4; ctr++) {
    elemptr = sample_data[ctr] + start_col;

    

    tmp0 = GETJSAMPLE(elemptr[0]) + GETJSAMPLE(elemptr[7]);
    tmp1 = GETJSAMPLE(elemptr[1]) + GETJSAMPLE(elemptr[6]);
    tmp2 = GETJSAMPLE(elemptr[2]) + GETJSAMPLE(elemptr[5]);
    tmp3 = GETJSAMPLE(elemptr[3]) + GETJSAMPLE(elemptr[4]);

    tmp10 = tmp0 + tmp3;
    tmp12 = tmp0 - tmp3;
    tmp11 = tmp1 + tmp2;
    tmp13 = tmp1 - tmp2;

    tmp0 = GETJSAMPLE(elemptr[0]) - GETJSAMPLE(elemptr[7]);
    tmp1 = GETJSAMPLE(elemptr[1]) - GETJSAMPLE(elemptr[6]);
    tmp2 = GETJSAMPLE(elemptr[2]) - GETJSAMPLE(elemptr[5]);
    tmp3 = GETJSAMPLE(elemptr[3]) - GETJSAMPLE(elemptr[4]);

    
    dataptr[0] = (DCTELEM)
      ((tmp10 + tmp11 - 8 * CENTERJSAMPLE) << (PASS1_BITS+1));
    dataptr[4] = (DCTELEM) ((tmp10 - tmp11) << (PASS1_BITS+1));

    z1 = MULTIPLY(tmp12 + tmp13, FIX_0_541196100);
    
    z1 += ONE << (CONST_BITS-PASS1_BITS-2);
    dataptr[2] = (DCTELEM) RIGHT_SHIFT(z1 + MULTIPLY(tmp12, FIX_0_765366865),
				       CONST_BITS-PASS1_BITS-1);
    dataptr[6] = (DCTELEM) RIGHT_SHIFT(z1 - MULTIPLY(tmp13, FIX_1_847759065),
				       CONST_BITS-PASS1_BITS-1);

    

    tmp10 = tmp0 + tmp3;
    tmp11 = tmp1 + tmp2;
    tmp12 = tmp0 + tmp2;
    tmp13 = tmp1 + tmp3;
    z1 = MULTIPLY(tmp12 + tmp13, FIX_1_175875602); 
    
    z1 += ONE << (CONST_BITS-PASS1_BITS-2);

    tmp0  = MULTIPLY(tmp0,    FIX_1_501321110);    
    tmp1  = MULTIPLY(tmp1,    FIX_3_072711026);    
    tmp2  = MULTIPLY(tmp2,    FIX_2_053119869);    
    tmp3  = MULTIPLY(tmp3,    FIX_0_298631336);    
    tmp10 = MULTIPLY(tmp10, - FIX_0_899976223);    
    tmp11 = MULTIPLY(tmp11, - FIX_2_562915447);    
    tmp12 = MULTIPLY(tmp12, - FIX_0_390180644);    
    tmp13 = MULTIPLY(tmp13, - FIX_1_961570560);    

    tmp12 += z1;
    tmp13 += z1;

    dataptr[1] = (DCTELEM)
      RIGHT_SHIFT(tmp0 + tmp10 + tmp12, CONST_BITS-PASS1_BITS-1);
    dataptr[3] = (DCTELEM)
      RIGHT_SHIFT(tmp1 + tmp11 + tmp13, CONST_BITS-PASS1_BITS-1);
    dataptr[5] = (DCTELEM)
      RIGHT_SHIFT(tmp2 + tmp11 + tmp12, CONST_BITS-PASS1_BITS-1);
    dataptr[7] = (DCTELEM)
      RIGHT_SHIFT(tmp3 + tmp10 + tmp13, CONST_BITS-PASS1_BITS-1);

    dataptr += DCTSIZE;		
  }

  

  dataptr = data;
  for (ctr = DCTSIZE-1; ctr >= 0; ctr--) {
    

    
    tmp0 = dataptr[DCTSIZE*0] + dataptr[DCTSIZE*3] + (ONE << (PASS1_BITS-1));
    tmp1 = dataptr[DCTSIZE*1] + dataptr[DCTSIZE*2];

    tmp10 = dataptr[DCTSIZE*0] - dataptr[DCTSIZE*3];
    tmp11 = dataptr[DCTSIZE*1] - dataptr[DCTSIZE*2];

    dataptr[DCTSIZE*0] = (DCTELEM) RIGHT_SHIFT(tmp0 + tmp1, PASS1_BITS);
    dataptr[DCTSIZE*2] = (DCTELEM) RIGHT_SHIFT(tmp0 - tmp1, PASS1_BITS);

    

    tmp0 = MULTIPLY(tmp10 + tmp11, FIX_0_541196100);   
    
    tmp0 += ONE << (CONST_BITS+PASS1_BITS-1);

    dataptr[DCTSIZE*1] = (DCTELEM)
      RIGHT_SHIFT(tmp0 + MULTIPLY(tmp10, FIX_0_765366865), 
		  CONST_BITS+PASS1_BITS);
    dataptr[DCTSIZE*3] = (DCTELEM)
      RIGHT_SHIFT(tmp0 - MULTIPLY(tmp11, FIX_1_847759065), 
		  CONST_BITS+PASS1_BITS);

    dataptr++;			
  }
}

GLOBAL(void)
jpeg_fdct_6x3 (DCTELEM * data, JSAMPARRAY sample_data, JDIMENSION start_col)
{
  INT32 tmp0, tmp1, tmp2;
  INT32 tmp10, tmp11, tmp12;
  DCTELEM *dataptr;
  JSAMPROW elemptr;
  int ctr;
  SHIFT_TEMPS

  
  MEMZERO(data, SIZEOF(DCTELEM) * DCTSIZE2);

  
  
  
  
  
  

  dataptr = data;
  for (ctr = 0; ctr < 3; ctr++) {
    elemptr = sample_data[ctr] + start_col;

    

    tmp0 = GETJSAMPLE(elemptr[0]) + GETJSAMPLE(elemptr[5]);
    tmp11 = GETJSAMPLE(elemptr[1]) + GETJSAMPLE(elemptr[4]);
    tmp2 = GETJSAMPLE(elemptr[2]) + GETJSAMPLE(elemptr[3]);

    tmp10 = tmp0 + tmp2;
    tmp12 = tmp0 - tmp2;

    tmp0 = GETJSAMPLE(elemptr[0]) - GETJSAMPLE(elemptr[5]);
    tmp1 = GETJSAMPLE(elemptr[1]) - GETJSAMPLE(elemptr[4]);
    tmp2 = GETJSAMPLE(elemptr[2]) - GETJSAMPLE(elemptr[3]);

    
    dataptr[0] = (DCTELEM)
      ((tmp10 + tmp11 - 6 * CENTERJSAMPLE) << (PASS1_BITS+1));
    dataptr[2] = (DCTELEM)
      DESCALE(MULTIPLY(tmp12, FIX(1.224744871)),                 
	      CONST_BITS-PASS1_BITS-1);
    dataptr[4] = (DCTELEM)
      DESCALE(MULTIPLY(tmp10 - tmp11 - tmp11, FIX(0.707106781)), 
	      CONST_BITS-PASS1_BITS-1);

    

    tmp10 = DESCALE(MULTIPLY(tmp0 + tmp2, FIX(0.366025404)),     
		    CONST_BITS-PASS1_BITS-1);

    dataptr[1] = (DCTELEM) (tmp10 + ((tmp0 + tmp1) << (PASS1_BITS+1)));
    dataptr[3] = (DCTELEM) ((tmp0 - tmp1 - tmp2) << (PASS1_BITS+1));
    dataptr[5] = (DCTELEM) (tmp10 + ((tmp2 - tmp1) << (PASS1_BITS+1)));

    dataptr += DCTSIZE;		
  }

  

  dataptr = data;
  for (ctr = 0; ctr < 6; ctr++) {
    

    tmp0 = dataptr[DCTSIZE*0] + dataptr[DCTSIZE*2];
    tmp1 = dataptr[DCTSIZE*1];

    tmp2 = dataptr[DCTSIZE*0] - dataptr[DCTSIZE*2];

    dataptr[DCTSIZE*0] = (DCTELEM)
      DESCALE(MULTIPLY(tmp0 + tmp1, FIX(1.777777778)),        
	      CONST_BITS+PASS1_BITS);
    dataptr[DCTSIZE*2] = (DCTELEM)
      DESCALE(MULTIPLY(tmp0 - tmp1 - tmp1, FIX(1.257078722)), 
	      CONST_BITS+PASS1_BITS);

    

    dataptr[DCTSIZE*1] = (DCTELEM)
      DESCALE(MULTIPLY(tmp2, FIX(2.177324216)),               
	      CONST_BITS+PASS1_BITS);

    dataptr++;			
  }
}

GLOBAL(void)
jpeg_fdct_4x2 (DCTELEM * data, JSAMPARRAY sample_data, JDIMENSION start_col)
{
  INT32 tmp0, tmp1;
  INT32 tmp10, tmp11;
  DCTELEM *dataptr;
  JSAMPROW elemptr;
  int ctr;
  SHIFT_TEMPS

  
  MEMZERO(data, SIZEOF(DCTELEM) * DCTSIZE2);

  
  
  
  
  
  

  dataptr = data;
  for (ctr = 0; ctr < 2; ctr++) {
    elemptr = sample_data[ctr] + start_col;

    

    tmp0 = GETJSAMPLE(elemptr[0]) + GETJSAMPLE(elemptr[3]);
    tmp1 = GETJSAMPLE(elemptr[1]) + GETJSAMPLE(elemptr[2]);

    tmp10 = GETJSAMPLE(elemptr[0]) - GETJSAMPLE(elemptr[3]);
    tmp11 = GETJSAMPLE(elemptr[1]) - GETJSAMPLE(elemptr[2]);

    
    dataptr[0] = (DCTELEM)
      ((tmp0 + tmp1 - 4 * CENTERJSAMPLE) << (PASS1_BITS+3));
    dataptr[2] = (DCTELEM) ((tmp0 - tmp1) << (PASS1_BITS+3));

    

    tmp0 = MULTIPLY(tmp10 + tmp11, FIX_0_541196100);       
    
    tmp0 += ONE << (CONST_BITS-PASS1_BITS-4);

    dataptr[1] = (DCTELEM)
      RIGHT_SHIFT(tmp0 + MULTIPLY(tmp10, FIX_0_765366865), 
		  CONST_BITS-PASS1_BITS-3);
    dataptr[3] = (DCTELEM)
      RIGHT_SHIFT(tmp0 - MULTIPLY(tmp11, FIX_1_847759065), 
		  CONST_BITS-PASS1_BITS-3);

    dataptr += DCTSIZE;		
  }

  

  dataptr = data;
  for (ctr = 0; ctr < 4; ctr++) {
    

    
    tmp0 = dataptr[DCTSIZE*0] + (ONE << (PASS1_BITS-1));
    tmp1 = dataptr[DCTSIZE*1];

    dataptr[DCTSIZE*0] = (DCTELEM) RIGHT_SHIFT(tmp0 + tmp1, PASS1_BITS);

    

    dataptr[DCTSIZE*1] = (DCTELEM) RIGHT_SHIFT(tmp0 - tmp1, PASS1_BITS);

    dataptr++;			
  }
}

GLOBAL(void)
jpeg_fdct_2x1 (DCTELEM * data, JSAMPARRAY sample_data, JDIMENSION start_col)
{
  INT32 tmp0, tmp1;
  JSAMPROW elemptr;

  
  MEMZERO(data, SIZEOF(DCTELEM) * DCTSIZE2);

  elemptr = sample_data[0] + start_col;

  tmp0 = GETJSAMPLE(elemptr[0]);
  tmp1 = GETJSAMPLE(elemptr[1]);

  

  
  
  data[0] = (DCTELEM) ((tmp0 + tmp1 - 2 * CENTERJSAMPLE) << 5);

  
  data[1] = (DCTELEM) ((tmp0 - tmp1) << 5);
}

GLOBAL(void)
jpeg_fdct_8x16 (DCTELEM * data, JSAMPARRAY sample_data, JDIMENSION start_col)
{
  INT32 tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7;
  INT32 tmp10, tmp11, tmp12, tmp13, tmp14, tmp15, tmp16, tmp17;
  INT32 z1;
  DCTELEM workspace[DCTSIZE2];
  DCTELEM *dataptr;
  DCTELEM *wsptr;
  JSAMPROW elemptr;
  int ctr;
  SHIFT_TEMPS

  
  
  

  dataptr = data;
  ctr = 0;
  for (;;) {
    elemptr = sample_data[ctr] + start_col;

    

    tmp0 = GETJSAMPLE(elemptr[0]) + GETJSAMPLE(elemptr[7]);
    tmp1 = GETJSAMPLE(elemptr[1]) + GETJSAMPLE(elemptr[6]);
    tmp2 = GETJSAMPLE(elemptr[2]) + GETJSAMPLE(elemptr[5]);
    tmp3 = GETJSAMPLE(elemptr[3]) + GETJSAMPLE(elemptr[4]);

    tmp10 = tmp0 + tmp3;
    tmp12 = tmp0 - tmp3;
    tmp11 = tmp1 + tmp2;
    tmp13 = tmp1 - tmp2;

    tmp0 = GETJSAMPLE(elemptr[0]) - GETJSAMPLE(elemptr[7]);
    tmp1 = GETJSAMPLE(elemptr[1]) - GETJSAMPLE(elemptr[6]);
    tmp2 = GETJSAMPLE(elemptr[2]) - GETJSAMPLE(elemptr[5]);
    tmp3 = GETJSAMPLE(elemptr[3]) - GETJSAMPLE(elemptr[4]);

    
    dataptr[0] = (DCTELEM) ((tmp10 + tmp11 - 8 * CENTERJSAMPLE) << PASS1_BITS);
    dataptr[4] = (DCTELEM) ((tmp10 - tmp11) << PASS1_BITS);

    z1 = MULTIPLY(tmp12 + tmp13, FIX_0_541196100);
    dataptr[2] = (DCTELEM) DESCALE(z1 + MULTIPLY(tmp12, FIX_0_765366865),
				   CONST_BITS-PASS1_BITS);
    dataptr[6] = (DCTELEM) DESCALE(z1 - MULTIPLY(tmp13, FIX_1_847759065),
				   CONST_BITS-PASS1_BITS);

    

    tmp10 = tmp0 + tmp3;
    tmp11 = tmp1 + tmp2;
    tmp12 = tmp0 + tmp2;
    tmp13 = tmp1 + tmp3;
    z1 = MULTIPLY(tmp12 + tmp13, FIX_1_175875602); 

    tmp0  = MULTIPLY(tmp0,    FIX_1_501321110);    
    tmp1  = MULTIPLY(tmp1,    FIX_3_072711026);    
    tmp2  = MULTIPLY(tmp2,    FIX_2_053119869);    
    tmp3  = MULTIPLY(tmp3,    FIX_0_298631336);    
    tmp10 = MULTIPLY(tmp10, - FIX_0_899976223);    
    tmp11 = MULTIPLY(tmp11, - FIX_2_562915447);    
    tmp12 = MULTIPLY(tmp12, - FIX_0_390180644);    
    tmp13 = MULTIPLY(tmp13, - FIX_1_961570560);    

    tmp12 += z1;
    tmp13 += z1;

    dataptr[1] = (DCTELEM) DESCALE(tmp0 + tmp10 + tmp12, CONST_BITS-PASS1_BITS);
    dataptr[3] = (DCTELEM) DESCALE(tmp1 + tmp11 + tmp13, CONST_BITS-PASS1_BITS);
    dataptr[5] = (DCTELEM) DESCALE(tmp2 + tmp11 + tmp12, CONST_BITS-PASS1_BITS);
    dataptr[7] = (DCTELEM) DESCALE(tmp3 + tmp10 + tmp13, CONST_BITS-PASS1_BITS);

    ctr++;

    if (ctr != DCTSIZE) {
      if (ctr == DCTSIZE * 2)
	break;			
      dataptr += DCTSIZE;	
    } else
      dataptr = workspace;	
  }

  

  dataptr = data;
  wsptr = workspace;
  for (ctr = DCTSIZE-1; ctr >= 0; ctr--) {
    

    tmp0 = dataptr[DCTSIZE*0] + wsptr[DCTSIZE*7];
    tmp1 = dataptr[DCTSIZE*1] + wsptr[DCTSIZE*6];
    tmp2 = dataptr[DCTSIZE*2] + wsptr[DCTSIZE*5];
    tmp3 = dataptr[DCTSIZE*3] + wsptr[DCTSIZE*4];
    tmp4 = dataptr[DCTSIZE*4] + wsptr[DCTSIZE*3];
    tmp5 = dataptr[DCTSIZE*5] + wsptr[DCTSIZE*2];
    tmp6 = dataptr[DCTSIZE*6] + wsptr[DCTSIZE*1];
    tmp7 = dataptr[DCTSIZE*7] + wsptr[DCTSIZE*0];

    tmp10 = tmp0 + tmp7;
    tmp14 = tmp0 - tmp7;
    tmp11 = tmp1 + tmp6;
    tmp15 = tmp1 - tmp6;
    tmp12 = tmp2 + tmp5;
    tmp16 = tmp2 - tmp5;
    tmp13 = tmp3 + tmp4;
    tmp17 = tmp3 - tmp4;

    tmp0 = dataptr[DCTSIZE*0] - wsptr[DCTSIZE*7];
    tmp1 = dataptr[DCTSIZE*1] - wsptr[DCTSIZE*6];
    tmp2 = dataptr[DCTSIZE*2] - wsptr[DCTSIZE*5];
    tmp3 = dataptr[DCTSIZE*3] - wsptr[DCTSIZE*4];
    tmp4 = dataptr[DCTSIZE*4] - wsptr[DCTSIZE*3];
    tmp5 = dataptr[DCTSIZE*5] - wsptr[DCTSIZE*2];
    tmp6 = dataptr[DCTSIZE*6] - wsptr[DCTSIZE*1];
    tmp7 = dataptr[DCTSIZE*7] - wsptr[DCTSIZE*0];

    dataptr[DCTSIZE*0] = (DCTELEM)
      DESCALE(tmp10 + tmp11 + tmp12 + tmp13, PASS1_BITS+1);
    dataptr[DCTSIZE*4] = (DCTELEM)
      DESCALE(MULTIPLY(tmp10 - tmp13, FIX(1.306562965)) + 
	      MULTIPLY(tmp11 - tmp12, FIX_0_541196100),   
	      CONST_BITS+PASS1_BITS+1);

    tmp10 = MULTIPLY(tmp17 - tmp15, FIX(0.275899379)) +   
	    MULTIPLY(tmp14 - tmp16, FIX(1.387039845));    

    dataptr[DCTSIZE*2] = (DCTELEM)
      DESCALE(tmp10 + MULTIPLY(tmp15, FIX(1.451774982))   
	      + MULTIPLY(tmp16, FIX(2.172734804)),        
	      CONST_BITS+PASS1_BITS+1);
    dataptr[DCTSIZE*6] = (DCTELEM)
      DESCALE(tmp10 - MULTIPLY(tmp14, FIX(0.211164243))   
	      - MULTIPLY(tmp17, FIX(1.061594338)),        
	      CONST_BITS+PASS1_BITS+1);

    

    tmp11 = MULTIPLY(tmp0 + tmp1, FIX(1.353318001)) +         
	    MULTIPLY(tmp6 - tmp7, FIX(0.410524528));          
    tmp12 = MULTIPLY(tmp0 + tmp2, FIX(1.247225013)) +         
	    MULTIPLY(tmp5 + tmp7, FIX(0.666655658));          
    tmp13 = MULTIPLY(tmp0 + tmp3, FIX(1.093201867)) +         
	    MULTIPLY(tmp4 - tmp7, FIX(0.897167586));          
    tmp14 = MULTIPLY(tmp1 + tmp2, FIX(0.138617169)) +         
	    MULTIPLY(tmp6 - tmp5, FIX(1.407403738));          
    tmp15 = MULTIPLY(tmp1 + tmp3, - FIX(0.666655658)) +       
	    MULTIPLY(tmp4 + tmp6, - FIX(1.247225013));        
    tmp16 = MULTIPLY(tmp2 + tmp3, - FIX(1.353318001)) +       
	    MULTIPLY(tmp5 - tmp4, FIX(0.410524528));          
    tmp10 = tmp11 + tmp12 + tmp13 -
	    MULTIPLY(tmp0, FIX(2.286341144)) +                
	    MULTIPLY(tmp7, FIX(0.779653625));                 
    tmp11 += tmp14 + tmp15 + MULTIPLY(tmp1, FIX(0.071888074)) 
	     - MULTIPLY(tmp6, FIX(1.663905119));              
    tmp12 += tmp14 + tmp16 - MULTIPLY(tmp2, FIX(1.125726048)) 
	     + MULTIPLY(tmp5, FIX(1.227391138));              
    tmp13 += tmp15 + tmp16 + MULTIPLY(tmp3, FIX(1.065388962)) 
	     + MULTIPLY(tmp4, FIX(2.167985692));              

    dataptr[DCTSIZE*1] = (DCTELEM) DESCALE(tmp10, CONST_BITS+PASS1_BITS+1);
    dataptr[DCTSIZE*3] = (DCTELEM) DESCALE(tmp11, CONST_BITS+PASS1_BITS+1);
    dataptr[DCTSIZE*5] = (DCTELEM) DESCALE(tmp12, CONST_BITS+PASS1_BITS+1);
    dataptr[DCTSIZE*7] = (DCTELEM) DESCALE(tmp13, CONST_BITS+PASS1_BITS+1);

    dataptr++;			
    wsptr++;			
  }
}

GLOBAL(void)
jpeg_fdct_7x14 (DCTELEM * data, JSAMPARRAY sample_data, JDIMENSION start_col)
{
  INT32 tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6;
  INT32 tmp10, tmp11, tmp12, tmp13, tmp14, tmp15, tmp16;
  INT32 z1, z2, z3;
  DCTELEM workspace[8*6];
  DCTELEM *dataptr;
  DCTELEM *wsptr;
  JSAMPROW elemptr;
  int ctr;
  SHIFT_TEMPS

  
  MEMZERO(data, SIZEOF(DCTELEM) * DCTSIZE2);

  
  
  
  

  dataptr = data;
  ctr = 0;
  for (;;) {
    elemptr = sample_data[ctr] + start_col;

    

    tmp0 = GETJSAMPLE(elemptr[0]) + GETJSAMPLE(elemptr[6]);
    tmp1 = GETJSAMPLE(elemptr[1]) + GETJSAMPLE(elemptr[5]);
    tmp2 = GETJSAMPLE(elemptr[2]) + GETJSAMPLE(elemptr[4]);
    tmp3 = GETJSAMPLE(elemptr[3]);

    tmp10 = GETJSAMPLE(elemptr[0]) - GETJSAMPLE(elemptr[6]);
    tmp11 = GETJSAMPLE(elemptr[1]) - GETJSAMPLE(elemptr[5]);
    tmp12 = GETJSAMPLE(elemptr[2]) - GETJSAMPLE(elemptr[4]);

    z1 = tmp0 + tmp2;
    
    dataptr[0] = (DCTELEM)
      ((z1 + tmp1 + tmp3 - 7 * CENTERJSAMPLE) << PASS1_BITS);
    tmp3 += tmp3;
    z1 -= tmp3;
    z1 -= tmp3;
    z1 = MULTIPLY(z1, FIX(0.353553391));                
    z2 = MULTIPLY(tmp0 - tmp2, FIX(0.920609002));       
    z3 = MULTIPLY(tmp1 - tmp2, FIX(0.314692123));       
    dataptr[2] = (DCTELEM) DESCALE(z1 + z2 + z3, CONST_BITS-PASS1_BITS);
    z1 -= z2;
    z2 = MULTIPLY(tmp0 - tmp1, FIX(0.881747734));       
    dataptr[4] = (DCTELEM)
      DESCALE(z2 + z3 - MULTIPLY(tmp1 - tmp3, FIX(0.707106781)), 
	      CONST_BITS-PASS1_BITS);
    dataptr[6] = (DCTELEM) DESCALE(z1 + z2, CONST_BITS-PASS1_BITS);

    

    tmp1 = MULTIPLY(tmp10 + tmp11, FIX(0.935414347));   
    tmp2 = MULTIPLY(tmp10 - tmp11, FIX(0.170262339));   
    tmp0 = tmp1 - tmp2;
    tmp1 += tmp2;
    tmp2 = MULTIPLY(tmp11 + tmp12, - FIX(1.378756276)); 
    tmp1 += tmp2;
    tmp3 = MULTIPLY(tmp10 + tmp12, FIX(0.613604268));   
    tmp0 += tmp3;
    tmp2 += tmp3 + MULTIPLY(tmp12, FIX(1.870828693));   

    dataptr[1] = (DCTELEM) DESCALE(tmp0, CONST_BITS-PASS1_BITS);
    dataptr[3] = (DCTELEM) DESCALE(tmp1, CONST_BITS-PASS1_BITS);
    dataptr[5] = (DCTELEM) DESCALE(tmp2, CONST_BITS-PASS1_BITS);

    ctr++;

    if (ctr != DCTSIZE) {
      if (ctr == 14)
	break;			
      dataptr += DCTSIZE;	
    } else
      dataptr = workspace;	
  }

  

  dataptr = data;
  wsptr = workspace;
  for (ctr = 0; ctr < 7; ctr++) {
    

    tmp0 = dataptr[DCTSIZE*0] + wsptr[DCTSIZE*5];
    tmp1 = dataptr[DCTSIZE*1] + wsptr[DCTSIZE*4];
    tmp2 = dataptr[DCTSIZE*2] + wsptr[DCTSIZE*3];
    tmp13 = dataptr[DCTSIZE*3] + wsptr[DCTSIZE*2];
    tmp4 = dataptr[DCTSIZE*4] + wsptr[DCTSIZE*1];
    tmp5 = dataptr[DCTSIZE*5] + wsptr[DCTSIZE*0];
    tmp6 = dataptr[DCTSIZE*6] + dataptr[DCTSIZE*7];

    tmp10 = tmp0 + tmp6;
    tmp14 = tmp0 - tmp6;
    tmp11 = tmp1 + tmp5;
    tmp15 = tmp1 - tmp5;
    tmp12 = tmp2 + tmp4;
    tmp16 = tmp2 - tmp4;

    tmp0 = dataptr[DCTSIZE*0] - wsptr[DCTSIZE*5];
    tmp1 = dataptr[DCTSIZE*1] - wsptr[DCTSIZE*4];
    tmp2 = dataptr[DCTSIZE*2] - wsptr[DCTSIZE*3];
    tmp3 = dataptr[DCTSIZE*3] - wsptr[DCTSIZE*2];
    tmp4 = dataptr[DCTSIZE*4] - wsptr[DCTSIZE*1];
    tmp5 = dataptr[DCTSIZE*5] - wsptr[DCTSIZE*0];
    tmp6 = dataptr[DCTSIZE*6] - dataptr[DCTSIZE*7];

    dataptr[DCTSIZE*0] = (DCTELEM)
      DESCALE(MULTIPLY(tmp10 + tmp11 + tmp12 + tmp13,
		       FIX(0.653061224)),                 
	      CONST_BITS+PASS1_BITS);
    tmp13 += tmp13;
    dataptr[DCTSIZE*4] = (DCTELEM)
      DESCALE(MULTIPLY(tmp10 - tmp13, FIX(0.832106052)) + 
	      MULTIPLY(tmp11 - tmp13, FIX(0.205513223)) - 
	      MULTIPLY(tmp12 - tmp13, FIX(0.575835255)),  
	      CONST_BITS+PASS1_BITS);

    tmp10 = MULTIPLY(tmp14 + tmp15, FIX(0.722074570));    

    dataptr[DCTSIZE*2] = (DCTELEM)
      DESCALE(tmp10 + MULTIPLY(tmp14, FIX(0.178337691))   
	      + MULTIPLY(tmp16, FIX(0.400721155)),        
	      CONST_BITS+PASS1_BITS);
    dataptr[DCTSIZE*6] = (DCTELEM)
      DESCALE(tmp10 - MULTIPLY(tmp15, FIX(1.122795725))   
	      - MULTIPLY(tmp16, FIX(0.900412262)),        
	      CONST_BITS+PASS1_BITS);

    

    tmp10 = tmp1 + tmp2;
    tmp11 = tmp5 - tmp4;
    dataptr[DCTSIZE*7] = (DCTELEM)
      DESCALE(MULTIPLY(tmp0 - tmp10 + tmp3 - tmp11 - tmp6,
		       FIX(0.653061224)),                 
	      CONST_BITS+PASS1_BITS);
    tmp3  = MULTIPLY(tmp3 , FIX(0.653061224));            
    tmp10 = MULTIPLY(tmp10, - FIX(0.103406812));          
    tmp11 = MULTIPLY(tmp11, FIX(0.917760839));            
    tmp10 += tmp11 - tmp3;
    tmp11 = MULTIPLY(tmp0 + tmp2, FIX(0.782007410)) +     
	    MULTIPLY(tmp4 + tmp6, FIX(0.491367823));      
    dataptr[DCTSIZE*5] = (DCTELEM)
      DESCALE(tmp10 + tmp11 - MULTIPLY(tmp2, FIX(1.550341076)) 
	      + MULTIPLY(tmp4, FIX(0.731428202)),         
	      CONST_BITS+PASS1_BITS);
    tmp12 = MULTIPLY(tmp0 + tmp1, FIX(0.871740478)) +     
	    MULTIPLY(tmp5 - tmp6, FIX(0.305035186));      
    dataptr[DCTSIZE*3] = (DCTELEM)
      DESCALE(tmp10 + tmp12 - MULTIPLY(tmp1, FIX(0.276965844)) 
	      - MULTIPLY(tmp5, FIX(2.004803435)),         
	      CONST_BITS+PASS1_BITS);
    dataptr[DCTSIZE*1] = (DCTELEM)
      DESCALE(tmp11 + tmp12 + tmp3
	      - MULTIPLY(tmp0, FIX(0.735987049))          
	      - MULTIPLY(tmp6, FIX(0.082925825)),         
	      CONST_BITS+PASS1_BITS);

    dataptr++;			
    wsptr++;			
  }
}

GLOBAL(void)
jpeg_fdct_6x12 (DCTELEM * data, JSAMPARRAY sample_data, JDIMENSION start_col)
{
  INT32 tmp0, tmp1, tmp2, tmp3, tmp4, tmp5;
  INT32 tmp10, tmp11, tmp12, tmp13, tmp14, tmp15;
  DCTELEM workspace[8*4];
  DCTELEM *dataptr;
  DCTELEM *wsptr;
  JSAMPROW elemptr;
  int ctr;
  SHIFT_TEMPS

  
  MEMZERO(data, SIZEOF(DCTELEM) * DCTSIZE2);

  
  
  
  

  dataptr = data;
  ctr = 0;
  for (;;) {
    elemptr = sample_data[ctr] + start_col;

    

    tmp0 = GETJSAMPLE(elemptr[0]) + GETJSAMPLE(elemptr[5]);
    tmp11 = GETJSAMPLE(elemptr[1]) + GETJSAMPLE(elemptr[4]);
    tmp2 = GETJSAMPLE(elemptr[2]) + GETJSAMPLE(elemptr[3]);

    tmp10 = tmp0 + tmp2;
    tmp12 = tmp0 - tmp2;

    tmp0 = GETJSAMPLE(elemptr[0]) - GETJSAMPLE(elemptr[5]);
    tmp1 = GETJSAMPLE(elemptr[1]) - GETJSAMPLE(elemptr[4]);
    tmp2 = GETJSAMPLE(elemptr[2]) - GETJSAMPLE(elemptr[3]);

    
    dataptr[0] = (DCTELEM)
      ((tmp10 + tmp11 - 6 * CENTERJSAMPLE) << PASS1_BITS);
    dataptr[2] = (DCTELEM)
      DESCALE(MULTIPLY(tmp12, FIX(1.224744871)),                 
	      CONST_BITS-PASS1_BITS);
    dataptr[4] = (DCTELEM)
      DESCALE(MULTIPLY(tmp10 - tmp11 - tmp11, FIX(0.707106781)), 
	      CONST_BITS-PASS1_BITS);

    

    tmp10 = DESCALE(MULTIPLY(tmp0 + tmp2, FIX(0.366025404)),     
		    CONST_BITS-PASS1_BITS);

    dataptr[1] = (DCTELEM) (tmp10 + ((tmp0 + tmp1) << PASS1_BITS));
    dataptr[3] = (DCTELEM) ((tmp0 - tmp1 - tmp2) << PASS1_BITS);
    dataptr[5] = (DCTELEM) (tmp10 + ((tmp2 - tmp1) << PASS1_BITS));

    ctr++;

    if (ctr != DCTSIZE) {
      if (ctr == 12)
	break;			
      dataptr += DCTSIZE;	
    } else
      dataptr = workspace;	
  }

  

  dataptr = data;
  wsptr = workspace;
  for (ctr = 0; ctr < 6; ctr++) {
    

    tmp0 = dataptr[DCTSIZE*0] + wsptr[DCTSIZE*3];
    tmp1 = dataptr[DCTSIZE*1] + wsptr[DCTSIZE*2];
    tmp2 = dataptr[DCTSIZE*2] + wsptr[DCTSIZE*1];
    tmp3 = dataptr[DCTSIZE*3] + wsptr[DCTSIZE*0];
    tmp4 = dataptr[DCTSIZE*4] + dataptr[DCTSIZE*7];
    tmp5 = dataptr[DCTSIZE*5] + dataptr[DCTSIZE*6];

    tmp10 = tmp0 + tmp5;
    tmp13 = tmp0 - tmp5;
    tmp11 = tmp1 + tmp4;
    tmp14 = tmp1 - tmp4;
    tmp12 = tmp2 + tmp3;
    tmp15 = tmp2 - tmp3;

    tmp0 = dataptr[DCTSIZE*0] - wsptr[DCTSIZE*3];
    tmp1 = dataptr[DCTSIZE*1] - wsptr[DCTSIZE*2];
    tmp2 = dataptr[DCTSIZE*2] - wsptr[DCTSIZE*1];
    tmp3 = dataptr[DCTSIZE*3] - wsptr[DCTSIZE*0];
    tmp4 = dataptr[DCTSIZE*4] - dataptr[DCTSIZE*7];
    tmp5 = dataptr[DCTSIZE*5] - dataptr[DCTSIZE*6];

    dataptr[DCTSIZE*0] = (DCTELEM)
      DESCALE(MULTIPLY(tmp10 + tmp11 + tmp12, FIX(0.888888889)), 
	      CONST_BITS+PASS1_BITS);
    dataptr[DCTSIZE*6] = (DCTELEM)
      DESCALE(MULTIPLY(tmp13 - tmp14 - tmp15, FIX(0.888888889)), 
	      CONST_BITS+PASS1_BITS);
    dataptr[DCTSIZE*4] = (DCTELEM)
      DESCALE(MULTIPLY(tmp10 - tmp12, FIX(1.088662108)),         
	      CONST_BITS+PASS1_BITS);
    dataptr[DCTSIZE*2] = (DCTELEM)
      DESCALE(MULTIPLY(tmp14 - tmp15, FIX(0.888888889)) +        
	      MULTIPLY(tmp13 + tmp15, FIX(1.214244803)),         
	      CONST_BITS+PASS1_BITS);

    

    tmp10 = MULTIPLY(tmp1 + tmp4, FIX(0.481063200));   
    tmp14 = tmp10 + MULTIPLY(tmp1, FIX(0.680326102));  
    tmp15 = tmp10 - MULTIPLY(tmp4, FIX(1.642452502));  
    tmp12 = MULTIPLY(tmp0 + tmp2, FIX(0.997307603));   
    tmp13 = MULTIPLY(tmp0 + tmp3, FIX(0.765261039));   
    tmp10 = tmp12 + tmp13 + tmp14 - MULTIPLY(tmp0, FIX(0.516244403)) 
	    + MULTIPLY(tmp5, FIX(0.164081699));        
    tmp11 = MULTIPLY(tmp2 + tmp3, - FIX(0.164081699)); 
    tmp12 += tmp11 - tmp15 - MULTIPLY(tmp2, FIX(2.079550144)) 
	    + MULTIPLY(tmp5, FIX(0.765261039));        
    tmp13 += tmp11 - tmp14 + MULTIPLY(tmp3, FIX(0.645144899)) 
	    - MULTIPLY(tmp5, FIX(0.997307603));        
    tmp11 = tmp15 + MULTIPLY(tmp0 - tmp3, FIX(1.161389302)) 
	    - MULTIPLY(tmp2 + tmp5, FIX(0.481063200)); 

    dataptr[DCTSIZE*1] = (DCTELEM) DESCALE(tmp10, CONST_BITS+PASS1_BITS);
    dataptr[DCTSIZE*3] = (DCTELEM) DESCALE(tmp11, CONST_BITS+PASS1_BITS);
    dataptr[DCTSIZE*5] = (DCTELEM) DESCALE(tmp12, CONST_BITS+PASS1_BITS);
    dataptr[DCTSIZE*7] = (DCTELEM) DESCALE(tmp13, CONST_BITS+PASS1_BITS);

    dataptr++;			
    wsptr++;			
  }
}

GLOBAL(void)
jpeg_fdct_5x10 (DCTELEM * data, JSAMPARRAY sample_data, JDIMENSION start_col)
{
  INT32 tmp0, tmp1, tmp2, tmp3, tmp4;
  INT32 tmp10, tmp11, tmp12, tmp13, tmp14;
  DCTELEM workspace[8*2];
  DCTELEM *dataptr;
  DCTELEM *wsptr;
  JSAMPROW elemptr;
  int ctr;
  SHIFT_TEMPS

  
  MEMZERO(data, SIZEOF(DCTELEM) * DCTSIZE2);

  
  
  
  

  dataptr = data;
  ctr = 0;
  for (;;) {
    elemptr = sample_data[ctr] + start_col;

    

    tmp0 = GETJSAMPLE(elemptr[0]) + GETJSAMPLE(elemptr[4]);
    tmp1 = GETJSAMPLE(elemptr[1]) + GETJSAMPLE(elemptr[3]);
    tmp2 = GETJSAMPLE(elemptr[2]);

    tmp10 = tmp0 + tmp1;
    tmp11 = tmp0 - tmp1;

    tmp0 = GETJSAMPLE(elemptr[0]) - GETJSAMPLE(elemptr[4]);
    tmp1 = GETJSAMPLE(elemptr[1]) - GETJSAMPLE(elemptr[3]);

    
    dataptr[0] = (DCTELEM)
      ((tmp10 + tmp2 - 5 * CENTERJSAMPLE) << PASS1_BITS);
    tmp11 = MULTIPLY(tmp11, FIX(0.790569415));          
    tmp10 -= tmp2 << 2;
    tmp10 = MULTIPLY(tmp10, FIX(0.353553391));          
    dataptr[2] = (DCTELEM) DESCALE(tmp11 + tmp10, CONST_BITS-PASS1_BITS);
    dataptr[4] = (DCTELEM) DESCALE(tmp11 - tmp10, CONST_BITS-PASS1_BITS);

    

    tmp10 = MULTIPLY(tmp0 + tmp1, FIX(0.831253876));    

    dataptr[1] = (DCTELEM)
      DESCALE(tmp10 + MULTIPLY(tmp0, FIX(0.513743148)), 
	      CONST_BITS-PASS1_BITS);
    dataptr[3] = (DCTELEM)
      DESCALE(tmp10 - MULTIPLY(tmp1, FIX(2.176250899)), 
	      CONST_BITS-PASS1_BITS);

    ctr++;

    if (ctr != DCTSIZE) {
      if (ctr == 10)
	break;			
      dataptr += DCTSIZE;	
    } else
      dataptr = workspace;	
  }

  

  dataptr = data;
  wsptr = workspace;
  for (ctr = 0; ctr < 5; ctr++) {
    

    tmp0 = dataptr[DCTSIZE*0] + wsptr[DCTSIZE*1];
    tmp1 = dataptr[DCTSIZE*1] + wsptr[DCTSIZE*0];
    tmp12 = dataptr[DCTSIZE*2] + dataptr[DCTSIZE*7];
    tmp3 = dataptr[DCTSIZE*3] + dataptr[DCTSIZE*6];
    tmp4 = dataptr[DCTSIZE*4] + dataptr[DCTSIZE*5];

    tmp10 = tmp0 + tmp4;
    tmp13 = tmp0 - tmp4;
    tmp11 = tmp1 + tmp3;
    tmp14 = tmp1 - tmp3;

    tmp0 = dataptr[DCTSIZE*0] - wsptr[DCTSIZE*1];
    tmp1 = dataptr[DCTSIZE*1] - wsptr[DCTSIZE*0];
    tmp2 = dataptr[DCTSIZE*2] - dataptr[DCTSIZE*7];
    tmp3 = dataptr[DCTSIZE*3] - dataptr[DCTSIZE*6];
    tmp4 = dataptr[DCTSIZE*4] - dataptr[DCTSIZE*5];

    dataptr[DCTSIZE*0] = (DCTELEM)
      DESCALE(MULTIPLY(tmp10 + tmp11 + tmp12, FIX(1.28)), 
	      CONST_BITS+PASS1_BITS);
    tmp12 += tmp12;
    dataptr[DCTSIZE*4] = (DCTELEM)
      DESCALE(MULTIPLY(tmp10 - tmp12, FIX(1.464477191)) - 
	      MULTIPLY(tmp11 - tmp12, FIX(0.559380511)),  
	      CONST_BITS+PASS1_BITS);
    tmp10 = MULTIPLY(tmp13 + tmp14, FIX(1.064004961));    
    dataptr[DCTSIZE*2] = (DCTELEM)
      DESCALE(tmp10 + MULTIPLY(tmp13, FIX(0.657591230)),  
	      CONST_BITS+PASS1_BITS);
    dataptr[DCTSIZE*6] = (DCTELEM)
      DESCALE(tmp10 - MULTIPLY(tmp14, FIX(2.785601151)),  
	      CONST_BITS+PASS1_BITS);

    

    tmp10 = tmp0 + tmp4;
    tmp11 = tmp1 - tmp3;
    dataptr[DCTSIZE*5] = (DCTELEM)
      DESCALE(MULTIPLY(tmp10 - tmp11 - tmp2, FIX(1.28)),  
	      CONST_BITS+PASS1_BITS);
    tmp2 = MULTIPLY(tmp2, FIX(1.28));                     
    dataptr[DCTSIZE*1] = (DCTELEM)
      DESCALE(MULTIPLY(tmp0, FIX(1.787906876)) +          
	      MULTIPLY(tmp1, FIX(1.612894094)) + tmp2 +   
	      MULTIPLY(tmp3, FIX(0.821810588)) +          
	      MULTIPLY(tmp4, FIX(0.283176630)),           
	      CONST_BITS+PASS1_BITS);
    tmp12 = MULTIPLY(tmp0 - tmp4, FIX(1.217352341)) -     
	    MULTIPLY(tmp1 + tmp3, FIX(0.752365123));      
    tmp13 = MULTIPLY(tmp10 + tmp11, FIX(0.395541753)) +   
	    MULTIPLY(tmp11, FIX(0.64)) - tmp2;            
    dataptr[DCTSIZE*3] = (DCTELEM) DESCALE(tmp12 + tmp13, CONST_BITS+PASS1_BITS);
    dataptr[DCTSIZE*7] = (DCTELEM) DESCALE(tmp12 - tmp13, CONST_BITS+PASS1_BITS);

    dataptr++;			
    wsptr++;			
  }
}

GLOBAL(void)
jpeg_fdct_4x8 (DCTELEM * data, JSAMPARRAY sample_data, JDIMENSION start_col)
{
  INT32 tmp0, tmp1, tmp2, tmp3;
  INT32 tmp10, tmp11, tmp12, tmp13;
  INT32 z1;
  DCTELEM *dataptr;
  JSAMPROW elemptr;
  int ctr;
  SHIFT_TEMPS

  
  MEMZERO(data, SIZEOF(DCTELEM) * DCTSIZE2);

  
  
  
  
  

  dataptr = data;
  for (ctr = 0; ctr < DCTSIZE; ctr++) {
    elemptr = sample_data[ctr] + start_col;

    

    tmp0 = GETJSAMPLE(elemptr[0]) + GETJSAMPLE(elemptr[3]);
    tmp1 = GETJSAMPLE(elemptr[1]) + GETJSAMPLE(elemptr[2]);

    tmp10 = GETJSAMPLE(elemptr[0]) - GETJSAMPLE(elemptr[3]);
    tmp11 = GETJSAMPLE(elemptr[1]) - GETJSAMPLE(elemptr[2]);

    
    dataptr[0] = (DCTELEM)
      ((tmp0 + tmp1 - 4 * CENTERJSAMPLE) << (PASS1_BITS+1));
    dataptr[2] = (DCTELEM) ((tmp0 - tmp1) << (PASS1_BITS+1));

    

    tmp0 = MULTIPLY(tmp10 + tmp11, FIX_0_541196100);       
    
    tmp0 += ONE << (CONST_BITS-PASS1_BITS-2);

    dataptr[1] = (DCTELEM)
      RIGHT_SHIFT(tmp0 + MULTIPLY(tmp10, FIX_0_765366865), 
		  CONST_BITS-PASS1_BITS-1);
    dataptr[3] = (DCTELEM)
      RIGHT_SHIFT(tmp0 - MULTIPLY(tmp11, FIX_1_847759065), 
		  CONST_BITS-PASS1_BITS-1);

    dataptr += DCTSIZE;		
  }

  

  dataptr = data;
  for (ctr = 0; ctr < 4; ctr++) {
    

    tmp0 = dataptr[DCTSIZE*0] + dataptr[DCTSIZE*7];
    tmp1 = dataptr[DCTSIZE*1] + dataptr[DCTSIZE*6];
    tmp2 = dataptr[DCTSIZE*2] + dataptr[DCTSIZE*5];
    tmp3 = dataptr[DCTSIZE*3] + dataptr[DCTSIZE*4];

    
    tmp10 = tmp0 + tmp3 + (ONE << (PASS1_BITS-1));
    tmp12 = tmp0 - tmp3;
    tmp11 = tmp1 + tmp2;
    tmp13 = tmp1 - tmp2;

    tmp0 = dataptr[DCTSIZE*0] - dataptr[DCTSIZE*7];
    tmp1 = dataptr[DCTSIZE*1] - dataptr[DCTSIZE*6];
    tmp2 = dataptr[DCTSIZE*2] - dataptr[DCTSIZE*5];
    tmp3 = dataptr[DCTSIZE*3] - dataptr[DCTSIZE*4];

    dataptr[DCTSIZE*0] = (DCTELEM) RIGHT_SHIFT(tmp10 + tmp11, PASS1_BITS);
    dataptr[DCTSIZE*4] = (DCTELEM) RIGHT_SHIFT(tmp10 - tmp11, PASS1_BITS);

    z1 = MULTIPLY(tmp12 + tmp13, FIX_0_541196100);
    
    z1 += ONE << (CONST_BITS+PASS1_BITS-1);
    dataptr[DCTSIZE*2] = (DCTELEM)
      RIGHT_SHIFT(z1 + MULTIPLY(tmp12, FIX_0_765366865), CONST_BITS+PASS1_BITS);
    dataptr[DCTSIZE*6] = (DCTELEM)
      RIGHT_SHIFT(z1 - MULTIPLY(tmp13, FIX_1_847759065), CONST_BITS+PASS1_BITS);

    

    tmp10 = tmp0 + tmp3;
    tmp11 = tmp1 + tmp2;
    tmp12 = tmp0 + tmp2;
    tmp13 = tmp1 + tmp3;
    z1 = MULTIPLY(tmp12 + tmp13, FIX_1_175875602); 
    
    z1 += ONE << (CONST_BITS+PASS1_BITS-1);

    tmp0  = MULTIPLY(tmp0,    FIX_1_501321110);    
    tmp1  = MULTIPLY(tmp1,    FIX_3_072711026);    
    tmp2  = MULTIPLY(tmp2,    FIX_2_053119869);    
    tmp3  = MULTIPLY(tmp3,    FIX_0_298631336);    
    tmp10 = MULTIPLY(tmp10, - FIX_0_899976223);    
    tmp11 = MULTIPLY(tmp11, - FIX_2_562915447);    
    tmp12 = MULTIPLY(tmp12, - FIX_0_390180644);    
    tmp13 = MULTIPLY(tmp13, - FIX_1_961570560);    

    tmp12 += z1;
    tmp13 += z1;

    dataptr[DCTSIZE*1] = (DCTELEM)
      RIGHT_SHIFT(tmp0 + tmp10 + tmp12, CONST_BITS+PASS1_BITS);
    dataptr[DCTSIZE*3] = (DCTELEM)
      RIGHT_SHIFT(tmp1 + tmp11 + tmp13, CONST_BITS+PASS1_BITS);
    dataptr[DCTSIZE*5] = (DCTELEM)
      RIGHT_SHIFT(tmp2 + tmp11 + tmp12, CONST_BITS+PASS1_BITS);
    dataptr[DCTSIZE*7] = (DCTELEM)
      RIGHT_SHIFT(tmp3 + tmp10 + tmp13, CONST_BITS+PASS1_BITS);

    dataptr++;			
  }
}

GLOBAL(void)
jpeg_fdct_3x6 (DCTELEM * data, JSAMPARRAY sample_data, JDIMENSION start_col)
{
  INT32 tmp0, tmp1, tmp2;
  INT32 tmp10, tmp11, tmp12;
  DCTELEM *dataptr;
  JSAMPROW elemptr;
  int ctr;
  SHIFT_TEMPS

  
  MEMZERO(data, SIZEOF(DCTELEM) * DCTSIZE2);

  
  
  
  
  
  

  dataptr = data;
  for (ctr = 0; ctr < 6; ctr++) {
    elemptr = sample_data[ctr] + start_col;

    

    tmp0 = GETJSAMPLE(elemptr[0]) + GETJSAMPLE(elemptr[2]);
    tmp1 = GETJSAMPLE(elemptr[1]);

    tmp2 = GETJSAMPLE(elemptr[0]) - GETJSAMPLE(elemptr[2]);

    
    dataptr[0] = (DCTELEM)
      ((tmp0 + tmp1 - 3 * CENTERJSAMPLE) << (PASS1_BITS+1));
    dataptr[2] = (DCTELEM)
      DESCALE(MULTIPLY(tmp0 - tmp1 - tmp1, FIX(0.707106781)), 
	      CONST_BITS-PASS1_BITS-1);

    

    dataptr[1] = (DCTELEM)
      DESCALE(MULTIPLY(tmp2, FIX(1.224744871)),               
	      CONST_BITS-PASS1_BITS-1);

    dataptr += DCTSIZE;		
  }

  

  dataptr = data;
  for (ctr = 0; ctr < 3; ctr++) {
    

    tmp0 = dataptr[DCTSIZE*0] + dataptr[DCTSIZE*5];
    tmp11 = dataptr[DCTSIZE*1] + dataptr[DCTSIZE*4];
    tmp2 = dataptr[DCTSIZE*2] + dataptr[DCTSIZE*3];

    tmp10 = tmp0 + tmp2;
    tmp12 = tmp0 - tmp2;

    tmp0 = dataptr[DCTSIZE*0] - dataptr[DCTSIZE*5];
    tmp1 = dataptr[DCTSIZE*1] - dataptr[DCTSIZE*4];
    tmp2 = dataptr[DCTSIZE*2] - dataptr[DCTSIZE*3];

    dataptr[DCTSIZE*0] = (DCTELEM)
      DESCALE(MULTIPLY(tmp10 + tmp11, FIX(1.777777778)),         
	      CONST_BITS+PASS1_BITS);
    dataptr[DCTSIZE*2] = (DCTELEM)
      DESCALE(MULTIPLY(tmp12, FIX(2.177324216)),                 
	      CONST_BITS+PASS1_BITS);
    dataptr[DCTSIZE*4] = (DCTELEM)
      DESCALE(MULTIPLY(tmp10 - tmp11 - tmp11, FIX(1.257078722)), 
	      CONST_BITS+PASS1_BITS);

    

    tmp10 = MULTIPLY(tmp0 + tmp2, FIX(0.650711829));             

    dataptr[DCTSIZE*1] = (DCTELEM)
      DESCALE(tmp10 + MULTIPLY(tmp0 + tmp1, FIX(1.777777778)),   
	      CONST_BITS+PASS1_BITS);
    dataptr[DCTSIZE*3] = (DCTELEM)
      DESCALE(MULTIPLY(tmp0 - tmp1 - tmp2, FIX(1.777777778)),    
	      CONST_BITS+PASS1_BITS);
    dataptr[DCTSIZE*5] = (DCTELEM)
      DESCALE(tmp10 + MULTIPLY(tmp2 - tmp1, FIX(1.777777778)),   
	      CONST_BITS+PASS1_BITS);

    dataptr++;			
  }
}

GLOBAL(void)
jpeg_fdct_2x4 (DCTELEM * data, JSAMPARRAY sample_data, JDIMENSION start_col)
{
  INT32 tmp0, tmp1;
  INT32 tmp10, tmp11;
  DCTELEM *dataptr;
  JSAMPROW elemptr;
  int ctr;
  SHIFT_TEMPS

  
  MEMZERO(data, SIZEOF(DCTELEM) * DCTSIZE2);

  
  
  

  dataptr = data;
  for (ctr = 0; ctr < 4; ctr++) {
    elemptr = sample_data[ctr] + start_col;

    

    tmp0 = GETJSAMPLE(elemptr[0]);
    tmp1 = GETJSAMPLE(elemptr[1]);

    
    dataptr[0] = (DCTELEM) ((tmp0 + tmp1 - 2 * CENTERJSAMPLE) << 3);

    

    dataptr[1] = (DCTELEM) ((tmp0 - tmp1) << 3);

    dataptr += DCTSIZE;		
  }

  

  dataptr = data;
  for (ctr = 0; ctr < 2; ctr++) {
    

    tmp0 = dataptr[DCTSIZE*0] + dataptr[DCTSIZE*3];
    tmp1 = dataptr[DCTSIZE*1] + dataptr[DCTSIZE*2];

    tmp10 = dataptr[DCTSIZE*0] - dataptr[DCTSIZE*3];
    tmp11 = dataptr[DCTSIZE*1] - dataptr[DCTSIZE*2];

    dataptr[DCTSIZE*0] = (DCTELEM) (tmp0 + tmp1);
    dataptr[DCTSIZE*2] = (DCTELEM) (tmp0 - tmp1);

    

    tmp0 = MULTIPLY(tmp10 + tmp11, FIX_0_541196100);       
    
    tmp0 += ONE << (CONST_BITS-1);

    dataptr[DCTSIZE*1] = (DCTELEM)
      RIGHT_SHIFT(tmp0 + MULTIPLY(tmp10, FIX_0_765366865), 
		  CONST_BITS);
    dataptr[DCTSIZE*3] = (DCTELEM)
      RIGHT_SHIFT(tmp0 - MULTIPLY(tmp11, FIX_1_847759065), 
		  CONST_BITS);

    dataptr++;			
  }
}

GLOBAL(void)
jpeg_fdct_1x2 (DCTELEM * data, JSAMPARRAY sample_data, JDIMENSION start_col)
{
  INT32 tmp0, tmp1;

  
  MEMZERO(data, SIZEOF(DCTELEM) * DCTSIZE2);

  tmp0 = GETJSAMPLE(sample_data[0][start_col]);
  tmp1 = GETJSAMPLE(sample_data[1][start_col]);

  

  
  
  data[DCTSIZE*0] = (DCTELEM) ((tmp0 + tmp1 - 2 * CENTERJSAMPLE) << 5);

  
  data[DCTSIZE*1] = (DCTELEM) ((tmp0 - tmp1) << 5);
}

#endif 
#endif 
