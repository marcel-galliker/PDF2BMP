

#ifdef USE_JPWL

#include "opj_includes.h"
#include <limits.h>

#define MIN_V1 0.0
#define MAX_V1 17293822569102704640.0
#define MIN_V2 0.000030517578125
#define MAX_V2 131040.0

unsigned short int jpwl_double_to_pfp(double V, int bytes);

double jpwl_pfp_to_double(unsigned short int em, int bytes);

	

int jpwl_markcomp(const void *arg1, const void *arg2)
{
   
   double diff = (((jpwl_marker_t *) arg1)->dpos - ((jpwl_marker_t *) arg2)->dpos);

   if (diff == 0.0)
	   return (0);
   else if (diff < 0)
	   return (-1);
   else
	   return (+1);
}

int jpwl_epbs_add(opj_j2k_t *j2k, jpwl_marker_t *jwmarker, int *jwmarker_num,
				  opj_bool latest, opj_bool packed, opj_bool insideMH, int *idx, int hprot,
				  double place_pos, int tileno,
				  unsigned long int pre_len, unsigned long int post_len) {

	jpwl_epb_ms_t *epb_mark = NULL;

	int k_pre, k_post, n_pre, n_post;
	
	unsigned long int L1, L2, dL4, max_postlen, epbs_len = 0;

	
	if (insideMH && (*idx == 0)) {
		 
		k_pre = 64;
		n_pre = 160;
	} else if (!insideMH && (*idx == 0)) {
		
		k_pre = 25;
		n_pre = 80;
	} else {
		
		k_pre = 13;
		n_pre = 40;
	};

	
	
	L1 = pre_len + 13;

	
	
	L2 = (n_pre - k_pre) * (unsigned long int) ceil((double) L1 / (double) k_pre);

	
	if ((hprot == 16) || (hprot == 32)) {
		
		k_post = post_len;
		n_post = post_len + (hprot >> 3);
		 

	} else if ((hprot >= 37) && (hprot <= 128)) {
		
		k_post = 32;
		n_post = hprot;

	} else {
		
		n_post = n_pre;
		k_post = k_pre;
	};

	
	while (post_len > 0) {

		
		
		max_postlen = k_post * (unsigned long int) floor((double) JPWL_MAXIMUM_EPB_ROOM / (double) (n_post - k_post));

		
		if (*idx == 0)
			
			
			max_postlen = k_post * (unsigned long int) floor((double) (JPWL_MAXIMUM_EPB_ROOM - L2) / (double) (n_post - k_post));

		else
			
			
			max_postlen = k_post * (unsigned long int) floor((double) JPWL_MAXIMUM_EPB_ROOM / (double) (n_post - k_post));

		
		
		if (hprot == 0)
			max_postlen = INT_MAX;
		
		
		dL4 = min(max_postlen, post_len);

		if ((epb_mark = jpwl_epb_create(
			j2k, 
			latest ? (dL4 < max_postlen) : OPJ_FALSE, 
			packed, 
			tileno, 
			*idx, 
			hprot, 
			0, 
			dL4 
			))) {
			
			
			if (*jwmarker_num < JPWL_MAX_NO_MARKERS) {
				jwmarker[*jwmarker_num].id = J2K_MS_EPB; 
				jwmarker[*jwmarker_num].m.epbmark = epb_mark; 
				jwmarker[*jwmarker_num].pos = (int) place_pos; 
				jwmarker[*jwmarker_num].dpos = place_pos + 0.0000001 * (double)(*idx); 
				jwmarker[*jwmarker_num].len = epb_mark->Lepb; 
				jwmarker[*jwmarker_num].len_ready = OPJ_TRUE; 
				jwmarker[*jwmarker_num].pos_ready = OPJ_TRUE; 
				jwmarker[*jwmarker_num].parms_ready = OPJ_TRUE; 
				jwmarker[*jwmarker_num].data_ready = OPJ_FALSE; 
				(*jwmarker_num)++;
			}

			
			(*idx)++;

			
			post_len -= dL4;

			
			epbs_len += epb_mark->Lepb + 2;

		} else {
			
			opj_event_msg(j2k->cinfo, EVT_ERROR, "Could not create TPH EPB for UEP in tile %d\n", tileno);				
		};
	}

	return epbs_len;
}

jpwl_epb_ms_t *jpwl_epb_create(opj_j2k_t *j2k, opj_bool latest, opj_bool packed, int tileno, int idx, int hprot,
						  unsigned long int pre_len, unsigned long int post_len) {

	jpwl_epb_ms_t *epb = NULL;
	
	unsigned short int L2, L3;
	unsigned long int L1, L4;
	

	opj_bool insideMH = (tileno == -1);

	
	if (!(epb = (jpwl_epb_ms_t *) opj_malloc((size_t) 1 * sizeof (jpwl_epb_ms_t)))) {
		opj_event_msg(j2k->cinfo, EVT_ERROR, "Could not allocate room for one EPB MS\n");
		return NULL;
	};

	
	if (insideMH && (idx == 0)) {
		 
		epb->k_pre = 64;
		epb->n_pre = 160;
	} else if (!insideMH && (idx == 0)) {
		
		epb->k_pre = 25;
		epb->n_pre = 80;
	} else {
		
		epb->k_pre = 13;
		epb->n_pre = 40;
	};

	
	
	L1 = pre_len + 13;
	epb->pre_len = pre_len;

	
	
	L2 = (epb->n_pre - epb->k_pre) * (unsigned short int) ceil((double) L1 / (double) epb->k_pre);

	
	L4 = post_len;
	epb->post_len = post_len;

	
	if ((hprot == 16) || (hprot == 32)) {
		
		epb->Pepb = 0x10000000 | ((unsigned long int) hprot >> 5); 
		epb->k_post = post_len;
		epb->n_post = post_len + (hprot >> 3);
		 

	} else if ((hprot >= 37) && (hprot <= 128)) {
		
		epb->Pepb = 0x20000020 | (((unsigned long int) hprot & 0x000000FF) << 8);
		epb->k_post = 32;
		epb->n_post = hprot;

	} else if (hprot == 1) {
		
		epb->Pepb = (unsigned long int) 0x00000000;
		epb->n_post = epb->n_pre;
		epb->k_post = epb->k_pre;
	
	} else if (hprot == 0) {
		
		epb->Pepb = (unsigned long int) 0xFFFFFFFF;
		epb->n_post = 1;
		epb->k_post = 1;
	
	} else {
		opj_event_msg(j2k->cinfo, EVT_ERROR, "Invalid protection value for EPB h = %d\n", hprot);				
		return NULL;
	}

	epb->hprot = hprot;

	
	L3 = (epb->n_post - epb->k_post) * (unsigned short int) ceil((double) L4 / (double) epb->k_post);

	
	epb->tileno = tileno;

	

	
	
	epb->Lepb = 11 + L2 + L3;

	
	epb->Depb = ((packed & 0x0001) << 7) | ((latest & 0x0001) << 6) | (idx & 0x003F);

	
	epb->LDPepb = L1 + L4;

	return epb;
}

void jpwl_epb_write(opj_j2k_t *j2k, jpwl_epb_ms_t *epb, unsigned char *buf) {

	
	*(buf++) = (unsigned char) (J2K_MS_EPB >> 8); 
	*(buf++) = (unsigned char) (J2K_MS_EPB >> 0); 

	
	*(buf++) = (unsigned char) (epb->Lepb >> 8); 
	*(buf++) = (unsigned char) (epb->Lepb >> 0); 

	
	*(buf++) = (unsigned char) (epb->Depb >> 0); 

	
	*(buf++) = (unsigned char) (epb->LDPepb >> 24); 
	*(buf++) = (unsigned char) (epb->LDPepb >> 16); 
	*(buf++) = (unsigned char) (epb->LDPepb >> 8); 
	*(buf++) = (unsigned char) (epb->LDPepb >> 0); 

	
	*(buf++) = (unsigned char) (epb->Pepb >> 24); 
	*(buf++) = (unsigned char) (epb->Pepb >> 16); 
	*(buf++) = (unsigned char) (epb->Pepb >> 8); 
	*(buf++) = (unsigned char) (epb->Pepb >> 0); 

	
	
	memset(buf, 0, (size_t) epb->Lepb - 11);

	
	j2k_add_marker(j2k->cstr_info, J2K_MS_EPB, -1, epb->Lepb + 2);

};

jpwl_epc_ms_t *jpwl_epc_create(opj_j2k_t *j2k, opj_bool esd_on, opj_bool red_on, opj_bool epb_on, opj_bool info_on) {

	jpwl_epc_ms_t *epc = NULL;

	
	if (!(epc = (jpwl_epc_ms_t *) opj_malloc((size_t) 1 * sizeof (jpwl_epc_ms_t)))) {
		opj_event_msg(j2k->cinfo, EVT_ERROR, "Could not allocate room for EPC MS\n");
		return NULL;
	};

	
	epc->esd_on = esd_on;
	epc->epb_on = epb_on;
	epc->red_on = red_on;
	epc->info_on = info_on;

	
	epc->Lepc = 9;
	epc->Pcrc = 0x0000;
	epc->DL = 0x00000000;
	epc->Pepc = ((j2k->cp->esd_on & 0x0001) << 4) | ((j2k->cp->red_on & 0x0001) << 5) |
		((j2k->cp->epb_on & 0x0001) << 6) | ((j2k->cp->info_on & 0x0001) << 7);

	return (epc);
}

opj_bool jpwl_epb_fill(opj_j2k_t *j2k, jpwl_epb_ms_t *epb, unsigned char *buf, unsigned char *post_buf) {

	unsigned long int L1, L2, L3, L4;
	int remaining;
	unsigned long int P, NN_P;

	
	static unsigned char codeword[NN], *parityword;

	unsigned char *L1_buf, *L2_buf;
	
	static unsigned char *L3_buf, *L4_buf;

	
	if (!buf) {
		opj_event_msg(j2k->cinfo, EVT_ERROR, "There is no operating buffer for EPBs\n");
		return OPJ_FALSE;
	}

	if (!post_buf && !L4_buf) {
		opj_event_msg(j2k->cinfo, EVT_ERROR, "There is no operating buffer for EPBs data\n");
		return OPJ_FALSE;
	}

	

	
	P = epb->n_pre - epb->k_pre;
	NN_P = NN - P;
	memset(codeword, 0, NN);
	parityword = codeword + NN_P;
	init_rs(NN_P);

	
	L1_buf = buf - epb->pre_len;
	L1 = epb->pre_len + 13;

	
	L2_buf = buf + 13;
	L2 = (epb->n_pre - epb->k_pre) * (unsigned short int) ceil((double) L1 / (double) epb->k_pre);

	
	if (post_buf)
		L4_buf = post_buf;
	L4 = epb->post_len;

	
	L3_buf = L2_buf + L2;
	L3 = (epb->n_post - epb->k_post) * (unsigned short int) ceil((double) L4 / (double) epb->k_post);

	
	if (epb->Lepb < (11 + L2 + L3))
		opj_event_msg(j2k->cinfo, EVT_ERROR, "There is no room in EPB data field for writing redundancy data\n");
	

	
	remaining = L1;
	while (remaining) {

		
		if (remaining < epb->k_pre) {
			
			memset(codeword, 0, NN);
			memcpy(codeword, L1_buf, remaining);
			L1_buf += remaining;
			remaining = 0;

		} else {
			memcpy(codeword, L1_buf, epb->k_pre);
			L1_buf += epb->k_pre;
			remaining -= epb->k_pre;

		}

		
		if (encode_rs(codeword, parityword))
			opj_event_msg(j2k->cinfo, EVT_WARNING,
				"Possible encoding error in codeword @ position #%d\n", (L1_buf - buf) / epb->k_pre);

		
		memcpy(L2_buf, parityword, P); 

		
		L2_buf += P;
	}

	
	
	if (epb->hprot < 0) {

		
		
	} else if (epb->hprot == 0) {

		
		
		L4_buf += epb->post_len;

	} else if (epb->hprot == 16) {

		
		unsigned short int mycrc = 0x0000;

		
		remaining = L4;
		while (remaining--)
			jpwl_updateCRC16(&mycrc, *(L4_buf++));

		
		*(L3_buf++) = (unsigned char) (mycrc >> 8);
		*(L3_buf++) = (unsigned char) (mycrc >> 0);

	} else if (epb->hprot == 32) {

		
		unsigned long int mycrc = 0x00000000;

		
		remaining = L4;
		while (remaining--)
			jpwl_updateCRC32(&mycrc, *(L4_buf++));

		
		*(L3_buf++) = (unsigned char) (mycrc >> 24);
		*(L3_buf++) = (unsigned char) (mycrc >> 16);
		*(L3_buf++) = (unsigned char) (mycrc >> 8);
		*(L3_buf++) = (unsigned char) (mycrc >> 0);

	} else {

		

		
		P = epb->n_post - epb->k_post;
		NN_P = NN - P;
		memset(codeword, 0, NN);
		parityword = codeword + NN_P;
		init_rs(NN_P);

		
		remaining = L4;
		while (remaining) {

			
			if (remaining < epb->k_post) {
				
				memset(codeword, 0, NN);
				memcpy(codeword, L4_buf, remaining);
				L4_buf += remaining;
				remaining = 0;

			} else {
				memcpy(codeword, L4_buf, epb->k_post);
				L4_buf += epb->k_post;
				remaining -= epb->k_post;

			}

			
			if (encode_rs(codeword, parityword))
				opj_event_msg(j2k->cinfo, EVT_WARNING,
					"Possible encoding error in codeword @ position #%d\n", (L4_buf - buf) / epb->k_post);

			
			memcpy(L3_buf, parityword, P); 

			
			L3_buf += P;
		}

	}

	return OPJ_TRUE;
}

opj_bool jpwl_correct(opj_j2k_t *j2k) {

	opj_cio_t *cio = j2k->cio;
	opj_bool status;
	static opj_bool mh_done = OPJ_FALSE;
	int mark_pos, id, len, skips, sot_pos;
	unsigned long int Psot = 0;

	
	mark_pos = cio_tell(cio) - 2;
	cio_seek(cio, mark_pos);

	if ((j2k->state == J2K_STATE_MHSOC) && !mh_done) {

		int mark_val = 0, skipnum = 0;

		
		
		
		skipnum = 2  +     38     + 3 * j2k->cp->exp_comps  +         2;
		if ((cio->bp + skipnum) < cio->end) {

			cio_skip(cio, skipnum);

			

			
			status = jpwl_epb_correct(j2k,     
									  cio->bp, 
									  0,       
									  skipnum,      
									  -1,      
									  NULL,
									  NULL
									 );

			
			mark_val = (*(cio->bp) << 8) | *(cio->bp + 1);

			if (status && (mark_val == J2K_MS_EPB)) {
				
				mh_done = OPJ_TRUE;
				return OPJ_TRUE;
			}

			
			
			
			
			if (!status) {
				j2k->cp->correct = OPJ_FALSE;
				opj_event_msg(j2k->cinfo, EVT_WARNING, "Couldn't find the MH EPB: disabling JPWL\n");
			}

		}

	}

	if (OPJ_TRUE ) {
		
		cio_seek(cio, mark_pos);
		if ((cio->bp + 12) < cio->end) {

			cio_skip(cio, 12);

			
			status = jpwl_epb_correct(j2k,     
									  cio->bp, 
									  1,       
									  12,      
									  -1,      
									  NULL,
									  NULL
									 );
			if (status)
				
				return OPJ_TRUE;
		}
	}

	return OPJ_FALSE;

	

	
	if (mark_pos > 64) {
		
		cio_seek(cio, mark_pos);
		cio_skip(cio, 0);

		
		status = jpwl_epb_correct(j2k,     
								  cio->bp, 
								  2,       
								  0,       
								  -1,      
								  NULL,
								  NULL
								 );
		if (status)
			
			return OPJ_TRUE;
	}

	
	return OPJ_FALSE;
	
	return OPJ_TRUE;

	
	

	
	cio_seek(cio, 0);

	
	j2k->state = J2K_STATE_MHSOC;

	
	while (cio_tell(cio) < cio->length) {

		
		mark_pos = cio_tell(cio);
		id = cio_read(cio, 2);

		
		printf("Marker@%d: %X\n", cio_tell(cio) - 2, id);

		
		switch (id) {

		

			
		case J2K_MS_SOC:
			j2k->state = J2K_STATE_MHSIZ;
			len = 0;
			skips = 0;
			break;

			
		case J2K_MS_EOC:
			j2k->state = J2K_STATE_MT;
			len = 0;
			skips = 0;
			break;

			
		case J2K_MS_SOD:
			len = Psot - (mark_pos - sot_pos) - 2;
			skips = len;
			break;

		

			
		case J2K_MS_SOT:
			j2k->state = J2K_STATE_TPH;
			sot_pos = mark_pos; 
			len = cio_read(cio, 2); 
			cio_skip(cio, 2); 
			Psot = cio_read(cio, 4); 
			skips = len - 8;
			break;

			
		case J2K_MS_SIZ:
			j2k->state = J2K_STATE_MH;
			
			len = cio_read(cio, 2);
			skips = len - 2;
			break;

			
		default:
			
			len = cio_read(cio, 2);
			skips = len - 2;
			break;

		}

		
		cio_skip(cio, skips);	

	}

}

opj_bool jpwl_epb_correct(opj_j2k_t *j2k, unsigned char *buffer, int type, int pre_len, int post_len, int *conn,
					  unsigned char **L4_bufp) {

	
	unsigned char codeword[NN], *parityword;

	unsigned long int P, NN_P;
	unsigned long int L1, L4;
	int remaining, n_pre, k_pre, n_post, k_post;

	int status, tt;

	int orig_pos = cio_tell(j2k->cio);

	unsigned char *L1_buf, *L2_buf;
	unsigned char *L3_buf, *L4_buf;

	unsigned long int LDPepb, Pepb;
	unsigned short int Lepb;
	unsigned char Depb;
	char str1[25] = "";
	int myconn, errnum = 0;
	opj_bool errflag = OPJ_FALSE;
	
	opj_cio_t *cio = j2k->cio;

	
	if (!buffer) {
		opj_event_msg(j2k->cinfo, EVT_ERROR, "The EPB pointer is a NULL buffer\n");
		return OPJ_FALSE;
	}
	
	
	L1 = pre_len + 13;

	
	switch (type) {

	case 0:
		
		k_pre = 64;
		n_pre = 160;
		break;

	case 1:
		
		k_pre = 25;
		n_pre = 80;
		break;

	case 2:
		
		k_pre = 13;
		n_pre = 40;
		break;

	case 3:
		
		opj_event_msg(j2k->cinfo, EVT_ERROR, "Auto. setup not yet implemented\n");
		return OPJ_FALSE;
		break;

	default:
		
		opj_event_msg(j2k->cinfo, EVT_ERROR, "Unknown expected EPB type\n");
		return OPJ_FALSE;
		break;

	}

	
	P = n_pre - k_pre;
	NN_P = NN - P;
	tt = (int) floor((float) P / 2.0F); 
	memset(codeword, 0, NN);
	parityword = codeword + NN_P;
	init_rs(NN_P);

	
	L1_buf = buffer - pre_len;
	L2_buf = buffer + 13;
	remaining = L1;
	while (remaining) {
 
		
		
		memset(codeword, 0, NN);

		
		if (remaining < k_pre)
			memcpy(codeword, L1_buf, remaining);
		else
			memcpy(codeword, L1_buf, k_pre);

		
		memcpy(parityword, L2_buf, P); 

		
		status = eras_dec_rs(codeword, NULL, 0);
		if (status == -1) {
			
			errflag = OPJ_TRUE;
			
			

		} else if (status == 0) {
			

		} else if (status <= tt) {
			
			
			errnum += status;

		} else {
			
			errflag = OPJ_TRUE;
		}

		
		if ((status >= 0) && (status <= tt))
			
			memcpy(L2_buf, parityword, P);
		L2_buf += P;

		
		if (remaining < k_pre) {
			if ((status >= 0) && (status <= tt))
				
				memcpy(L1_buf, codeword, remaining);
			L1_buf += remaining;
			remaining = 0;

		} else {
			if ((status >= 0) && (status <= tt))
				
				memcpy(L1_buf, codeword, k_pre);
			L1_buf += k_pre;
			remaining -= k_pre;

		}
	}

	
	if (!conn) {

		
		if (errflag) {
			
			return OPJ_FALSE;
		}

	}

	
	

	
	if (conn)
		cio->bp = buffer;
	cio_skip(cio, 2); 
	Lepb = cio_read(cio, 2);
	Depb = cio_read(cio, 1);
	LDPepb = cio_read(cio, 4);
	Pepb = cio_read(cio, 4);

	
	if (((Pepb & 0xF0000000) >> 28) == 0)
		sprintf(str1, "pred"); 
	else if (((Pepb & 0xF0000000) >> 28) == 1)
		sprintf(str1, "crc-%lu", 16 * ((Pepb & 0x00000001) + 1)); 
	else if (((Pepb & 0xF0000000) >> 28) == 2)
		sprintf(str1, "rs(%lu,32)", (Pepb & 0x0000FF00) >> 8); 
	else if (Pepb == 0xFFFFFFFF)
		sprintf(str1, "nometh"); 
	else
		sprintf(str1, "unknown"); 

	
	if (!conn && post_len)
		opj_event_msg(j2k->cinfo, EVT_INFO,
			"EPB(%d): (%sl, %sp, %u), %lu, %s\n",
			cio_tell(cio) - 13,
			(Depb & 0x40) ? "" : "n", 
			(Depb & 0x80) ? "" : "n", 
			(Depb & 0x3F), 
			LDPepb, 
			str1); 

	
	myconn = Lepb + 2;
	if ((Depb & 0x40) == 0) 
		jpwl_epb_correct(j2k,      
					     buffer + Lepb + 2,   
					     2,     
					     0,  
					     0, 
						 &myconn,
						 NULL
					     );
	if (conn)
		*conn += myconn;

	

	

	
	if (!(L4_bufp))
		L4_buf = buffer + myconn;
	else if (!(*L4_bufp))
		L4_buf = buffer + myconn;
	else
		L4_buf = *L4_bufp;
	if (post_len == -1) 
		L4 = LDPepb - pre_len - 13;
	else if (post_len == 0)
		L4 = 0;
	else
		L4 = post_len;

	L3_buf = L2_buf;

	
	if (L4 > (unsigned long) cio_numbytesleft(j2k->cio))
		
		return OPJ_FALSE;

	
	if (((Pepb & 0xF0000000) >> 28) == 1) {
		
		if ((16 * ((Pepb & 0x00000001) + 1)) == 16) {

			
			unsigned short int mycrc = 0x0000, filecrc = 0x0000;

			
			remaining = L4;
			while (remaining--)
				jpwl_updateCRC16(&mycrc, *(L4_buf++));

			
			filecrc = *(L3_buf++) << 8;
			filecrc |= *(L3_buf++);

			
			if (mycrc == filecrc) {
				if (conn == NULL)
					opj_event_msg(j2k->cinfo, EVT_INFO, "- CRC is OK\n");
			} else {
				if (conn == NULL)
					opj_event_msg(j2k->cinfo, EVT_WARNING, "- CRC is KO (r=%d, c=%d)\n", filecrc, mycrc);
				errflag = OPJ_TRUE;
			}	
		}

		if ((16 * ((Pepb & 0x00000001) + 1)) == 32) {

			
			unsigned long int mycrc = 0x00000000, filecrc = 0x00000000;

			
			remaining = L4;
			while (remaining--)
				jpwl_updateCRC32(&mycrc, *(L4_buf++));

			
			filecrc = *(L3_buf++) << 24;
			filecrc |= *(L3_buf++) << 16;
			filecrc |= *(L3_buf++) << 8;
			filecrc |= *(L3_buf++);

			
			if (mycrc == filecrc) {
				if (conn == NULL)
					opj_event_msg(j2k->cinfo, EVT_INFO, "- CRC is OK\n");
			} else {
				if (conn == NULL)
					opj_event_msg(j2k->cinfo, EVT_WARNING, "- CRC is KO (r=%d, c=%d)\n", filecrc, mycrc);
				errflag = OPJ_TRUE;
			}
		}

	} else if (Pepb == 0xFFFFFFFF) {
		

		
		remaining = L4;
		while (remaining--)
			L4_buf++;

	} else if ((((Pepb & 0xF0000000) >> 28) == 2) || (((Pepb & 0xF0000000) >> 28) == 0)) {
		

		if (((Pepb & 0xF0000000) >> 28) == 0) {

			k_post = k_pre;
			n_post = n_pre;

		} else {

			k_post = 32;
			n_post = (Pepb & 0x0000FF00) >> 8;
		}

		
		P = n_post - k_post;
		NN_P = NN - P;
		tt = (int) floor((float) P / 2.0F); 
		memset(codeword, 0, NN);
		parityword = codeword + NN_P;
		init_rs(NN_P);

		
		
		L3_buf = L2_buf;
		remaining = L4;
		while (remaining) {
 
			
			
			memset(codeword, 0, NN);

			
			if (remaining < k_post)
				memcpy(codeword, L4_buf, remaining);
			else
				memcpy(codeword, L4_buf, k_post);

			
			memcpy(parityword, L3_buf, P); 

			
			status = eras_dec_rs(codeword, NULL, 0);
			if (status == -1) {
				
				errflag = OPJ_TRUE;

			} else if (status == 0) {
				

			} else if (status <= tt) {
				
				errnum += status;

			} else {
				
				errflag = OPJ_TRUE;
			}

			
			if ((status >= 0) && (status <= tt))
				
				memcpy(L3_buf, parityword, P);
			L3_buf += P;

			
			if (remaining < k_post) {
				if ((status >= 0) && (status <= tt))
					
					memcpy(L4_buf, codeword, remaining);
				L4_buf += remaining;
				remaining = 0;

			} else {
				if ((status >= 0) && (status <= tt))
					
					memcpy(L4_buf, codeword, k_post);
				L4_buf += k_post;
				remaining -= k_post;

			}
		}
	}

	
	if (L4_bufp)
		*L4_bufp = L4_buf;

	
	if (!conn) {

		if (errnum)
			opj_event_msg(j2k->cinfo, EVT_INFO, "- %d symbol errors corrected (Ps=%.1e)\n", errnum,
				(float) errnum / (float) LDPepb);
		if (errflag)
			opj_event_msg(j2k->cinfo, EVT_INFO, "- there were unrecoverable errors\n");

	}

	cio_seek(j2k->cio, orig_pos);

	return OPJ_TRUE;
}

void jpwl_epc_write(opj_j2k_t *j2k, jpwl_epc_ms_t *epc, unsigned char *buf) {

	
	*(buf++) = (unsigned char) (J2K_MS_EPC >> 8); 
	*(buf++) = (unsigned char) (J2K_MS_EPC >> 0); 

	
	*(buf++) = (unsigned char) (epc->Lepc >> 8); 
	*(buf++) = (unsigned char) (epc->Lepc >> 0); 

	
	*(buf++) = (unsigned char) (epc->Pcrc >> 8); 
	*(buf++) = (unsigned char) (epc->Pcrc >> 0);

	
	*(buf++) = (unsigned char) (epc->DL >> 24); 
	*(buf++) = (unsigned char) (epc->DL >> 16); 
	*(buf++) = (unsigned char) (epc->DL >> 8); 
	*(buf++) = (unsigned char) (epc->DL >> 0); 

	
	*(buf++) = (unsigned char) (epc->Pepc >> 0); 

	
	
	memset(buf, 0, (size_t) epc->Lepc - 9);

	
	j2k_add_marker(j2k->cstr_info, J2K_MS_EPC, -1, epc->Lepc + 2);

};

int jpwl_esds_add(opj_j2k_t *j2k, jpwl_marker_t *jwmarker, int *jwmarker_num,
				  int comps, unsigned char addrm, unsigned char ad_size,
				  unsigned char senst, unsigned char se_size,
				  double place_pos, int tileno) {

	return 0;
}

jpwl_esd_ms_t *jpwl_esd_create(opj_j2k_t *j2k, int comp, 
	unsigned char addrm, unsigned char ad_size,
	unsigned char senst, int se_size, int tileno,
	unsigned long int svalnum, void *sensval) {

	jpwl_esd_ms_t *esd = NULL;

	
	if (!(esd = (jpwl_esd_ms_t *) opj_malloc((size_t) 1 * sizeof (jpwl_esd_ms_t)))) {
		opj_event_msg(j2k->cinfo, EVT_ERROR, "Could not allocate room for ESD MS\n");
		return NULL;
	};

	
	if (senst == 0)
		addrm = 1;

	
	if ((ad_size != 0) && (ad_size != 2) && (ad_size != 4)) {
		opj_event_msg(j2k->cinfo, EVT_ERROR, "Address size %d for ESD MS is forbidden\n", ad_size);
		return NULL;
	}
	if ((se_size != 1) && (se_size != 2)) {
		opj_event_msg(j2k->cinfo, EVT_ERROR, "Sensitivity size %d for ESD MS is forbidden\n", se_size);
		return NULL;
	}
	
	
	switch (addrm) {

	
	case (0):
		ad_size = 0; 
		esd->sensval_size = (unsigned int)se_size; 
		break;

	
	case (1):
		
		if (ad_size == 0)
			
			ad_size = (j2k->cstr_info->codestream_size > (1 * 65535 / 3)) ? 4 : 2;
		esd->sensval_size = ad_size + ad_size + se_size; 
		break;

	
	case (2):
		
		if (ad_size == 0)
			
			ad_size = (j2k->cstr_info->packno > 65535) ? 4 : 2;
		esd->sensval_size = ad_size + ad_size + se_size; 
		break;

	case (3):
		opj_event_msg(j2k->cinfo, EVT_ERROR, "Address mode %d for ESD MS is unimplemented\n", addrm);
		return NULL;

	default:
		opj_event_msg(j2k->cinfo, EVT_ERROR, "Address mode %d for ESD MS is forbidden\n", addrm);
		return NULL;
	}

	
	if (svalnum <= 0) {

		switch (senst) {

		
		case (0):
			
			svalnum = 1 + (j2k->cstr_info->tw * j2k->cstr_info->th) * (1 + j2k->cstr_info->packno);
			break;

		
		default:
			if (tileno < 0)
				
				svalnum = j2k->cstr_info->tw * j2k->cstr_info->th * j2k->cstr_info->packno;
			else
				
				svalnum = j2k->cstr_info->packno;
			break;

		}
	}		

	
	esd->senst = senst;
	esd->ad_size = ad_size;
	esd->se_size = se_size;
	esd->addrm = addrm;
	esd->svalnum = svalnum;
	esd->numcomps = j2k->image->numcomps;
	esd->tileno = tileno;
	
	
	
	if (esd->numcomps < 257)
		esd->Lesd = 4 + (unsigned short int) (esd->svalnum * esd->sensval_size);
	else
		esd->Lesd = 5 + (unsigned short int) (esd->svalnum * esd->sensval_size);

	
	if (comp >= 0)
		esd->Cesd = comp;
	else
		
		esd->Cesd = 0;

	
	esd->Pesd = 0x00;
	esd->Pesd |= (esd->addrm & 0x03) << 6; 
	esd->Pesd |= (esd->senst & 0x07) << 3; 
	esd->Pesd |= ((esd->se_size >> 1) & 0x01) << 2; 
	esd->Pesd |= ((esd->ad_size >> 2) & 0x01) << 1; 
	esd->Pesd |= (comp < 0) ? 0x01 : 0x00; 

	
	if (!sensval) {

		
		esd->data = NULL;

	} else {
			
			esd->data = (unsigned char *) sensval;
	}

	return (esd);
}

opj_bool jpwl_esd_fill(opj_j2k_t *j2k, jpwl_esd_ms_t *esd, unsigned char *buf) {

	int i;
	unsigned long int vv;
	unsigned long int addr1 = 0L, addr2 = 0L;
	double dvalue = 0.0, Omax2, tmp, TSE = 0.0, MSE, oldMSE = 0.0, PSNR, oldPSNR = 0.0;
	unsigned short int pfpvalue;
	unsigned long int addrmask = 0x00000000;
	opj_bool doneMH = OPJ_FALSE, doneTPH = OPJ_FALSE;

	

	
	Omax2 = 0.0;
	for (i = 0; i < j2k->image->numcomps; i++) {
		tmp = pow(2.0, (double) (j2k->image->comps[i].sgnd ?
			(j2k->image->comps[i].bpp - 1) : (j2k->image->comps[i].bpp))) - 1;
		if (tmp > Omax2)
			Omax2 = tmp;
	}
	Omax2 = Omax2 * Omax2;

	
	if (esd->data) {
		for (i = 0; i < (int) esd->svalnum; i++)
			*(buf++) = esd->data[i]; 
		return OPJ_TRUE;
	}

	
	if (esd->ad_size == 2)
		addrmask = 0x0000FFFF; 
	else
		addrmask = 0xFFFFFFFF; 

	
	if (esd->numcomps < 257)
		buf += 6;
	else
		buf += 7;

	
	for (vv = (esd->tileno < 0) ? 0 : (j2k->cstr_info->packno * esd->tileno); vv < esd->svalnum; vv++) {

		int thistile = vv / j2k->cstr_info->packno, thispacket = vv % j2k->cstr_info->packno;

		
		if (thistile == j2k->cstr_info->tw * j2k->cstr_info->th)
			break;

		
		if (thispacket == 0) {
			TSE = j2k->cstr_info->tile[thistile].distotile;
			oldMSE = TSE / j2k->cstr_info->tile[thistile].numpix;
			oldPSNR = 10.0 * log10(Omax2 / oldMSE);
		}

		
		TSE -= j2k->cstr_info->tile[thistile].packet[thispacket].disto;

		
		MSE = TSE / j2k->cstr_info->tile[thistile].numpix;

		
		PSNR = 10.0 * log10(Omax2 / MSE);

		
		switch (esd->addrm) {

		
		case (0):
			
			break;

		
		case (1):
			
			addr1 = (j2k->cstr_info->tile[thistile].packet[thispacket].start_pos) & addrmask;
			
			addr2 = (j2k->cstr_info->tile[thistile].packet[thispacket].end_pos) & addrmask;
			break;

		
		case (2):
			
			opj_event_msg(j2k->cinfo, EVT_WARNING, "Addressing mode packet_range is not implemented\n");
			break;

		
		default:
			
			opj_event_msg(j2k->cinfo, EVT_WARNING, "Unknown addressing mode\n");
			break;

		}

		
		if ((esd->senst == 0) && (thispacket == 0)) {

			
			if ((thistile == 0) && !doneMH) {
				
				addr1 = 0; 
				addr2 = j2k->cstr_info->main_head_end; 
				
				dvalue = -10.0;
				doneMH = OPJ_TRUE; 
				vv--; 

			} else if (!doneTPH) {
				
				addr1 = j2k->cstr_info->tile[thistile].start_pos;
				addr2 = j2k->cstr_info->tile[thistile].end_header;
				
				dvalue = -1.0;
				doneTPH = OPJ_TRUE; 
				vv--; 
			}

		} else
			doneTPH = OPJ_FALSE; 

		
		switch (esd->ad_size) {

		case (0):
			
			break;

		case (2):
			
			*(buf++) = (unsigned char) (addr1 >> 8); 
			*(buf++) = (unsigned char) (addr1 >> 0); 
			*(buf++) = (unsigned char) (addr2 >> 8); 
			*(buf++) = (unsigned char) (addr2 >> 0); 
			break;

		case (4):
			
			*(buf++) = (unsigned char) (addr1 >> 24); 
			*(buf++) = (unsigned char) (addr1 >> 16); 
			*(buf++) = (unsigned char) (addr1 >> 8); 
			*(buf++) = (unsigned char) (addr1 >> 0); 
			*(buf++) = (unsigned char) (addr2 >> 24); 
			*(buf++) = (unsigned char) (addr2 >> 16); 
			*(buf++) = (unsigned char) (addr2 >> 8); 
			*(buf++) = (unsigned char) (addr2 >> 0); 
			break;

		default:
			
			break;
		}

		
		switch (esd->senst) {

		
		case (0):
			
			if (dvalue == -10)
				
				dvalue = MAX_V1 + 1000.0; 
			else if (dvalue == -1)
				
				dvalue = MAX_V1 + 1000.0; 
			else
				
				dvalue = jpwl_pfp_to_double((unsigned short) (j2k->cstr_info->packno - thispacket), esd->se_size);
			break;

		
		case (1):
			
			dvalue = MSE;
			break;

		
		case (2):
			dvalue = oldMSE - MSE;
			oldMSE = MSE;
			break;

		
		case (3):
			dvalue = PSNR;
			break;

		
		case (4):
			dvalue = PSNR - oldPSNR;
			oldPSNR = PSNR;
			break;

		
		case (5):
			dvalue = 0.0;
			opj_event_msg(j2k->cinfo, EVT_WARNING, "MAXERR sensitivity mode is not implemented\n");
			break;

		
		case (6):
			dvalue = TSE;
			break;

		
		case (7):
			dvalue = 0.0;
			opj_event_msg(j2k->cinfo, EVT_WARNING, "Reserved sensitivity mode is not implemented\n");
			break;

		default:
			dvalue = 0.0;
			break;
		}

		
		pfpvalue = jpwl_double_to_pfp(dvalue, esd->se_size);

		
		switch (esd->se_size) {

		case (1):
			
			*(buf++) = (unsigned char) (pfpvalue >> 0); 
			break;

		case (2):
			
			*(buf++) = (unsigned char) (pfpvalue >> 8); 
			*(buf++) = (unsigned char) (pfpvalue >> 0); 
			break;
		}

	}

	return OPJ_TRUE;
}

void jpwl_esd_write(opj_j2k_t *j2k, jpwl_esd_ms_t *esd, unsigned char *buf) {

	
	*(buf++) = (unsigned char) (J2K_MS_ESD >> 8); 
	*(buf++) = (unsigned char) (J2K_MS_ESD >> 0); 

	
	*(buf++) = (unsigned char) (esd->Lesd >> 8); 
	*(buf++) = (unsigned char) (esd->Lesd >> 0); 

	
	if (esd->numcomps >= 257)
		*(buf++) = (unsigned char) (esd->Cesd >> 8); 
	*(buf++) = (unsigned char) (esd->Cesd >> 0); 

	
	*(buf++) = (unsigned char) (esd->Pesd >> 0); 

	
	if (esd->numcomps < 257)
		memset(buf, 0xAA, (size_t) esd->Lesd - 4);
		
	else
		memset(buf, 0xAA, (size_t) esd->Lesd - 5);
		

	
	j2k_add_marker(j2k->cstr_info, J2K_MS_ESD, -1, esd->Lesd + 2);

}

unsigned short int jpwl_double_to_pfp(double V, int bytes) {

	unsigned short int em, e, m;

	switch (bytes) {

	case (1):

		if (V < MIN_V1) {
			e = 0x0000;
			m = 0x0000;
		} else if (V > MAX_V1) {
			e = 0x000F;
			m = 0x000F;
		} else {
			e = (unsigned short int) (floor(log(V) * 1.44269504088896) / 4.0);
			m = (unsigned short int) (0.5 + (V / (pow(2.0, (double) (4 * e)))));
		}
		em = ((e & 0x000F) << 4) + (m & 0x000F);		
		break;

	case (2):

		if (V < MIN_V2) {
			e = 0x0000;
			m = 0x0000;
		} else if (V > MAX_V2) {
			e = 0x001F;
			m = 0x07FF;
		} else {
			e = (unsigned short int) floor(log(V) * 1.44269504088896) + 15;
			m = (unsigned short int) (0.5 + 2048.0 * ((V / (pow(2.0, (double) e - 15.0))) - 1.0));
		}
		em = ((e & 0x001F) << 11) + (m & 0x07FF);
		break;

	default:

		em = 0x0000;
		break;
	};

	return em;
}

double jpwl_pfp_to_double(unsigned short int em, int bytes) {

	double V;

	switch (bytes) {

	case 1:
		V = (double) (em & 0x0F) * pow(2.0, (double) (em & 0xF0));
		break;

	case 2:

		V = pow(2.0, (double) ((em & 0xF800) >> 11) - 15.0) * (1.0 + (double) (em & 0x07FF) / 2048.0);
		break;

	default:
		V = 0.0;
		break;

	}

	return V;

}

opj_bool jpwl_update_info(opj_j2k_t *j2k, jpwl_marker_t *jwmarker, int jwmarker_num) {

	int mm;
	unsigned long int addlen;

	opj_codestream_info_t *info = j2k->cstr_info;
	int tileno, tpno, packno, numtiles = info->th * info->tw, numpacks = info->packno;

	if (!j2k || !jwmarker ) {
		opj_event_msg(j2k->cinfo, EVT_ERROR, "J2K handle or JPWL markers list badly allocated\n");
		return OPJ_FALSE;
	}

	
	addlen = 0;
	for (mm = 0; mm < jwmarker_num; mm++)
		if (jwmarker[mm].pos < (unsigned long int) info->main_head_end)
			addlen += jwmarker[mm].len + 2;
	info->main_head_end += addlen;

	
	addlen = 0;
	for (mm = 0; mm < jwmarker_num; mm++)
		addlen += jwmarker[mm].len + 2;
	info->codestream_size += addlen;

	
	for (tileno = 0; tileno < numtiles; tileno++) {

		
		addlen = 0;
		for (mm = 0; mm < jwmarker_num; mm++)
			if (jwmarker[mm].pos < (unsigned long int) info->tile[tileno].start_pos)
				addlen += jwmarker[mm].len + 2;
		info->tile[tileno].start_pos += addlen;

		
		addlen = 0;
		for (mm = 0; mm < jwmarker_num; mm++)
			if (jwmarker[mm].pos < (unsigned long int) info->tile[tileno].end_header)
				addlen += jwmarker[mm].len + 2;
		info->tile[tileno].end_header += addlen;

		
		
		addlen = 0;
		for (mm = 0; mm < jwmarker_num; mm++)
			if (jwmarker[mm].pos < (unsigned long int) info->tile[tileno].end_pos)
				addlen += jwmarker[mm].len + 2;
		info->tile[tileno].end_pos += addlen;

		
		for (tpno = 0; tpno < info->tile[tileno].num_tps; tpno++) {

			
			addlen = 0;
			for (mm = 0; mm < jwmarker_num; mm++)
				if (jwmarker[mm].pos < (unsigned long int) info->tile[tileno].tp[tpno].tp_start_pos)
					addlen += jwmarker[mm].len + 2;
			info->tile[tileno].tp[tpno].tp_start_pos += addlen;

			
			addlen = 0;
			for (mm = 0; mm < jwmarker_num; mm++)
				if (jwmarker[mm].pos < (unsigned long int) info->tile[tileno].tp[tpno].tp_end_header)
					addlen += jwmarker[mm].len + 2;
			info->tile[tileno].tp[tpno].tp_end_header += addlen;

			
			addlen = 0;
			for (mm = 0; mm < jwmarker_num; mm++)
				if (jwmarker[mm].pos < (unsigned long int) info->tile[tileno].tp[tpno].tp_end_pos)
					addlen += jwmarker[mm].len + 2;
			info->tile[tileno].tp[tpno].tp_end_pos += addlen;

		}

		
		for (packno = 0; packno < numpacks; packno++) {
			
			
			
			addlen = 0;
			for (mm = 0; mm < jwmarker_num; mm++)
				if (jwmarker[mm].pos <= (unsigned long int) info->tile[tileno].packet[packno].start_pos)
					addlen += jwmarker[mm].len + 2;
			info->tile[tileno].packet[packno].start_pos += addlen;

			
			
			
			info->tile[tileno].packet[packno].end_ph_pos += addlen;

			
			
			
			info->tile[tileno].packet[packno].end_pos += addlen;

		}
	}

	

	return OPJ_TRUE;
}

#endif 
