

#define JPEG_INTERNALS
#include "jinclude.h"
#include "jpeglib.h"

typedef enum {
	main_pass,		
	huff_opt_pass,		
	output_pass		
} c_pass_type;

typedef struct {
  struct jpeg_comp_master pub;	

  c_pass_type pass_type;	

  int pass_number;		
  int total_passes;		

  int scan_number;		
} my_comp_master;

typedef my_comp_master * my_master_ptr;

GLOBAL(void)
jpeg_calc_jpeg_dimensions (j_compress_ptr cinfo)

{
#ifdef DCT_SCALING_SUPPORTED

  
  if (((long) cinfo->image_width >> 24) || ((long) cinfo->image_height >> 24))
    ERREXIT1(cinfo, JERR_IMAGE_TOO_BIG, (unsigned int) JPEG_MAX_DIMENSION);

  
  if (cinfo->scale_num >= cinfo->scale_denom * cinfo->block_size) {
    
    cinfo->jpeg_width = cinfo->image_width * cinfo->block_size;
    cinfo->jpeg_height = cinfo->image_height * cinfo->block_size;
    cinfo->min_DCT_h_scaled_size = 1;
    cinfo->min_DCT_v_scaled_size = 1;
  } else if (cinfo->scale_num * 2 >= cinfo->scale_denom * cinfo->block_size) {
    
    cinfo->jpeg_width = (JDIMENSION)
      jdiv_round_up((long) cinfo->image_width * cinfo->block_size, 2L);
    cinfo->jpeg_height = (JDIMENSION)
      jdiv_round_up((long) cinfo->image_height * cinfo->block_size, 2L);
    cinfo->min_DCT_h_scaled_size = 2;
    cinfo->min_DCT_v_scaled_size = 2;
  } else if (cinfo->scale_num * 3 >= cinfo->scale_denom * cinfo->block_size) {
    
    cinfo->jpeg_width = (JDIMENSION)
      jdiv_round_up((long) cinfo->image_width * cinfo->block_size, 3L);
    cinfo->jpeg_height = (JDIMENSION)
      jdiv_round_up((long) cinfo->image_height * cinfo->block_size, 3L);
    cinfo->min_DCT_h_scaled_size = 3;
    cinfo->min_DCT_v_scaled_size = 3;
  } else if (cinfo->scale_num * 4 >= cinfo->scale_denom * cinfo->block_size) {
    
    cinfo->jpeg_width = (JDIMENSION)
      jdiv_round_up((long) cinfo->image_width * cinfo->block_size, 4L);
    cinfo->jpeg_height = (JDIMENSION)
      jdiv_round_up((long) cinfo->image_height * cinfo->block_size, 4L);
    cinfo->min_DCT_h_scaled_size = 4;
    cinfo->min_DCT_v_scaled_size = 4;
  } else if (cinfo->scale_num * 5 >= cinfo->scale_denom * cinfo->block_size) {
    
    cinfo->jpeg_width = (JDIMENSION)
      jdiv_round_up((long) cinfo->image_width * cinfo->block_size, 5L);
    cinfo->jpeg_height = (JDIMENSION)
      jdiv_round_up((long) cinfo->image_height * cinfo->block_size, 5L);
    cinfo->min_DCT_h_scaled_size = 5;
    cinfo->min_DCT_v_scaled_size = 5;
  } else if (cinfo->scale_num * 6 >= cinfo->scale_denom * cinfo->block_size) {
    
    cinfo->jpeg_width = (JDIMENSION)
      jdiv_round_up((long) cinfo->image_width * cinfo->block_size, 6L);
    cinfo->jpeg_height = (JDIMENSION)
      jdiv_round_up((long) cinfo->image_height * cinfo->block_size, 6L);
    cinfo->min_DCT_h_scaled_size = 6;
    cinfo->min_DCT_v_scaled_size = 6;
  } else if (cinfo->scale_num * 7 >= cinfo->scale_denom * cinfo->block_size) {
    
    cinfo->jpeg_width = (JDIMENSION)
      jdiv_round_up((long) cinfo->image_width * cinfo->block_size, 7L);
    cinfo->jpeg_height = (JDIMENSION)
      jdiv_round_up((long) cinfo->image_height * cinfo->block_size, 7L);
    cinfo->min_DCT_h_scaled_size = 7;
    cinfo->min_DCT_v_scaled_size = 7;
  } else if (cinfo->scale_num * 8 >= cinfo->scale_denom * cinfo->block_size) {
    
    cinfo->jpeg_width = (JDIMENSION)
      jdiv_round_up((long) cinfo->image_width * cinfo->block_size, 8L);
    cinfo->jpeg_height = (JDIMENSION)
      jdiv_round_up((long) cinfo->image_height * cinfo->block_size, 8L);
    cinfo->min_DCT_h_scaled_size = 8;
    cinfo->min_DCT_v_scaled_size = 8;
  } else if (cinfo->scale_num * 9 >= cinfo->scale_denom * cinfo->block_size) {
    
    cinfo->jpeg_width = (JDIMENSION)
      jdiv_round_up((long) cinfo->image_width * cinfo->block_size, 9L);
    cinfo->jpeg_height = (JDIMENSION)
      jdiv_round_up((long) cinfo->image_height * cinfo->block_size, 9L);
    cinfo->min_DCT_h_scaled_size = 9;
    cinfo->min_DCT_v_scaled_size = 9;
  } else if (cinfo->scale_num * 10 >= cinfo->scale_denom * cinfo->block_size) {
    
    cinfo->jpeg_width = (JDIMENSION)
      jdiv_round_up((long) cinfo->image_width * cinfo->block_size, 10L);
    cinfo->jpeg_height = (JDIMENSION)
      jdiv_round_up((long) cinfo->image_height * cinfo->block_size, 10L);
    cinfo->min_DCT_h_scaled_size = 10;
    cinfo->min_DCT_v_scaled_size = 10;
  } else if (cinfo->scale_num * 11 >= cinfo->scale_denom * cinfo->block_size) {
    
    cinfo->jpeg_width = (JDIMENSION)
      jdiv_round_up((long) cinfo->image_width * cinfo->block_size, 11L);
    cinfo->jpeg_height = (JDIMENSION)
      jdiv_round_up((long) cinfo->image_height * cinfo->block_size, 11L);
    cinfo->min_DCT_h_scaled_size = 11;
    cinfo->min_DCT_v_scaled_size = 11;
  } else if (cinfo->scale_num * 12 >= cinfo->scale_denom * cinfo->block_size) {
    
    cinfo->jpeg_width = (JDIMENSION)
      jdiv_round_up((long) cinfo->image_width * cinfo->block_size, 12L);
    cinfo->jpeg_height = (JDIMENSION)
      jdiv_round_up((long) cinfo->image_height * cinfo->block_size, 12L);
    cinfo->min_DCT_h_scaled_size = 12;
    cinfo->min_DCT_v_scaled_size = 12;
  } else if (cinfo->scale_num * 13 >= cinfo->scale_denom * cinfo->block_size) {
    
    cinfo->jpeg_width = (JDIMENSION)
      jdiv_round_up((long) cinfo->image_width * cinfo->block_size, 13L);
    cinfo->jpeg_height = (JDIMENSION)
      jdiv_round_up((long) cinfo->image_height * cinfo->block_size, 13L);
    cinfo->min_DCT_h_scaled_size = 13;
    cinfo->min_DCT_v_scaled_size = 13;
  } else if (cinfo->scale_num * 14 >= cinfo->scale_denom * cinfo->block_size) {
    
    cinfo->jpeg_width = (JDIMENSION)
      jdiv_round_up((long) cinfo->image_width * cinfo->block_size, 14L);
    cinfo->jpeg_height = (JDIMENSION)
      jdiv_round_up((long) cinfo->image_height * cinfo->block_size, 14L);
    cinfo->min_DCT_h_scaled_size = 14;
    cinfo->min_DCT_v_scaled_size = 14;
  } else if (cinfo->scale_num * 15 >= cinfo->scale_denom * cinfo->block_size) {
    
    cinfo->jpeg_width = (JDIMENSION)
      jdiv_round_up((long) cinfo->image_width * cinfo->block_size, 15L);
    cinfo->jpeg_height = (JDIMENSION)
      jdiv_round_up((long) cinfo->image_height * cinfo->block_size, 15L);
    cinfo->min_DCT_h_scaled_size = 15;
    cinfo->min_DCT_v_scaled_size = 15;
  } else {
    
    cinfo->jpeg_width = (JDIMENSION)
      jdiv_round_up((long) cinfo->image_width * cinfo->block_size, 16L);
    cinfo->jpeg_height = (JDIMENSION)
      jdiv_round_up((long) cinfo->image_height * cinfo->block_size, 16L);
    cinfo->min_DCT_h_scaled_size = 16;
    cinfo->min_DCT_v_scaled_size = 16;
  }

#else 

  
  cinfo->jpeg_width = cinfo->image_width;
  cinfo->jpeg_height = cinfo->image_height;
  cinfo->min_DCT_h_scaled_size = DCTSIZE;
  cinfo->min_DCT_v_scaled_size = DCTSIZE;

#endif 
}

LOCAL(void)
jpeg_calc_trans_dimensions (j_compress_ptr cinfo)
{
  if (cinfo->min_DCT_h_scaled_size != cinfo->min_DCT_v_scaled_size)
    ERREXIT2(cinfo, JERR_BAD_DCTSIZE,
	     cinfo->min_DCT_h_scaled_size, cinfo->min_DCT_v_scaled_size);

  cinfo->block_size = cinfo->min_DCT_h_scaled_size;
}

LOCAL(void)
initial_setup (j_compress_ptr cinfo, boolean transcode_only)

{
  int ci, ssize;
  jpeg_component_info *compptr;
  long samplesperrow;
  JDIMENSION jd_samplesperrow;

  if (transcode_only)
    jpeg_calc_trans_dimensions(cinfo);
  else
    jpeg_calc_jpeg_dimensions(cinfo);

  
  if (cinfo->block_size < 1 || cinfo->block_size > 16)
    ERREXIT2(cinfo, JERR_BAD_DCTSIZE, cinfo->block_size, cinfo->block_size);

  
  switch (cinfo->block_size) {
  case 2: cinfo->natural_order = jpeg_natural_order2; break;
  case 3: cinfo->natural_order = jpeg_natural_order3; break;
  case 4: cinfo->natural_order = jpeg_natural_order4; break;
  case 5: cinfo->natural_order = jpeg_natural_order5; break;
  case 6: cinfo->natural_order = jpeg_natural_order6; break;
  case 7: cinfo->natural_order = jpeg_natural_order7; break;
  default: cinfo->natural_order = jpeg_natural_order; break;
  }

  
  cinfo->lim_Se = cinfo->block_size < DCTSIZE ?
    cinfo->block_size * cinfo->block_size - 1 : DCTSIZE2-1;

  
  if (cinfo->jpeg_height <= 0 || cinfo->jpeg_width <= 0 ||
      cinfo->num_components <= 0 || cinfo->input_components <= 0)
    ERREXIT(cinfo, JERR_EMPTY_IMAGE);

  
  if ((long) cinfo->jpeg_height > (long) JPEG_MAX_DIMENSION ||
      (long) cinfo->jpeg_width > (long) JPEG_MAX_DIMENSION)
    ERREXIT1(cinfo, JERR_IMAGE_TOO_BIG, (unsigned int) JPEG_MAX_DIMENSION);

  
  samplesperrow = (long) cinfo->image_width * (long) cinfo->input_components;
  jd_samplesperrow = (JDIMENSION) samplesperrow;
  if ((long) jd_samplesperrow != samplesperrow)
    ERREXIT(cinfo, JERR_WIDTH_OVERFLOW);

  
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

  
  for (ci = 0, compptr = cinfo->comp_info; ci < cinfo->num_components;
       ci++, compptr++) {
    
    compptr->component_index = ci;
    
    ssize = 1;
#ifdef DCT_SCALING_SUPPORTED
    while (cinfo->min_DCT_h_scaled_size * ssize <=
	   (cinfo->do_fancy_downsampling ? DCTSIZE : DCTSIZE / 2) &&
	   (cinfo->max_h_samp_factor % (compptr->h_samp_factor * ssize * 2)) == 0) {
      ssize = ssize * 2;
    }
#endif
    compptr->DCT_h_scaled_size = cinfo->min_DCT_h_scaled_size * ssize;
    ssize = 1;
#ifdef DCT_SCALING_SUPPORTED
    while (cinfo->min_DCT_v_scaled_size * ssize <=
	   (cinfo->do_fancy_downsampling ? DCTSIZE : DCTSIZE / 2) &&
	   (cinfo->max_v_samp_factor % (compptr->v_samp_factor * ssize * 2)) == 0) {
      ssize = ssize * 2;
    }
#endif
    compptr->DCT_v_scaled_size = cinfo->min_DCT_v_scaled_size * ssize;

    
    if (compptr->DCT_h_scaled_size > compptr->DCT_v_scaled_size * 2)
	compptr->DCT_h_scaled_size = compptr->DCT_v_scaled_size * 2;
    else if (compptr->DCT_v_scaled_size > compptr->DCT_h_scaled_size * 2)
	compptr->DCT_v_scaled_size = compptr->DCT_h_scaled_size * 2;

    
    compptr->width_in_blocks = (JDIMENSION)
      jdiv_round_up((long) cinfo->jpeg_width * (long) compptr->h_samp_factor,
		    (long) (cinfo->max_h_samp_factor * cinfo->block_size));
    compptr->height_in_blocks = (JDIMENSION)
      jdiv_round_up((long) cinfo->jpeg_height * (long) compptr->v_samp_factor,
		    (long) (cinfo->max_v_samp_factor * cinfo->block_size));
    
    compptr->downsampled_width = (JDIMENSION)
      jdiv_round_up((long) cinfo->jpeg_width *
		    (long) (compptr->h_samp_factor * compptr->DCT_h_scaled_size),
		    (long) (cinfo->max_h_samp_factor * cinfo->block_size));
    compptr->downsampled_height = (JDIMENSION)
      jdiv_round_up((long) cinfo->jpeg_height *
		    (long) (compptr->v_samp_factor * compptr->DCT_v_scaled_size),
		    (long) (cinfo->max_v_samp_factor * cinfo->block_size));
    
    compptr->component_needed = TRUE;
  }

  
  cinfo->total_iMCU_rows = (JDIMENSION)
    jdiv_round_up((long) cinfo->jpeg_height,
		  (long) (cinfo->max_v_samp_factor * cinfo->block_size));
}

#ifdef C_MULTISCAN_FILES_SUPPORTED

LOCAL(void)
validate_script (j_compress_ptr cinfo)

{
  const jpeg_scan_info * scanptr;
  int scanno, ncomps, ci, coefi, thisi;
  int Ss, Se, Ah, Al;
  boolean component_sent[MAX_COMPONENTS];
#ifdef C_PROGRESSIVE_SUPPORTED
  int * last_bitpos_ptr;
  int last_bitpos[MAX_COMPONENTS][DCTSIZE2];
  
#endif

  if (cinfo->num_scans <= 0)
    ERREXIT1(cinfo, JERR_BAD_SCAN_SCRIPT, 0);

  
  scanptr = cinfo->scan_info;
  if (scanptr->Ss != 0 || scanptr->Se != DCTSIZE2-1) {
#ifdef C_PROGRESSIVE_SUPPORTED
    cinfo->progressive_mode = TRUE;
    last_bitpos_ptr = & last_bitpos[0][0];
    for (ci = 0; ci < cinfo->num_components; ci++) 
      for (coefi = 0; coefi < DCTSIZE2; coefi++)
	*last_bitpos_ptr++ = -1;
#else
    ERREXIT(cinfo, JERR_NOT_COMPILED);
#endif
  } else {
    cinfo->progressive_mode = FALSE;
    for (ci = 0; ci < cinfo->num_components; ci++) 
      component_sent[ci] = FALSE;
  }

  for (scanno = 1; scanno <= cinfo->num_scans; scanptr++, scanno++) {
    
    ncomps = scanptr->comps_in_scan;
    if (ncomps <= 0 || ncomps > MAX_COMPS_IN_SCAN)
      ERREXIT2(cinfo, JERR_COMPONENT_COUNT, ncomps, MAX_COMPS_IN_SCAN);
    for (ci = 0; ci < ncomps; ci++) {
      thisi = scanptr->component_index[ci];
      if (thisi < 0 || thisi >= cinfo->num_components)
	ERREXIT1(cinfo, JERR_BAD_SCAN_SCRIPT, scanno);
      
      if (ci > 0 && thisi <= scanptr->component_index[ci-1])
	ERREXIT1(cinfo, JERR_BAD_SCAN_SCRIPT, scanno);
    }
    
    Ss = scanptr->Ss;
    Se = scanptr->Se;
    Ah = scanptr->Ah;
    Al = scanptr->Al;
    if (cinfo->progressive_mode) {
#ifdef C_PROGRESSIVE_SUPPORTED
      
#if BITS_IN_JSAMPLE == 8
#define MAX_AH_AL 10
#else
#define MAX_AH_AL 13
#endif
      if (Ss < 0 || Ss >= DCTSIZE2 || Se < Ss || Se >= DCTSIZE2 ||
	  Ah < 0 || Ah > MAX_AH_AL || Al < 0 || Al > MAX_AH_AL)
	ERREXIT1(cinfo, JERR_BAD_PROG_SCRIPT, scanno);
      if (Ss == 0) {
	if (Se != 0)		
	  ERREXIT1(cinfo, JERR_BAD_PROG_SCRIPT, scanno);
      } else {
	if (ncomps != 1)	
	  ERREXIT1(cinfo, JERR_BAD_PROG_SCRIPT, scanno);
      }
      for (ci = 0; ci < ncomps; ci++) {
	last_bitpos_ptr = & last_bitpos[scanptr->component_index[ci]][0];
	if (Ss != 0 && last_bitpos_ptr[0] < 0) 
	  ERREXIT1(cinfo, JERR_BAD_PROG_SCRIPT, scanno);
	for (coefi = Ss; coefi <= Se; coefi++) {
	  if (last_bitpos_ptr[coefi] < 0) {
	    
	    if (Ah != 0)
	      ERREXIT1(cinfo, JERR_BAD_PROG_SCRIPT, scanno);
	  } else {
	    
	    if (Ah != last_bitpos_ptr[coefi] || Al != Ah-1)
	      ERREXIT1(cinfo, JERR_BAD_PROG_SCRIPT, scanno);
	  }
	  last_bitpos_ptr[coefi] = Al;
	}
      }
#endif
    } else {
      
      if (Ss != 0 || Se != DCTSIZE2-1 || Ah != 0 || Al != 0)
	ERREXIT1(cinfo, JERR_BAD_PROG_SCRIPT, scanno);
      
      for (ci = 0; ci < ncomps; ci++) {
	thisi = scanptr->component_index[ci];
	if (component_sent[thisi])
	  ERREXIT1(cinfo, JERR_BAD_SCAN_SCRIPT, scanno);
	component_sent[thisi] = TRUE;
      }
    }
  }

  
  if (cinfo->progressive_mode) {
#ifdef C_PROGRESSIVE_SUPPORTED
    
    for (ci = 0; ci < cinfo->num_components; ci++) {
      if (last_bitpos[ci][0] < 0)
	ERREXIT(cinfo, JERR_MISSING_DATA);
    }
#endif
  } else {
    for (ci = 0; ci < cinfo->num_components; ci++) {
      if (! component_sent[ci])
	ERREXIT(cinfo, JERR_MISSING_DATA);
    }
  }
}

LOCAL(void)
reduce_script (j_compress_ptr cinfo)

{
  jpeg_scan_info * scanptr;
  int idxout, idxin;

  
  scanptr = (jpeg_scan_info *) cinfo->scan_info;
  idxout = 0;

  for (idxin = 0; idxin < cinfo->num_scans; idxin++) {
    
    if (idxin != idxout)
      
      scanptr[idxout] = scanptr[idxin];
    if (scanptr[idxout].Ss > cinfo->lim_Se)
      
      continue;
    if (scanptr[idxout].Se > cinfo->lim_Se)
      
      scanptr[idxout].Se = cinfo->lim_Se;
    idxout++;
  }

  cinfo->num_scans = idxout;
}

#endif 

LOCAL(void)
select_scan_parameters (j_compress_ptr cinfo)

{
  int ci;

#ifdef C_MULTISCAN_FILES_SUPPORTED
  if (cinfo->scan_info != NULL) {
    
    my_master_ptr master = (my_master_ptr) cinfo->master;
    const jpeg_scan_info * scanptr = cinfo->scan_info + master->scan_number;

    cinfo->comps_in_scan = scanptr->comps_in_scan;
    for (ci = 0; ci < scanptr->comps_in_scan; ci++) {
      cinfo->cur_comp_info[ci] =
	&cinfo->comp_info[scanptr->component_index[ci]];
    }
    if (cinfo->progressive_mode) {
      cinfo->Ss = scanptr->Ss;
      cinfo->Se = scanptr->Se;
      cinfo->Ah = scanptr->Ah;
      cinfo->Al = scanptr->Al;
      return;
    }
  }
  else
#endif
  {
    
    if (cinfo->num_components > MAX_COMPS_IN_SCAN)
      ERREXIT2(cinfo, JERR_COMPONENT_COUNT, cinfo->num_components,
	       MAX_COMPS_IN_SCAN);
    cinfo->comps_in_scan = cinfo->num_components;
    for (ci = 0; ci < cinfo->num_components; ci++) {
      cinfo->cur_comp_info[ci] = &cinfo->comp_info[ci];
    }
  }
  cinfo->Ss = 0;
  cinfo->Se = cinfo->block_size * cinfo->block_size - 1;
  cinfo->Ah = 0;
  cinfo->Al = 0;
}

LOCAL(void)
per_scan_setup (j_compress_ptr cinfo)

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
      jdiv_round_up((long) cinfo->jpeg_width,
		    (long) (cinfo->max_h_samp_factor * cinfo->block_size));
    cinfo->MCU_rows_in_scan = (JDIMENSION)
      jdiv_round_up((long) cinfo->jpeg_height,
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
      if (cinfo->blocks_in_MCU + mcublks > C_MAX_BLOCKS_IN_MCU)
	ERREXIT(cinfo, JERR_BAD_MCU_SIZE);
      while (mcublks-- > 0) {
	cinfo->MCU_membership[cinfo->blocks_in_MCU++] = ci;
      }
    }
    
  }

  
  
  if (cinfo->restart_in_rows > 0) {
    long nominal = (long) cinfo->restart_in_rows * (long) cinfo->MCUs_per_row;
    cinfo->restart_interval = (unsigned int) MIN(nominal, 65535L);
  }
}

METHODDEF(void)
prepare_for_pass (j_compress_ptr cinfo)
{
  my_master_ptr master = (my_master_ptr) cinfo->master;

  switch (master->pass_type) {
  case main_pass:
    
    select_scan_parameters(cinfo);
    per_scan_setup(cinfo);
    if (! cinfo->raw_data_in) {
      (*cinfo->cconvert->start_pass) (cinfo);
      (*cinfo->downsample->start_pass) (cinfo);
      (*cinfo->prep->start_pass) (cinfo, JBUF_PASS_THRU);
    }
    (*cinfo->fdct->start_pass) (cinfo);
    (*cinfo->entropy->start_pass) (cinfo, cinfo->optimize_coding);
    (*cinfo->coef->start_pass) (cinfo,
				(master->total_passes > 1 ?
				 JBUF_SAVE_AND_PASS : JBUF_PASS_THRU));
    (*cinfo->main->start_pass) (cinfo, JBUF_PASS_THRU);
    if (cinfo->optimize_coding) {
      
      master->pub.call_pass_startup = FALSE;
    } else {
      
      master->pub.call_pass_startup = TRUE;
    }
    break;
#ifdef ENTROPY_OPT_SUPPORTED
  case huff_opt_pass:
    
    select_scan_parameters(cinfo);
    per_scan_setup(cinfo);
    if (cinfo->Ss != 0 || cinfo->Ah == 0) {
      (*cinfo->entropy->start_pass) (cinfo, TRUE);
      (*cinfo->coef->start_pass) (cinfo, JBUF_CRANK_DEST);
      master->pub.call_pass_startup = FALSE;
      break;
    }
    
    master->pass_type = output_pass;
    master->pass_number++;
    
#endif
  case output_pass:
    
    
    if (! cinfo->optimize_coding) {
      select_scan_parameters(cinfo);
      per_scan_setup(cinfo);
    }
    (*cinfo->entropy->start_pass) (cinfo, FALSE);
    (*cinfo->coef->start_pass) (cinfo, JBUF_CRANK_DEST);
    
    if (master->scan_number == 0)
      (*cinfo->marker->write_frame_header) (cinfo);
    (*cinfo->marker->write_scan_header) (cinfo);
    master->pub.call_pass_startup = FALSE;
    break;
  default:
    ERREXIT(cinfo, JERR_NOT_COMPILED);
  }

  master->pub.is_last_pass = (master->pass_number == master->total_passes-1);

  
  if (cinfo->progress != NULL) {
    cinfo->progress->completed_passes = master->pass_number;
    cinfo->progress->total_passes = master->total_passes;
  }
}

METHODDEF(void)
pass_startup (j_compress_ptr cinfo)
{
  cinfo->master->call_pass_startup = FALSE; 

  (*cinfo->marker->write_frame_header) (cinfo);
  (*cinfo->marker->write_scan_header) (cinfo);
}

METHODDEF(void)
finish_pass_master (j_compress_ptr cinfo)
{
  my_master_ptr master = (my_master_ptr) cinfo->master;

  
  (*cinfo->entropy->finish_pass) (cinfo);

  
  switch (master->pass_type) {
  case main_pass:
    
    master->pass_type = output_pass;
    if (! cinfo->optimize_coding)
      master->scan_number++;
    break;
  case huff_opt_pass:
    
    master->pass_type = output_pass;
    break;
  case output_pass:
    
    if (cinfo->optimize_coding)
      master->pass_type = huff_opt_pass;
    master->scan_number++;
    break;
  }

  master->pass_number++;
}

GLOBAL(void)
jinit_c_master_control (j_compress_ptr cinfo, boolean transcode_only)
{
  my_master_ptr master;

  master = (my_master_ptr)
      (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
				  SIZEOF(my_comp_master));
  cinfo->master = (struct jpeg_comp_master *) master;
  master->pub.prepare_for_pass = prepare_for_pass;
  master->pub.pass_startup = pass_startup;
  master->pub.finish_pass = finish_pass_master;
  master->pub.is_last_pass = FALSE;

  
  initial_setup(cinfo, transcode_only);

  if (cinfo->scan_info != NULL) {
#ifdef C_MULTISCAN_FILES_SUPPORTED
    validate_script(cinfo);
    if (cinfo->block_size < DCTSIZE)
      reduce_script(cinfo);
#else
    ERREXIT(cinfo, JERR_NOT_COMPILED);
#endif
  } else {
    cinfo->progressive_mode = FALSE;
    cinfo->num_scans = 1;
  }

  if ((cinfo->progressive_mode || cinfo->block_size < DCTSIZE) &&
      !cinfo->arith_code)			
    
    cinfo->optimize_coding = TRUE;

  
  if (transcode_only) {
    
    if (cinfo->optimize_coding)
      master->pass_type = huff_opt_pass;
    else
      master->pass_type = output_pass;
  } else {
    
    master->pass_type = main_pass;
  }
  master->scan_number = 0;
  master->pass_number = 0;
  if (cinfo->optimize_coding)
    master->total_passes = cinfo->num_scans * 2;
  else
    master->total_passes = cinfo->num_scans;
}
