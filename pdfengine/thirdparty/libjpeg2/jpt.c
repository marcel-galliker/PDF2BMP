

#include "opj_includes.h"

unsigned int jpt_read_VBAS_info(opj_cio_t *cio, unsigned int value) {
	unsigned char elmt;

	elmt = cio_read(cio, 1);
	while ((elmt >> 7) == 1) {
		value = (value << 7);
		value |= (elmt & 0x7f);
		elmt = cio_read(cio, 1);
	}
	value = (value << 7);
	value |= (elmt & 0x7f);

	return value;
}

void jpt_init_msg_header(opj_jpt_msg_header_t * header) {
	header->Id = 0;		
	header->last_byte = 0;	
	header->Class_Id = 0;		
	header->CSn_Id = 0;		
	header->Msg_offset = 0;	
	header->Msg_length = 0;	
	header->Layer_nb = 0;		
}

void jpt_reinit_msg_header(opj_jpt_msg_header_t * header) {
	header->Id = 0;		
	header->last_byte = 0;	
	header->Msg_offset = 0;	
	header->Msg_length = 0;	
}

void jpt_read_msg_header(opj_common_ptr cinfo, opj_cio_t *cio, opj_jpt_msg_header_t *header) {
	unsigned char elmt, Class = 0, CSn = 0;
	jpt_reinit_msg_header(header);

	
	
	
	elmt = cio_read(cio, 1);

	
	switch ((elmt >> 5) & 0x03) {
		case 0:
			opj_event_msg(cinfo, EVT_ERROR, "Forbidden value encounter in message header !!\n");
			break;
		case 1:
			Class = 0;
			CSn = 0;
			break;
		case 2:
			Class = 1;
			CSn = 0;
			break;
		case 3:
			Class = 1;
			CSn = 1;
			break;
		default:
			break;
	}

	
	if (((elmt >> 4) & 0x01) == 1)
		header->last_byte = 1;

	
	header->Id |= (elmt & 0x0f);
	if ((elmt >> 7) == 1)
		header->Id = jpt_read_VBAS_info(cio, header->Id);

	
	
	
	if (Class == 1) {
		header->Class_Id = 0;
		header->Class_Id = jpt_read_VBAS_info(cio, header->Class_Id);
	}

	
	
	
	if (CSn == 1) {
		header->CSn_Id = 0;
		header->CSn_Id = jpt_read_VBAS_info(cio, header->CSn_Id);
	}

	
	
	
	header->Msg_offset = jpt_read_VBAS_info(cio, header->Msg_offset);

	
	
	
	header->Msg_length = jpt_read_VBAS_info(cio, header->Msg_length);

	
	
	
	if ((header->Class_Id & 0x01) == 1) {
		header->Layer_nb = 0;
		header->Layer_nb = jpt_read_VBAS_info(cio, header->Layer_nb);
	}
}
