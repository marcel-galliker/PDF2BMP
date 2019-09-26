

#define JPEG_INTERNALS
#include "jinclude.h"
#include "jpeglib.h"
#include "jdct.h"		

typedef struct {
  struct jpeg_forward_dct pub;	

  
  forward_DCT_method_ptr do_dct[MAX_COMPONENTS];

  
  DCTELEM * divisors[NUM_QUANT_TBLS];

#ifdef DCT_FLOAT_SUPPORTED
  
  float_DCT_method_ptr do_float_dct[MAX_COMPONENTS];
  FAST_FLOAT * float_divisors[NUM_QUANT_TBLS];
#endif
} my_fdct_controller;

typedef my_fdct_controller * my_fdct_ptr;

#ifdef DCT_ISLOW_SUPPORTED
#define PROVIDE_ISLOW_TABLES
#else
#ifdef DCT_SCALING_SUPPORTED
#define PROVIDE_ISLOW_TABLES
#endif
#endif

METHODDEF(void)
forward_DCT (j_compress_ptr cinfo, jpeg_component_info * compptr,
	     JSAMPARRAY sample_data, JBLOCKROW coef_blocks,
	     JDIMENSION start_row, JDIMENSION start_col,
	     JDIMENSION num_blocks)

{
  
  my_fdct_ptr fdct = (my_fdct_ptr) cinfo->fdct;
  forward_DCT_method_ptr do_dct = fdct->do_dct[compptr->component_index];
  DCTELEM * divisors = fdct->divisors[compptr->quant_tbl_no];
  DCTELEM workspace[DCTSIZE2];	
  JDIMENSION bi;

  sample_data += start_row;	

  for (bi = 0; bi < num_blocks; bi++, start_col += compptr->DCT_h_scaled_size) {
    
    (*do_dct) (workspace, sample_data, start_col);

    
    { register DCTELEM temp, qval;
      register int i;
      register JCOEFPTR output_ptr = coef_blocks[bi];

      for (i = 0; i < DCTSIZE2; i++) {
	qval = divisors[i];
	temp = workspace[i];
	
#ifdef FAST_DIVIDE
#define DIVIDE_BY(a,b)	a /= b
#else
#define DIVIDE_BY(a,b)	if (a >= b) a /= b; else a = 0
#endif
	if (temp < 0) {
	  temp = -temp;
	  temp += qval>>1;	
	  DIVIDE_BY(temp, qval);
	  temp = -temp;
	} else {
	  temp += qval>>1;	
	  DIVIDE_BY(temp, qval);
	}
	output_ptr[i] = (JCOEF) temp;
      }
    }
  }
}

#ifdef DCT_FLOAT_SUPPORTED

METHODDEF(void)
forward_DCT_float (j_compress_ptr cinfo, jpeg_component_info * compptr,
		   JSAMPARRAY sample_data, JBLOCKROW coef_blocks,
		   JDIMENSION start_row, JDIMENSION start_col,
		   JDIMENSION num_blocks)

{
  
  my_fdct_ptr fdct = (my_fdct_ptr) cinfo->fdct;
  float_DCT_method_ptr do_dct = fdct->do_float_dct[compptr->component_index];
  FAST_FLOAT * divisors = fdct->float_divisors[compptr->quant_tbl_no];
  FAST_FLOAT workspace[DCTSIZE2]; 
  JDIMENSION bi;

  sample_data += start_row;	

  for (bi = 0; bi < num_blocks; bi++, start_col += compptr->DCT_h_scaled_size) {
    
    (*do_dct) (workspace, sample_data, start_col);

    
    { register FAST_FLOAT temp;
      register int i;
      register JCOEFPTR output_ptr = coef_blocks[bi];

      for (i = 0; i < DCTSIZE2; i++) {
	
	temp = workspace[i] * divisors[i];
	
	output_ptr[i] = (JCOEF) ((int) (temp + (FAST_FLOAT) 16384.5) - 16384);
      }
    }
  }
}

#endif 

METHODDEF(void)
start_pass_fdctmgr (j_compress_ptr cinfo)
{
  my_fdct_ptr fdct = (my_fdct_ptr) cinfo->fdct;
  int ci, qtblno, i;
  jpeg_component_info *compptr;
  int method = 0;
  JQUANT_TBL * qtbl;
  DCTELEM * dtbl;

  for (ci = 0, compptr = cinfo->comp_info; ci < cinfo->num_components;
       ci++, compptr++) {
    
    switch ((compptr->DCT_h_scaled_size << 8) + compptr->DCT_v_scaled_size) {
#ifdef DCT_SCALING_SUPPORTED
    case ((1 << 8) + 1):
      fdct->do_dct[ci] = jpeg_fdct_1x1;
      method = JDCT_ISLOW;	
      break;
    case ((2 << 8) + 2):
      fdct->do_dct[ci] = jpeg_fdct_2x2;
      method = JDCT_ISLOW;	
      break;
    case ((3 << 8) + 3):
      fdct->do_dct[ci] = jpeg_fdct_3x3;
      method = JDCT_ISLOW;	
      break;
    case ((4 << 8) + 4):
      fdct->do_dct[ci] = jpeg_fdct_4x4;
      method = JDCT_ISLOW;	
      break;
    case ((5 << 8) + 5):
      fdct->do_dct[ci] = jpeg_fdct_5x5;
      method = JDCT_ISLOW;	
      break;
    case ((6 << 8) + 6):
      fdct->do_dct[ci] = jpeg_fdct_6x6;
      method = JDCT_ISLOW;	
      break;
    case ((7 << 8) + 7):
      fdct->do_dct[ci] = jpeg_fdct_7x7;
      method = JDCT_ISLOW;	
      break;
    case ((9 << 8) + 9):
      fdct->do_dct[ci] = jpeg_fdct_9x9;
      method = JDCT_ISLOW;	
      break;
    case ((10 << 8) + 10):
      fdct->do_dct[ci] = jpeg_fdct_10x10;
      method = JDCT_ISLOW;	
      break;
    case ((11 << 8) + 11):
      fdct->do_dct[ci] = jpeg_fdct_11x11;
      method = JDCT_ISLOW;	
      break;
    case ((12 << 8) + 12):
      fdct->do_dct[ci] = jpeg_fdct_12x12;
      method = JDCT_ISLOW;	
      break;
    case ((13 << 8) + 13):
      fdct->do_dct[ci] = jpeg_fdct_13x13;
      method = JDCT_ISLOW;	
      break;
    case ((14 << 8) + 14):
      fdct->do_dct[ci] = jpeg_fdct_14x14;
      method = JDCT_ISLOW;	
      break;
    case ((15 << 8) + 15):
      fdct->do_dct[ci] = jpeg_fdct_15x15;
      method = JDCT_ISLOW;	
      break;
    case ((16 << 8) + 16):
      fdct->do_dct[ci] = jpeg_fdct_16x16;
      method = JDCT_ISLOW;	
      break;
    case ((16 << 8) + 8):
      fdct->do_dct[ci] = jpeg_fdct_16x8;
      method = JDCT_ISLOW;	
      break;
    case ((14 << 8) + 7):
      fdct->do_dct[ci] = jpeg_fdct_14x7;
      method = JDCT_ISLOW;	
      break;
    case ((12 << 8) + 6):
      fdct->do_dct[ci] = jpeg_fdct_12x6;
      method = JDCT_ISLOW;	
      break;
    case ((10 << 8) + 5):
      fdct->do_dct[ci] = jpeg_fdct_10x5;
      method = JDCT_ISLOW;	
      break;
    case ((8 << 8) + 4):
      fdct->do_dct[ci] = jpeg_fdct_8x4;
      method = JDCT_ISLOW;	
      break;
    case ((6 << 8) + 3):
      fdct->do_dct[ci] = jpeg_fdct_6x3;
      method = JDCT_ISLOW;	
      break;
    case ((4 << 8) + 2):
      fdct->do_dct[ci] = jpeg_fdct_4x2;
      method = JDCT_ISLOW;	
      break;
    case ((2 << 8) + 1):
      fdct->do_dct[ci] = jpeg_fdct_2x1;
      method = JDCT_ISLOW;	
      break;
    case ((8 << 8) + 16):
      fdct->do_dct[ci] = jpeg_fdct_8x16;
      method = JDCT_ISLOW;	
      break;
    case ((7 << 8) + 14):
      fdct->do_dct[ci] = jpeg_fdct_7x14;
      method = JDCT_ISLOW;	
      break;
    case ((6 << 8) + 12):
      fdct->do_dct[ci] = jpeg_fdct_6x12;
      method = JDCT_ISLOW;	
      break;
    case ((5 << 8) + 10):
      fdct->do_dct[ci] = jpeg_fdct_5x10;
      method = JDCT_ISLOW;	
      break;
    case ((4 << 8) + 8):
      fdct->do_dct[ci] = jpeg_fdct_4x8;
      method = JDCT_ISLOW;	
      break;
    case ((3 << 8) + 6):
      fdct->do_dct[ci] = jpeg_fdct_3x6;
      method = JDCT_ISLOW;	
      break;
    case ((2 << 8) + 4):
      fdct->do_dct[ci] = jpeg_fdct_2x4;
      method = JDCT_ISLOW;	
      break;
    case ((1 << 8) + 2):
      fdct->do_dct[ci] = jpeg_fdct_1x2;
      method = JDCT_ISLOW;	
      break;
#endif
    case ((DCTSIZE << 8) + DCTSIZE):
      switch (cinfo->dct_method) {
#ifdef DCT_ISLOW_SUPPORTED
      case JDCT_ISLOW:
	fdct->do_dct[ci] = jpeg_fdct_islow;
	method = JDCT_ISLOW;
	break;
#endif
#ifdef DCT_IFAST_SUPPORTED
      case JDCT_IFAST:
	fdct->do_dct[ci] = jpeg_fdct_ifast;
	method = JDCT_IFAST;
	break;
#endif
#ifdef DCT_FLOAT_SUPPORTED
      case JDCT_FLOAT:
	fdct->do_float_dct[ci] = jpeg_fdct_float;
	method = JDCT_FLOAT;
	break;
#endif
      default:
	ERREXIT(cinfo, JERR_NOT_COMPILED);
	break;
      }
      break;
    default:
      ERREXIT2(cinfo, JERR_BAD_DCTSIZE,
	       compptr->DCT_h_scaled_size, compptr->DCT_v_scaled_size);
      break;
    }
    qtblno = compptr->quant_tbl_no;
    
    if (qtblno < 0 || qtblno >= NUM_QUANT_TBLS ||
	cinfo->quant_tbl_ptrs[qtblno] == NULL)
      ERREXIT1(cinfo, JERR_NO_QUANT_TABLE, qtblno);
    qtbl = cinfo->quant_tbl_ptrs[qtblno];
    
    
    switch (method) {
#ifdef PROVIDE_ISLOW_TABLES
    case JDCT_ISLOW:
      
      if (fdct->divisors[qtblno] == NULL) {
	fdct->divisors[qtblno] = (DCTELEM *)
	  (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
				      DCTSIZE2 * SIZEOF(DCTELEM));
      }
      dtbl = fdct->divisors[qtblno];
      for (i = 0; i < DCTSIZE2; i++) {
	dtbl[i] = ((DCTELEM) qtbl->quantval[i]) << 3;
      }
      fdct->pub.forward_DCT[ci] = forward_DCT;
      break;
#endif
#ifdef DCT_IFAST_SUPPORTED
    case JDCT_IFAST:
      {
	
#define CONST_BITS 14
	static const INT16 aanscales[DCTSIZE2] = {
	  
	  16384, 22725, 21407, 19266, 16384, 12873,  8867,  4520,
	  22725, 31521, 29692, 26722, 22725, 17855, 12299,  6270,
	  21407, 29692, 27969, 25172, 21407, 16819, 11585,  5906,
	  19266, 26722, 25172, 22654, 19266, 15137, 10426,  5315,
	  16384, 22725, 21407, 19266, 16384, 12873,  8867,  4520,
	  12873, 17855, 16819, 15137, 12873, 10114,  6967,  3552,
	   8867, 12299, 11585, 10426,  8867,  6967,  4799,  2446,
	   4520,  6270,  5906,  5315,  4520,  3552,  2446,  1247
	};
	SHIFT_TEMPS

	if (fdct->divisors[qtblno] == NULL) {
	  fdct->divisors[qtblno] = (DCTELEM *)
	    (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
					DCTSIZE2 * SIZEOF(DCTELEM));
	}
	dtbl = fdct->divisors[qtblno];
	for (i = 0; i < DCTSIZE2; i++) {
	  dtbl[i] = (DCTELEM)
	    DESCALE(MULTIPLY16V16((INT32) qtbl->quantval[i],
				  (INT32) aanscales[i]),
		    CONST_BITS-3);
	}
      }
      fdct->pub.forward_DCT[ci] = forward_DCT;
      break;
#endif
#ifdef DCT_FLOAT_SUPPORTED
    case JDCT_FLOAT:
      {
	
	FAST_FLOAT * fdtbl;
	int row, col;
	static const double aanscalefactor[DCTSIZE] = {
	  1.0, 1.387039845, 1.306562965, 1.175875602,
	  1.0, 0.785694958, 0.541196100, 0.275899379
	};

	if (fdct->float_divisors[qtblno] == NULL) {
	  fdct->float_divisors[qtblno] = (FAST_FLOAT *)
	    (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
					DCTSIZE2 * SIZEOF(FAST_FLOAT));
	}
	fdtbl = fdct->float_divisors[qtblno];
	i = 0;
	for (row = 0; row < DCTSIZE; row++) {
	  for (col = 0; col < DCTSIZE; col++) {
	    fdtbl[i] = (FAST_FLOAT)
	      (1.0 / (((double) qtbl->quantval[i] *
		       aanscalefactor[row] * aanscalefactor[col] * 8.0)));
	    i++;
	  }
	}
      }
      fdct->pub.forward_DCT[ci] = forward_DCT_float;
      break;
#endif
    default:
      ERREXIT(cinfo, JERR_NOT_COMPILED);
      break;
    }
  }
}

GLOBAL(void)
jinit_forward_dct (j_compress_ptr cinfo)
{
  my_fdct_ptr fdct;
  int i;

  fdct = (my_fdct_ptr)
    (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
				SIZEOF(my_fdct_controller));
  cinfo->fdct = (struct jpeg_forward_dct *) fdct;
  fdct->pub.start_pass = start_pass_fdctmgr;

  
  for (i = 0; i < NUM_QUANT_TBLS; i++) {
    fdct->divisors[i] = NULL;
#ifdef DCT_FLOAT_SUPPORTED
    fdct->float_divisors[i] = NULL;
#endif
  }
}
