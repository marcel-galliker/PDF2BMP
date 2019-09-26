 
#ifndef OPENJPEG_H
#define OPENJPEG_H

#if defined(OPJ_STATIC) || !defined(_WIN32)
#define OPJ_API
#define OPJ_CALLCONV
#else
#define OPJ_CALLCONV __stdcall

#if defined(OPJ_EXPORTS) || defined(DLL_EXPORT)
#define OPJ_API __declspec(dllexport)
#else
#define OPJ_API __declspec(dllimport)
#endif 
#endif 

typedef int opj_bool;
#define OPJ_TRUE 1
#define OPJ_FALSE 0

#define OPJ_ARG_NOT_USED(x) (void)(x)

#define OPJ_PATH_LEN 4096 

#define J2K_MAXRLVLS 33					
#define J2K_MAXBANDS (3*J2K_MAXRLVLS-2)	

#define JPWL_MAX_NO_TILESPECS	16 
#define JPWL_MAX_NO_PACKSPECS	16 
#define JPWL_MAX_NO_MARKERS	512 
#define JPWL_PRIVATEINDEX_NAME "jpwl_index_privatefilename" 
#define JPWL_EXPECTED_COMPONENTS 3 
#define JPWL_MAXIMUM_TILES 8192 
#define JPWL_MAXIMUM_HAMMING 2 
#define JPWL_MAXIMUM_EPB_ROOM 65450 

typedef enum RSIZ_CAPABILITIES {
	STD_RSIZ = 0,		
	CINEMA2K = 3,		
	CINEMA4K = 4		
} OPJ_RSIZ_CAPABILITIES;

typedef enum CINEMA_MODE {
	OFF = 0,					
	CINEMA2K_24 = 1,	
	CINEMA2K_48 = 2,	
	CINEMA4K_24 = 3		
}OPJ_CINEMA_MODE;

typedef enum PROG_ORDER {
	PROG_UNKNOWN = -1,	
	LRCP = 0,		
	RLCP = 1,		
	RPCL = 2,		
	PCRL = 3,		
	CPRL = 4		
} OPJ_PROG_ORDER;

typedef enum COLOR_SPACE {
	CLRSPC_UNKNOWN = -1,	
	CLRSPC_UNSPECIFIED = 0,  
	CLRSPC_SRGB = 1,		
	CLRSPC_GRAY = 2,		
	CLRSPC_SYCC = 3			
} OPJ_COLOR_SPACE;

typedef enum CODEC_FORMAT {
	CODEC_UNKNOWN = -1,	
	CODEC_J2K  = 0,		
	CODEC_JPT  = 1,		
	CODEC_JP2  = 2 		
} OPJ_CODEC_FORMAT;

typedef enum LIMIT_DECODING {
	NO_LIMITATION = 0,				  
	LIMIT_TO_MAIN_HEADER = 1,		
	DECODE_ALL_BUT_PACKETS = 2	
} OPJ_LIMIT_DECODING;

typedef void (*opj_msg_callback) (const char *msg, void *client_data);

typedef struct opj_event_mgr {
	
	opj_msg_callback error_handler;
	
	opj_msg_callback warning_handler;
	
	opj_msg_callback info_handler;
} opj_event_mgr_t;

typedef struct opj_poc {
	
	int resno0, compno0;
	
	int layno1, resno1, compno1;
	
	int layno0, precno0, precno1;
	
	OPJ_PROG_ORDER prg1,prg;
	
	char progorder[5];
	
	int tile;
	
	int tx0,tx1,ty0,ty1;
	
	int layS, resS, compS, prcS;
	
	int layE, resE, compE, prcE;
	
	int txS,txE,tyS,tyE,dx,dy;
	
	int lay_t, res_t, comp_t, prc_t,tx0_t,ty0_t;
} opj_poc_t;

typedef struct opj_cparameters {
	
	opj_bool tile_size_on;
	
	int cp_tx0;
	
	int cp_ty0;
	
	int cp_tdx;
	
	int cp_tdy;
	
	int cp_disto_alloc;
	
	int cp_fixed_alloc;
	
	int cp_fixed_quality;
	
	int *cp_matrice;
	
	char *cp_comment;
	
	int csty;
	
	OPJ_PROG_ORDER prog_order;
	
	opj_poc_t POC[32];
	
	int numpocs;
	
	int tcp_numlayers;
	
	float tcp_rates[100];
	
	float tcp_distoratio[100];
	
	int numresolution;
	
 	int cblockw_init;
	
	int cblockh_init;
	
	int mode;
	
	int irreversible;
	
	int roi_compno;
	
	int roi_shift;
	
	int res_spec;
	
	int prcw_init[J2K_MAXRLVLS];
	
	int prch_init[J2K_MAXRLVLS];

	
	
	
	char infile[OPJ_PATH_LEN];
	
	char outfile[OPJ_PATH_LEN];
	
	int index_on;
	
	char index[OPJ_PATH_LEN];
	
	int image_offset_x0;
	
	int image_offset_y0;
	
	int subsampling_dx;
	
	int subsampling_dy;
	
	int decod_format;
	
	int cod_format;
	

	
	
	
	opj_bool jpwl_epc_on;
	
	int jpwl_hprot_MH;
	
	int jpwl_hprot_TPH_tileno[JPWL_MAX_NO_TILESPECS];
	
	int jpwl_hprot_TPH[JPWL_MAX_NO_TILESPECS];
	
	int jpwl_pprot_tileno[JPWL_MAX_NO_PACKSPECS];
	
	int jpwl_pprot_packno[JPWL_MAX_NO_PACKSPECS];
	
	int jpwl_pprot[JPWL_MAX_NO_PACKSPECS];
	
	int jpwl_sens_size;
	
	int jpwl_sens_addr;
	
	int jpwl_sens_range;
	
	int jpwl_sens_MH;
	
	int jpwl_sens_TPH_tileno[JPWL_MAX_NO_TILESPECS];
	
	int jpwl_sens_TPH[JPWL_MAX_NO_TILESPECS];
	

	
	OPJ_CINEMA_MODE cp_cinema;
	
	int max_comp_size;
	
	OPJ_RSIZ_CAPABILITIES cp_rsiz;
	
	char tp_on;
	
	char tp_flag;
	
	char tcp_mct;
	
	opj_bool jpip_on;
} opj_cparameters_t;

#define OPJ_DPARAMETERS_IGNORE_PCLR_CMAP_CDEF_FLAG	0x0001

typedef struct opj_dparameters {
	
	int cp_reduce;
	
	int cp_layer;

	
	
	
	char infile[OPJ_PATH_LEN];
	
	char outfile[OPJ_PATH_LEN];
	
	int decod_format;
	
	int cod_format;
	

	
	
	
	opj_bool jpwl_correct;
	
	int jpwl_exp_comps;
	
	int jpwl_max_tiles;
	

	
	OPJ_LIMIT_DECODING cp_limit_decoding;

	unsigned int flags;
} opj_dparameters_t;

#define opj_common_fields \
	opj_event_mgr_t *event_mgr;	\
	void * client_data;			\
	opj_bool is_decompressor;	\
	OPJ_CODEC_FORMAT codec_format;	\
	void *j2k_handle;			\
	void *jp2_handle;			\
	void *mj2_handle			
	

typedef struct opj_common_struct {
  opj_common_fields;		
  
} opj_common_struct_t;

typedef opj_common_struct_t * opj_common_ptr;

typedef struct opj_cinfo {
	
	opj_common_fields;	
	
} opj_cinfo_t;

typedef struct opj_dinfo {
	
	opj_common_fields;	
	
} opj_dinfo_t;

#define OPJ_STREAM_READ	0x0001

#define OPJ_STREAM_WRITE 0x0002

typedef struct opj_cio {
	
	opj_common_ptr cinfo;

	
	int openmode;
	
	unsigned char *buffer;
	
	int length;

	
	unsigned char *start;
	
	unsigned char *end;
	
	unsigned char *bp;
} opj_cio_t;

typedef struct opj_image_comp {
	
	int dx;
	
	int dy;
	
	int w;
	
	int h;
	
	int x0;
	
	int y0;
	
	int prec;
	
	int bpp;
	
	int sgnd;
	
	int resno_decoded;
	
	int factor;
	
	int *data;
} opj_image_comp_t;

typedef struct opj_image {
	
	int x0;
	
	int y0;
	
	int x1;
	
	int y1;
	
	int numcomps;
	
	OPJ_COLOR_SPACE color_space;
	
	opj_image_comp_t *comps;
	
	unsigned char *icc_profile_buf;
	
	int icc_profile_len;
} opj_image_t;

typedef struct opj_image_comptparm {
	
	int dx;
	
	int dy;
	
	int w;
	
	int h;
	
	int x0;
	
	int y0;
	
	int prec;
	
	int bpp;
	
	int sgnd;
} opj_image_cmptparm_t;

typedef struct opj_packet_info {
	
	int start_pos;
	
	int end_ph_pos;
	
	int end_pos;
	
	double disto;
} opj_packet_info_t;

typedef struct opj_marker_info_t {
	
	unsigned short int type;
	
	int pos;
	
	int len;
} opj_marker_info_t;

typedef struct opj_tp_info {
	
	int tp_start_pos;
	
	int tp_end_header;
	
	int tp_end_pos;
	
	int tp_start_pack;
	
	int tp_numpacks;
} opj_tp_info_t;

typedef struct opj_tile_info {
	
	double *thresh;
	
	int tileno;
	
	int start_pos;
	
	int end_header;
	
	int end_pos;
	
	int pw[33];
	
	int ph[33];
	
	int pdx[33];
	
	int pdy[33];
	
	opj_packet_info_t *packet;
	
	int numpix;
	
	double distotile;
  	
	int marknum;
	
	opj_marker_info_t *marker;
	
	int maxmarknum;
	
	int num_tps;
	
	opj_tp_info_t *tp;
} opj_tile_info_t;

typedef struct opj_codestream_info {
	
	double D_max;
	
	int packno;
	
	int index_write;
	
	int image_w;
	
	int image_h;
	
	OPJ_PROG_ORDER prog;
	
	int tile_x;
	
	int tile_y;
	
	int tile_Ox;
	
	int tile_Oy;
	
	int tw;
	
	int th;
	
	int numcomps;
	
	int numlayers;
	
	int *numdecompos;

	
	int marknum;
	
	opj_marker_info_t *marker;
	
	int maxmarknum;

	
	int main_head_start;
	
	int main_head_end;
	
	int codestream_size;
	
	opj_tile_info_t *tile;
} opj_codestream_info_t;

#ifdef __cplusplus
extern "C" {
#endif

OPJ_API const char * OPJ_CALLCONV opj_version(void);

OPJ_API opj_image_t* OPJ_CALLCONV opj_image_create(int numcmpts, opj_image_cmptparm_t *cmptparms, OPJ_COLOR_SPACE clrspc);

OPJ_API void OPJ_CALLCONV opj_image_destroy(opj_image_t *image);

OPJ_API opj_cio_t* OPJ_CALLCONV opj_cio_open(opj_common_ptr cinfo, unsigned char *buffer, int length);

OPJ_API void OPJ_CALLCONV opj_cio_close(opj_cio_t *cio);

OPJ_API int OPJ_CALLCONV cio_tell(opj_cio_t *cio);

OPJ_API void OPJ_CALLCONV cio_seek(opj_cio_t *cio, int pos);

OPJ_API opj_event_mgr_t* OPJ_CALLCONV opj_set_event_mgr(opj_common_ptr cinfo, opj_event_mgr_t *event_mgr, void *context);

OPJ_API opj_dinfo_t* OPJ_CALLCONV opj_create_decompress(OPJ_CODEC_FORMAT format);

OPJ_API void OPJ_CALLCONV opj_destroy_decompress(opj_dinfo_t *dinfo);

OPJ_API void OPJ_CALLCONV opj_set_default_decoder_parameters(opj_dparameters_t *parameters);

OPJ_API void OPJ_CALLCONV opj_setup_decoder(opj_dinfo_t *dinfo, opj_dparameters_t *parameters);

OPJ_API opj_image_t* OPJ_CALLCONV opj_decode(opj_dinfo_t *dinfo, opj_cio_t *cio);

OPJ_API opj_image_t* OPJ_CALLCONV opj_decode_with_info(opj_dinfo_t *dinfo, opj_cio_t *cio, opj_codestream_info_t *cstr_info);

OPJ_API opj_cinfo_t* OPJ_CALLCONV opj_create_compress(OPJ_CODEC_FORMAT format);

OPJ_API void OPJ_CALLCONV opj_destroy_compress(opj_cinfo_t *cinfo);

OPJ_API void OPJ_CALLCONV opj_set_default_encoder_parameters(opj_cparameters_t *parameters);

OPJ_API void OPJ_CALLCONV opj_setup_encoder(opj_cinfo_t *cinfo, opj_cparameters_t *parameters, opj_image_t *image);

OPJ_API opj_bool OPJ_CALLCONV opj_encode(opj_cinfo_t *cinfo, opj_cio_t *cio, opj_image_t *image, char *index);

OPJ_API opj_bool OPJ_CALLCONV opj_encode_with_info(opj_cinfo_t *cinfo, opj_cio_t *cio, opj_image_t *image, opj_codestream_info_t *cstr_info);

OPJ_API void OPJ_CALLCONV opj_destroy_cstr_info(opj_codestream_info_t *cstr_info);

#ifdef __cplusplus
}
#endif

#endif 
