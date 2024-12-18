
#include "opj_includes.h"

static opj_bool jp2_read_boxhdr(opj_common_ptr cinfo, opj_cio_t *cio, opj_jp2_box_t *box);

static opj_bool jp2_read_ihdr(opj_jp2_t *jp2, opj_cio_t *cio);
static void jp2_write_ihdr(opj_jp2_t *jp2, opj_cio_t *cio);
static void jp2_write_bpcc(opj_jp2_t *jp2, opj_cio_t *cio);
static opj_bool jp2_read_bpcc(opj_jp2_t *jp2, opj_cio_t *cio);
static void jp2_write_colr(opj_jp2_t *jp2, opj_cio_t *cio);

static void jp2_write_ftyp(opj_jp2_t *jp2, opj_cio_t *cio);

static opj_bool jp2_read_ftyp(opj_jp2_t *jp2, opj_cio_t *cio);
static int jp2_write_jp2c(opj_jp2_t *jp2, opj_cio_t *cio, opj_image_t *image, opj_codestream_info_t *cstr_info);
static opj_bool jp2_read_jp2c(opj_jp2_t *jp2, opj_cio_t *cio, unsigned int *j2k_codestream_length, unsigned int *j2k_codestream_offset);
static void jp2_write_jp(opj_cio_t *cio);

static opj_bool jp2_read_jp(opj_jp2_t *jp2, opj_cio_t *cio);

static opj_bool jp2_read_struct(opj_jp2_t *jp2, opj_cio_t *cio,
	opj_jp2_color_t *color);

static void jp2_apply_pclr(opj_jp2_color_t *color, opj_image_t *image, opj_common_ptr cinfo);

static opj_bool jp2_read_pclr(opj_jp2_t *jp2, opj_cio_t *cio,
    opj_jp2_box_t *box, opj_jp2_color_t *color);

static opj_bool jp2_read_cmap(opj_jp2_t *jp2, opj_cio_t *cio,
    opj_jp2_box_t *box, opj_jp2_color_t *color);

static opj_bool jp2_read_colr(opj_jp2_t *jp2, opj_cio_t *cio,
    opj_jp2_box_t *box, opj_jp2_color_t *color);

static int write_fidx( int offset_jp2c, int length_jp2c, int offset_idx, int length_idx, opj_cio_t *cio);

static void write_iptr( int offset, int length, opj_cio_t *cio);

static void write_prxy( int offset_jp2c, int length_jp2c, int offset_idx, int length_idx, opj_cio_t *cio);

static opj_bool jp2_read_boxhdr(opj_common_ptr cinfo, opj_cio_t *cio, opj_jp2_box_t *box) {
	box->init_pos = cio_tell(cio);
	box->length = cio_read(cio, 4);
	box->type = cio_read(cio, 4);
	if (box->length == 1) {
		if (cio_read(cio, 4) != 0) {
			opj_event_msg(cinfo, EVT_ERROR, "Cannot handle box sizes higher than 2^32\n");
			return OPJ_FALSE;
		}
		box->length = cio_read(cio, 4);
		if (box->length == 0) 
			box->length = cio_numbytesleft(cio) + 12;
	}
	else if (box->length == 0) {
		box->length = cio_numbytesleft(cio) + 8;
	}
	
	return OPJ_TRUE;
}

#if 0
static void jp2_write_url(opj_cio_t *cio, char *Idx_file) {
	unsigned int i;
	opj_jp2_box_t box;

	box.init_pos = cio_tell(cio);
	cio_skip(cio, 4);
	cio_write(cio, JP2_URL, 4);	
	cio_write(cio, 0, 1);		
	cio_write(cio, 0, 3);		

	if(Idx_file) {
		for (i = 0; i < strlen(Idx_file); i++) {
			cio_write(cio, Idx_file[i], 1);
		}
	}

	box.length = cio_tell(cio) - box.init_pos;
	cio_seek(cio, box.init_pos);
	cio_write(cio, box.length, 4);	
	cio_seek(cio, box.init_pos + box.length);
}
#endif

static opj_bool jp2_read_ihdr(opj_jp2_t *jp2, opj_cio_t *cio) {
	opj_jp2_box_t box;

	opj_common_ptr cinfo = jp2->cinfo;

	jp2_read_boxhdr(cinfo, cio, &box);
	if (JP2_IHDR != box.type) {
		opj_event_msg(cinfo, EVT_ERROR, "Expected IHDR Marker\n");
		return OPJ_FALSE;
	}

	jp2->h = cio_read(cio, 4);			
	jp2->w = cio_read(cio, 4);			
	jp2->numcomps = cio_read(cio, 2);	
	jp2->comps = (opj_jp2_comps_t*) opj_malloc(jp2->numcomps * sizeof(opj_jp2_comps_t));

	jp2->bpc = cio_read(cio, 1);		

	jp2->C = cio_read(cio, 1);			
	jp2->UnkC = cio_read(cio, 1);		
	jp2->IPR = cio_read(cio, 1);		

	if (cio_tell(cio) - box.init_pos != box.length) {
		opj_event_msg(cinfo, EVT_ERROR, "Error with IHDR Box\n");
		return OPJ_FALSE;
	}

	return OPJ_TRUE;
}

static void jp2_write_ihdr(opj_jp2_t *jp2, opj_cio_t *cio) {
	opj_jp2_box_t box;

	box.init_pos = cio_tell(cio);
	cio_skip(cio, 4);
	cio_write(cio, JP2_IHDR, 4);		

	cio_write(cio, jp2->h, 4);			
	cio_write(cio, jp2->w, 4);			
	cio_write(cio, jp2->numcomps, 2);	

	cio_write(cio, jp2->bpc, 1);		

	cio_write(cio, jp2->C, 1);			
	cio_write(cio, jp2->UnkC, 1);		
	cio_write(cio, jp2->IPR, 1);		

	box.length = cio_tell(cio) - box.init_pos;
	cio_seek(cio, box.init_pos);
	cio_write(cio, box.length, 4);	
	cio_seek(cio, box.init_pos + box.length);
}

static void jp2_write_bpcc(opj_jp2_t *jp2, opj_cio_t *cio) {
	unsigned int i;
	opj_jp2_box_t box;

	box.init_pos = cio_tell(cio);
	cio_skip(cio, 4);
	cio_write(cio, JP2_BPCC, 4);	

	for (i = 0; i < jp2->numcomps; i++) {
		cio_write(cio, jp2->comps[i].bpcc, 1);
	}

	box.length = cio_tell(cio) - box.init_pos;
	cio_seek(cio, box.init_pos);
	cio_write(cio, box.length, 4);	
	cio_seek(cio, box.init_pos + box.length);
}

static opj_bool jp2_read_bpcc(opj_jp2_t *jp2, opj_cio_t *cio) {
	unsigned int i;
	opj_jp2_box_t box;

	opj_common_ptr cinfo = jp2->cinfo;

	jp2_read_boxhdr(cinfo, cio, &box);
	if (JP2_BPCC != box.type) {
		opj_event_msg(cinfo, EVT_ERROR, "Expected BPCC Marker\n");
		return OPJ_FALSE;
	}

	for (i = 0; i < jp2->numcomps; i++) {
		jp2->comps[i].bpcc = cio_read(cio, 1);
	}

	if (cio_tell(cio) - box.init_pos != box.length) {
		opj_event_msg(cinfo, EVT_ERROR, "Error with BPCC Box\n");
		return OPJ_FALSE;
	}

	return OPJ_TRUE;
}

static void jp2_write_colr(opj_jp2_t *jp2, opj_cio_t *cio) {
	opj_jp2_box_t box;

	box.init_pos = cio_tell(cio);
	cio_skip(cio, 4);
	cio_write(cio, JP2_COLR, 4);		

	cio_write(cio, jp2->meth, 1);		
	cio_write(cio, jp2->precedence, 1);	
	cio_write(cio, jp2->approx, 1);		

	if(jp2->meth == 2)
	 jp2->enumcs = 0;

	cio_write(cio, jp2->enumcs, 4);	

	box.length = cio_tell(cio) - box.init_pos;
	cio_seek(cio, box.init_pos);
	cio_write(cio, box.length, 4);	
	cio_seek(cio, box.init_pos + box.length);
}

static void jp2_free_pclr(opj_jp2_color_t *color)
{
    opj_free(color->jp2_pclr->channel_sign);
    opj_free(color->jp2_pclr->channel_size);
    opj_free(color->jp2_pclr->entries);

	if(color->jp2_pclr->cmap) opj_free(color->jp2_pclr->cmap);

    opj_free(color->jp2_pclr); color->jp2_pclr = NULL;
}

static void free_color_data(opj_jp2_color_t *color)
{
	if(color->jp2_pclr)
   {
	jp2_free_pclr(color);
   }
	if(color->jp2_cdef) 
   {
	if(color->jp2_cdef->info) opj_free(color->jp2_cdef->info);
	opj_free(color->jp2_cdef);
   }
	if(color->icc_profile_buf) opj_free(color->icc_profile_buf);
}

static void jp2_apply_pclr(opj_jp2_color_t *color, opj_image_t *image, opj_common_ptr cinfo)
{
	opj_image_comp_t *old_comps, *new_comps;
	unsigned char *channel_size, *channel_sign;
	unsigned int *entries;
	opj_jp2_cmap_comp_t *cmap;
	int *src, *dst;
	unsigned int j, max;
	unsigned short i, nr_channels, cmp, pcol;
	int k, top_k;

	channel_size = color->jp2_pclr->channel_size;
	channel_sign = color->jp2_pclr->channel_sign;
	entries = color->jp2_pclr->entries;
	cmap = color->jp2_pclr->cmap;
	nr_channels = color->jp2_pclr->nr_channels;

	old_comps = image->comps;
	new_comps = (opj_image_comp_t*)
	 opj_malloc(nr_channels * sizeof(opj_image_comp_t));

	for(i = 0; i < nr_channels; ++i)
   {
	pcol = cmap[i].pcol; cmp = cmap[i].cmp;

  if( pcol < nr_channels )
    new_comps[pcol] = old_comps[cmp];
  else
    {
    opj_event_msg(cinfo, EVT_ERROR, "Error with pcol value %d (max: %d). skipping\n", pcol, nr_channels);
    continue;
    }

	if(cmap[i].mtyp == 0) 
  {
	old_comps[cmp].data = NULL; continue;
  }

	new_comps[pcol].data = (int*)
	 opj_malloc(old_comps[cmp].w * old_comps[cmp].h * sizeof(int));
	new_comps[pcol].prec = channel_size[i];
	new_comps[pcol].sgnd = channel_sign[i];
   }
	top_k = color->jp2_pclr->nr_entries - 1;

	for(i = 0; i < nr_channels; ++i)
   {

	if(cmap[i].mtyp == 0) continue;

	cmp = cmap[i].cmp; pcol = cmap[i].pcol;
	src = old_comps[cmp].data; 
	dst = new_comps[pcol].data;
	max = new_comps[pcol].w * new_comps[pcol].h;

	for(j = 0; j < max; ++j)
  {

	if((k = src[j]) < 0) k = 0; else if(k > top_k) k = top_k;

	dst[j] = entries[k * nr_channels + pcol];
  }
   }
	max = image->numcomps;
	for(i = 0; i < max; ++i)
   {
	if(old_comps[i].data) opj_free(old_comps[i].data);
   }
	opj_free(old_comps);
	image->comps = new_comps;
	image->numcomps = nr_channels;

	jp2_free_pclr(color);

}

static opj_bool jp2_read_pclr(opj_jp2_t *jp2, opj_cio_t *cio,
	opj_jp2_box_t *box, opj_jp2_color_t *color)
{
	opj_jp2_pclr_t *jp2_pclr;
	unsigned char *channel_size, *channel_sign;
	unsigned int *entries;
	unsigned short nr_entries, nr_channels;
	unsigned short i, j;
	unsigned char uc;

	OPJ_ARG_NOT_USED(box);
	OPJ_ARG_NOT_USED(jp2);

	if(color->jp2_pclr) return OPJ_FALSE;

	nr_entries = (unsigned short)cio_read(cio, 2); 
	nr_channels = (unsigned short)cio_read(cio, 1);

	entries = (unsigned int*)
	 opj_malloc(nr_channels * nr_entries * sizeof(unsigned int));
	channel_size = (unsigned char*)opj_malloc(nr_channels);
	channel_sign = (unsigned char*)opj_malloc(nr_channels);

	jp2_pclr = (opj_jp2_pclr_t*)opj_malloc(sizeof(opj_jp2_pclr_t));
	jp2_pclr->channel_sign = channel_sign;
	jp2_pclr->channel_size = channel_size;
	jp2_pclr->entries = entries;
	jp2_pclr->nr_entries = nr_entries;
	jp2_pclr->nr_channels = nr_channels;
	jp2_pclr->cmap = NULL;

	color->jp2_pclr = jp2_pclr;

	for(i = 0; i < nr_channels; ++i)
   {
	uc = cio_read(cio, 1); 
	channel_size[i] = (uc & 0x7f) + 1;
	channel_sign[i] = (uc & 0x80)?1:0;
   }

	for(j = 0; j < nr_entries; ++j)
   {
	for(i = 0; i < nr_channels; ++i)
  {

	*entries++ = cio_read(cio, channel_size[i]>>3);
  }
   }

	return OPJ_TRUE;
}

static opj_bool jp2_read_cmap(opj_jp2_t *jp2, opj_cio_t *cio,
	opj_jp2_box_t *box, opj_jp2_color_t *color)
{
	opj_jp2_cmap_comp_t *cmap;
	unsigned short i, nr_channels;

	OPJ_ARG_NOT_USED(box);
	OPJ_ARG_NOT_USED(jp2);

	if(color->jp2_pclr == NULL) return OPJ_FALSE;

	if(color->jp2_pclr->cmap) return OPJ_FALSE;

	nr_channels = color->jp2_pclr->nr_channels;
	cmap = (opj_jp2_cmap_comp_t*)
	 opj_malloc(nr_channels * sizeof(opj_jp2_cmap_comp_t));

	for(i = 0; i < nr_channels; ++i)
   {
	cmap[i].cmp = (unsigned short)cio_read(cio, 2);
	cmap[i].mtyp = cio_read(cio, 1);
	cmap[i].pcol = cio_read(cio, 1);

   }
	color->jp2_pclr->cmap = cmap;

	return OPJ_TRUE;
}

static void jp2_apply_cdef(opj_image_t *image, opj_jp2_color_t *color)
{
	opj_jp2_cdef_info_t *info;
	int color_space;
	unsigned short i, n, cn, typ, asoc, acn;

	color_space = image->color_space;
	info = color->jp2_cdef->info;
	n = color->jp2_cdef->n;

	for(i = 0; i < n; ++i)
   {

	if((asoc = info[i].asoc) == 0) continue;

	cn = info[i].cn; typ = info[i].typ; acn = asoc - 1;

	if(cn != acn)
  {
	opj_image_comp_t saved;

	memcpy(&saved, &image->comps[cn], sizeof(opj_image_comp_t));
	memcpy(&image->comps[cn], &image->comps[acn], sizeof(opj_image_comp_t));
	memcpy(&image->comps[acn], &saved, sizeof(opj_image_comp_t));

	info[i].asoc = cn + 1;
	info[acn].asoc = info[acn].cn + 1;
  }
   }
	if(color->jp2_cdef->info) opj_free(color->jp2_cdef->info);

	opj_free(color->jp2_cdef); color->jp2_cdef = NULL;

}

static opj_bool jp2_read_cdef(opj_jp2_t *jp2, opj_cio_t *cio,
	opj_jp2_box_t *box, opj_jp2_color_t *color)
{
	opj_jp2_cdef_info_t *info;
	unsigned short i, n;

	OPJ_ARG_NOT_USED(box);
	OPJ_ARG_NOT_USED(jp2);

	if(color->jp2_cdef) return OPJ_FALSE;

	if((n = (unsigned short)cio_read(cio, 2)) == 0) return OPJ_FALSE; 

	info = (opj_jp2_cdef_info_t*)
	 opj_malloc(n * sizeof(opj_jp2_cdef_info_t));

	color->jp2_cdef = (opj_jp2_cdef_t*)opj_malloc(sizeof(opj_jp2_cdef_t));
	color->jp2_cdef->info = info;
	color->jp2_cdef->n = n;

	for(i = 0; i < n; ++i)
   {
	info[i].cn = (unsigned short)cio_read(cio, 2);
	info[i].typ = (unsigned short)cio_read(cio, 2);
	info[i].asoc = (unsigned short)cio_read(cio, 2);

   }
	return OPJ_TRUE;
}

static opj_bool jp2_read_colr(opj_jp2_t *jp2, opj_cio_t *cio,
	opj_jp2_box_t *box, opj_jp2_color_t *color) 
{
	int skip_len;
    opj_common_ptr cinfo;

	if(color->jp2_has_colr) return OPJ_FALSE;

	cinfo = jp2->cinfo;

	jp2->meth = cio_read(cio, 1);		
	jp2->precedence = cio_read(cio, 1);	
	jp2->approx = cio_read(cio, 1);		

	if (jp2->meth == 1) 
   {
	jp2->enumcs = cio_read(cio, 4);	
   } 
	else
	if (jp2->meth == 2) 
   {

	skip_len = box->init_pos + box->length - cio_tell(cio);
	if (skip_len < 0) 
  {
	opj_event_msg(cinfo, EVT_ERROR, "Error with COLR box size\n");
	return OPJ_FALSE;
  }
	if(skip_len > 0)
  {
	unsigned char *start;

	start = cio_getbp(cio);
	color->icc_profile_buf = (unsigned char*)opj_malloc(skip_len);
	color->icc_profile_len = skip_len;

	cio_skip(cio, box->init_pos + box->length - cio_tell(cio));

	memcpy(color->icc_profile_buf, start, skip_len);
  }
   }

	if (cio_tell(cio) - box->init_pos != box->length) 
   {
	opj_event_msg(cinfo, EVT_ERROR, "Error with COLR Box\n");
	return OPJ_FALSE;
   }
	color->jp2_has_colr = 1;

	return OPJ_TRUE;
}

opj_bool jp2_read_jp2h(opj_jp2_t *jp2, opj_cio_t *cio, opj_jp2_color_t *color) 
{
	opj_jp2_box_t box;
	int jp2h_end;

	opj_common_ptr cinfo = jp2->cinfo;

	jp2_read_boxhdr(cinfo, cio, &box);
	do 
   {
	if (JP2_JP2H != box.type) 
  {
	if (box.type == JP2_JP2C) 
 {
	opj_event_msg(cinfo, EVT_ERROR, "Expected JP2H Marker\n");
	return OPJ_FALSE;
 }
	cio_skip(cio, box.length - 8);

	if(cio->bp >= cio->end) return OPJ_FALSE;

	jp2_read_boxhdr(cinfo, cio, &box);
  }
   } while(JP2_JP2H != box.type);

	if (!jp2_read_ihdr(jp2, cio))
		return OPJ_FALSE;
	jp2h_end = box.init_pos + box.length;

	if (jp2->bpc == 255) 
   {
	if (!jp2_read_bpcc(jp2, cio))
		return OPJ_FALSE;
   }
	jp2_read_boxhdr(cinfo, cio, &box);

	while(cio_tell(cio) < jp2h_end)
   {
	if(box.type == JP2_COLR)
  {
	if( !jp2_read_colr(jp2, cio, &box, color))
 {
    cio_seek(cio, box.init_pos + 8);
    cio_skip(cio, box.length - 8);
 }
    jp2_read_boxhdr(cinfo, cio, &box);
    continue;
  }
    if(box.type == JP2_CDEF && !jp2->ignore_pclr_cmap_cdef)
  {
    if( !jp2_read_cdef(jp2, cio, &box, color))
 {
    cio_seek(cio, box.init_pos + 8);
    cio_skip(cio, box.length - 8);
 }
    jp2_read_boxhdr(cinfo, cio, &box);
    continue;
  }
    if(box.type == JP2_PCLR && !jp2->ignore_pclr_cmap_cdef)
  {
    if( !jp2_read_pclr(jp2, cio, &box, color))
 {
    cio_seek(cio, box.init_pos + 8);
    cio_skip(cio, box.length - 8);
 }
    jp2_read_boxhdr(cinfo, cio, &box);
    continue;
  }
    if(box.type == JP2_CMAP && !jp2->ignore_pclr_cmap_cdef)
  {
    if( !jp2_read_cmap(jp2, cio, &box, color))
 {
    cio_seek(cio, box.init_pos + 8);
    cio_skip(cio, box.length - 8);
 }
    jp2_read_boxhdr(cinfo, cio, &box);
    continue;
  }
	cio_seek(cio, box.init_pos + 8);
	cio_skip(cio, box.length - 8);
	jp2_read_boxhdr(cinfo, cio, &box);

   }

	cio_seek(cio, jp2h_end);

	return (color->jp2_has_colr == 1);

}

opj_image_t* opj_jp2_decode(opj_jp2_t *jp2, opj_cio_t *cio, 
	opj_codestream_info_t *cstr_info) 
{
	opj_common_ptr cinfo;
	opj_image_t *image = NULL;
	opj_jp2_color_t color;

	if(!jp2 || !cio) 
   {
	return NULL;
   }
	memset(&color, 0, sizeof(opj_jp2_color_t));
	cinfo = jp2->cinfo;

	if(!jp2_read_struct(jp2, cio, &color)) 
   {
	free_color_data(&color);
	opj_event_msg(cinfo, EVT_ERROR, "Failed to decode jp2 structure\n");
	return NULL;
   }

	image = j2k_decode(jp2->j2k, cio, cstr_info);

	if(!image) 
   {
	free_color_data(&color);
	opj_event_msg(cinfo, EVT_ERROR, "Failed to decode J2K image\n");
	return NULL;
   }
   
    if (!jp2->ignore_pclr_cmap_cdef){

    
	if (jp2->enumcs == 16)
		image->color_space = CLRSPC_SRGB;
	else if (jp2->enumcs == 17)
		image->color_space = CLRSPC_GRAY;
	else if (jp2->enumcs == 18)
		image->color_space = CLRSPC_SYCC;
	else
		image->color_space = CLRSPC_UNKNOWN;

	if(color.jp2_cdef)
   {
	jp2_apply_cdef(image, &color);
   }
	if(color.jp2_pclr)
   {

	if( !color.jp2_pclr->cmap) 
	 jp2_free_pclr(&color);
	else
	 jp2_apply_pclr(&color, image, cinfo);
   }
	if(color.icc_profile_buf)
   {
	image->icc_profile_buf = color.icc_profile_buf;
	color.icc_profile_buf = NULL;
	image->icc_profile_len = color.icc_profile_len;
   }
   }
   
	return image;

}

void jp2_write_jp2h(opj_jp2_t *jp2, opj_cio_t *cio) {
	opj_jp2_box_t box;

	box.init_pos = cio_tell(cio);
	cio_skip(cio, 4);
	cio_write(cio, JP2_JP2H, 4);	

	jp2_write_ihdr(jp2, cio);

	if (jp2->bpc == 255) {
		jp2_write_bpcc(jp2, cio);
	}
	jp2_write_colr(jp2, cio);

	box.length = cio_tell(cio) - box.init_pos;
	cio_seek(cio, box.init_pos);
	cio_write(cio, box.length, 4);	
	cio_seek(cio, box.init_pos + box.length);
}

static void jp2_write_ftyp(opj_jp2_t *jp2, opj_cio_t *cio) {
	unsigned int i;
	opj_jp2_box_t box;

	box.init_pos = cio_tell(cio);
	cio_skip(cio, 4);
	cio_write(cio, JP2_FTYP, 4);		

	cio_write(cio, jp2->brand, 4);		
	cio_write(cio, jp2->minversion, 4);	

	for (i = 0; i < jp2->numcl; i++) {
		cio_write(cio, jp2->cl[i], 4);	
	}

	box.length = cio_tell(cio) - box.init_pos;
	cio_seek(cio, box.init_pos);
	cio_write(cio, box.length, 4);	
	cio_seek(cio, box.init_pos + box.length);
}

static opj_bool jp2_read_ftyp(opj_jp2_t *jp2, opj_cio_t *cio) {
	int i;
	opj_jp2_box_t box;

	opj_common_ptr cinfo = jp2->cinfo;

	jp2_read_boxhdr(cinfo, cio, &box);

	if (JP2_FTYP != box.type) {
		opj_event_msg(cinfo, EVT_ERROR, "Expected FTYP Marker\n");
		return OPJ_FALSE;
	}

	jp2->brand = cio_read(cio, 4);		
	jp2->minversion = cio_read(cio, 4);	
	jp2->numcl = (box.length - 16) / 4;
	jp2->cl = (unsigned int *) opj_malloc(jp2->numcl * sizeof(unsigned int));

	for (i = 0; i < (int)jp2->numcl; i++) {
		jp2->cl[i] = cio_read(cio, 4);	
	}

	if (cio_tell(cio) - box.init_pos != box.length) {
		opj_event_msg(cinfo, EVT_ERROR, "Error with FTYP Box\n");
		return OPJ_FALSE;
	}

	return OPJ_TRUE;
}

static int jp2_write_jp2c(opj_jp2_t *jp2, opj_cio_t *cio, opj_image_t *image, opj_codestream_info_t *cstr_info) {
	unsigned int j2k_codestream_offset, j2k_codestream_length;
	opj_jp2_box_t box;

	opj_j2k_t *j2k = jp2->j2k;

	box.init_pos = cio_tell(cio);
	cio_skip(cio, 4);
	cio_write(cio, JP2_JP2C, 4);	

	
	j2k_codestream_offset = cio_tell(cio);
	if(!j2k_encode(j2k, cio, image, cstr_info)) {
		opj_event_msg(j2k->cinfo, EVT_ERROR, "Failed to encode image\n");
		return 0;
	}
	j2k_codestream_length = cio_tell(cio) - j2k_codestream_offset;

	jp2->j2k_codestream_offset = j2k_codestream_offset;
	jp2->j2k_codestream_length = j2k_codestream_length;

	box.length = 8 + jp2->j2k_codestream_length;
	cio_seek(cio, box.init_pos);
	cio_write(cio, box.length, 4);	
	cio_seek(cio, box.init_pos + box.length);

	return box.length;
}

static opj_bool jp2_read_jp2c(opj_jp2_t *jp2, opj_cio_t *cio, unsigned int *j2k_codestream_length, unsigned int *j2k_codestream_offset) {
	opj_jp2_box_t box;

	opj_common_ptr cinfo = jp2->cinfo;

	jp2_read_boxhdr(cinfo, cio, &box);
	do {
		if(JP2_JP2C != box.type) {
			cio_skip(cio, box.length - 8);
			jp2_read_boxhdr(cinfo, cio, &box);
		}
	} while(JP2_JP2C != box.type);

	*j2k_codestream_offset = cio_tell(cio);
	*j2k_codestream_length = box.length - 8;

	return OPJ_TRUE;
}

static void jp2_write_jp(opj_cio_t *cio) {
	opj_jp2_box_t box;

	box.init_pos = cio_tell(cio);
	cio_skip(cio, 4);
	cio_write(cio, JP2_JP, 4);		
	cio_write(cio, 0x0d0a870a, 4);

	box.length = cio_tell(cio) - box.init_pos;
	cio_seek(cio, box.init_pos);
	cio_write(cio, box.length, 4);	
	cio_seek(cio, box.init_pos + box.length);
}

static opj_bool jp2_read_jp(opj_jp2_t *jp2, opj_cio_t *cio) {
	opj_jp2_box_t box;

	opj_common_ptr cinfo = jp2->cinfo;

	jp2_read_boxhdr(cinfo, cio, &box);
	if (JP2_JP != box.type) {
		opj_event_msg(cinfo, EVT_ERROR, "Expected JP Marker\n");
		return OPJ_FALSE;
	}
	if (0x0d0a870a != cio_read(cio, 4)) {
		opj_event_msg(cinfo, EVT_ERROR, "Error with JP Marker\n");
		return OPJ_FALSE;
	}
	if (cio_tell(cio) - box.init_pos != box.length) {
		opj_event_msg(cinfo, EVT_ERROR, "Error with JP Box size\n");
		return OPJ_FALSE;
	}

	return OPJ_TRUE;
}

static opj_bool jp2_read_struct(opj_jp2_t *jp2, opj_cio_t *cio,
	opj_jp2_color_t *color) {
	if (!jp2_read_jp(jp2, cio))
		return OPJ_FALSE;
	if (!jp2_read_ftyp(jp2, cio))
		return OPJ_FALSE;
	if (!jp2_read_jp2h(jp2, cio, color))
		return OPJ_FALSE;
	if (!jp2_read_jp2c(jp2, cio, &jp2->j2k_codestream_length, &jp2->j2k_codestream_offset))
		return OPJ_FALSE;
	
	return OPJ_TRUE;
}

static int write_fidx( int offset_jp2c, int length_jp2c, int offset_idx, int length_idx, opj_cio_t *cio)
{  
  int len, lenp;
  
  lenp = cio_tell( cio);
  cio_skip( cio, 4);              
  cio_write( cio, JPIP_FIDX, 4);  
  
  write_prxy( offset_jp2c, length_jp2c, offset_idx, length_idx, cio);

  len = cio_tell( cio)-lenp;
  cio_seek( cio, lenp);
  cio_write( cio, len, 4);        
  cio_seek( cio, lenp+len);  

  return len;
}

static void write_prxy( int offset_jp2c, int length_jp2c, int offset_idx, int length_idx, opj_cio_t *cio)
{
  int len, lenp;

  lenp = cio_tell( cio);
  cio_skip( cio, 4);              
  cio_write( cio, JPIP_PRXY, 4);  
  
  cio_write( cio, offset_jp2c, 8); 
  cio_write( cio, length_jp2c, 4); 
  cio_write( cio, JP2_JP2C, 4);        
  
  cio_write( cio, 1,1);           

  cio_write( cio, offset_idx, 8);  
  cio_write( cio, length_idx, 4);  
  cio_write( cio, JPIP_CIDX, 4);   

  len = cio_tell( cio)-lenp;
  cio_seek( cio, lenp);
  cio_write( cio, len, 4);        
  cio_seek( cio, lenp+len);
}

static void write_iptr( int offset, int length, opj_cio_t *cio)
{
  int len, lenp;
  
  lenp = cio_tell( cio);
  cio_skip( cio, 4);              
  cio_write( cio, JPIP_IPTR, 4);  
  
  cio_write( cio, offset, 8);
  cio_write( cio, length, 8);

  len = cio_tell( cio)-lenp;
  cio_seek( cio, lenp);
  cio_write( cio, len, 4);        
  cio_seek( cio, lenp+len);
}

opj_jp2_t* jp2_create_decompress(opj_common_ptr cinfo) {
	opj_jp2_t *jp2 = (opj_jp2_t*) opj_calloc(1, sizeof(opj_jp2_t));
	if(jp2) {
		jp2->cinfo = cinfo;
		
		jp2->j2k = j2k_create_decompress(cinfo);
		if(jp2->j2k == NULL) {
			jp2_destroy_decompress(jp2);
			return NULL;
		}
	}
	return jp2;
}

void jp2_destroy_decompress(opj_jp2_t *jp2) {
	if(jp2) {
		
		j2k_destroy_decompress(jp2->j2k);

		if(jp2->comps) {
			opj_free(jp2->comps);
		}
		if(jp2->cl) {
			opj_free(jp2->cl);
		}
		opj_free(jp2);
	}
}

void jp2_setup_decoder(opj_jp2_t *jp2, opj_dparameters_t *parameters) {
	
	j2k_setup_decoder(jp2->j2k, parameters);
	
	jp2->ignore_pclr_cmap_cdef = parameters->flags & OPJ_DPARAMETERS_IGNORE_PCLR_CMAP_CDEF_FLAG;
}

opj_jp2_t* jp2_create_compress(opj_common_ptr cinfo) {
	opj_jp2_t *jp2 = (opj_jp2_t*)opj_malloc(sizeof(opj_jp2_t));
	if(jp2) {
		jp2->cinfo = cinfo;
		
		jp2->j2k = j2k_create_compress(cinfo);
		if(jp2->j2k == NULL) {
			jp2_destroy_compress(jp2);
			return NULL;
		}
	}
	return jp2;
}

void jp2_destroy_compress(opj_jp2_t *jp2) {
	if(jp2) {
		
		j2k_destroy_compress(jp2->j2k);

		if(jp2->comps) {
			opj_free(jp2->comps);
		}
		if(jp2->cl) {
			opj_free(jp2->cl);
		}
		opj_free(jp2);
	}
}

void jp2_setup_encoder(opj_jp2_t *jp2, opj_cparameters_t *parameters, opj_image_t *image) {
	int i;
	int depth_0, sign;

	if(!jp2 || !parameters || !image)
		return;

	
	

	
	if (image->numcomps < 1 || image->numcomps > 16384) {
		opj_event_msg(jp2->cinfo, EVT_ERROR, "Invalid number of components specified while setting up JP2 encoder\n");
		return;
	}

	j2k_setup_encoder(jp2->j2k, parameters, image);

	
	
	
	

	jp2->brand = JP2_JP2;	
	jp2->minversion = 0;	
	jp2->numcl = 1;
	jp2->cl = (unsigned int*) opj_malloc(jp2->numcl * sizeof(unsigned int));
	jp2->cl[0] = JP2_JP2;	

	

	jp2->numcomps = image->numcomps;	
	jp2->comps = (opj_jp2_comps_t*) opj_malloc(jp2->numcomps * sizeof(opj_jp2_comps_t));
	jp2->h = image->y1 - image->y0;		
	jp2->w = image->x1 - image->x0;		
	
	depth_0 = image->comps[0].prec - 1;
	sign = image->comps[0].sgnd;
	jp2->bpc = depth_0 + (sign << 7);
	for (i = 1; i < image->numcomps; i++) {
		int depth = image->comps[i].prec - 1;
		sign = image->comps[i].sgnd;
		if (depth_0 != depth)
			jp2->bpc = 255;
	}
	jp2->C = 7;			
	jp2->UnkC = 0;		
	jp2->IPR = 0;		
	
	

	for (i = 0; i < image->numcomps; i++) {
		jp2->comps[i].bpcc = image->comps[i].prec - 1 + (image->comps[i].sgnd << 7);
	}
	jp2->meth = 1;
	if (image->color_space == 1)
		jp2->enumcs = 16;	
	else if (image->color_space == 2)
		jp2->enumcs = 17;	
	else if (image->color_space == 3)
		jp2->enumcs = 18;	
	jp2->precedence = 0;	
	jp2->approx = 0;		
	
	jp2->jpip_on = parameters->jpip_on;
}

opj_bool opj_jp2_encode(opj_jp2_t *jp2, opj_cio_t *cio, opj_image_t *image, opj_codestream_info_t *cstr_info) {

	int pos_iptr, pos_cidx, pos_jp2c, len_jp2c, len_cidx, end_pos, pos_fidx, len_fidx;
	pos_jp2c = pos_iptr = -1; 

	

	
	jp2_write_jp(cio);
	
	jp2_write_ftyp(jp2, cio);
	
	jp2_write_jp2h(jp2, cio);

	if( jp2->jpip_on){
	  pos_iptr = cio_tell( cio);
	  cio_skip( cio, 24); 
	  
	  pos_jp2c = cio_tell( cio);
	}

	
	if(!(len_jp2c = jp2_write_jp2c( jp2, cio, image, cstr_info))){
	    opj_event_msg(jp2->cinfo, EVT_ERROR, "Failed to encode image\n");
	    return OPJ_FALSE;
	}

	if( jp2->jpip_on){
	  pos_cidx = cio_tell( cio);
	  
	  len_cidx = write_cidx( pos_jp2c+8, cio, image, *cstr_info, len_jp2c-8);
	  
	  pos_fidx = cio_tell( cio);
	  len_fidx = write_fidx( pos_jp2c, len_jp2c, pos_cidx, len_cidx, cio);
	  
	  end_pos = cio_tell( cio);
	  
	  cio_seek( cio, pos_iptr);
	  write_iptr( pos_fidx, len_fidx, cio);
	  
	  cio_seek( cio, end_pos);
	}

	return OPJ_TRUE;
}
