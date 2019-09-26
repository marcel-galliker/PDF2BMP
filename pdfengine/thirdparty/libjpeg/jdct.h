

#if BITS_IN_JSAMPLE == 8
typedef int DCTELEM;		
#else
typedef INT32 DCTELEM;		
#endif

typedef JMETHOD(void, forward_DCT_method_ptr, (DCTELEM * data,
					       JSAMPARRAY sample_data,
					       JDIMENSION start_col));
typedef JMETHOD(void, float_DCT_method_ptr, (FAST_FLOAT * data,
					     JSAMPARRAY sample_data,
					     JDIMENSION start_col));

typedef MULTIPLIER ISLOW_MULT_TYPE; 
#if BITS_IN_JSAMPLE == 8
typedef MULTIPLIER IFAST_MULT_TYPE; 
#define IFAST_SCALE_BITS  2	
#else
typedef INT32 IFAST_MULT_TYPE;	
#define IFAST_SCALE_BITS  13	
#endif
typedef FAST_FLOAT FLOAT_MULT_TYPE; 

#define IDCT_range_limit(cinfo)  ((cinfo)->sample_range_limit + CENTERJSAMPLE)

#define RANGE_MASK  (MAXJSAMPLE * 4 + 3) 

#ifdef NEED_SHORT_EXTERNAL_NAMES
#define jpeg_fdct_islow		jFDislow
#define jpeg_fdct_ifast		jFDifast
#define jpeg_fdct_float		jFDfloat
#define jpeg_fdct_7x7		jFD7x7
#define jpeg_fdct_6x6		jFD6x6
#define jpeg_fdct_5x5		jFD5x5
#define jpeg_fdct_4x4		jFD4x4
#define jpeg_fdct_3x3		jFD3x3
#define jpeg_fdct_2x2		jFD2x2
#define jpeg_fdct_1x1		jFD1x1
#define jpeg_fdct_9x9		jFD9x9
#define jpeg_fdct_10x10		jFD10x10
#define jpeg_fdct_11x11		jFD11x11
#define jpeg_fdct_12x12		jFD12x12
#define jpeg_fdct_13x13		jFD13x13
#define jpeg_fdct_14x14		jFD14x14
#define jpeg_fdct_15x15		jFD15x15
#define jpeg_fdct_16x16		jFD16x16
#define jpeg_fdct_16x8		jFD16x8
#define jpeg_fdct_14x7		jFD14x7
#define jpeg_fdct_12x6		jFD12x6
#define jpeg_fdct_10x5		jFD10x5
#define jpeg_fdct_8x4		jFD8x4
#define jpeg_fdct_6x3		jFD6x3
#define jpeg_fdct_4x2		jFD4x2
#define jpeg_fdct_2x1		jFD2x1
#define jpeg_fdct_8x16		jFD8x16
#define jpeg_fdct_7x14		jFD7x14
#define jpeg_fdct_6x12		jFD6x12
#define jpeg_fdct_5x10		jFD5x10
#define jpeg_fdct_4x8		jFD4x8
#define jpeg_fdct_3x6		jFD3x6
#define jpeg_fdct_2x4		jFD2x4
#define jpeg_fdct_1x2		jFD1x2
#define jpeg_idct_islow		jRDislow
#define jpeg_idct_ifast		jRDifast
#define jpeg_idct_float		jRDfloat
#define jpeg_idct_7x7		jRD7x7
#define jpeg_idct_6x6		jRD6x6
#define jpeg_idct_5x5		jRD5x5
#define jpeg_idct_4x4		jRD4x4
#define jpeg_idct_3x3		jRD3x3
#define jpeg_idct_2x2		jRD2x2
#define jpeg_idct_1x1		jRD1x1
#define jpeg_idct_9x9		jRD9x9
#define jpeg_idct_10x10		jRD10x10
#define jpeg_idct_11x11		jRD11x11
#define jpeg_idct_12x12		jRD12x12
#define jpeg_idct_13x13		jRD13x13
#define jpeg_idct_14x14		jRD14x14
#define jpeg_idct_15x15		jRD15x15
#define jpeg_idct_16x16		jRD16x16
#define jpeg_idct_16x8		jRD16x8
#define jpeg_idct_14x7		jRD14x7
#define jpeg_idct_12x6		jRD12x6
#define jpeg_idct_10x5		jRD10x5
#define jpeg_idct_8x4		jRD8x4
#define jpeg_idct_6x3		jRD6x3
#define jpeg_idct_4x2		jRD4x2
#define jpeg_idct_2x1		jRD2x1
#define jpeg_idct_8x16		jRD8x16
#define jpeg_idct_7x14		jRD7x14
#define jpeg_idct_6x12		jRD6x12
#define jpeg_idct_5x10		jRD5x10
#define jpeg_idct_4x8		jRD4x8
#define jpeg_idct_3x6		jRD3x8
#define jpeg_idct_2x4		jRD2x4
#define jpeg_idct_1x2		jRD1x2
#endif 

EXTERN(void) jpeg_fdct_islow
    JPP((DCTELEM * data, JSAMPARRAY sample_data, JDIMENSION start_col));
EXTERN(void) jpeg_fdct_ifast
    JPP((DCTELEM * data, JSAMPARRAY sample_data, JDIMENSION start_col));
EXTERN(void) jpeg_fdct_float
    JPP((FAST_FLOAT * data, JSAMPARRAY sample_data, JDIMENSION start_col));
EXTERN(void) jpeg_fdct_7x7
    JPP((DCTELEM * data, JSAMPARRAY sample_data, JDIMENSION start_col));
EXTERN(void) jpeg_fdct_6x6
    JPP((DCTELEM * data, JSAMPARRAY sample_data, JDIMENSION start_col));
EXTERN(void) jpeg_fdct_5x5
    JPP((DCTELEM * data, JSAMPARRAY sample_data, JDIMENSION start_col));
EXTERN(void) jpeg_fdct_4x4
    JPP((DCTELEM * data, JSAMPARRAY sample_data, JDIMENSION start_col));
EXTERN(void) jpeg_fdct_3x3
    JPP((DCTELEM * data, JSAMPARRAY sample_data, JDIMENSION start_col));
EXTERN(void) jpeg_fdct_2x2
    JPP((DCTELEM * data, JSAMPARRAY sample_data, JDIMENSION start_col));
EXTERN(void) jpeg_fdct_1x1
    JPP((DCTELEM * data, JSAMPARRAY sample_data, JDIMENSION start_col));
EXTERN(void) jpeg_fdct_9x9
    JPP((DCTELEM * data, JSAMPARRAY sample_data, JDIMENSION start_col));
EXTERN(void) jpeg_fdct_10x10
    JPP((DCTELEM * data, JSAMPARRAY sample_data, JDIMENSION start_col));
EXTERN(void) jpeg_fdct_11x11
    JPP((DCTELEM * data, JSAMPARRAY sample_data, JDIMENSION start_col));
EXTERN(void) jpeg_fdct_12x12
    JPP((DCTELEM * data, JSAMPARRAY sample_data, JDIMENSION start_col));
EXTERN(void) jpeg_fdct_13x13
    JPP((DCTELEM * data, JSAMPARRAY sample_data, JDIMENSION start_col));
EXTERN(void) jpeg_fdct_14x14
    JPP((DCTELEM * data, JSAMPARRAY sample_data, JDIMENSION start_col));
EXTERN(void) jpeg_fdct_15x15
    JPP((DCTELEM * data, JSAMPARRAY sample_data, JDIMENSION start_col));
EXTERN(void) jpeg_fdct_16x16
    JPP((DCTELEM * data, JSAMPARRAY sample_data, JDIMENSION start_col));
EXTERN(void) jpeg_fdct_16x8
    JPP((DCTELEM * data, JSAMPARRAY sample_data, JDIMENSION start_col));
EXTERN(void) jpeg_fdct_14x7
    JPP((DCTELEM * data, JSAMPARRAY sample_data, JDIMENSION start_col));
EXTERN(void) jpeg_fdct_12x6
    JPP((DCTELEM * data, JSAMPARRAY sample_data, JDIMENSION start_col));
EXTERN(void) jpeg_fdct_10x5
    JPP((DCTELEM * data, JSAMPARRAY sample_data, JDIMENSION start_col));
EXTERN(void) jpeg_fdct_8x4
    JPP((DCTELEM * data, JSAMPARRAY sample_data, JDIMENSION start_col));
EXTERN(void) jpeg_fdct_6x3
    JPP((DCTELEM * data, JSAMPARRAY sample_data, JDIMENSION start_col));
EXTERN(void) jpeg_fdct_4x2
    JPP((DCTELEM * data, JSAMPARRAY sample_data, JDIMENSION start_col));
EXTERN(void) jpeg_fdct_2x1
    JPP((DCTELEM * data, JSAMPARRAY sample_data, JDIMENSION start_col));
EXTERN(void) jpeg_fdct_8x16
    JPP((DCTELEM * data, JSAMPARRAY sample_data, JDIMENSION start_col));
EXTERN(void) jpeg_fdct_7x14
    JPP((DCTELEM * data, JSAMPARRAY sample_data, JDIMENSION start_col));
EXTERN(void) jpeg_fdct_6x12
    JPP((DCTELEM * data, JSAMPARRAY sample_data, JDIMENSION start_col));
EXTERN(void) jpeg_fdct_5x10
    JPP((DCTELEM * data, JSAMPARRAY sample_data, JDIMENSION start_col));
EXTERN(void) jpeg_fdct_4x8
    JPP((DCTELEM * data, JSAMPARRAY sample_data, JDIMENSION start_col));
EXTERN(void) jpeg_fdct_3x6
    JPP((DCTELEM * data, JSAMPARRAY sample_data, JDIMENSION start_col));
EXTERN(void) jpeg_fdct_2x4
    JPP((DCTELEM * data, JSAMPARRAY sample_data, JDIMENSION start_col));
EXTERN(void) jpeg_fdct_1x2
    JPP((DCTELEM * data, JSAMPARRAY sample_data, JDIMENSION start_col));

EXTERN(void) jpeg_idct_islow
    JPP((j_decompress_ptr cinfo, jpeg_component_info * compptr,
	 JCOEFPTR coef_block, JSAMPARRAY output_buf, JDIMENSION output_col));
EXTERN(void) jpeg_idct_ifast
    JPP((j_decompress_ptr cinfo, jpeg_component_info * compptr,
	 JCOEFPTR coef_block, JSAMPARRAY output_buf, JDIMENSION output_col));
EXTERN(void) jpeg_idct_float
    JPP((j_decompress_ptr cinfo, jpeg_component_info * compptr,
	 JCOEFPTR coef_block, JSAMPARRAY output_buf, JDIMENSION output_col));
EXTERN(void) jpeg_idct_7x7
    JPP((j_decompress_ptr cinfo, jpeg_component_info * compptr,
	 JCOEFPTR coef_block, JSAMPARRAY output_buf, JDIMENSION output_col));
EXTERN(void) jpeg_idct_6x6
    JPP((j_decompress_ptr cinfo, jpeg_component_info * compptr,
	 JCOEFPTR coef_block, JSAMPARRAY output_buf, JDIMENSION output_col));
EXTERN(void) jpeg_idct_5x5
    JPP((j_decompress_ptr cinfo, jpeg_component_info * compptr,
	 JCOEFPTR coef_block, JSAMPARRAY output_buf, JDIMENSION output_col));
EXTERN(void) jpeg_idct_4x4
    JPP((j_decompress_ptr cinfo, jpeg_component_info * compptr,
	 JCOEFPTR coef_block, JSAMPARRAY output_buf, JDIMENSION output_col));
EXTERN(void) jpeg_idct_3x3
    JPP((j_decompress_ptr cinfo, jpeg_component_info * compptr,
	 JCOEFPTR coef_block, JSAMPARRAY output_buf, JDIMENSION output_col));
EXTERN(void) jpeg_idct_2x2
    JPP((j_decompress_ptr cinfo, jpeg_component_info * compptr,
	 JCOEFPTR coef_block, JSAMPARRAY output_buf, JDIMENSION output_col));
EXTERN(void) jpeg_idct_1x1
    JPP((j_decompress_ptr cinfo, jpeg_component_info * compptr,
	 JCOEFPTR coef_block, JSAMPARRAY output_buf, JDIMENSION output_col));
EXTERN(void) jpeg_idct_9x9
    JPP((j_decompress_ptr cinfo, jpeg_component_info * compptr,
	 JCOEFPTR coef_block, JSAMPARRAY output_buf, JDIMENSION output_col));
EXTERN(void) jpeg_idct_10x10
    JPP((j_decompress_ptr cinfo, jpeg_component_info * compptr,
	 JCOEFPTR coef_block, JSAMPARRAY output_buf, JDIMENSION output_col));
EXTERN(void) jpeg_idct_11x11
    JPP((j_decompress_ptr cinfo, jpeg_component_info * compptr,
	 JCOEFPTR coef_block, JSAMPARRAY output_buf, JDIMENSION output_col));
EXTERN(void) jpeg_idct_12x12
    JPP((j_decompress_ptr cinfo, jpeg_component_info * compptr,
	 JCOEFPTR coef_block, JSAMPARRAY output_buf, JDIMENSION output_col));
EXTERN(void) jpeg_idct_13x13
    JPP((j_decompress_ptr cinfo, jpeg_component_info * compptr,
	 JCOEFPTR coef_block, JSAMPARRAY output_buf, JDIMENSION output_col));
EXTERN(void) jpeg_idct_14x14
    JPP((j_decompress_ptr cinfo, jpeg_component_info * compptr,
	 JCOEFPTR coef_block, JSAMPARRAY output_buf, JDIMENSION output_col));
EXTERN(void) jpeg_idct_15x15
    JPP((j_decompress_ptr cinfo, jpeg_component_info * compptr,
	 JCOEFPTR coef_block, JSAMPARRAY output_buf, JDIMENSION output_col));
EXTERN(void) jpeg_idct_16x16
    JPP((j_decompress_ptr cinfo, jpeg_component_info * compptr,
	 JCOEFPTR coef_block, JSAMPARRAY output_buf, JDIMENSION output_col));
EXTERN(void) jpeg_idct_16x8
    JPP((j_decompress_ptr cinfo, jpeg_component_info * compptr,
	 JCOEFPTR coef_block, JSAMPARRAY output_buf, JDIMENSION output_col));
EXTERN(void) jpeg_idct_14x7
    JPP((j_decompress_ptr cinfo, jpeg_component_info * compptr,
	 JCOEFPTR coef_block, JSAMPARRAY output_buf, JDIMENSION output_col));
EXTERN(void) jpeg_idct_12x6
    JPP((j_decompress_ptr cinfo, jpeg_component_info * compptr,
	 JCOEFPTR coef_block, JSAMPARRAY output_buf, JDIMENSION output_col));
EXTERN(void) jpeg_idct_10x5
    JPP((j_decompress_ptr cinfo, jpeg_component_info * compptr,
	 JCOEFPTR coef_block, JSAMPARRAY output_buf, JDIMENSION output_col));
EXTERN(void) jpeg_idct_8x4
    JPP((j_decompress_ptr cinfo, jpeg_component_info * compptr,
	 JCOEFPTR coef_block, JSAMPARRAY output_buf, JDIMENSION output_col));
EXTERN(void) jpeg_idct_6x3
    JPP((j_decompress_ptr cinfo, jpeg_component_info * compptr,
	 JCOEFPTR coef_block, JSAMPARRAY output_buf, JDIMENSION output_col));
EXTERN(void) jpeg_idct_4x2
    JPP((j_decompress_ptr cinfo, jpeg_component_info * compptr,
	 JCOEFPTR coef_block, JSAMPARRAY output_buf, JDIMENSION output_col));
EXTERN(void) jpeg_idct_2x1
    JPP((j_decompress_ptr cinfo, jpeg_component_info * compptr,
	 JCOEFPTR coef_block, JSAMPARRAY output_buf, JDIMENSION output_col));
EXTERN(void) jpeg_idct_8x16
    JPP((j_decompress_ptr cinfo, jpeg_component_info * compptr,
	 JCOEFPTR coef_block, JSAMPARRAY output_buf, JDIMENSION output_col));
EXTERN(void) jpeg_idct_7x14
    JPP((j_decompress_ptr cinfo, jpeg_component_info * compptr,
	 JCOEFPTR coef_block, JSAMPARRAY output_buf, JDIMENSION output_col));
EXTERN(void) jpeg_idct_6x12
    JPP((j_decompress_ptr cinfo, jpeg_component_info * compptr,
	 JCOEFPTR coef_block, JSAMPARRAY output_buf, JDIMENSION output_col));
EXTERN(void) jpeg_idct_5x10
    JPP((j_decompress_ptr cinfo, jpeg_component_info * compptr,
	 JCOEFPTR coef_block, JSAMPARRAY output_buf, JDIMENSION output_col));
EXTERN(void) jpeg_idct_4x8
    JPP((j_decompress_ptr cinfo, jpeg_component_info * compptr,
	 JCOEFPTR coef_block, JSAMPARRAY output_buf, JDIMENSION output_col));
EXTERN(void) jpeg_idct_3x6
    JPP((j_decompress_ptr cinfo, jpeg_component_info * compptr,
	 JCOEFPTR coef_block, JSAMPARRAY output_buf, JDIMENSION output_col));
EXTERN(void) jpeg_idct_2x4
    JPP((j_decompress_ptr cinfo, jpeg_component_info * compptr,
	 JCOEFPTR coef_block, JSAMPARRAY output_buf, JDIMENSION output_col));
EXTERN(void) jpeg_idct_1x2
    JPP((j_decompress_ptr cinfo, jpeg_component_info * compptr,
	 JCOEFPTR coef_block, JSAMPARRAY output_buf, JDIMENSION output_col));

#define ONE	((INT32) 1)
#define CONST_SCALE (ONE << CONST_BITS)

#define FIX(x)	((INT32) ((x) * CONST_SCALE + 0.5))

#define DESCALE(x,n)  RIGHT_SHIFT((x) + (ONE << ((n)-1)), n)

#ifdef SHORTxSHORT_32		
#define MULTIPLY16C16(var,const)  (((INT16) (var)) * ((INT16) (const)))
#endif
#ifdef SHORTxLCONST_32		
#define MULTIPLY16C16(var,const)  (((INT16) (var)) * ((INT32) (const)))
#endif

#ifndef MULTIPLY16C16		
#define MULTIPLY16C16(var,const)  ((var) * (const))
#endif

#ifdef SHORTxSHORT_32		
#define MULTIPLY16V16(var1,var2)  (((INT16) (var1)) * ((INT16) (var2)))
#endif

#ifndef MULTIPLY16V16		
#define MULTIPLY16V16(var1,var2)  ((var1) * (var2))
#endif
