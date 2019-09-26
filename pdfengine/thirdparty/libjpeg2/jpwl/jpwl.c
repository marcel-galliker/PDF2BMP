

#include "opj_includes.h"

#ifdef USE_JPWL

static int jwmarker_num;

static jpwl_marker_t jwmarker[JPWL_MAX_NO_MARKERS]; 

jpwl_epc_ms_t *jpwl_epc_create(opj_j2k_t *j2k, opj_bool esd_on, opj_bool red_on, opj_bool epb_on, opj_bool info_on);

jpwl_esd_ms_t *jpwl_esd_create(opj_j2k_t *j2k, int comps, 
	unsigned char addrm, unsigned char ad_size,
	unsigned char senst, int se_size, int tileno,
	unsigned long int svalnum, void *sensval);
			

int jpwl_markcomp(const void *arg1, const void *arg2);

void jpwl_epb_write(opj_j2k_t *j2k, jpwl_epb_ms_t *epbmark, unsigned char *buf);

void jpwl_epc_write(opj_j2k_t *j2k, jpwl_epc_ms_t *epcmark, unsigned char *buf);

void jpwl_esd_write(opj_j2k_t *j2k, jpwl_esd_ms_t *esdmark, unsigned char *buf);

void jpwl_encode(opj_j2k_t *j2k, opj_cio_t *cio, opj_image_t *image) {

	int mm;

	

	
	for (mm = 0; mm < jwmarker_num; mm++) {

		switch (jwmarker[mm].id) {

		case J2K_MS_EPB:
			opj_free(jwmarker[mm].m.epbmark);
			break;

		case J2K_MS_EPC:
			opj_free(jwmarker[mm].m.epcmark);
			break;

		case J2K_MS_ESD:
			opj_free(jwmarker[mm].m.esdmark);
			break;

		case J2K_MS_RED:
			opj_free(jwmarker[mm].m.redmark);
			break;

		default:
			break;
		}
	}

	
	memset(jwmarker, 0, sizeof(jpwl_marker_t) * JPWL_MAX_NO_MARKERS);

	
	jwmarker_num = 0;

	
	jpwl_prepare_marks(j2k, cio, image);

	
	jpwl_dump_marks(j2k, cio, image);

	
	j2k->pos_correction = 0;

}

void j2k_add_marker(opj_codestream_info_t *cstr_info, unsigned short int type, int pos, int len) {

	if (!cstr_info)
		return;

	
	if ((cstr_info->marknum + 1) > cstr_info->maxmarknum) {
		cstr_info->maxmarknum += 100;
		cstr_info->marker = (opj_marker_info_t*)opj_realloc(cstr_info->marker, cstr_info->maxmarknum * sizeof(opj_marker_info_t));
	}

	
	cstr_info->marker[cstr_info->marknum].type = type;
	cstr_info->marker[cstr_info->marknum].pos = pos;
	cstr_info->marker[cstr_info->marknum].len = len;
	cstr_info->marknum++;

}

void jpwl_prepare_marks(opj_j2k_t *j2k, opj_cio_t *cio, opj_image_t *image) {

	unsigned short int socsiz_len = 0;
	int ciopos = cio_tell(cio), soc_pos = j2k->cstr_info->main_head_start;
	unsigned char *socp = NULL;

	int tileno, acc_tpno, tpno, tilespec, hprot, sens, pprot, packspec, lastileno, packno;

	jpwl_epb_ms_t *epb_mark;
	jpwl_epc_ms_t *epc_mark;
	jpwl_esd_ms_t *esd_mark;

	
	
	cio_seek(cio, soc_pos + 4);
	socsiz_len = (unsigned short int) cio_read(cio, 2) + 4; 
	cio_seek(cio, soc_pos + 0);
	socp = cio_getbp(cio); 

	
	
	if ((epc_mark = jpwl_epc_create(
			j2k,
			j2k->cp->esd_on, 
			j2k->cp->red_on, 
			j2k->cp->epb_on, 
			OPJ_FALSE 
		))) {

		
		if (epc_mark) {
			jwmarker[jwmarker_num].id = J2K_MS_EPC; 
			jwmarker[jwmarker_num].m.epcmark = epc_mark; 
			jwmarker[jwmarker_num].pos = soc_pos + socsiz_len; 
			jwmarker[jwmarker_num].dpos = (double) jwmarker[jwmarker_num].pos + 0.1; 
			jwmarker[jwmarker_num].len = epc_mark->Lepc; 
			jwmarker[jwmarker_num].len_ready = OPJ_TRUE; 
			jwmarker[jwmarker_num].pos_ready = OPJ_TRUE; 
			jwmarker[jwmarker_num].parms_ready = OPJ_FALSE; 
			jwmarker[jwmarker_num].data_ready = OPJ_TRUE; 
			jwmarker_num++;
		};

		opj_event_msg(j2k->cinfo, EVT_INFO,
			"MH  EPC : setting %s%s%s\n",
			j2k->cp->esd_on ? "ESD, " : "",
			j2k->cp->red_on ? "RED, " : "",
			j2k->cp->epb_on ? "EPB, " : ""
			);

	} else {
		
		opj_event_msg(j2k->cinfo, EVT_ERROR, "Could not create MH EPC\n");				
	};

	
	
	if (j2k->cp->esd_on && (j2k->cp->sens_MH >= 0)) {

		
		if ((esd_mark = jpwl_esd_create(
			j2k, 
			-1, 
			(unsigned char) j2k->cp->sens_range, 
			(unsigned char) j2k->cp->sens_addr, 
			(unsigned char) j2k->cp->sens_MH, 
			j2k->cp->sens_size, 
			-1, 
			0 , 
			NULL  
			))) {
			
			
			if (jwmarker_num < JPWL_MAX_NO_MARKERS) {
				jwmarker[jwmarker_num].id = J2K_MS_ESD; 
				jwmarker[jwmarker_num].m.esdmark = esd_mark; 
				jwmarker[jwmarker_num].pos = soc_pos + socsiz_len; 
				jwmarker[jwmarker_num].dpos = (double) jwmarker[jwmarker_num].pos + 0.2; 
				jwmarker[jwmarker_num].len = esd_mark->Lesd; 
				jwmarker[jwmarker_num].len_ready = OPJ_TRUE; 
				jwmarker[jwmarker_num].pos_ready = OPJ_TRUE; 
				jwmarker[jwmarker_num].parms_ready = OPJ_TRUE; 
				jwmarker[jwmarker_num].data_ready = OPJ_FALSE; 
				jwmarker_num++;
			}

			opj_event_msg(j2k->cinfo, EVT_INFO,
				"MH  ESDs: method %d\n",
				j2k->cp->sens_MH
				);

		} else {
			
			opj_event_msg(j2k->cinfo, EVT_ERROR, "Could not create MH ESD\n");				
		};

	}

	
	
	sens = -1; 
	tilespec = 0; 
	acc_tpno = 0;
	for (tileno = 0; tileno < j2k->cstr_info->tw * j2k->cstr_info->th; tileno++) {

		opj_event_msg(j2k->cinfo, EVT_INFO,
			"Tile %d has %d tile part(s)\n",
			tileno, j2k->cstr_info->tile[tileno].num_tps
			);

		
		for (tpno = 0; tpno < j2k->cstr_info->tile[tileno].num_tps; tpno++, acc_tpno++) {
	
			int sot_len, Psot, Psotp, mm;
			unsigned long sot_pos, post_sod_pos;

			unsigned long int left_THmarks_len;

			
			sot_pos = j2k->cstr_info->tile[tileno].tp[tpno].tp_start_pos;
			cio_seek(cio, sot_pos + 2); 
			sot_len = cio_read(cio, 2); 
			cio_skip(cio, 2);
			Psotp = cio_tell(cio);
			Psot = cio_read(cio, 4); 

			
			post_sod_pos = j2k->cstr_info->tile[tileno].tp[tpno].tp_end_header + 1;
			left_THmarks_len = post_sod_pos - sot_pos;

			
			for (mm = 0; mm < jwmarker_num; mm++) {
				if ((jwmarker[mm].pos >= sot_pos) && (jwmarker[mm].pos < post_sod_pos)) {
					if (jwmarker[mm].len_ready)
						left_THmarks_len += jwmarker[mm].len + 2;
					else {
						opj_event_msg(j2k->cinfo, EVT_ERROR, "MS %x in %f is not len-ready: could not set up TH EPB\n",
							jwmarker[mm].id, jwmarker[mm].dpos);				
						exit(1);
					}
				}
			}

			
			if ((tilespec < JPWL_MAX_NO_TILESPECS) && (j2k->cp->sens_TPH_tileno[tilespec] == acc_tpno))
				
				sens = j2k->cp->sens_TPH[tilespec++];
		
			
			if (j2k->cp->esd_on && (sens >= 0)) {

				
				if ((esd_mark = jpwl_esd_create(
					j2k, 
					-1, 
					(unsigned char) j2k->cp->sens_range, 
					(unsigned char) j2k->cp->sens_addr, 
					(unsigned char) sens, 
					j2k->cp->sens_size, 
					tileno, 
					0, 
					NULL 
					))) {
					
					
					if (jwmarker_num < JPWL_MAX_NO_MARKERS) {
						jwmarker[jwmarker_num].id = J2K_MS_ESD; 
						jwmarker[jwmarker_num].m.esdmark = esd_mark; 
						 
						jwmarker[jwmarker_num].pos = j2k->cstr_info->tile[tileno].tp[tpno].tp_start_pos + sot_len + 2; 
						jwmarker[jwmarker_num].dpos = (double) jwmarker[jwmarker_num].pos + 0.2; 
						jwmarker[jwmarker_num].len = esd_mark->Lesd; 
						jwmarker[jwmarker_num].len_ready = OPJ_TRUE; 
						jwmarker[jwmarker_num].pos_ready = OPJ_TRUE; 
						jwmarker[jwmarker_num].parms_ready = OPJ_TRUE; 
						jwmarker[jwmarker_num].data_ready = OPJ_FALSE; 
						jwmarker_num++;
					}

					
					cio_seek(cio, Psotp);
					cio_write(cio, Psot + esd_mark->Lesd + 2, 4);

					opj_event_msg(j2k->cinfo, EVT_INFO,
						
						"TPH ESDs: tile %02d, part %02d, method %d\n",
						
						tileno, tpno,
						sens
						);

				} else {
					
					
					opj_event_msg(j2k->cinfo, EVT_ERROR, "Could not create TPH ESD #%d,%d\n", tileno, tpno);
				};

			}
			
		}
	
	};

	
	
	if (j2k->cp->epb_on && (j2k->cp->hprot_MH > 0)) {

		int mm;

		
		unsigned int sot_pos = j2k->cstr_info->main_head_end + 1;

		
		int left_MHmarks_len = sot_pos - socsiz_len;

		
		for (mm = 0; mm < jwmarker_num; mm++) {
			if ((jwmarker[mm].pos >=0) && (jwmarker[mm].pos < sot_pos)) {
				if (jwmarker[mm].len_ready)
					left_MHmarks_len += jwmarker[mm].len + 2;
				else {
					opj_event_msg(j2k->cinfo, EVT_ERROR, "MS %x in %f is not len-ready: could not set up MH EPB\n",
						jwmarker[mm].id, jwmarker[mm].dpos);				
					exit(1);
				}
			}
		}

		
		if ((epb_mark = jpwl_epb_create(
			j2k, 
			OPJ_TRUE, 
			OPJ_TRUE, 
			-1, 
			0, 
			j2k->cp->hprot_MH, 
			socsiz_len, 
			left_MHmarks_len 
			))) {
			
			
			if (jwmarker_num < JPWL_MAX_NO_MARKERS) {
				jwmarker[jwmarker_num].id = J2K_MS_EPB; 
				jwmarker[jwmarker_num].m.epbmark = epb_mark; 
				jwmarker[jwmarker_num].pos = soc_pos + socsiz_len; 
				jwmarker[jwmarker_num].dpos = (double) jwmarker[jwmarker_num].pos; 
				jwmarker[jwmarker_num].len = epb_mark->Lepb; 
				jwmarker[jwmarker_num].len_ready = OPJ_TRUE; 
				jwmarker[jwmarker_num].pos_ready = OPJ_TRUE; 
				jwmarker[jwmarker_num].parms_ready = OPJ_TRUE; 
				jwmarker[jwmarker_num].data_ready = OPJ_FALSE; 
				jwmarker_num++;
			}

			opj_event_msg(j2k->cinfo, EVT_INFO,
				"MH  EPB : prot. %d\n",
				j2k->cp->hprot_MH
				);

		} else {
			
			opj_event_msg(j2k->cinfo, EVT_ERROR, "Could not create MH EPB\n");				
		};
	}

	
	
	hprot = j2k->cp->hprot_MH; 
	tilespec = 0; 
	lastileno = 0;
	packspec = 0;
	pprot = -1;
	acc_tpno = 0;
	for (tileno = 0; tileno < j2k->cstr_info->tw * j2k->cstr_info->th; tileno++) {

		opj_event_msg(j2k->cinfo, EVT_INFO,
			"Tile %d has %d tile part(s)\n",
			tileno, j2k->cstr_info->tile[tileno].num_tps
			);

		
		for (tpno = 0; tpno < j2k->cstr_info->tile[tileno].num_tps; tpno++, acc_tpno++) { 
		
			int sot_len, Psot, Psotp, mm, epb_index = 0, prot_len = 0;
			unsigned long sot_pos, post_sod_pos;
			unsigned long int left_THmarks_len;
			int startpack = 0, stoppack = j2k->cstr_info->packno;
			int first_tp_pack, last_tp_pack;
			jpwl_epb_ms_t *tph_epb = NULL;

			
			sot_pos = j2k->cstr_info->tile[tileno].tp[tpno].tp_start_pos;
			cio_seek(cio, sot_pos + 2); 
			sot_len = cio_read(cio, 2); 
			cio_skip(cio, 2);
			Psotp = cio_tell(cio);
			Psot = cio_read(cio, 4); 

			
			
			post_sod_pos = j2k->cstr_info->tile[tileno].tp[tpno].tp_end_header + 1;
			left_THmarks_len = post_sod_pos - (sot_pos + sot_len + 2);

			
			for (mm = 0; mm < jwmarker_num; mm++) {
				if ((jwmarker[mm].pos >= sot_pos) && (jwmarker[mm].pos < post_sod_pos)) {
					if (jwmarker[mm].len_ready)
						left_THmarks_len += jwmarker[mm].len + 2;
					else {
						opj_event_msg(j2k->cinfo, EVT_ERROR, "MS %x in %f is not len-ready: could not set up TH EPB\n",
							jwmarker[mm].id, jwmarker[mm].dpos);				
						exit(1);
					}
				}
			}

			
			if ((tilespec < JPWL_MAX_NO_TILESPECS) && (j2k->cp->hprot_TPH_tileno[tilespec] == acc_tpno))
				
				hprot = j2k->cp->hprot_TPH[tilespec++];
		
			
			if (j2k->cp->epb_on && (hprot > 0)) {

				
				if ((epb_mark = jpwl_epb_create(
					j2k, 
					OPJ_FALSE, 
					OPJ_TRUE, 
					tileno, 
					epb_index++, 
					hprot, 
					sot_len + 2, 
					left_THmarks_len 
					))) {
					
					
					if (jwmarker_num < JPWL_MAX_NO_MARKERS) {
						jwmarker[jwmarker_num].id = J2K_MS_EPB; 
						jwmarker[jwmarker_num].m.epbmark = epb_mark; 
						 
						jwmarker[jwmarker_num].pos = j2k->cstr_info->tile[tileno].tp[tpno].tp_start_pos + sot_len + 2; 
						jwmarker[jwmarker_num].dpos = (double) jwmarker[jwmarker_num].pos; 
						jwmarker[jwmarker_num].len = epb_mark->Lepb; 
						jwmarker[jwmarker_num].len_ready = OPJ_TRUE; 
						jwmarker[jwmarker_num].pos_ready = OPJ_TRUE; 
						jwmarker[jwmarker_num].parms_ready = OPJ_TRUE; 
						jwmarker[jwmarker_num].data_ready = OPJ_FALSE; 
						jwmarker_num++;
					}

					
					Psot += epb_mark->Lepb + 2;

					opj_event_msg(j2k->cinfo, EVT_INFO,
						
						"TPH EPB : tile %02d, part %02d, prot. %d\n",
						
						tileno, tpno,
						hprot
						);

					
					tph_epb = epb_mark;

				} else {
					
					
					opj_event_msg(j2k->cinfo, EVT_ERROR, "Could not create TPH EPB in #%d,d\n", tileno, tpno);
				};

			}				
		
			startpack = 0;
			
			
			
			first_tp_pack = j2k->cstr_info->tile[tileno].tp[tpno].tp_start_pack;
			last_tp_pack = first_tp_pack + j2k->cstr_info->tile[tileno].tp[tpno].tp_numpacks - 1;
			for (packno = 0; packno < j2k->cstr_info->tile[tileno].tp[tpno].tp_numpacks; packno++) {

				
				if ((packspec < JPWL_MAX_NO_PACKSPECS) &&
					(j2k->cp->pprot_tileno[packspec] == acc_tpno) && (j2k->cp->pprot_packno[packspec] == packno)) {

					
					
					if (packno > 0) {
						stoppack = packno - 1;				
						opj_event_msg(j2k->cinfo, EVT_INFO,
							
							"UEP EPBs: tile %02d, part %02d, packs. %02d-%02d (B %d-%d), prot. %d\n",
							
							tileno, tpno,
							startpack,
							stoppack,
							
							j2k->cstr_info->tile[tileno].packet[first_tp_pack + startpack].start_pos,
							
							j2k->cstr_info->tile[tileno].packet[first_tp_pack + stoppack].end_pos,
							pprot);

						
						prot_len = j2k->cstr_info->tile[tileno].packet[first_tp_pack + stoppack].end_pos + 1 -
							j2k->cstr_info->tile[tileno].packet[first_tp_pack + startpack].start_pos;

						
						
						if ((tileno == ((j2k->cstr_info->tw * j2k->cstr_info->th) - 1)) &&
							(tpno == (j2k->cstr_info->tile[tileno].num_tps - 1)) &&
							(stoppack == last_tp_pack))
							
							prot_len += 2;

						
						Psot += jpwl_epbs_add(
							j2k, 
							jwmarker, 
							&jwmarker_num, 
							OPJ_FALSE, 
							OPJ_TRUE, 
							OPJ_FALSE, 
							&epb_index, 
							pprot, 
							 
							(double) (j2k->cstr_info->tile[tileno].tp[tpno].tp_start_pos + sot_len + 2) + 0.0001, 
							tileno, 
							0, 
							prot_len  
							);
					}

					startpack = packno;
					pprot = j2k->cp->pprot[packspec++];
				}

				
		
			}

			
			stoppack = packno - 1;
			if (pprot >= 0) {

				opj_event_msg(j2k->cinfo, EVT_INFO,
					
					"UEP EPBs: tile %02d, part %02d, packs. %02d-%02d (B %d-%d), prot. %d\n",
					
					tileno, tpno,
					startpack,
					stoppack,
					
					j2k->cstr_info->tile[tileno].packet[first_tp_pack + startpack].start_pos,
					j2k->cstr_info->tile[tileno].packet[first_tp_pack + stoppack].end_pos,
					pprot);

				
				prot_len = j2k->cstr_info->tile[tileno].packet[first_tp_pack + stoppack].end_pos + 1 -
					j2k->cstr_info->tile[tileno].packet[first_tp_pack + startpack].start_pos;

				
				
				if ((tileno == ((j2k->cstr_info->tw * j2k->cstr_info->th) - 1)) &&
					(tpno == (j2k->cstr_info->tile[tileno].num_tps - 1)) &&
					(stoppack == last_tp_pack))
					
					prot_len += 2;

				
				Psot += jpwl_epbs_add(
							j2k, 
							jwmarker, 
							&jwmarker_num, 
							OPJ_TRUE, 
							OPJ_TRUE, 
							OPJ_FALSE, 
							&epb_index, 
							pprot, 
							 
							(double) (j2k->cstr_info->tile[tileno].tp[tpno].tp_start_pos + sot_len + 2) + 0.0001, 
							tileno, 
							0, 
							prot_len  
							);
			}

			
			if (tph_epb && (epb_index == 1)) {
				
				tph_epb->Depb |= (unsigned char) ((OPJ_TRUE & 0x0001) << 6);
				tph_epb = NULL;
			}

			
			cio_seek(cio, Psotp);
			cio_write(cio, Psot, 4);
		
		}

	};

	
	cio_seek(cio, ciopos);

}

void jpwl_dump_marks(opj_j2k_t *j2k, opj_cio_t *cio, opj_image_t *image) {

	int mm;
	unsigned long int old_size = j2k->cstr_info->codestream_size;
	unsigned long int new_size = old_size;
	int  soc_pos = j2k->cstr_info->main_head_start;
	unsigned char *jpwl_buf, *orig_buf;
	unsigned long int orig_pos;
	double epbcoding_time = 0.0, esdcoding_time = 0.0;

	
	qsort((void *) jwmarker, (size_t) jwmarker_num, sizeof (jpwl_marker_t), jpwl_markcomp);

	 
	for (mm = 0; mm < jwmarker_num; mm++) {
		
		new_size += jwmarker[mm].len + 2;
	}

	
	if (!(jpwl_buf = (unsigned char *) opj_malloc((size_t) (new_size + soc_pos) * sizeof(unsigned char)))) {
		opj_event_msg(j2k->cinfo, EVT_ERROR, "Could not allocate room for JPWL codestream buffer\n");
		exit(1);
	};

	
	orig_buf = jpwl_buf;
	memcpy(jpwl_buf, cio->buffer, soc_pos);
	jpwl_buf += soc_pos;

	
	orig_pos = soc_pos + 0; 
	cio_seek(cio, soc_pos + 0); 
	for (mm = 0; mm < jwmarker_num; mm++) {

		
		memcpy(jpwl_buf, cio_getbp(cio), jwmarker[mm].pos - orig_pos);
		jpwl_buf += jwmarker[mm].pos - orig_pos;
		orig_pos = jwmarker[mm].pos;
		cio_seek(cio, orig_pos);

		
		switch (jwmarker[mm].id) {

		case J2K_MS_EPB:
			jpwl_epb_write(j2k, jwmarker[mm].m.epbmark, jpwl_buf);
			break;

		case J2K_MS_EPC:
			jpwl_epc_write(j2k, jwmarker[mm].m.epcmark, jpwl_buf);
			break;

		case J2K_MS_ESD:
			jpwl_esd_write(j2k, jwmarker[mm].m.esdmark, jpwl_buf);
			break;

		case J2K_MS_RED:
			memset(jpwl_buf, 0, jwmarker[mm].len + 2); 
			break;

		default:
			break;
		};

		
		if (j2k->cstr_info)
			j2k->cstr_info->marker[j2k->cstr_info->marknum - 1].pos = (jpwl_buf - orig_buf);
		
		
		jwmarker[mm].dpos = (double) (jpwl_buf - orig_buf);

		
		jpwl_buf += jwmarker[mm].len + 2;

	}

	
	memcpy(jpwl_buf, cio_getbp(cio), old_size - (orig_pos - soc_pos));
	jpwl_buf += old_size - (orig_pos - soc_pos);
	cio_seek(cio, soc_pos + old_size);
	
	
	if (!jpwl_update_info(j2k, jwmarker, jwmarker_num))
		opj_event_msg(j2k->cinfo, EVT_ERROR, "Could not update OPJ cstr_info structure\n");

	
	
	 
	for (mm = 0; mm < jwmarker_num; mm++) {

		
		if (jwmarker[mm].id == J2K_MS_EPC) {

			int epc_pos = (int) jwmarker[mm].dpos, pp;
			unsigned short int mycrc = 0x0000;

			
			jwmarker[mm].m.epcmark->DL = new_size;
			orig_buf[epc_pos + 6] = (unsigned char) (jwmarker[mm].m.epcmark->DL >> 24);
			orig_buf[epc_pos + 7] = (unsigned char) (jwmarker[mm].m.epcmark->DL >> 16);
			orig_buf[epc_pos + 8] = (unsigned char) (jwmarker[mm].m.epcmark->DL >> 8);
			orig_buf[epc_pos + 9] = (unsigned char) (jwmarker[mm].m.epcmark->DL >> 0);

			
			for (pp = 0; pp < 4; pp++)
				jpwl_updateCRC16(&mycrc, orig_buf[epc_pos + pp]);
			for (pp = 6; pp < (jwmarker[mm].len + 2); pp++)
				jpwl_updateCRC16(&mycrc, orig_buf[epc_pos + pp]);

			
			jwmarker[mm].m.epcmark->Pcrc = mycrc;
			orig_buf[epc_pos + 4] = (unsigned char) (jwmarker[mm].m.epcmark->Pcrc >> 8);
			orig_buf[epc_pos + 5] = (unsigned char) (jwmarker[mm].m.epcmark->Pcrc >> 0);

		}
	}

	 
	esdcoding_time = opj_clock();
	for (mm = 0; mm < jwmarker_num; mm++) {

		
		if (jwmarker[mm].id == J2K_MS_ESD) {

			
			int esd_pos = (int) jwmarker[mm].dpos;

			jpwl_esd_fill(j2k, jwmarker[mm].m.esdmark, &orig_buf[esd_pos]);
		
		}

	}
	esdcoding_time = opj_clock() - esdcoding_time;
	if (j2k->cp->esd_on)
		opj_event_msg(j2k->cinfo, EVT_INFO, "ESDs sensitivities computed in %f s\n", esdcoding_time);

	 
	epbcoding_time = opj_clock();
	for (mm = 0; mm < jwmarker_num; mm++) {

		
		if (jwmarker[mm].id == J2K_MS_EPB) {

			
			int nn, accum_len;

			
			
			
			accum_len = 0;
			for (nn = mm; (nn < jwmarker_num) && (jwmarker[nn].id == J2K_MS_EPB) &&
				(jwmarker[nn].pos == jwmarker[mm].pos); nn++)
				accum_len += jwmarker[nn].m.epbmark->Lepb + 2;

			
			jpwl_epb_fill(j2k, jwmarker[mm].m.epbmark, &orig_buf[(int) jwmarker[mm].dpos],
				&orig_buf[(int) jwmarker[mm].dpos + accum_len]);
		
			
			for (nn = mm + 1; (nn < jwmarker_num) && (jwmarker[nn].id == J2K_MS_EPB) &&
				(jwmarker[nn].pos == jwmarker[mm].pos); nn++)
				jpwl_epb_fill(j2k, jwmarker[nn].m.epbmark, &orig_buf[(int) jwmarker[nn].dpos], NULL);

			
			mm = nn - 1;
		}

	}
	epbcoding_time = opj_clock() - epbcoding_time;
	if (j2k->cp->epb_on)
		opj_event_msg(j2k->cinfo, EVT_INFO, "EPBs redundancy computed in %f s\n", epbcoding_time);

	
	opj_free(cio->buffer);
	cio->cinfo = cio->cinfo; 
	cio->openmode = cio->openmode; 
	cio->buffer = orig_buf;
	cio->length = new_size + soc_pos;
	cio->start = cio->buffer;
	cio->end = cio->buffer + cio->length;
	cio->bp = cio->buffer;
	cio_seek(cio, soc_pos + new_size);

}

void j2k_read_epc(opj_j2k_t *j2k) {
	unsigned long int DL, Lepcp, Pcrcp, l;
	unsigned short int Lepc, Pcrc = 0x0000;
	unsigned char Pepc;	
	opj_cio_t *cio = j2k->cio;
	const char *ans1;

	
	Lepcp = cio_tell(cio);
	Lepc = cio_read(cio, 2);
	Pcrcp = cio_tell(cio);
	cio_skip(cio, 2); 
	DL = cio_read(cio, 4);
	Pepc = cio_read(cio, 1);

	
	cio_seek(cio, Lepcp - 2);

		
		jpwl_updateCRC16(&Pcrc, (unsigned char) cio_read(cio, 1)); 
		jpwl_updateCRC16(&Pcrc, (unsigned char) cio_read(cio, 1)); 

		
		jpwl_updateCRC16(&Pcrc, (unsigned char) cio_read(cio, 1)); 
		jpwl_updateCRC16(&Pcrc, (unsigned char) cio_read(cio, 1)); 

		
		cio_skip(cio, 2);

		
		for (l = 4; l < Lepc; l++)
			jpwl_updateCRC16(&Pcrc, (unsigned char) cio_read(cio, 1)); 

		
		cio_seek(cio, Pcrcp);
		ans1 = (Pcrc == (unsigned short int) cio_read(cio, 2)) ? "crc-ok" : "crc-ko";

	
	opj_event_msg(j2k->cinfo, EVT_INFO, 
		"EPC(%u,%d): %s, DL=%d%s %s %s\n",
		Lepcp - 2,
		Lepc,
		ans1,
		DL, 
		(Pepc & 0x10) ? ", esd" : "", 
		(Pepc & 0x20) ? ", red" : "", 
		(Pepc & 0x40) ? ", epb" : ""); 

	cio_seek(cio, Lepcp + Lepc);  
}

void j2k_write_epc(opj_j2k_t *j2k) {

	unsigned long int DL, Lepcp, Pcrcp, l;
	unsigned short int Lepc, Pcrc;
	unsigned char Pepc;	

	opj_cio_t *cio = j2k->cio;

	cio_write(cio, J2K_MS_EPC, 2);	
	Lepcp = cio_tell(cio);
	cio_skip(cio, 2);

	
	Pcrc = 0x0000; 
	Pcrcp = cio_tell(cio);
	cio_write(cio, Pcrc, 2); 

	
	DL = 0x00000000; 
	cio_write(cio, DL, 4);   

	
	Pepc = 0x00;
	cio_write(cio, Pepc, 1); 

	
	

	Lepc = (unsigned short) (cio_tell(cio) - Lepcp);
	cio_seek(cio, Lepcp);
	cio_write(cio, Lepc, 2); 

	
	cio_seek(cio, Lepcp - 2);

		
		jpwl_updateCRC16(&Pcrc, (unsigned char) cio_read(cio, 1)); 
		jpwl_updateCRC16(&Pcrc, (unsigned char) cio_read(cio, 1)); 

		
		jpwl_updateCRC16(&Pcrc, (unsigned char) cio_read(cio, 1)); 
		jpwl_updateCRC16(&Pcrc, (unsigned char) cio_read(cio, 1)); 

		
		cio_skip(cio, 2);

		
		for (l = 4; l < Lepc; l++)
			jpwl_updateCRC16(&Pcrc, (unsigned char) cio_read(cio, 1)); 

		
		cio_seek(cio, Pcrcp);
		cio_write(cio, Pcrc, 2);

	cio_seek(cio, Lepcp + Lepc);

	
	j2k_add_marker(j2k->cstr_info, J2K_MS_EPC, Lepcp - 2, Lepc + 2);

}

void j2k_read_epb(opj_j2k_t *j2k) {
	unsigned long int LDPepb, Pepb;
	unsigned short int Lepb;
	unsigned char Depb;
	char str1[25] = "";
	opj_bool status;
	static opj_bool first_in_tph = OPJ_TRUE;
	int type, pre_len, post_len;
	static unsigned char *redund = NULL;
	
	opj_cio_t *cio = j2k->cio;

	
	
	int skipnum = 2  +     38     + 3 * j2k->cp->exp_comps  +         2;

	if (j2k->cp->correct) {

		
		cio_seek(cio, cio_tell(cio) - 2);

		
		if (j2k->state == J2K_STATE_MH) {
			
			type = 0; 
			pre_len = skipnum; 
			post_len = -1; 

		} else if ((j2k->state == J2K_STATE_TPH) && first_in_tph) {
			
			type = 1; 
			pre_len = 12; 
			first_in_tph = OPJ_FALSE;
			post_len = -1; 

		} else {
			
			type = 2; 
			pre_len = 0; 
			post_len = -1; 

		}

		
		
		status = jpwl_epb_correct(j2k,      
								  cio->bp,  
								  type,     
								  pre_len,  
								  post_len, 
								  NULL,     
								  &redund
								 );
		

		
		cio_skip(cio, 2);
		Lepb = cio_read(cio, 2);
		Depb = cio_read(cio, 1);
		LDPepb = cio_read(cio, 4);
		Pepb = cio_read(cio, 4);

		if (!status) {

			opj_event_msg(j2k->cinfo, EVT_ERROR, "JPWL correction could not be performed\n");

			
			cio_skip(cio, Lepb + 2);  

			return;
		}

		
		if (Depb & 0x40) {
			redund = NULL; 
			first_in_tph = OPJ_TRUE;
		}

		
		cio_skip(cio, Lepb - 11);  

	} else {

		
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

		
		opj_event_msg(j2k->cinfo, EVT_INFO,
			"EPB(%d): (%sl, %sp, %u), %lu, %s\n",
			cio_tell(cio) - 13,
			(Depb & 0x40) ? "" : "n", 
			(Depb & 0x80) ? "" : "n", 
			(Depb & 0x3F), 
			LDPepb, 
			str1); 

		cio_skip(cio, Lepb - 11);  
	}
}

void j2k_write_epb(opj_j2k_t *j2k) {
	unsigned long int LDPepb, Pepb, Lepbp;
	unsigned short int Lepb;
	unsigned char Depb;

	opj_cio_t *cio = j2k->cio;

	cio_write(cio, J2K_MS_EPB, 2);	
	Lepbp = cio_tell(cio);
	cio_skip(cio, 2);

	
	Depb = 0x00; 
	cio_write(cio, Depb, 1);   

	
	LDPepb = 0x00000000; 
	cio_write(cio, LDPepb, 4);   

	
	Pepb = 0x00000000; 
	cio_write(cio, Pepb, 4);   

	
	

	Lepb = (unsigned short) (cio_tell(cio) - Lepbp);
	cio_seek(cio, Lepbp);
	cio_write(cio, Lepb, 2);		

	cio_seek(cio, Lepbp + Lepb);

	
	j2k_add_marker(j2k->cstr_info, J2K_MS_EPB, Lepbp - 2, Lepb + 2);
}

void j2k_read_esd(opj_j2k_t *j2k) {
	unsigned short int Lesd, Cesd;
	unsigned char Pesd;

	int cesdsize = (j2k->image->numcomps >= 257) ? 2 : 1;

	char str1[4][4] = {"p", "br", "pr", "res"};
	char str2[8][8] = {"res", "mse", "mse-r", "psnr", "psnr-i", "maxerr", "tse", "res"};
	
	opj_cio_t *cio = j2k->cio;

	
	Lesd = cio_read(cio, 2);
	Cesd = cio_read(cio, cesdsize);
	Pesd = cio_read(cio, 1);

	
	opj_event_msg(j2k->cinfo, EVT_INFO,
		"ESD(%d): c%d, %s, %s, %s, %s, %s\n",
		cio_tell(cio) - (5 + cesdsize),
		Cesd, 
		str1[(Pesd & (unsigned char) 0xC0) >> 6], 
		str2[(Pesd & (unsigned char) 0x38) >> 3], 
		((Pesd & (unsigned char) 0x04) >> 2) ? "2Bs" : "1Bs",
		((Pesd & (unsigned char) 0x02) >> 1) ? "4Ba" : "2Ba",
		(Pesd & (unsigned char) 0x01) ? "avgc" : "");

	cio_skip(cio, Lesd - (3 + cesdsize));  
}

void j2k_read_red(opj_j2k_t *j2k) {
	unsigned short int Lred;
	unsigned char Pred;
	char str1[4][4] = {"p", "br", "pr", "res"};
	
	opj_cio_t *cio = j2k->cio;

	
	Lred = cio_read(cio, 2);
	Pred = cio_read(cio, 1);

	
	opj_event_msg(j2k->cinfo, EVT_INFO,
		"RED(%d): %s, %dc, %s, %s\n",
		cio_tell(cio) - 5,
		str1[(Pred & (unsigned char) 0xC0) >> 6], 
		(Pred & (unsigned char) 0x38) >> 3, 
		((Pred & (unsigned char) 0x02) >> 1) ? "4Ba" : "2Ba", 
		(Pred & (unsigned char) 0x01) ? "errs" : "free"); 

	cio_skip(cio, Lred - 3);  
}

opj_bool jpwl_check_tile(opj_j2k_t *j2k, opj_tcd_t *tcd, int tileno) {

#ifdef oerhgierhgvhreit4u
	
	int compno, resno, precno,  bandno, blockno;
	int numprecincts, numblocks;

	
	opj_tcd_tile_t *tile = &(tcd->tcd_image->tiles[tileno]);

	
	opj_tcd_tilecomp_t *comp = NULL;

	
	opj_tcd_resolution_t *res;

	
	opj_tcd_band_t *band; 

	
	opj_tcd_precinct_t *prec; 

	
	opj_tcd_cblk_t *block;

	
	for (compno = 0; compno < tile->numcomps; compno++) {
		comp = &(tile->comps[compno]);

		
		for (resno = 0; resno < comp->numresolutions; resno++) {
			res = &(comp->resolutions[resno]);
			numprecincts = res->pw * res->ph;

			
			for (bandno = 0; bandno < res->numbands; bandno++) {
				band = &(res->bands[bandno]);

				
				for (precno = 0; precno < numprecincts; precno++) {
					prec = &(band->precincts[precno]);
					numblocks = prec->ch * prec->cw;

					
					for (blockno = 0; blockno < numblocks; blockno++) {
						block = &(prec->cblks[blockno]);

						
						if ((block->x0 < prec->x0) || (block->x0 > prec->x1)) {
							opj_event_msg(j2k->cinfo, JPWL_ASSUME ? EVT_WARNING : EVT_ERROR,
								"JPWL: wrong x-cord of block origin %d => x-prec is (%d, %d)\n",
								block->x0, prec->x0, prec->x1);
							if (!JPWL_ASSUME || JPWL_ASSUME)
								return OPJ_FALSE;
						};
					}
				}				
			}
		}
	}

#endif

	return OPJ_TRUE;
}

#endif 

#ifdef USE_JPSEC

void j2k_read_sec(opj_j2k_t *j2k) {
	unsigned short int Lsec;
	
	opj_cio_t *cio = j2k->cio;

	
	Lsec = cio_read(cio, 2);

	
	opj_event_msg(j2k->cinfo, EVT_INFO,
		"SEC(%d)\n",
		cio_tell(cio) - 2
		);

	cio_skip(cio, Lsec - 2);  
}

void j2k_write_sec(opj_j2k_t *j2k) {
	unsigned short int Lsec = 24;
	int i;

	opj_cio_t *cio = j2k->cio;

	cio_write(cio, J2K_MS_SEC, 2);	
	cio_write(cio, Lsec, 2);

	
	for (i = 0; i < Lsec - 2; i++)
		cio_write(cio, 0, 1);
}

void j2k_read_insec(opj_j2k_t *j2k) {
	unsigned short int Linsec;
	
	opj_cio_t *cio = j2k->cio;

	
	Linsec = cio_read(cio, 2);

	
	opj_event_msg(j2k->cinfo, EVT_INFO,
		"INSEC(%d)\n",
		cio_tell(cio) - 2
		);

	cio_skip(cio, Linsec - 2);  
}

#endif 

