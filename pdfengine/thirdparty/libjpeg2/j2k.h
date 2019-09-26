
#ifndef __J2K_H
#define __J2K_H

#define J2K_CP_CSTY_PRT 0x01
#define J2K_CP_CSTY_SOP 0x02
#define J2K_CP_CSTY_EPH 0x04
#define J2K_CCP_CSTY_PRT 0x01
#define J2K_CCP_CBLKSTY_LAZY 0x01     
#define J2K_CCP_CBLKSTY_RESET 0x02    
#define J2K_CCP_CBLKSTY_TERMALL 0x04  
#define J2K_CCP_CBLKSTY_VSC 0x08      
#define J2K_CCP_CBLKSTY_PTERM 0x10    
#define J2K_CCP_CBLKSTY_SEGSYM 0x20   
#define J2K_CCP_QNTSTY_NOQNT 0
#define J2K_CCP_QNTSTY_SIQNT 1
#define J2K_CCP_QNTSTY_SEQNT 2

#define J2K_MS_SOC 0xff4f	
#define J2K_MS_SOT 0xff90	
#define J2K_MS_SOD 0xff93	
#define J2K_MS_EOC 0xffd9	
#define J2K_MS_SIZ 0xff51	
#define J2K_MS_COD 0xff52	
#define J2K_MS_COC 0xff53	
#define J2K_MS_RGN 0xff5e	
#define J2K_MS_QCD 0xff5c	
#define J2K_MS_QCC 0xff5d	
#define J2K_MS_POC 0xff5f	
#define J2K_MS_TLM 0xff55	
#define J2K_MS_PLM 0xff57	
#define J2K_MS_PLT 0xff58	
#define J2K_MS_PPM 0xff60	
#define J2K_MS_PPT 0xff61	
#define J2K_MS_SOP 0xff91	
#define J2K_MS_EPH 0xff92	
#define J2K_MS_CRG 0xff63	
#define J2K_MS_COM 0xff64	

#ifdef USE_JPWL
#define J2K_MS_EPC 0xff68	
#define J2K_MS_EPB 0xff66	 
#define J2K_MS_ESD 0xff67	 
#define J2K_MS_RED 0xff69	
#endif 
#ifdef USE_JPSEC
#define J2K_MS_SEC 0xff65    
#define J2K_MS_INSEC 0xff94  
#endif 

typedef enum J2K_STATUS {
	J2K_STATE_MHSOC  = 0x0001, 
	J2K_STATE_MHSIZ  = 0x0002, 
	J2K_STATE_MH     = 0x0004, 
	J2K_STATE_TPHSOT = 0x0008, 
	J2K_STATE_TPH    = 0x0010, 
	J2K_STATE_MT     = 0x0020, 
	J2K_STATE_NEOC   = 0x0040, 
	J2K_STATE_ERR    = 0x0080  
} J2K_STATUS;

typedef enum T2_MODE {
	THRESH_CALC = 0,	
	FINAL_PASS = 1		
}J2K_T2_MODE;

typedef struct opj_stepsize {
	
	int expn;
	
	int mant;
} opj_stepsize_t;

typedef struct opj_tccp {
	
	int csty;
	
	int numresolutions;
	
	int cblkw;
	
	int cblkh;
	
	int cblksty;
	
	int qmfbid;
	
	int qntsty;
	
	opj_stepsize_t stepsizes[J2K_MAXBANDS];
	
	int numgbits;
	
	int roishift;
	
	int prcw[J2K_MAXRLVLS];
	
	int prch[J2K_MAXRLVLS];	
} opj_tccp_t;

typedef struct opj_tcp {
	
	int first;
	
	int csty;
	
	OPJ_PROG_ORDER prg;
	
	int numlayers;
	
	int mct;
	
	float rates[100];
	
	int numpocs;
	
	int POC;
	
	opj_poc_t pocs[32];
	
	unsigned char *ppt_data;
	
	unsigned char *ppt_data_first;
	
	int ppt;
	
	int ppt_store;
	
	int ppt_len;
	
	float distoratio[100];
	
	opj_tccp_t *tccps;
} opj_tcp_t;

typedef struct opj_cp {
	
	OPJ_CINEMA_MODE cinema;
	
	int max_comp_size;
	
	int img_size;
	
	OPJ_RSIZ_CAPABILITIES rsiz;
	
	char tp_on;
	
	char tp_flag;
	
	int tp_pos;
	
	int disto_alloc;
	
	int fixed_alloc;
	
	int fixed_quality;
	
	int reduce;
	
	int layer;
	
	OPJ_LIMIT_DECODING limit_decoding;
	
	int tx0;
	
	int ty0;
	
	int tdx;
	
	int tdy;
	
	char *comment;
	
	int tw;
	
	int th;
	
	int *tileno;
	
	int tileno_size;
	
	unsigned char *ppm_data;
	
	unsigned char *ppm_data_first;
	
	int ppm;
	
	int ppm_store;
	
	int ppm_previous;
	
	int ppm_len;
	
	opj_tcp_t *tcps;
	
	int *matrice;

#ifdef USE_JPWL
	
	opj_bool epc_on;
	
	opj_bool epb_on;
	
	opj_bool esd_on;
	
	opj_bool info_on;
	
	opj_bool red_on;
	
	int hprot_MH;
	
	int hprot_TPH_tileno[JPWL_MAX_NO_TILESPECS];
	
	int hprot_TPH[JPWL_MAX_NO_TILESPECS];
	
	int pprot_tileno[JPWL_MAX_NO_PACKSPECS];
	
	int pprot_packno[JPWL_MAX_NO_PACKSPECS];
	
	int pprot[JPWL_MAX_NO_PACKSPECS];
	
	int sens_size;
	
	int sens_addr;
	
	int sens_range;
	
	int sens_MH;
	
	int sens_TPH_tileno[JPWL_MAX_NO_TILESPECS];
	
	int sens_TPH[JPWL_MAX_NO_TILESPECS];
	
	opj_bool correct;
	
	int exp_comps;
	
	int max_tiles;
#endif 

} opj_cp_t;

typedef struct opj_j2k {
	
	opj_common_ptr cinfo;

	
	int state;
	
	int curtileno;
	
	int tp_num;
	
	int cur_tp_num;
	
	int *cur_totnum_tp;
	
	int tlm_start;
	
	
	int totnum_tp;	
	
	unsigned char *eot;
	
	int sot_start;
	int sod_start;
	
	int pos_correction;
	
	unsigned char **tile_data;
	
	int *tile_len;
	
	opj_tcp_t *default_tcp;
	
	opj_image_t *image;
	
	opj_cp_t *cp;
	
	opj_codestream_info_t *cstr_info;
	
	opj_cio_t *cio;
} opj_j2k_t;

opj_j2k_t* j2k_create_decompress(opj_common_ptr cinfo);

void j2k_destroy_decompress(opj_j2k_t *j2k);

void j2k_setup_decoder(opj_j2k_t *j2k, opj_dparameters_t *parameters);

opj_image_t* j2k_decode(opj_j2k_t *j2k, opj_cio_t *cio, opj_codestream_info_t *cstr_info);

opj_image_t* j2k_decode_jpt_stream(opj_j2k_t *j2k, opj_cio_t *cio, opj_codestream_info_t *cstr_info);

opj_j2k_t* j2k_create_compress(opj_common_ptr cinfo);

void j2k_destroy_compress(opj_j2k_t *j2k);

void j2k_setup_encoder(opj_j2k_t *j2k, opj_cparameters_t *parameters, opj_image_t *image);

char *j2k_convert_progression_order(OPJ_PROG_ORDER prg_order);

opj_bool j2k_encode(opj_j2k_t *j2k, opj_cio_t *cio, opj_image_t *image, opj_codestream_info_t *cstr_info);

#endif 
