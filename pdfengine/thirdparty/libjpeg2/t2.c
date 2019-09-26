

#include "opj_includes.h"

static void t2_putcommacode(opj_bio_t *bio, int n);
static int t2_getcommacode(opj_bio_t *bio);

static void t2_putnumpasses(opj_bio_t *bio, int n);
static int t2_getnumpasses(opj_bio_t *bio);

static int t2_encode_packet(opj_tcd_tile_t *tile, opj_tcp_t *tcp, opj_pi_iterator_t *pi, unsigned char *dest, int len, opj_codestream_info_t *cstr_info, int tileno);

static void t2_init_seg(opj_tcd_cblk_dec_t* cblk, int index, int cblksty, int first);

static int t2_decode_packet(opj_t2_t* t2, unsigned char *src, int len, opj_tcd_tile_t *tile, 
														opj_tcp_t *tcp, opj_pi_iterator_t *pi, opj_packet_info_t *pack_info);

static void t2_putcommacode(opj_bio_t *bio, int n) {
	while (--n >= 0) {
		bio_write(bio, 1, 1);
	}
	bio_write(bio, 0, 1);
}

static int t2_getcommacode(opj_bio_t *bio) {
	int n;
	for (n = 0; bio_read(bio, 1); n++) {
		;
	}
	return n;
}

static void t2_putnumpasses(opj_bio_t *bio, int n) {
	if (n == 1) {
		bio_write(bio, 0, 1);
	} else if (n == 2) {
		bio_write(bio, 2, 2);
	} else if (n <= 5) {
		bio_write(bio, 0xc | (n - 3), 4);
	} else if (n <= 36) {
		bio_write(bio, 0x1e0 | (n - 6), 9);
	} else if (n <= 164) {
		bio_write(bio, 0xff80 | (n - 37), 16);
	}
}

static int t2_getnumpasses(opj_bio_t *bio) {
	int n;
	if (!bio_read(bio, 1))
		return 1;
	if (!bio_read(bio, 1))
		return 2;
	if ((n = bio_read(bio, 2)) != 3)
		return (3 + n);
	if ((n = bio_read(bio, 5)) != 31)
		return (6 + n);
	return (37 + bio_read(bio, 7));
}

static int t2_encode_packet(opj_tcd_tile_t * tile, opj_tcp_t * tcp, opj_pi_iterator_t *pi, unsigned char *dest, int length, opj_codestream_info_t *cstr_info, int tileno) {
	int bandno, cblkno;
	unsigned char *c = dest;

	int compno = pi->compno;	
	int resno  = pi->resno;		
	int precno = pi->precno;	
	int layno  = pi->layno;		

	opj_tcd_tilecomp_t *tilec = &tile->comps[compno];
	opj_tcd_resolution_t *res = &tilec->resolutions[resno];
	
	opj_bio_t *bio = NULL;	
	
	
	if (tcp->csty & J2K_CP_CSTY_SOP) {
		c[0] = 255;
		c[1] = 145;
		c[2] = 0;
		c[3] = 4;
		c[4] = (unsigned char)((tile->packno % 65536) / 256);
		c[5] = (unsigned char)((tile->packno % 65536) % 256);
		c += 6;
	}
	
	
	if (!layno) {
		for (bandno = 0; bandno < res->numbands; bandno++) {
			opj_tcd_band_t *band = &res->bands[bandno];
			opj_tcd_precinct_t *prc = &band->precincts[precno];
			tgt_reset(prc->incltree);
			tgt_reset(prc->imsbtree);
			for (cblkno = 0; cblkno < prc->cw * prc->ch; cblkno++) {
				opj_tcd_cblk_enc_t* cblk = &prc->cblks.enc[cblkno];
				cblk->numpasses = 0;
				tgt_setvalue(prc->imsbtree, cblkno, band->numbps - cblk->numbps);
			}
		}
	}
	
	bio = bio_create();
	bio_init_enc(bio, c, length);
	bio_write(bio, 1, 1);		
	
	
	for (bandno = 0; bandno < res->numbands; bandno++) {
		opj_tcd_band_t *band = &res->bands[bandno];
		opj_tcd_precinct_t *prc = &band->precincts[precno];
		for (cblkno = 0; cblkno < prc->cw * prc->ch; cblkno++) {
			opj_tcd_cblk_enc_t* cblk = &prc->cblks.enc[cblkno];
			opj_tcd_layer_t *layer = &cblk->layers[layno];
			if (!cblk->numpasses && layer->numpasses) {
				tgt_setvalue(prc->incltree, cblkno, layno);
			}
		}
		for (cblkno = 0; cblkno < prc->cw * prc->ch; cblkno++) {
			opj_tcd_cblk_enc_t* cblk = &prc->cblks.enc[cblkno];
			opj_tcd_layer_t *layer = &cblk->layers[layno];
			int increment = 0;
			int nump = 0;
			int len = 0, passno;
			
			if (!cblk->numpasses) {
				tgt_encode(bio, prc->incltree, cblkno, layno + 1);
			} else {
				bio_write(bio, layer->numpasses != 0, 1);
			}
			
			if (!layer->numpasses) {
				continue;
			}
			
			if (!cblk->numpasses) {
				cblk->numlenbits = 3;
				tgt_encode(bio, prc->imsbtree, cblkno, 999);
			}
			
			t2_putnumpasses(bio, layer->numpasses);
			
			
			for (passno = cblk->numpasses; passno < cblk->numpasses + layer->numpasses; passno++) {
				opj_tcd_pass_t *pass = &cblk->passes[passno];
				nump++;
				len += pass->len;
				if (pass->term || passno == (cblk->numpasses + layer->numpasses) - 1) {
					increment = int_max(increment, int_floorlog2(len) + 1 - (cblk->numlenbits + int_floorlog2(nump)));
					len = 0;
					nump = 0;
				}
			}
			t2_putcommacode(bio, increment);

			
			cblk->numlenbits += increment;

			
			for (passno = cblk->numpasses; passno < cblk->numpasses + layer->numpasses; passno++) {
				opj_tcd_pass_t *pass = &cblk->passes[passno];
				nump++;
				len += pass->len;
				if (pass->term || passno == (cblk->numpasses + layer->numpasses) - 1) {
					bio_write(bio, len, cblk->numlenbits + int_floorlog2(nump));
					len = 0;
					nump = 0;
				}
			}
		}
	}

	if (bio_flush(bio)) {
		bio_destroy(bio);
		return -999;		
	}

	c += bio_numbytes(bio);
	bio_destroy(bio);
	
	
	if (tcp->csty & J2K_CP_CSTY_EPH) {
		c[0] = 255;
		c[1] = 146;
		c += 2;
	}
	

	
	
	if(cstr_info && cstr_info->index_write) {
		opj_packet_info_t *info_PK = &cstr_info->tile[tileno].packet[cstr_info->packno];
		info_PK->end_ph_pos = (int)(c - dest);
	}
	
	
	
	
	for (bandno = 0; bandno < res->numbands; bandno++) {
		opj_tcd_band_t *band = &res->bands[bandno];
		opj_tcd_precinct_t *prc = &band->precincts[precno];
		for (cblkno = 0; cblkno < prc->cw * prc->ch; cblkno++) {
			opj_tcd_cblk_enc_t* cblk = &prc->cblks.enc[cblkno];
			opj_tcd_layer_t *layer = &cblk->layers[layno];
			if (!layer->numpasses) {
				continue;
			}
			if (c + layer->len > dest + length) {
				return -999;
			}
			
			memcpy(c, layer->data, layer->len);
			cblk->numpasses += layer->numpasses;
			c += layer->len;
			 
			if(cstr_info && cstr_info->index_write) {
				opj_packet_info_t *info_PK = &cstr_info->tile[tileno].packet[cstr_info->packno];
				info_PK->disto += layer->disto;
				if (cstr_info->D_max < info_PK->disto) {
					cstr_info->D_max = info_PK->disto;
				}
			}
			
		}
	}
	
	return (c - dest);
}

static void t2_init_seg(opj_tcd_cblk_dec_t* cblk, int index, int cblksty, int first) {
	opj_tcd_seg_t* seg;
	cblk->segs = (opj_tcd_seg_t*) opj_realloc(cblk->segs, (index + 1) * sizeof(opj_tcd_seg_t));
	seg = &cblk->segs[index];
	seg->data = NULL;
	seg->dataindex = 0;
	seg->numpasses = 0;
	seg->len = 0;
	if (cblksty & J2K_CCP_CBLKSTY_TERMALL) {
		seg->maxpasses = 1;
	}
	else if (cblksty & J2K_CCP_CBLKSTY_LAZY) {
		if (first) {
			seg->maxpasses = 10;
		} else {
			seg->maxpasses = (((seg - 1)->maxpasses == 1) || ((seg - 1)->maxpasses == 10)) ? 2 : 1;
		}
	} else {
		seg->maxpasses = 109;
	}
}

static int t2_decode_packet(opj_t2_t* t2, unsigned char *src, int len, opj_tcd_tile_t *tile, 
														opj_tcp_t *tcp, opj_pi_iterator_t *pi, opj_packet_info_t *pack_info) {
	int bandno, cblkno;
	unsigned char *c = src;

	opj_cp_t *cp = t2->cp;

	int compno = pi->compno;	
	int resno  = pi->resno;		
	int precno = pi->precno;	
	int layno  = pi->layno;		

	opj_tcd_resolution_t* res = &tile->comps[compno].resolutions[resno];

	unsigned char *hd = NULL;
	int present;
	
	opj_bio_t *bio = NULL;	
	
	if (layno == 0) {
		for (bandno = 0; bandno < res->numbands; bandno++) {
			opj_tcd_band_t *band = &res->bands[bandno];
			opj_tcd_precinct_t *prc = &band->precincts[precno];
			
			if ((band->x1-band->x0 == 0)||(band->y1-band->y0 == 0)) continue;
			
			tgt_reset(prc->incltree);
			tgt_reset(prc->imsbtree);
			for (cblkno = 0; cblkno < prc->cw * prc->ch; cblkno++) {
				opj_tcd_cblk_dec_t* cblk = &prc->cblks.dec[cblkno];
				cblk->numsegs = 0;
			}
		}
	}
	
	
	
	if (tcp->csty & J2K_CP_CSTY_SOP) {
		if ((*c) != 0xff || (*(c + 1) != 0x91)) {
			opj_event_msg(t2->cinfo, EVT_WARNING, "Expected SOP marker\n");
		} else {
			c += 6;
		}
		
		
	}
	
	

	bio = bio_create();
	
	if (cp->ppm == 1) {		
		hd = cp->ppm_data;
		bio_init_dec(bio, hd, cp->ppm_len);
	} else if (tcp->ppt == 1) {	
		hd = tcp->ppt_data;
		bio_init_dec(bio, hd, tcp->ppt_len);
	} else {			
		hd = c;
		bio_init_dec(bio, hd, src+len-hd);
	}
	
	present = bio_read(bio, 1);
	
	if (!present) {
		bio_inalign(bio);
		hd += bio_numbytes(bio);
		bio_destroy(bio);
		
		
		
		if (tcp->csty & J2K_CP_CSTY_EPH) {
			if ((*hd) != 0xff || (*(hd + 1) != 0x92)) {
				printf("Error : expected EPH marker\n");
			} else {
				hd += 2;
			}
		}

		
		
		if(pack_info) {
			pack_info->end_ph_pos = (int)(c - src);
		}
		
		
		if (cp->ppm == 1) {		
			cp->ppm_len += cp->ppm_data-hd;
			cp->ppm_data = hd;
			return (c - src);
		}
		if (tcp->ppt == 1) {	
			tcp->ppt_len+=tcp->ppt_data-hd;
			tcp->ppt_data = hd;
			return (c - src);
		}
		
		return (hd - src);
	}
	
	for (bandno = 0; bandno < res->numbands; bandno++) {
		opj_tcd_band_t *band = &res->bands[bandno];
		opj_tcd_precinct_t *prc = &band->precincts[precno];
		
		if ((band->x1-band->x0 == 0)||(band->y1-band->y0 == 0)) continue;
		
		for (cblkno = 0; cblkno < prc->cw * prc->ch; cblkno++) {
			int included, increment, n, segno;
			opj_tcd_cblk_dec_t* cblk = &prc->cblks.dec[cblkno];
			
			if (!cblk->numsegs) {
				included = tgt_decode(bio, prc->incltree, cblkno, layno + 1);
				
			} else {
				included = bio_read(bio, 1);
			}
			
			if (!included) {
				cblk->numnewpasses = 0;
				continue;
			}
			
			if (!cblk->numsegs) {
				int i, numimsbs;
				for (i = 0; !tgt_decode(bio, prc->imsbtree, cblkno, i); i++) {
					;
				}
				numimsbs = i - 1;
				cblk->numbps = band->numbps - numimsbs;
				cblk->numlenbits = 3;
			}
			
			cblk->numnewpasses = t2_getnumpasses(bio);
			increment = t2_getcommacode(bio);
			
			cblk->numlenbits += increment;
			segno = 0;
			if (!cblk->numsegs) {
				t2_init_seg(cblk, segno, tcp->tccps[compno].cblksty, 1);
			} else {
				segno = cblk->numsegs - 1;
				if (cblk->segs[segno].numpasses == cblk->segs[segno].maxpasses) {
					++segno;
					t2_init_seg(cblk, segno, tcp->tccps[compno].cblksty, 0);
				}
			}
			n = cblk->numnewpasses;
			
			do {
				cblk->segs[segno].numnewpasses = int_min(cblk->segs[segno].maxpasses - cblk->segs[segno].numpasses, n);
				cblk->segs[segno].newlen = bio_read(bio, cblk->numlenbits + int_floorlog2(cblk->segs[segno].numnewpasses));
				n -= cblk->segs[segno].numnewpasses;
				if (n > 0) {
					++segno;
					t2_init_seg(cblk, segno, tcp->tccps[compno].cblksty, 0);
				}
			} while (n > 0);
		}
	}
	
	if (bio_inalign(bio)) {
		bio_destroy(bio);
		return -999;
	}
	
	hd += bio_numbytes(bio);
	bio_destroy(bio);
	
	
	if (tcp->csty & J2K_CP_CSTY_EPH) {
		if ((*hd) != 0xff || (*(hd + 1) != 0x92)) {
			opj_event_msg(t2->cinfo, EVT_ERROR, "Expected EPH marker\n");
			return -999;
		} else {
			hd += 2;
		}
	}

	
	
	if(pack_info) {
		pack_info->end_ph_pos = (int)(hd - src);
	}
	
	
	if (cp->ppm==1) {
		cp->ppm_len+=cp->ppm_data-hd;
		cp->ppm_data = hd;
	} else if (tcp->ppt == 1) {
		tcp->ppt_len+=tcp->ppt_data-hd;
		tcp->ppt_data = hd;
	} else {
		c=hd;
	}
	
	for (bandno = 0; bandno < res->numbands; bandno++) {
		opj_tcd_band_t *band = &res->bands[bandno];
		opj_tcd_precinct_t *prc = &band->precincts[precno];
		
		if ((band->x1-band->x0 == 0)||(band->y1-band->y0 == 0)) continue;
		
		for (cblkno = 0; cblkno < prc->cw * prc->ch; cblkno++) {
			opj_tcd_cblk_dec_t* cblk = &prc->cblks.dec[cblkno];
			opj_tcd_seg_t *seg = NULL;
			if (!cblk->numnewpasses)
				continue;
			if (!cblk->numsegs) {
				seg = &cblk->segs[0];
				cblk->numsegs++;
				cblk->len = 0;
			} else {
				seg = &cblk->segs[cblk->numsegs - 1];
				if (seg->numpasses == seg->maxpasses) {
					seg++;
					cblk->numsegs++;
				}
			}
			
			do {
				if (c + seg->newlen > src + len) {
					return -999;
				}

#ifdef USE_JPWL
			

				
				if ((cblk->len + seg->newlen) > 8192) {
					opj_event_msg(t2->cinfo, EVT_WARNING,
						"JPWL: segment too long (%d) for codeblock %d (p=%d, b=%d, r=%d, c=%d)\n",
						seg->newlen, cblkno, precno, bandno, resno, compno);
					if (!JPWL_ASSUME) {
						opj_event_msg(t2->cinfo, EVT_ERROR, "JPWL: giving up\n");
						return -999;
					}
					seg->newlen = 8192 - cblk->len;
					opj_event_msg(t2->cinfo, EVT_WARNING, "      - truncating segment to %d\n", seg->newlen);
					break;
				};

#endif 
				
				cblk->data = (unsigned char*) opj_realloc(cblk->data, (cblk->len + seg->newlen) * sizeof(unsigned char));
				memcpy(cblk->data + cblk->len, c, seg->newlen);
				if (seg->numpasses == 0) {
					seg->data = &cblk->data;
					seg->dataindex = cblk->len;
				}
				c += seg->newlen;
				cblk->len += seg->newlen;
				seg->len += seg->newlen;
				seg->numpasses += seg->numnewpasses;
				cblk->numnewpasses -= seg->numnewpasses;
				if (cblk->numnewpasses > 0) {
					seg++;
					cblk->numsegs++;
				}
			} while (cblk->numnewpasses > 0);
		}
	}
	
	return (c - src);
}

int t2_encode_packets(opj_t2_t* t2,int tileno, opj_tcd_tile_t *tile, int maxlayers, unsigned char *dest, int len, opj_codestream_info_t *cstr_info,int tpnum, int tppos,int pino, J2K_T2_MODE t2_mode, int cur_totnum_tp){
	unsigned char *c = dest;
	int e = 0;
	int compno;
	opj_pi_iterator_t *pi = NULL;
	int poc;
	opj_image_t *image = t2->image;
	opj_cp_t *cp = t2->cp;
	opj_tcp_t *tcp = &cp->tcps[tileno];
	int pocno = cp->cinema == CINEMA4K_24? 2: 1;
	int maxcomp = cp->max_comp_size > 0 ? image->numcomps : 1;
	
	pi = pi_initialise_encode(image, cp, tileno, t2_mode);
	if(!pi) {
		
		return -999;
	}
	
	if(t2_mode == THRESH_CALC ){ 
		for(compno = 0; compno < maxcomp; compno++ ){
			for(poc = 0; poc < pocno ; poc++){
				int comp_len = 0;
				int tpnum = compno;
				if (pi_create_encode(pi, cp,tileno,poc,tpnum,tppos,t2_mode,cur_totnum_tp)) {
					opj_event_msg(t2->cinfo, EVT_ERROR, "Error initializing Packet Iterator\n");
					pi_destroy(pi, cp, tileno);
					return -999;
				}
				while (pi_next(&pi[poc])) {
					if (pi[poc].layno < maxlayers) {
						e = t2_encode_packet(tile, &cp->tcps[tileno], &pi[poc], c, dest + len - c, cstr_info, tileno);
						comp_len = comp_len + e;
						if (e == -999) {
							break;
						} else {
							c += e;
						}
					}
				}
				if (e == -999) break;
				if (cp->max_comp_size){
					if (comp_len > cp->max_comp_size){
						e = -999;
						break;
					}
				}
			}
			if (e == -999)  break;
		}
	}else{  
		pi_create_encode(pi, cp,tileno,pino,tpnum,tppos,t2_mode,cur_totnum_tp);
		while (pi_next(&pi[pino])) {
			if (pi[pino].layno < maxlayers) {
				e = t2_encode_packet(tile, &cp->tcps[tileno], &pi[pino], c, dest + len - c, cstr_info, tileno);
				if (e == -999) {
					break;
				} else {
					c += e;
				}
				
				if(cstr_info) {
					if(cstr_info->index_write) {
						opj_tile_info_t *info_TL = &cstr_info->tile[tileno];
						opj_packet_info_t *info_PK = &info_TL->packet[cstr_info->packno];
						if (!cstr_info->packno) {
							info_PK->start_pos = info_TL->end_header + 1;
						} else {
							info_PK->start_pos = ((cp->tp_on | tcp->POC)&& info_PK->start_pos) ? info_PK->start_pos : info_TL->packet[cstr_info->packno - 1].end_pos + 1;
						}
						info_PK->end_pos = info_PK->start_pos + e - 1;
						info_PK->end_ph_pos += info_PK->start_pos - 1;	
					}
					
					cstr_info->packno++;
				}
				
				tile->packno++;
			}
		}
	}
	
	pi_destroy(pi, cp, tileno);
	
	if (e == -999) {
		return e;
	}
	
  return (c - dest);
}

int t2_decode_packets(opj_t2_t *t2, unsigned char *src, int len, int tileno, opj_tcd_tile_t *tile, opj_codestream_info_t *cstr_info) {
	unsigned char *c = src;
	opj_pi_iterator_t *pi;
	int pino, e = 0;
	int n = 0, curtp = 0;
	int tp_start_packno;

	opj_image_t *image = t2->image;
	opj_cp_t *cp = t2->cp;
	
	
	pi = pi_create_decode(image, cp, tileno);
	if(!pi) {
		
		return -999;
	}

	tp_start_packno = 0;
	
	for (pino = 0; pino <= cp->tcps[tileno].numpocs; pino++) {
		while (pi_next(&pi[pino])) {
			if ((cp->layer==0) || (cp->layer>=((pi[pino].layno)+1))) {
				opj_packet_info_t *pack_info;
				if (cstr_info)
					pack_info = &cstr_info->tile[tileno].packet[cstr_info->packno];
				else
					pack_info = NULL;
				e = t2_decode_packet(t2, c, src + len - c, tile, &cp->tcps[tileno], &pi[pino], pack_info);
			} else {
				e = 0;
			}
			if(e == -999) return -999;
			
			image->comps[pi[pino].compno].resno_decoded =	
				(e > 0) ? 
				int_max(pi[pino].resno, image->comps[pi[pino].compno].resno_decoded) 
				: image->comps[pi[pino].compno].resno_decoded;
			n++;

			
			if(cstr_info) {
				opj_tile_info_t *info_TL = &cstr_info->tile[tileno];
				opj_packet_info_t *info_PK = &info_TL->packet[cstr_info->packno];
				if (!cstr_info->packno) {
					info_PK->start_pos = info_TL->end_header + 1;
				} else if (info_TL->packet[cstr_info->packno-1].end_pos >= (int)cstr_info->tile[tileno].tp[curtp].tp_end_pos){ 
					info_TL->tp[curtp].tp_numpacks = cstr_info->packno - tp_start_packno; 
          info_TL->tp[curtp].tp_start_pack = tp_start_packno;
					tp_start_packno = cstr_info->packno;
					curtp++;
					info_PK->start_pos = cstr_info->tile[tileno].tp[curtp].tp_end_header+1;
				} else {
					info_PK->start_pos = (cp->tp_on && info_PK->start_pos) ? info_PK->start_pos : info_TL->packet[cstr_info->packno - 1].end_pos + 1;
				}
				info_PK->end_pos = info_PK->start_pos + e - 1;
				info_PK->end_ph_pos += info_PK->start_pos - 1;	
				cstr_info->packno++;
			}
			
			
			if (e == -999) {		
				break;
			} else {
				c += e;
			}			
		}
	}
	
	if(cstr_info) {
		cstr_info->tile[tileno].tp[curtp].tp_numpacks = cstr_info->packno - tp_start_packno; 
    cstr_info->tile[tileno].tp[curtp].tp_start_pack = tp_start_packno;
	}
	

	
	pi_destroy(pi, cp, tileno);
	
	if (e == -999) {
		return e;
	}
	
	return (c - src);
}

opj_t2_t* t2_create(opj_common_ptr cinfo, opj_image_t *image, opj_cp_t *cp) {
	
	opj_t2_t *t2 = (opj_t2_t*)opj_malloc(sizeof(opj_t2_t));
	if(!t2) return NULL;
	t2->cinfo = cinfo;
	t2->image = image;
	t2->cp = cp;

	return t2;
}

void t2_destroy(opj_t2_t *t2) {
	if(t2) {
		opj_free(t2);
	}
}

