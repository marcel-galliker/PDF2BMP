

#define JPEG_INTERNALS
#include "jinclude.h"
#include "jpeglib.h"
#include "jdct.h"		

typedef struct {
  struct jpeg_inverse_dct pub;	

  
  int cur_method[MAX_COMPONENTS];
} my_idct_controller;

typedef my_idct_controller * my_idct_ptr;

typedef union {
  ISLOW_MULT_TYPE islow_array[DCTSIZE2];
#ifdef DCT_IFAST_SUPPORTED
  IFAST_MULT_TYPE ifast_array[DCTSIZE2];
#endif
#ifdef DCT_FLOAT_SUPPORTED
  FLOAT_MULT_TYPE float_array[DCTSIZE2];
#endif
} multiplier_table;

#ifdef DCT_ISLOW_SUPPORTED
#define PROVIDE_ISLOW_TABLES
#else
#ifdef IDCT_SCALING_SUPPORTED
#define PROVIDE_ISLOW_TABLES
#endif
#endif

METHODDEF(void)
start_pass (j_decompress_ptr cinfo)
{
  my_idct_ptr idct = (my_idct_ptr) cinfo->idct;
  int ci, i;
  jpeg_component_info *compptr;
  int method = 0;
  inverse_DCT_method_ptr method_ptr = NULL;
  JQUANT_TBL * qtbl;

  for (ci = 0, compptr = cinfo->comp_info; ci < cinfo->num_components;
       ci++, compptr++) {
    
    switch ((compptr->DCT_h_scaled_size << 8) + compptr->DCT_v_scaled_size) {
#ifdef IDCT_SCALING_SUPPORTED
    case ((1 << 8) + 1):
      method_ptr = jpeg_idct_1x1;
      method = JDCT_ISLOW;	
      break;
    case ((2 << 8) + 2):
      method_ptr = jpeg_idct_2x2;
      method = JDCT_ISLOW;	
      break;
    case ((3 << 8) + 3):
      method_ptr = jpeg_idct_3x3;
      method = JDCT_ISLOW;	
      break;
    case ((4 << 8) + 4):
      method_ptr = jpeg_idct_4x4;
      method = JDCT_ISLOW;	
      break;
    case ((5 << 8) + 5):
      method_ptr = jpeg_idct_5x5;
      method = JDCT_ISLOW;	
      break;
    case ((6 << 8) + 6):
      method_ptr = jpeg_idct_6x6;
      method = JDCT_ISLOW;	
      break;
    case ((7 << 8) + 7):
      method_ptr = jpeg_idct_7x7;
      method = JDCT_ISLOW;	
      break;
    case ((9 << 8) + 9):
      method_ptr = jpeg_idct_9x9;
      method = JDCT_ISLOW;	
      break;
    case ((10 << 8) + 10):
      method_ptr = jpeg_idct_10x10;
      method = JDCT_ISLOW;	
      break;
    case ((11 << 8) + 11):
      method_ptr = jpeg_idct_11x11;
      method = JDCT_ISLOW;	
      break;
    case ((12 << 8) + 12):
      method_ptr = jpeg_idct_12x12;
      method = JDCT_ISLOW;	
      break;
    case ((13 << 8) + 13):
      method_ptr = jpeg_idct_13x13;
      method = JDCT_ISLOW;	
      break;
    case ((14 << 8) + 14):
      method_ptr = jpeg_idct_14x14;
      method = JDCT_ISLOW;	
      break;
    case ((15 << 8) + 15):
      method_ptr = jpeg_idct_15x15;
      method = JDCT_ISLOW;	
      break;
    case ((16 << 8) + 16):
      method_ptr = jpeg_idct_16x16;
      method = JDCT_ISLOW;	
      break;
    case ((16 << 8) + 8):
      method_ptr = jpeg_idct_16x8;
      method = JDCT_ISLOW;	
      break;
    case ((14 << 8) + 7):
      method_ptr = jpeg_idct_14x7;
      method = JDCT_ISLOW;	
      break;
    case ((12 << 8) + 6):
      method_ptr = jpeg_idct_12x6;
      method = JDCT_ISLOW;	
      break;
    case ((10 << 8) + 5):
      method_ptr = jpeg_idct_10x5;
      method = JDCT_ISLOW;	
      break;
    case ((8 << 8) + 4):
      method_ptr = jpeg_idct_8x4;
      method = JDCT_ISLOW;	
      break;
    case ((6 << 8) + 3):
      method_ptr = jpeg_idct_6x3;
      method = JDCT_ISLOW;	
      break;
    case ((4 << 8) + 2):
      method_ptr = jpeg_idct_4x2;
      method = JDCT_ISLOW;	
      break;
    case ((2 << 8) + 1):
      method_ptr = jpeg_idct_2x1;
      method = JDCT_ISLOW;	
      break;
    case ((8 << 8) + 16):
      method_ptr = jpeg_idct_8x16;
      method = JDCT_ISLOW;	
      break;
    case ((7 << 8) + 14):
      method_ptr = jpeg_idct_7x14;
      method = JDCT_ISLOW;	
      break;
    case ((6 << 8) + 12):
      method_ptr = jpeg_idct_6x12;
      method = JDCT_ISLOW;	
      break;
    case ((5 << 8) + 10):
      method_ptr = jpeg_idct_5x10;
      method = JDCT_ISLOW;	
      break;
    case ((4 << 8) + 8):
      method_ptr = jpeg_idct_4x8;
      method = JDCT_ISLOW;	
      break;
    case ((3 << 8) + 6):
      method_ptr = jpeg_idct_3x6;
      method = JDCT_ISLOW;	
      break;
    case ((2 << 8) + 4):
      method_ptr = jpeg_idct_2x4;
      method = JDCT_ISLOW;	
      break;
    case ((1 << 8) + 2):
      method_ptr = jpeg_idct_1x2;
      method = JDCT_ISLOW;	
      break;
#endif
    case ((DCTSIZE << 8) + DCTSIZE):
      switch (cinfo->dct_method) {
#ifdef DCT_ISLOW_SUPPORTED
      case JDCT_ISLOW:
	method_ptr = jpeg_idct_islow;
	method = JDCT_ISLOW;
	break;
#endif
#ifdef DCT_IFAST_SUPPORTED
      case JDCT_IFAST:
	method_ptr = jpeg_idct_ifast;
	method = JDCT_IFAST;
	break;
#endif
#ifdef DCT_FLOAT_SUPPORTED
      case JDCT_FLOAT:
	method_ptr = jpeg_idct_float;
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
    idct->pub.inverse_DCT[ci] = method_ptr;
    
    if (! compptr->component_needed || idct->cur_method[ci] == method)
      continue;
    qtbl = compptr->quant_table;
    if (qtbl == NULL)		
      continue;
    idct->cur_method[ci] = method;
    switch (method) {
#ifdef PROVIDE_ISLOW_TABLES
    case JDCT_ISLOW:
      {
	
	ISLOW_MULT_TYPE * ismtbl = (ISLOW_MULT_TYPE *) compptr->dct_table;
	for (i = 0; i < DCTSIZE2; i++) {
	  ismtbl[i] = (ISLOW_MULT_TYPE) qtbl->quantval[i];
	}
      }
      break;
#endif
#ifdef DCT_IFAST_SUPPORTED
    case JDCT_IFAST:
      {
	
	IFAST_MULT_TYPE * ifmtbl = (IFAST_MULT_TYPE *) compptr->dct_table;
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

	for (i = 0; i < DCTSIZE2; i++) {
	  ifmtbl[i] = (IFAST_MULT_TYPE)
	    DESCALE(MULTIPLY16V16((INT32) qtbl->quantval[i],
				  (INT32) aanscales[i]),
		    CONST_BITS-IFAST_SCALE_BITS);
	}
      }
      break;
#endif
#ifdef DCT_FLOAT_SUPPORTED
    case JDCT_FLOAT:
      {
	
	FLOAT_MULT_TYPE * fmtbl = (FLOAT_MULT_TYPE *) compptr->dct_table;
	int row, col;
	static const double aanscalefactor[DCTSIZE] = {
	  1.0, 1.387039845, 1.306562965, 1.175875602,
	  1.0, 0.785694958, 0.541196100, 0.275899379
	};

	i = 0;
	for (row = 0; row < DCTSIZE; row++) {
	  for (col = 0; col < DCTSIZE; col++) {
	    fmtbl[i] = (FLOAT_MULT_TYPE)
	      ((double) qtbl->quantval[i] *
	       aanscalefactor[row] * aanscalefactor[col] * 0.125);
	    i++;
	  }
	}
      }
      break;
#endif
    default:
      ERREXIT(cinfo, JERR_NOT_COMPILED);
      break;
    }
  }
}

GLOBAL(void)
jinit_inverse_dct (j_decompress_ptr cinfo)
{
  my_idct_ptr idct;
  int ci;
  jpeg_component_info *compptr;

  idct = (my_idct_ptr)
    (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
				SIZEOF(my_idct_controller));
  cinfo->idct = (struct jpeg_inverse_dct *) idct;
  idct->pub.start_pass = start_pass;

  for (ci = 0, compptr = cinfo->comp_info; ci < cinfo->num_components;
       ci++, compptr++) {
    
    compptr->dct_table =
      (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
				  SIZEOF(multiplier_table));
    MEMZERO(compptr->dct_table, SIZEOF(multiplier_table));
    
    idct->cur_method[ci] = -1;
  }
}
