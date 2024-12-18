

#define JPEG_INTERNALS
#include "jinclude.h"
#include "jpeglib.h"

typedef struct {
  struct jpeg_input_controller pub; 

  int inheaders;		
} my_input_controller;

typedef my_input_controller * my_inputctl_ptr;

METHODDEF(int) consume_markers JPP((j_decompress_ptr cinfo));

GLOBAL(void)
jpeg_core_output_dimensions (j_decompress_ptr cinfo)

{
#ifdef IDCT_SCALING_SUPPORTED
  int ci;
  jpeg_component_info *compptr;

  
  if (cinfo->scale_num * cinfo->block_size <= cinfo->scale_denom) {
    
    cinfo->output_width = (JDIMENSION)
      jdiv_round_up((long) cinfo->image_width, (long) cinfo->block_size);
    cinfo->output_height = (JDIMENSION)
      jdiv_round_up((long) cinfo->image_height, (long) cinfo->block_size);
    cinfo->min_DCT_h_scaled_size = 1;
    cinfo->min_DCT_v_scaled_size = 1;
  } else if (cinfo->scale_num * cinfo->block_size <= cinfo->scale_denom * 2) {
    
    cinfo->output_width = (JDIMENSION)
      jdiv_round_up((long) cinfo->image_width * 2L, (long) cinfo->block_size);
    cinfo->output_height = (JDIMENSION)
      jdiv_round_up((long) cinfo->image_height * 2L, (long) cinfo->block_size);
    cinfo->min_DCT_h_scaled_size = 2;
    cinfo->min_DCT_v_scaled_size = 2;
  } else if (cinfo->scale_num * cinfo->block_size <= cinfo->scale_denom * 3) {
    
    cinfo->output_width = (JDIMENSION)
      jdiv_round_up((long) cinfo->image_width * 3L, (long) cinfo->block_size);
    cinfo->output_height = (JDIMENSION)
      jdiv_round_up((long) cinfo->image_height * 3L, (long) cinfo->block_size);
    cinfo->min_DCT_h_scaled_size = 3;
    cinfo->min_DCT_v_scaled_size = 3;
  } else if (cinfo->scale_num * cinfo->block_size <= cinfo->scale_denom * 4) {
    
    cinfo->output_width = (JDIMENSION)
      jdiv_round_up((long) cinfo->image_width * 4L, (long) cinfo->block_size);
    cinfo->output_height = (JDIMENSION)
      jdiv_round_up((long) cinfo->image_height * 4L, (long) cinfo->block_size);
    cinfo->min_DCT_h_scaled_size = 4;
    cinfo->min_DCT_v_scaled_size = 4;
  } else if (cinfo->scale_num * cinfo->block_size <= cinfo->scale_denom * 5) {
    
    cinfo->output_width = (JDIMENSION)
      jdiv_round_up((long) cinfo->image_width * 5L, (long) cinfo->block_size);
    cinfo->output_height = (JDIMENSION)
      jdiv_round_up((long) cinfo->image_height * 5L, (long) cinfo->block_size);
    cinfo->min_DCT_h_scaled_size = 5;
    cinfo->min_DCT_v_scaled_size = 5;
  } else if (cinfo->scale_num * cinfo->block_size <= cinfo->scale_denom * 6) {
    
    cinfo->output_width = (JDIMENSION)
      jdiv_round_up((long) cinfo->image_width * 6L, (long) cinfo->block_size);
    cinfo->output_height = (JDIMENSION)
      jdiv_round_up((long) cinfo->image_height * 6L, (long) cinfo->block_size);
    cinfo->min_DCT_h_scaled_size = 6;
    cinfo->min_DCT_v_scaled_size = 6;
  } else if (cinfo->scale_num * cinfo->block_size <= cinfo->scale_denom * 7) {
    
    cinfo->output_width = (JDIMENSION)
      jdiv_round_up((long) cinfo->image_width * 7L, (long) cinfo->block_size);
    cinfo->output_height = (JDIMENSION)
      jdiv_round_up((long) cinfo->image_height * 7L, (long) cinfo->block_size);
    cinfo->min_DCT_h_scaled_size = 7;
    cinfo->min_DCT_v_scaled_size = 7;
  } else if (cinfo->scale_num * cinfo->block_size <= cinfo->scale_denom * 8) {
    
    cinfo->output_width = (JDIMENSION)
      jdiv_round_up((long) cinfo->image_width * 8L, (long) cinfo->block_size);
    cinfo->output_height = (JDIMENSION)
      jdiv_round_up((long) cinfo->image_height * 8L, (long) cinfo->block_size);
    cinfo->min_DCT_h_scaled_size = 8;
    cinfo->min_DCT_v_scaled_size = 8;
  } else if (cinfo->scale_num * cinfo->block_size <= cinfo->scale_denom * 9) {
    
    cinfo->output_width = (JDIMENSION)
      jdiv_round_up((long) cinfo->image_width * 9L, (long) cinfo->block_size);
    cinfo->output_height = (JDIMENSION)
      jdiv_round_up((long) cinfo->image_height * 9L, (long) cinfo->block_size);
    cinfo->min_DCT_h_scaled_size = 9;
    cinfo->min_DCT_v_scaled_size = 9;
  } else if (cinfo->scale_num * cinfo->block_size <= cinfo->scale_denom * 10) {
    
    cinfo->output_width = (JDIMENSION)
      jdiv_round_up((long) cinfo->image_width * 10L, (long) cinfo->block_size);
    cinfo->output_height = (JDIMENSION)
      jdiv_round_up((long) cinfo->image_height * 10L, (long) cinfo->block_size);
    cinfo->min_DCT_h_scaled_size = 10;
    cinfo->min_DCT_v_scaled_size = 10;
  } else if (cinfo->scale_num * cinfo->block_size <= cinfo->scale_denom * 11) {
    
    cinfo->output_width = (JDIMENSION)
      jdiv_round_up((long) cinfo->image_width * 11L, (long) cinfo->block_size);
    cinfo->output_height = (JDIMENSION)
      jdiv_round_up((long) cinfo->image_height * 11L, (long) cinfo->block_size);
    cinfo->min_DCT_h_scaled_size = 11;
    cinfo->min_DCT_v_scaled_size = 11;
  } else if (cinfo->scale_num * cinfo->block_size <= cinfo->scale_denom * 12) {
    
    cinfo->output_width = (JDIMENSION)
      jdiv_round_up((long) cinfo->image_width * 12L, (long) cinfo->block_size);
    cinfo->output_height = (JDIMENSION)
      jdiv_round_up((long) cinfo->image_height * 12L, (long) cinfo->block_size);
    cinfo->min_DCT_h_scaled_size = 12;
    cinfo->min_DCT_v_scaled_size = 12;
  } else if (cinfo->scale_num * cinfo->block_size <= cinfo->scale_denom * 13) {
    
    cinfo->output_width = (JDIMENSION)
      jdiv_round_up((long) cinfo->image_width * 13L, (long) cinfo->block_size);
    cinfo->output_height = (JDIMENSION)
      jdiv_round_up((long) cinfo->image_height * 13L, (long) cinfo->block_size);
    cinfo->min_DCT_h_scaled_size = 13;
    cinfo->min_DCT_v_scaled_size = 13;
  } else if (cinfo->scale_num * cinfo->block_size <= cinfo->scale_denom * 14) {
    
    cinfo->output_width = (JDIMENSION)
      jdiv_round_up((long) cinfo->image_width * 14L, (long) cinfo->block_size);
    cinfo->output_height = (JDIMENSION)
      jdiv_round_up((long) cinfo->image_height * 14L, (long) cinfo->block_size);
    cinfo->min_DCT_h_scaled_size = 14;
    cinfo->min_DCT_v_scaled_size = 14;
  } else if (cinfo->scale_num * cinfo->block_size <= cinfo->scale_denom * 15) {
    
    cinfo->output_width = (JDIMENSION)
      jdiv_round_up((long) cinfo->image_width * 15L, (long) cinfo->block_size);
    cinfo->output_height = (JDIMENSION)
      jdiv_round_up((long) cinfo->image_height * 15L, (long) cinfo->block_size);
    cinfo->min_DCT_h_scaled_size = 15;
    cinfo->min_DCT_v_scaled_size = 15;
  } else {
    
    cinfo->output_width = (JDIMENSION)
      jdiv_round_up((long) cinfo->image_width * 16L, (long) cinfo->block_size);
    cinfo->output_height = (JDIMENSION)
      jdiv_round_up((long) cinfo->image_height * 16L, (long) cinfo->block_size);
    cinfo->min_DCT_h_scaled_size = 16;
    cinfo->min_DCT_v_scaled_size = 16;
  }

  
  for (ci = 0, compptr = cinfo->comp_info; ci < cinfo->num_components;
       ci++, compptr++) {
    compptr->DCT_h_scaled_size = cinfo->min_DCT_h_scaled_size;
    compptr->DCT_v_scaled_size = cinfo->min_DCT_v_scaled_size;
  }

#else 

  
  cinfo->output_width = cinfo->image_width;
  cinfo->output_height = cinfo->image_height;
  

#endif 
}

LOCAL(void)
initial_setup (j_decompress_ptr cinfo)

{
  int ci;
  jpeg_component_info *compptr;

  
  if ((long) cinfo->image_height > (long) JPEG_MAX_DIMENSION ||
      (long) cinfo->image_width > (long) JPEG_MAX_DIMENSION)
    ERREXIT1(cinfo, JERR_IMAGE_TOO_BIG, (unsigned int) JPEG_MAX_DIMENSION);

  
  if (cinfo->data_precision != BITS_IN_JSAMPLE)
    ERREXIT1(cinfo, JERR_BAD_PRECISION, cinfo->data_precision);

  
  if (cinfo->num_components > MAX_COMPONENTS)
    ERREXIT2(cinfo, JERR_COMPONENT_COUNT, cinfo->num_components,
	     MAX_COMPONENTS);

  
  cinfo->max_h_samp_factor = 1;
  cinfo->max_v_samp_factor = 1;
  for (ci = 0, compptr = cinfo->comp_info; ci < cinfo->num_components;
       ci++, compptr++) {
    if (compptr->h_samp_factor<=0 || compptr->h_samp_factor>MAX_SAMP_FACTOR ||
	compptr->v_samp_factor<=0 || compptr->v_samp_factor>MAX_SAMP_FACTOR)
      ERREXIT(cinfo, JERR_BAD_SAMPLING);
    cinfo->max_h_samp_factor = MAX(cinfo->max_h_samp_factor,
				   compptr->h_samp_factor);
    cinfo->max_v_samp_factor = MAX(cinfo->max_v_samp_factor,
				   compptr->v_samp_factor);
  }

  
  if (cinfo->is_baseline || (cinfo->progressive_mode &&
      cinfo->comps_in_scan)) { 
    cinfo->block_size = DCTSIZE;
    cinfo->natural_order = jpeg_natural_order;
    cinfo->lim_Se = DCTSIZE2-1;
  } else
    switch (cinfo->Se) {
    case (1*1-1):
      cinfo->block_size = 1;
      cinfo->natural_order = jpeg_natural_order; 
      cinfo->lim_Se = cinfo->Se;
      break;
    case (2*2-1):
      cinfo->block_size = 2;
      cinfo->natural_order = jpeg_natural_order2;
      cinfo->lim_Se = cinfo->Se;
      break;
    case (3*3-1):
      cinfo->block_size = 3;
      cinfo->natural_order = jpeg_natural_order3;
      cinfo->lim_Se = cinfo->Se;
      break;
    case (4*4-1):
      cinfo->block_size = 4;
      cinfo->natural_order = jpeg_natural_order4;
      cinfo->lim_Se = cinfo->Se;
      break;
    case (5*5-1):
      cinfo->block_size = 5;
      cinfo->natural_order = jpeg_natural_order5;
      cinfo->lim_Se = cinfo->Se;
      break;
    case (6*6-1):
      cinfo->block_size = 6;
      cinfo->natural_order = jpeg_natural_order6;
      cinfo->lim_Se = cinfo->Se;
      break;
    case (7*7-1):
      cinfo->block_size = 7;
      cinfo->natural_order = jpeg_natural_order7;
      cinfo->lim_Se = cinfo->Se;
      break;
    case (8*8-1):
      cinfo->block_size = 8;
      cinfo->natural_order = jpeg_natural_order;
      cinfo->lim_Se = DCTSIZE2-1;
      break;
    case (9*9-1):
      cinfo->block_size = 9;
      cinfo->natural_order = jpeg_natural_order;
      cinfo->lim_Se = DCTSIZE2-1;
      break;
    case (10*10-1):
      cinfo->block_size = 10;
      cinfo->natural_order = jpeg_natural_order;
      cinfo->lim_Se = DCTSIZE2-1;
      break;
    case (11*11-1):
      cinfo->block_size = 11;
      cinfo->natural_order = jpeg_natural_order;
      cinfo->lim_Se = DCTSIZE2-1;
      break;
    case (12*12-1):
      cinfo->block_size = 12;
      cinfo->natural_order = jpeg_natural_order;
      cinfo->lim_Se = DCTSIZE2-1;
      break;
    case (13*13-1):
      cinfo->block_size = 13;
      cinfo->natural_order = jpeg_natural_order;
      cinfo->lim_Se = DCTSIZE2-1;
      break;
    case (14*14-1):
      cinfo->block_size = 14;
      cinfo->natural_order = jpeg_natural_order;
      cinfo->lim_Se = DCTSIZE2-1;
      break;
    case (15*15-1):
      cinfo->block_size = 15;
      cinfo->natural_order = jpeg_natural_order;
      cinfo->lim_Se = DCTSIZE2-1;
      break;
    case (16*16-1):
      cinfo->block_size = 16;
      cinfo->natural_order = jpeg_natural_order;
      cinfo->lim_Se = DCTSIZE2-1;
      break;
    default:
      ERREXIT4(cinfo, JERR_BAD_PROGRESSION,
	       cinfo->Ss, cinfo->Se, cinfo->Ah, cinfo->Al);
      break;
    }

  
  cinfo->min_DCT_h_scaled_size = cinfo->block_size;
  cinfo->min_DCT_v_scaled_size = cinfo->block_size;

  
  for (ci = 0, compptr = cinfo->comp_info; ci < cinfo->num_components;
       ci++, compptr++) {
    compptr->DCT_h_scaled_size = cinfo->block_size;
    compptr->DCT_v_scaled_size = cinfo->block_size;
    
    compptr->width_in_blocks = (JDIMENSION)
      jdiv_round_up((long) cinfo->image_width * (long) compptr->h_samp_factor,
		    (long) (cinfo->max_h_samp_factor * cinfo->block_size));
    compptr->height_in_blocks = (JDIMENSION)
      jdiv_round_up((long) cinfo->image_height * (long) compptr->v_samp_factor,
		    (long) (cinfo->max_v_samp_factor * cinfo->block_size));
    
    
    compptr->downsampled_width = (JDIMENSION)
      jdiv_round_up((long) cinfo->image_width * (long) compptr->h_samp_factor,
		    (long) cinfo->max_h_samp_factor);
    compptr->downsampled_height = (JDIMENSION)
      jdiv_round_up((long) cinfo->image_height * (long) compptr->v_samp_factor,
		    (long) cinfo->max_v_samp_factor);
    
    compptr->component_needed = TRUE;
    
    compptr->quant_table = NULL;
  }

  
  cinfo->total_iMCU_rows = (JDIMENSION)
    jdiv_round_up((long) cinfo->image_height,
	          (long) (cinfo->max_v_samp_factor * cinfo->block_size));

  
  if (cinfo->comps_in_scan < cinfo->num_components || cinfo->progressive_mode)
    cinfo->inputctl->has_multiple_scans = TRUE;
  else
    cinfo->inputctl->has_multiple_scans = FALSE;
}

LOCAL(void)
per_scan_setup (j_decompress_ptr cinfo)

{
  int ci, mcublks, tmp;
  jpeg_component_info *compptr;
  
  if (cinfo->comps_in_scan == 1) {
    
    
    compptr = cinfo->cur_comp_info[0];
    
    
    cinfo->MCUs_per_row = compptr->width_in_blocks;
    cinfo->MCU_rows_in_scan = compptr->height_in_blocks;
    
    
    compptr->MCU_width = 1;
    compptr->MCU_height = 1;
    compptr->MCU_blocks = 1;
    compptr->MCU_sample_width = compptr->DCT_h_scaled_size;
    compptr->last_col_width = 1;
    
    tmp = (int) (compptr->height_in_blocks % compptr->v_samp_factor);
    if (tmp == 0) tmp = compptr->v_samp_factor;
    compptr->last_row_height = tmp;
    
    
    cinfo->blocks_in_MCU = 1;
    cinfo->MCU_membership[0] = 0;
    
  } else {
    
    
    if (cinfo->comps_in_scan <= 0 || cinfo->comps_in_scan > MAX_COMPS_IN_SCAN)
      ERREXIT2(cinfo, JERR_COMPONENT_COUNT, cinfo->comps_in_scan,
	       MAX_COMPS_IN_SCAN);
    
    
    cinfo->MCUs_per_row = (JDIMENSION)
      jdiv_round_up((long) cinfo->image_width,
		    (long) (cinfo->max_h_samp_factor * cinfo->block_size));
    cinfo->MCU_rows_in_scan = (JDIMENSION)
      jdiv_round_up((long) cinfo->image_height,
		    (long) (cinfo->max_v_samp_factor * cinfo->block_size));
    
    cinfo->blocks_in_MCU = 0;
    
    for (ci = 0; ci < cinfo->comps_in_scan; ci++) {
      compptr = cinfo->cur_comp_info[ci];
      
      compptr->MCU_width = compptr->h_samp_factor;
      compptr->MCU_height = compptr->v_samp_factor;
      compptr->MCU_blocks = compptr->MCU_width * compptr->MCU_height;
      compptr->MCU_sample_width = compptr->MCU_width * compptr->DCT_h_scaled_size;
      
      tmp = (int) (compptr->width_in_blocks % compptr->MCU_width);
      if (tmp == 0) tmp = compptr->MCU_width;
      compptr->last_col_width = tmp;
      tmp = (int) (compptr->height_in_blocks % compptr->MCU_height);
      if (tmp == 0) tmp = compptr->MCU_height;
      compptr->last_row_height = tmp;
      
      mcublks = compptr->MCU_blocks;
      if (cinfo->blocks_in_MCU + mcublks > D_MAX_BLOCKS_IN_MCU)
	ERREXIT(cinfo, JERR_BAD_MCU_SIZE);
      while (mcublks-- > 0) {
	cinfo->MCU_membership[cinfo->blocks_in_MCU++] = ci;
      }
    }
    
  }
}

LOCAL(void)
latch_quant_tables (j_decompress_ptr cinfo)
{
  int ci, qtblno;
  jpeg_component_info *compptr;
  JQUANT_TBL * qtbl;

  for (ci = 0; ci < cinfo->comps_in_scan; ci++) {
    compptr = cinfo->cur_comp_info[ci];
    
    if (compptr->quant_table != NULL)
      continue;
    
    qtblno = compptr->quant_tbl_no;
    if (qtblno < 0 || qtblno >= NUM_QUANT_TBLS ||
	cinfo->quant_tbl_ptrs[qtblno] == NULL)
      ERREXIT1(cinfo, JERR_NO_QUANT_TABLE, qtblno);
    
    qtbl = (JQUANT_TBL *)
      (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
				  SIZEOF(JQUANT_TBL));
    MEMCOPY(qtbl, cinfo->quant_tbl_ptrs[qtblno], SIZEOF(JQUANT_TBL));
    compptr->quant_table = qtbl;
  }
}

METHODDEF(void)
start_input_pass (j_decompress_ptr cinfo)
{
  per_scan_setup(cinfo);
  latch_quant_tables(cinfo);
  (*cinfo->entropy->start_pass) (cinfo);
  (*cinfo->coef->start_input_pass) (cinfo);
  cinfo->inputctl->consume_input = cinfo->coef->consume_data;
}

METHODDEF(void)
finish_input_pass (j_decompress_ptr cinfo)
{
  cinfo->inputctl->consume_input = consume_markers;
}

METHODDEF(int)
consume_markers (j_decompress_ptr cinfo)
{
  my_inputctl_ptr inputctl = (my_inputctl_ptr) cinfo->inputctl;
  int val;

  if (inputctl->pub.eoi_reached) 
    return JPEG_REACHED_EOI;

  for (;;) {			
    val = (*cinfo->marker->read_markers) (cinfo);

    switch (val) {
    case JPEG_REACHED_SOS:	
      if (inputctl->inheaders) { 
	if (inputctl->inheaders == 1)
	  initial_setup(cinfo);
	if (cinfo->comps_in_scan == 0) { 
	  inputctl->inheaders = 2;
	  break;
	}
	inputctl->inheaders = 0;
	
      } else {			
	if (! inputctl->pub.has_multiple_scans)
	  ERREXIT(cinfo, JERR_EOI_EXPECTED); 
	if (cinfo->comps_in_scan == 0) 
	  break;
	start_input_pass(cinfo);
      }
      return val;
    case JPEG_REACHED_EOI:	
      inputctl->pub.eoi_reached = TRUE;
      if (inputctl->inheaders) { 
	if (cinfo->marker->saw_SOF)
	  ERREXIT(cinfo, JERR_SOF_NO_SOS);
      } else {
	
	if (cinfo->output_scan_number > cinfo->input_scan_number)
	  cinfo->output_scan_number = cinfo->input_scan_number;
      }
      return val;
    case JPEG_SUSPENDED:
      return val;
    default:
      return val;
    }
  }
}

METHODDEF(void)
reset_input_controller (j_decompress_ptr cinfo)
{
  my_inputctl_ptr inputctl = (my_inputctl_ptr) cinfo->inputctl;

  inputctl->pub.consume_input = consume_markers;
  inputctl->pub.has_multiple_scans = FALSE; 
  inputctl->pub.eoi_reached = FALSE;
  inputctl->inheaders = 1;
  
  (*cinfo->err->reset_error_mgr) ((j_common_ptr) cinfo);
  (*cinfo->marker->reset_marker_reader) (cinfo);
  
  cinfo->coef_bits = NULL;
}

GLOBAL(void)
jinit_input_controller (j_decompress_ptr cinfo)
{
  my_inputctl_ptr inputctl;

  
  inputctl = (my_inputctl_ptr)
    (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_PERMANENT,
				SIZEOF(my_input_controller));
  cinfo->inputctl = (struct jpeg_input_controller *) inputctl;
  
  inputctl->pub.consume_input = consume_markers;
  inputctl->pub.reset_input_controller = reset_input_controller;
  inputctl->pub.start_input_pass = start_input_pass;
  inputctl->pub.finish_input_pass = finish_input_pass;
  
  inputctl->pub.has_multiple_scans = FALSE; 
  inputctl->pub.eoi_reached = FALSE;
  inputctl->inheaders = 1;
}
