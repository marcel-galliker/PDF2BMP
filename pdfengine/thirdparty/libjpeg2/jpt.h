

#ifndef __JPT_H
#define __JPT_H

typedef struct opj_jpt_msg_header {
	
	unsigned int Id;
	
	unsigned int last_byte;	
	
	unsigned int Class_Id;	
	
	unsigned int CSn_Id;
	
	unsigned int Msg_offset;
	
	unsigned int Msg_length;
	
	unsigned int Layer_nb;
} opj_jpt_msg_header_t;

void jpt_init_msg_header(opj_jpt_msg_header_t * header);

void jpt_read_msg_header(opj_common_ptr cinfo, opj_cio_t *cio, opj_jpt_msg_header_t *header);

#endif
