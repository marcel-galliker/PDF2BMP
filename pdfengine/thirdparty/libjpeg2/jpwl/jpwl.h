
#ifndef __JPWL_H
#define __JPWL_H

#ifdef USE_JPWL

#include "crc.h"
#include "rs.h"

#define JPWL_ASSUME OPJ_TRUE

typedef struct jpwl_epb_ms {
	
	
	
	opj_bool latest;
	
	opj_bool packed;
	
	int tileno;
	
	unsigned char index;
	
	int hprot;
	
	int k_pre;
	
	int n_pre;
	
	int pre_len;
	
	int k_post;
	
	int n_post;
	
	int post_len;
	
	
	
	
	unsigned short int Lepb;
	
	unsigned char Depb; 
	
	unsigned long int LDPepb;
	
	unsigned long int Pepb;
	
	unsigned char *data;   
	
}	jpwl_epb_ms_t;

typedef struct jpwl_epc_ms {
	
	opj_bool esd_on;
	
	opj_bool red_on;
	
	opj_bool epb_on;
	
	opj_bool info_on;
	
	
	
	unsigned short int Lepc;   
	
	unsigned short int Pcrc;   
	
	unsigned long int DL;     
	
	unsigned char Pepc;	
	
	unsigned char *data;	
	
}	jpwl_epc_ms_t;

typedef struct jpwl_esd_ms {
	
	unsigned char addrm;
	
	unsigned char ad_size;
	
	unsigned char senst;
	
	unsigned char se_size;
	
	
	
	unsigned short int Lesd;   
	
	unsigned short int Cesd;
	
	unsigned char Pesd;	
	
	unsigned char *data;	
	
	
	
	
	int numcomps;
	
	int tileno;
	
	unsigned long int svalnum;
	
	size_t sensval_size;
	
}	jpwl_esd_ms_t;

typedef struct jpwl_red_ms {
	
	unsigned short int Lred;
	
	unsigned char Pred;	
	
	unsigned char *data;	
}	jpwl_red_ms_t;

typedef struct jpwl_marker {
	
	int id;
	
	union jpwl_marks {
		
		jpwl_epb_ms_t *epbmark;
		
		jpwl_epc_ms_t *epcmark;
		
		jpwl_esd_ms_t *esdmark;
		
		jpwl_red_ms_t *redmark;
	} m;
	 
	unsigned long int pos;
	
	double dpos;
	
	unsigned short int len;
	
	opj_bool len_ready;
	
	opj_bool pos_ready;
	
	opj_bool parms_ready;
	
	opj_bool data_ready;
}	jpwl_marker_t;

void jpwl_encode(opj_j2k_t *j2k, opj_cio_t *cio, opj_image_t *image);

void jpwl_prepare_marks(opj_j2k_t *j2k, opj_cio_t *cio, opj_image_t *image);

void jpwl_dump_marks(opj_j2k_t *j2k, opj_cio_t *cio, opj_image_t *image);

void j2k_read_epc(opj_j2k_t *j2k);

void j2k_write_epc(opj_j2k_t *j2k);

void j2k_read_epb(opj_j2k_t *j2k);

void j2k_write_epb(opj_j2k_t *j2k);

void j2k_read_esd(opj_j2k_t *j2k);

void j2k_read_red(opj_j2k_t *j2k);

jpwl_epb_ms_t *jpwl_epb_create(opj_j2k_t *j2k, opj_bool latest, opj_bool packed, int tileno, int idx, int hprot,
							   unsigned long int pre_len, unsigned long int post_len);

int jpwl_epbs_add(opj_j2k_t *j2k, jpwl_marker_t *jwmarker, int *jwmarker_num,
				  opj_bool latest, opj_bool packed, opj_bool insideMH, int *idx, int hprot,
				  double place_pos, int tileno,
				  unsigned long int pre_len, unsigned long int post_len);

int jpwl_esds_add(opj_j2k_t *j2k, jpwl_marker_t *jwmarker, int *jwmarker_num,
				  int comps, unsigned char addrm, unsigned char ad_size,
				  unsigned char senst, unsigned char se_size,
				  double place_pos, int tileno);
	
			  
opj_bool jpwl_update_info(opj_j2k_t *j2k, jpwl_marker_t *jwmarker, int jwmarker_num);

opj_bool jpwl_esd_fill(opj_j2k_t *j2k, jpwl_esd_ms_t *esdmark, unsigned char *buf);

opj_bool jpwl_epb_fill(opj_j2k_t *j2k, jpwl_epb_ms_t *epbmark, unsigned char *buf, unsigned char *post_buf);

void j2k_add_marker(opj_codestream_info_t *cstr_info, unsigned short int type, int pos, int len);

opj_bool jpwl_correct(opj_j2k_t *j2k);

opj_bool jpwl_epb_correct(opj_j2k_t *j2k, unsigned char *buffer, int type, int pre_len, int post_len, int *conn,
					  unsigned char **L4_bufp);

opj_bool jpwl_check_tile(opj_j2k_t *j2k, opj_tcd_t *tcd, int tileno);

#define jpwl_updateCRC16(CRC, DATA) updateCRC16(CRC, DATA)

#define jpwl_updateCRC32(CRC, DATA) updateCRC32(CRC, DATA)

#ifndef min
#define min(a,b)    (((a) < (b)) ? (a) : (b))
#endif 

#endif 

#ifdef USE_JPSEC

void j2k_read_sec(opj_j2k_t *j2k);

void j2k_write_sec(opj_j2k_t *j2k);

void j2k_read_insec(opj_j2k_t *j2k);

#endif 

#endif 

