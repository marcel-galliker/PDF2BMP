
#ifndef __JP2_H
#define __JP2_H

#define JPIP_JPIP 0x6a706970

#define JP2_JP   0x6a502020		
#define JP2_FTYP 0x66747970		
#define JP2_JP2H 0x6a703268		
#define JP2_IHDR 0x69686472		
#define JP2_COLR 0x636f6c72		
#define JP2_JP2C 0x6a703263		
#define JP2_URL  0x75726c20		
#define JP2_DTBL 0x6474626c		
#define JP2_BPCC 0x62706363		
#define JP2_JP2  0x6a703220		
#define JP2_PCLR 0x70636c72		
#define JP2_CMAP 0x636d6170		
#define JP2_CDEF 0x63646566		

typedef struct opj_jp2_cdef_info
{
    unsigned short cn, typ, asoc;
} opj_jp2_cdef_info_t;

typedef struct opj_jp2_cdef
{
    opj_jp2_cdef_info_t *info;
    unsigned short n;
} opj_jp2_cdef_t;

typedef struct opj_jp2_cmap_comp
{
    unsigned short cmp;
    unsigned char mtyp, pcol;
} opj_jp2_cmap_comp_t;

typedef struct opj_jp2_pclr
{
    unsigned int *entries;
    unsigned char *channel_sign;
    unsigned char *channel_size;
    opj_jp2_cmap_comp_t *cmap;
    unsigned short nr_entries, nr_channels;
} opj_jp2_pclr_t;

typedef struct opj_jp2_color
{
    unsigned char *icc_profile_buf;
    int icc_profile_len;

    opj_jp2_cdef_t *jp2_cdef;
    opj_jp2_pclr_t *jp2_pclr;
    unsigned char jp2_has_colr;
} opj_jp2_color_t;

typedef struct opj_jp2_comps {
  int depth;		  
  int sgnd;		   
  int bpcc;
} opj_jp2_comps_t;

typedef struct opj_jp2 {
	
	opj_common_ptr cinfo;
	
	opj_j2k_t *j2k;
	unsigned int w;
	unsigned int h;
	unsigned int numcomps;
	unsigned int bpc;
	unsigned int C;
	unsigned int UnkC;
	unsigned int IPR;
	unsigned int meth;
	unsigned int approx;
	unsigned int enumcs;
	unsigned int precedence;
	unsigned int brand;
	unsigned int minversion;
	unsigned int numcl;
	unsigned int *cl;
	opj_jp2_comps_t *comps;
	unsigned int j2k_codestream_offset;
	unsigned int j2k_codestream_length;
	opj_bool jpip_on;
	opj_bool ignore_pclr_cmap_cdef;
} opj_jp2_t;

typedef struct opj_jp2_box {
  int length;
  int type;
  int init_pos;
} opj_jp2_box_t;

void jp2_write_jp2h(opj_jp2_t *jp2, opj_cio_t *cio);

opj_bool jp2_read_jp2h(opj_jp2_t *jp2, opj_cio_t *cio, opj_jp2_color_t *color);

opj_jp2_t* jp2_create_decompress(opj_common_ptr cinfo);

void jp2_destroy_decompress(opj_jp2_t *jp2);

void jp2_setup_decoder(opj_jp2_t *jp2, opj_dparameters_t *parameters);

opj_image_t* opj_jp2_decode(opj_jp2_t *jp2, opj_cio_t *cio, opj_codestream_info_t *cstr_info);

opj_jp2_t* jp2_create_compress(opj_common_ptr cinfo);

void jp2_destroy_compress(opj_jp2_t *jp2);

void jp2_setup_encoder(opj_jp2_t *jp2, opj_cparameters_t *parameters, opj_image_t *image);

opj_bool opj_jp2_encode(opj_jp2_t *jp2, opj_cio_t *cio, opj_image_t *image, opj_codestream_info_t *cstr_info);

#endif 

