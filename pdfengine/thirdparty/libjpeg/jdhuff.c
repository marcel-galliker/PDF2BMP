

#define JPEG_INTERNALS
#include "jinclude.h"
#include "jpeglib.h"

#define HUFF_LOOKAHEAD	8	

typedef struct {
  
  INT32 maxcode[18];		
  
  INT32 valoffset[17];		
  

  
  JHUFF_TBL *pub;

  
  int look_nbits[1<<HUFF_LOOKAHEAD]; 
  UINT8 look_sym[1<<HUFF_LOOKAHEAD]; 
} d_derived_tbl;

typedef INT32 bit_buf_type;	
#define BIT_BUF_SIZE  32	

typedef struct {		
  bit_buf_type get_buffer;	
  int bits_left;		
} bitread_perm_state;

typedef struct {		
  
  
  const JOCTET * next_input_byte; 
  size_t bytes_in_buffer;	
  
  bit_buf_type get_buffer;	
  int bits_left;		
  
  j_decompress_ptr cinfo;	
} bitread_working_state;

#define BITREAD_STATE_VARS  \
	register bit_buf_type get_buffer;  \
	register int bits_left;  \
	bitread_working_state br_state

#define BITREAD_LOAD_STATE(cinfop,permstate)  \
	br_state.cinfo = cinfop; \
	br_state.next_input_byte = cinfop->src->next_input_byte; \
	br_state.bytes_in_buffer = cinfop->src->bytes_in_buffer; \
	get_buffer = permstate.get_buffer; \
	bits_left = permstate.bits_left;

#define BITREAD_SAVE_STATE(cinfop,permstate)  \
	cinfop->src->next_input_byte = br_state.next_input_byte; \
	cinfop->src->bytes_in_buffer = br_state.bytes_in_buffer; \
	permstate.get_buffer = get_buffer; \
	permstate.bits_left = bits_left

#define CHECK_BIT_BUFFER(state,nbits,action) \
	{ if (bits_left < (nbits)) {  \
	    if (! jpeg_fill_bit_buffer(&(state),get_buffer,bits_left,nbits))  \
	      { action; }  \
	    get_buffer = (state).get_buffer; bits_left = (state).bits_left; } }

#define GET_BITS(nbits) \
	(((int) (get_buffer >> (bits_left -= (nbits)))) & BIT_MASK(nbits))

#define PEEK_BITS(nbits) \
	(((int) (get_buffer >> (bits_left -  (nbits)))) & BIT_MASK(nbits))

#define DROP_BITS(nbits) \
	(bits_left -= (nbits))

#define HUFF_DECODE(result,state,htbl,failaction,slowlabel) \
{ register int nb, look; \
  if (bits_left < HUFF_LOOKAHEAD) { \
    if (! jpeg_fill_bit_buffer(&state,get_buffer,bits_left, 0)) {failaction;} \
    get_buffer = state.get_buffer; bits_left = state.bits_left; \
    if (bits_left < HUFF_LOOKAHEAD) { \
      nb = 1; goto slowlabel; \
    } \
  } \
  look = PEEK_BITS(HUFF_LOOKAHEAD); \
  if ((nb = htbl->look_nbits[look]) != 0) { \
    DROP_BITS(nb); \
    result = htbl->look_sym[look]; \
  } else { \
    nb = HUFF_LOOKAHEAD+1; \
slowlabel: \
    if ((result=jpeg_huff_decode(&state,get_buffer,bits_left,htbl,nb)) < 0) \
	{ failaction; } \
    get_buffer = state.get_buffer; bits_left = state.bits_left; \
  } \
}

typedef struct {
  unsigned int EOBRUN;			
  int last_dc_val[MAX_COMPS_IN_SCAN];	
} savable_state;

#ifndef NO_STRUCT_ASSIGN
#define ASSIGN_STATE(dest,src)  ((dest) = (src))
#else
#if MAX_COMPS_IN_SCAN == 4
#define ASSIGN_STATE(dest,src)  \
	((dest).EOBRUN = (src).EOBRUN, \
	 (dest).last_dc_val[0] = (src).last_dc_val[0], \
	 (dest).last_dc_val[1] = (src).last_dc_val[1], \
	 (dest).last_dc_val[2] = (src).last_dc_val[2], \
	 (dest).last_dc_val[3] = (src).last_dc_val[3])
#endif
#endif

typedef struct {
  struct jpeg_entropy_decoder pub; 

  
  bitread_perm_state bitstate;	
  savable_state saved;		

  
  boolean insufficient_data;	
  unsigned int restarts_to_go;	

  

  
  d_derived_tbl * derived_tbls[NUM_HUFF_TBLS];

  d_derived_tbl * ac_derived_tbl; 

  

  
  d_derived_tbl * dc_derived_tbls[NUM_HUFF_TBLS];
  d_derived_tbl * ac_derived_tbls[NUM_HUFF_TBLS];

  

  
  d_derived_tbl * dc_cur_tbls[D_MAX_BLOCKS_IN_MCU];
  d_derived_tbl * ac_cur_tbls[D_MAX_BLOCKS_IN_MCU];
  
  int coef_limit[D_MAX_BLOCKS_IN_MCU];
} huff_entropy_decoder;

typedef huff_entropy_decoder * huff_entropy_ptr;

static const int jpeg_zigzag_order[8][8] = {
  {  0,  1,  5,  6, 14, 15, 27, 28 },
  {  2,  4,  7, 13, 16, 26, 29, 42 },
  {  3,  8, 12, 17, 25, 30, 41, 43 },
  {  9, 11, 18, 24, 31, 40, 44, 53 },
  { 10, 19, 23, 32, 39, 45, 52, 54 },
  { 20, 22, 33, 38, 46, 51, 55, 60 },
  { 21, 34, 37, 47, 50, 56, 59, 61 },
  { 35, 36, 48, 49, 57, 58, 62, 63 }
};

static const int jpeg_zigzag_order7[7][7] = {
  {  0,  1,  5,  6, 14, 15, 27 },
  {  2,  4,  7, 13, 16, 26, 28 },
  {  3,  8, 12, 17, 25, 29, 38 },
  {  9, 11, 18, 24, 30, 37, 39 },
  { 10, 19, 23, 31, 36, 40, 45 },
  { 20, 22, 32, 35, 41, 44, 46 },
  { 21, 33, 34, 42, 43, 47, 48 }
};

static const int jpeg_zigzag_order6[6][6] = {
  {  0,  1,  5,  6, 14, 15 },
  {  2,  4,  7, 13, 16, 25 },
  {  3,  8, 12, 17, 24, 26 },
  {  9, 11, 18, 23, 27, 32 },
  { 10, 19, 22, 28, 31, 33 },
  { 20, 21, 29, 30, 34, 35 }
};

static const int jpeg_zigzag_order5[5][5] = {
  {  0,  1,  5,  6, 14 },
  {  2,  4,  7, 13, 15 },
  {  3,  8, 12, 16, 21 },
  {  9, 11, 17, 20, 22 },
  { 10, 18, 19, 23, 24 }
};

static const int jpeg_zigzag_order4[4][4] = {
  { 0,  1,  5,  6 },
  { 2,  4,  7, 12 },
  { 3,  8, 11, 13 },
  { 9, 10, 14, 15 }
};

static const int jpeg_zigzag_order3[3][3] = {
  { 0, 1, 5 },
  { 2, 4, 6 },
  { 3, 7, 8 }
};

static const int jpeg_zigzag_order2[2][2] = {
  { 0, 1 },
  { 2, 3 }
};

LOCAL(void)
jpeg_make_d_derived_tbl (j_decompress_ptr cinfo, boolean isDC, int tblno,
			 d_derived_tbl ** pdtbl)
{
  JHUFF_TBL *htbl;
  d_derived_tbl *dtbl;
  int p, i, l, si, numsymbols;
  int lookbits, ctr;
  char huffsize[257];
  unsigned int huffcode[257];
  unsigned int code;

  

  
  if (tblno < 0 || tblno >= NUM_HUFF_TBLS)
    ERREXIT1(cinfo, JERR_NO_HUFF_TABLE, tblno);
  htbl =
    isDC ? cinfo->dc_huff_tbl_ptrs[tblno] : cinfo->ac_huff_tbl_ptrs[tblno];
  if (htbl == NULL)
    ERREXIT1(cinfo, JERR_NO_HUFF_TABLE, tblno);

  
  if (*pdtbl == NULL)
    *pdtbl = (d_derived_tbl *)
      (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
				  SIZEOF(d_derived_tbl));
  dtbl = *pdtbl;
  dtbl->pub = htbl;		
  
  

  p = 0;
  for (l = 1; l <= 16; l++) {
    i = (int) htbl->bits[l];
    if (i < 0 || p + i > 256)	
      ERREXIT(cinfo, JERR_BAD_HUFF_TABLE);
    while (i--)
      huffsize[p++] = (char) l;
  }
  huffsize[p] = 0;
  numsymbols = p;
  
  
  
  
  code = 0;
  si = huffsize[0];
  p = 0;
  while (huffsize[p]) {
    while (((int) huffsize[p]) == si) {
      huffcode[p++] = code;
      code++;
    }
    
    if (((INT32) code) >= (((INT32) 1) << si))
      ERREXIT(cinfo, JERR_BAD_HUFF_TABLE);
    code <<= 1;
    si++;
  }

  

  p = 0;
  for (l = 1; l <= 16; l++) {
    if (htbl->bits[l]) {
      
      dtbl->valoffset[l] = (INT32) p - (INT32) huffcode[p];
      p += htbl->bits[l];
      dtbl->maxcode[l] = huffcode[p-1]; 
    } else {
      dtbl->maxcode[l] = -1;	
    }
  }
  dtbl->maxcode[17] = 0xFFFFFL; 

  

  MEMZERO(dtbl->look_nbits, SIZEOF(dtbl->look_nbits));

  p = 0;
  for (l = 1; l <= HUFF_LOOKAHEAD; l++) {
    for (i = 1; i <= (int) htbl->bits[l]; i++, p++) {
      
      
      lookbits = huffcode[p] << (HUFF_LOOKAHEAD-l);
      for (ctr = 1 << (HUFF_LOOKAHEAD-l); ctr > 0; ctr--) {
	dtbl->look_nbits[lookbits] = l;
	dtbl->look_sym[lookbits] = htbl->huffval[p];
	lookbits++;
      }
    }
  }

  
  if (isDC) {
    for (i = 0; i < numsymbols; i++) {
      int sym = htbl->huffval[i];
      if (sym < 0 || sym > 15)
	ERREXIT(cinfo, JERR_BAD_HUFF_TABLE);
    }
  }
}

#ifdef SLOW_SHIFT_32
#define MIN_GET_BITS  15	
#else
#define MIN_GET_BITS  (BIT_BUF_SIZE-7)
#endif

LOCAL(boolean)
jpeg_fill_bit_buffer (bitread_working_state * state,
		      register bit_buf_type get_buffer, register int bits_left,
		      int nbits)

{
  
  register const JOCTET * next_input_byte = state->next_input_byte;
  register size_t bytes_in_buffer = state->bytes_in_buffer;
  j_decompress_ptr cinfo = state->cinfo;

  
  
  

  if (cinfo->unread_marker == 0) {	
    while (bits_left < MIN_GET_BITS) {
      register int c;

      
      if (bytes_in_buffer == 0) {
	if (! (*cinfo->src->fill_input_buffer) (cinfo))
	  return FALSE;
	next_input_byte = cinfo->src->next_input_byte;
	bytes_in_buffer = cinfo->src->bytes_in_buffer;
      }
      bytes_in_buffer--;
      c = GETJOCTET(*next_input_byte++);

      
      if (c == 0xFF) {
	
	do {
	  if (bytes_in_buffer == 0) {
	    if (! (*cinfo->src->fill_input_buffer) (cinfo))
	      return FALSE;
	    next_input_byte = cinfo->src->next_input_byte;
	    bytes_in_buffer = cinfo->src->bytes_in_buffer;
	  }
	  bytes_in_buffer--;
	  c = GETJOCTET(*next_input_byte++);
	} while (c == 0xFF);

	if (c == 0) {
	  
	  c = 0xFF;
	} else {
	  
	  cinfo->unread_marker = c;
	  
	  goto no_more_bytes;
	}
      }

      
      get_buffer = (get_buffer << 8) | c;
      bits_left += 8;
    } 
  } else {
  no_more_bytes:
    
    if (nbits > bits_left) {
      
      if (! ((huff_entropy_ptr) cinfo->entropy)->insufficient_data) {
	WARNMS(cinfo, JWRN_HIT_MARKER);
	((huff_entropy_ptr) cinfo->entropy)->insufficient_data = TRUE;
      }
      
      get_buffer <<= MIN_GET_BITS - bits_left;
      bits_left = MIN_GET_BITS;
    }
  }

  
  state->next_input_byte = next_input_byte;
  state->bytes_in_buffer = bytes_in_buffer;
  state->get_buffer = get_buffer;
  state->bits_left = bits_left;

  return TRUE;
}

#ifdef AVOID_TABLES

#define BIT_MASK(nbits)   ((1<<(nbits))-1)
#define HUFF_EXTEND(x,s)  ((x) < (1<<((s)-1)) ? (x) - ((1<<(s))-1) : (x))

#else

#define BIT_MASK(nbits)   bmask[nbits]
#define HUFF_EXTEND(x,s)  ((x) <= bmask[(s) - 1] ? (x) - bmask[s] : (x))

static const int bmask[16] =	
  { 0, 0x0001, 0x0003, 0x0007, 0x000F, 0x001F, 0x003F, 0x007F, 0x00FF,
    0x01FF, 0x03FF, 0x07FF, 0x0FFF, 0x1FFF, 0x3FFF, 0x7FFF };

#endif 

LOCAL(int)
jpeg_huff_decode (bitread_working_state * state,
		  register bit_buf_type get_buffer, register int bits_left,
		  d_derived_tbl * htbl, int min_bits)
{
  register int l = min_bits;
  register INT32 code;

  
  

  CHECK_BIT_BUFFER(*state, l, return -1);
  code = GET_BITS(l);

  
  

  while (code > htbl->maxcode[l]) {
    code <<= 1;
    CHECK_BIT_BUFFER(*state, 1, return -1);
    code |= GET_BITS(1);
    l++;
  }

  
  state->get_buffer = get_buffer;
  state->bits_left = bits_left;

  

  if (l > 16) {
    WARNMS(state->cinfo, JWRN_HUFF_BAD_CODE);
    return 0;			
  }

  return htbl->pub->huffval[ (int) (code + htbl->valoffset[l]) ];
}

LOCAL(boolean)
process_restart (j_decompress_ptr cinfo)
{
  huff_entropy_ptr entropy = (huff_entropy_ptr) cinfo->entropy;
  int ci;

  
  
  cinfo->marker->discarded_bytes += entropy->bitstate.bits_left / 8;
  entropy->bitstate.bits_left = 0;

  
  if (! (*cinfo->marker->read_restart_marker) (cinfo))
    return FALSE;

  
  for (ci = 0; ci < cinfo->comps_in_scan; ci++)
    entropy->saved.last_dc_val[ci] = 0;
  
  entropy->saved.EOBRUN = 0;

  
  entropy->restarts_to_go = cinfo->restart_interval;

  
  if (cinfo->unread_marker == 0)
    entropy->insufficient_data = FALSE;

  return TRUE;
}

METHODDEF(boolean)
decode_mcu_DC_first (j_decompress_ptr cinfo, JBLOCKROW *MCU_data)
{   
  huff_entropy_ptr entropy = (huff_entropy_ptr) cinfo->entropy;
  int Al = cinfo->Al;
  register int s, r;
  int blkn, ci;
  JBLOCKROW block;
  BITREAD_STATE_VARS;
  savable_state state;
  d_derived_tbl * tbl;
  jpeg_component_info * compptr;

  
  if (cinfo->restart_interval) {
    if (entropy->restarts_to_go == 0)
      if (! process_restart(cinfo))
	return FALSE;
  }

  
  if (! entropy->insufficient_data) {

    
    BITREAD_LOAD_STATE(cinfo,entropy->bitstate);
    ASSIGN_STATE(state, entropy->saved);

    

    for (blkn = 0; blkn < cinfo->blocks_in_MCU; blkn++) {
      block = MCU_data[blkn];
      ci = cinfo->MCU_membership[blkn];
      compptr = cinfo->cur_comp_info[ci];
      tbl = entropy->derived_tbls[compptr->dc_tbl_no];

      

      
      HUFF_DECODE(s, br_state, tbl, return FALSE, label1);
      if (s) {
	CHECK_BIT_BUFFER(br_state, s, return FALSE);
	r = GET_BITS(s);
	s = HUFF_EXTEND(r, s);
      }

      
      s += state.last_dc_val[ci];
      state.last_dc_val[ci] = s;
      
      (*block)[0] = (JCOEF) (s << Al);
    }

    
    BITREAD_SAVE_STATE(cinfo,entropy->bitstate);
    ASSIGN_STATE(entropy->saved, state);
  }

  
  entropy->restarts_to_go--;

  return TRUE;
}

METHODDEF(boolean)
decode_mcu_AC_first (j_decompress_ptr cinfo, JBLOCKROW *MCU_data)
{   
  huff_entropy_ptr entropy = (huff_entropy_ptr) cinfo->entropy;
  register int s, k, r;
  unsigned int EOBRUN;
  int Se, Al;
  const int * natural_order;
  JBLOCKROW block;
  BITREAD_STATE_VARS;
  d_derived_tbl * tbl;

  
  if (cinfo->restart_interval) {
    if (entropy->restarts_to_go == 0)
      if (! process_restart(cinfo))
	return FALSE;
  }

  
  if (! entropy->insufficient_data) {

    Se = cinfo->Se;
    Al = cinfo->Al;
    natural_order = cinfo->natural_order;

    
    EOBRUN = entropy->saved.EOBRUN;	

    

    if (EOBRUN > 0)		
      EOBRUN--;			
    else {
      BITREAD_LOAD_STATE(cinfo,entropy->bitstate);
      block = MCU_data[0];
      tbl = entropy->ac_derived_tbl;

      for (k = cinfo->Ss; k <= Se; k++) {
	HUFF_DECODE(s, br_state, tbl, return FALSE, label2);
	r = s >> 4;
	s &= 15;
	if (s) {
	  k += r;
	  CHECK_BIT_BUFFER(br_state, s, return FALSE);
	  r = GET_BITS(s);
	  s = HUFF_EXTEND(r, s);
	  
	  (*block)[natural_order[k]] = (JCOEF) (s << Al);
	} else {
	  if (r == 15) {	
	    k += 15;		
	  } else {		
	    EOBRUN = 1 << r;
	    if (r) {		
	      CHECK_BIT_BUFFER(br_state, r, return FALSE);
	      r = GET_BITS(r);
	      EOBRUN += r;
	    }
	    EOBRUN--;		
	    break;		
	  }
	}
      }

      BITREAD_SAVE_STATE(cinfo,entropy->bitstate);
    }

    
    entropy->saved.EOBRUN = EOBRUN;	
  }

  
  entropy->restarts_to_go--;

  return TRUE;
}

METHODDEF(boolean)
decode_mcu_DC_refine (j_decompress_ptr cinfo, JBLOCKROW *MCU_data)
{   
  huff_entropy_ptr entropy = (huff_entropy_ptr) cinfo->entropy;
  int p1 = 1 << cinfo->Al;	
  int blkn;
  JBLOCKROW block;
  BITREAD_STATE_VARS;

  
  if (cinfo->restart_interval) {
    if (entropy->restarts_to_go == 0)
      if (! process_restart(cinfo))
	return FALSE;
  }

  

  
  BITREAD_LOAD_STATE(cinfo,entropy->bitstate);

  

  for (blkn = 0; blkn < cinfo->blocks_in_MCU; blkn++) {
    block = MCU_data[blkn];

    
    CHECK_BIT_BUFFER(br_state, 1, return FALSE);
    if (GET_BITS(1))
      (*block)[0] |= p1;
    
  }

  
  BITREAD_SAVE_STATE(cinfo,entropy->bitstate);

  
  entropy->restarts_to_go--;

  return TRUE;
}

METHODDEF(boolean)
decode_mcu_AC_refine (j_decompress_ptr cinfo, JBLOCKROW *MCU_data)
{   
  huff_entropy_ptr entropy = (huff_entropy_ptr) cinfo->entropy;
  register int s, k, r;
  unsigned int EOBRUN;
  int Se, p1, m1;
  const int * natural_order;
  JBLOCKROW block;
  JCOEFPTR thiscoef;
  BITREAD_STATE_VARS;
  d_derived_tbl * tbl;
  int num_newnz;
  int newnz_pos[DCTSIZE2];

  
  if (cinfo->restart_interval) {
    if (entropy->restarts_to_go == 0)
      if (! process_restart(cinfo))
	return FALSE;
  }

  
  if (! entropy->insufficient_data) {

    Se = cinfo->Se;
    p1 = 1 << cinfo->Al;	
    m1 = (-1) << cinfo->Al;	
    natural_order = cinfo->natural_order;

    
    BITREAD_LOAD_STATE(cinfo,entropy->bitstate);
    EOBRUN = entropy->saved.EOBRUN; 

    
    block = MCU_data[0];
    tbl = entropy->ac_derived_tbl;

    
    num_newnz = 0;

    
    k = cinfo->Ss;

    if (EOBRUN == 0) {
      for (; k <= Se; k++) {
	HUFF_DECODE(s, br_state, tbl, goto undoit, label3);
	r = s >> 4;
	s &= 15;
	if (s) {
	  if (s != 1)		
	    WARNMS(cinfo, JWRN_HUFF_BAD_CODE);
	  CHECK_BIT_BUFFER(br_state, 1, goto undoit);
	  if (GET_BITS(1))
	    s = p1;		
	  else
	    s = m1;		
	} else {
	  if (r != 15) {
	    EOBRUN = 1 << r;	
	    if (r) {
	      CHECK_BIT_BUFFER(br_state, r, goto undoit);
	      r = GET_BITS(r);
	      EOBRUN += r;
	    }
	    break;		
	  }
	  
	}
	
	do {
	  thiscoef = *block + natural_order[k];
	  if (*thiscoef != 0) {
	    CHECK_BIT_BUFFER(br_state, 1, goto undoit);
	    if (GET_BITS(1)) {
	      if ((*thiscoef & p1) == 0) { 
		if (*thiscoef >= 0)
		  *thiscoef += p1;
		else
		  *thiscoef += m1;
	      }
	    }
	  } else {
	    if (--r < 0)
	      break;		
	  }
	  k++;
	} while (k <= Se);
	if (s) {
	  int pos = natural_order[k];
	  
	  (*block)[pos] = (JCOEF) s;
	  
	  newnz_pos[num_newnz++] = pos;
	}
      }
    }

    if (EOBRUN > 0) {
      
      for (; k <= Se; k++) {
	thiscoef = *block + natural_order[k];
	if (*thiscoef != 0) {
	  CHECK_BIT_BUFFER(br_state, 1, goto undoit);
	  if (GET_BITS(1)) {
	    if ((*thiscoef & p1) == 0) { 
	      if (*thiscoef >= 0)
		*thiscoef += p1;
	      else
		*thiscoef += m1;
	    }
	  }
	}
      }
      
      EOBRUN--;
    }

    
    BITREAD_SAVE_STATE(cinfo,entropy->bitstate);
    entropy->saved.EOBRUN = EOBRUN; 
  }

  
  entropy->restarts_to_go--;

  return TRUE;

undoit:
  
  while (num_newnz > 0)
    (*block)[newnz_pos[--num_newnz]] = 0;

  return FALSE;
}

METHODDEF(boolean)
decode_mcu_sub (j_decompress_ptr cinfo, JBLOCKROW *MCU_data)
{
  huff_entropy_ptr entropy = (huff_entropy_ptr) cinfo->entropy;
  const int * natural_order;
  int Se, blkn;
  BITREAD_STATE_VARS;
  savable_state state;

  
  if (cinfo->restart_interval) {
    if (entropy->restarts_to_go == 0)
      if (! process_restart(cinfo))
	return FALSE;
  }

  
  if (! entropy->insufficient_data) {

    natural_order = cinfo->natural_order;
    Se = cinfo->lim_Se;

    
    BITREAD_LOAD_STATE(cinfo,entropy->bitstate);
    ASSIGN_STATE(state, entropy->saved);

    

    for (blkn = 0; blkn < cinfo->blocks_in_MCU; blkn++) {
      JBLOCKROW block = MCU_data[blkn];
      d_derived_tbl * htbl;
      register int s, k, r;
      int coef_limit, ci;

      

      
      htbl = entropy->dc_cur_tbls[blkn];
      HUFF_DECODE(s, br_state, htbl, return FALSE, label1);

      htbl = entropy->ac_cur_tbls[blkn];
      k = 1;
      coef_limit = entropy->coef_limit[blkn];
      if (coef_limit) {
	
	if (s) {
	  CHECK_BIT_BUFFER(br_state, s, return FALSE);
	  r = GET_BITS(s);
	  s = HUFF_EXTEND(r, s);
	}
	ci = cinfo->MCU_membership[blkn];
	s += state.last_dc_val[ci];
	state.last_dc_val[ci] = s;
	
	(*block)[0] = (JCOEF) s;

	
	
	for (; k < coef_limit; k++) {
	  HUFF_DECODE(s, br_state, htbl, return FALSE, label2);

	  r = s >> 4;
	  s &= 15;

	  if (s) {
	    k += r;
	    CHECK_BIT_BUFFER(br_state, s, return FALSE);
	    r = GET_BITS(s);
	    s = HUFF_EXTEND(r, s);
	    
	    (*block)[natural_order[k]] = (JCOEF) s;
	  } else {
	    if (r != 15)
	      goto EndOfBlock;
	    k += 15;
	  }
	}
      } else {
	if (s) {
	  CHECK_BIT_BUFFER(br_state, s, return FALSE);
	  DROP_BITS(s);
	}
      }

      
      
      for (; k <= Se; k++) {
	HUFF_DECODE(s, br_state, htbl, return FALSE, label3);

	r = s >> 4;
	s &= 15;

	if (s) {
	  k += r;
	  CHECK_BIT_BUFFER(br_state, s, return FALSE);
	  DROP_BITS(s);
	} else {
	  if (r != 15)
	    break;
	  k += 15;
	}
      }

      EndOfBlock: ;
    }

    
    BITREAD_SAVE_STATE(cinfo,entropy->bitstate);
    ASSIGN_STATE(entropy->saved, state);
  }

  
  entropy->restarts_to_go--;

  return TRUE;
}

METHODDEF(boolean)
decode_mcu (j_decompress_ptr cinfo, JBLOCKROW *MCU_data)
{
  huff_entropy_ptr entropy = (huff_entropy_ptr) cinfo->entropy;
  int blkn;
  BITREAD_STATE_VARS;
  savable_state state;

  
  if (cinfo->restart_interval) {
    if (entropy->restarts_to_go == 0)
      if (! process_restart(cinfo))
	return FALSE;
  }

  
  if (! entropy->insufficient_data) {

    
    BITREAD_LOAD_STATE(cinfo,entropy->bitstate);
    ASSIGN_STATE(state, entropy->saved);

    

    for (blkn = 0; blkn < cinfo->blocks_in_MCU; blkn++) {
      JBLOCKROW block = MCU_data[blkn];
      d_derived_tbl * htbl;
      register int s, k, r;
      int coef_limit, ci;

      

      
      htbl = entropy->dc_cur_tbls[blkn];
      HUFF_DECODE(s, br_state, htbl, return FALSE, label1);

      htbl = entropy->ac_cur_tbls[blkn];
      k = 1;
      coef_limit = entropy->coef_limit[blkn];
      if (coef_limit) {
	
	if (s) {
	  CHECK_BIT_BUFFER(br_state, s, return FALSE);
	  r = GET_BITS(s);
	  s = HUFF_EXTEND(r, s);
	}
	ci = cinfo->MCU_membership[blkn];
	s += state.last_dc_val[ci];
	state.last_dc_val[ci] = s;
	
	(*block)[0] = (JCOEF) s;

	
	
	for (; k < coef_limit; k++) {
	  HUFF_DECODE(s, br_state, htbl, return FALSE, label2);

	  r = s >> 4;
	  s &= 15;

	  if (s) {
	    k += r;
	    CHECK_BIT_BUFFER(br_state, s, return FALSE);
	    r = GET_BITS(s);
	    s = HUFF_EXTEND(r, s);
	    
	    (*block)[jpeg_natural_order[k]] = (JCOEF) s;
	  } else {
	    if (r != 15)
	      goto EndOfBlock;
	    k += 15;
	  }
	}
      } else {
	if (s) {
	  CHECK_BIT_BUFFER(br_state, s, return FALSE);
	  DROP_BITS(s);
	}
      }

      
      
      for (; k < DCTSIZE2; k++) {
	HUFF_DECODE(s, br_state, htbl, return FALSE, label3);

	r = s >> 4;
	s &= 15;

	if (s) {
	  k += r;
	  CHECK_BIT_BUFFER(br_state, s, return FALSE);
	  DROP_BITS(s);
	} else {
	  if (r != 15)
	    break;
	  k += 15;
	}
      }

      EndOfBlock: ;
    }

    
    BITREAD_SAVE_STATE(cinfo,entropy->bitstate);
    ASSIGN_STATE(entropy->saved, state);
  }

  
  entropy->restarts_to_go--;

  return TRUE;
}

METHODDEF(void)
start_pass_huff_decoder (j_decompress_ptr cinfo)
{
  huff_entropy_ptr entropy = (huff_entropy_ptr) cinfo->entropy;
  int ci, blkn, tbl, i;
  jpeg_component_info * compptr;

  if (cinfo->progressive_mode) {
    
    if (cinfo->Ss == 0) {
      if (cinfo->Se != 0)
	goto bad;
    } else {
      
      if (cinfo->Se < cinfo->Ss || cinfo->Se > cinfo->lim_Se)
	goto bad;
      
      if (cinfo->comps_in_scan != 1)
	goto bad;
    }
    if (cinfo->Ah != 0) {
      
      if (cinfo->Ah-1 != cinfo->Al)
	goto bad;
    }
    if (cinfo->Al > 13) {	
      
      bad:
      ERREXIT4(cinfo, JERR_BAD_PROGRESSION,
	       cinfo->Ss, cinfo->Se, cinfo->Ah, cinfo->Al);
    }
    
    for (ci = 0; ci < cinfo->comps_in_scan; ci++) {
      int coefi, cindex = cinfo->cur_comp_info[ci]->component_index;
      int *coef_bit_ptr = & cinfo->coef_bits[cindex][0];
      if (cinfo->Ss && coef_bit_ptr[0] < 0) 
	WARNMS2(cinfo, JWRN_BOGUS_PROGRESSION, cindex, 0);
      for (coefi = cinfo->Ss; coefi <= cinfo->Se; coefi++) {
	int expected = (coef_bit_ptr[coefi] < 0) ? 0 : coef_bit_ptr[coefi];
	if (cinfo->Ah != expected)
	  WARNMS2(cinfo, JWRN_BOGUS_PROGRESSION, cindex, coefi);
	coef_bit_ptr[coefi] = cinfo->Al;
      }
    }

    
    if (cinfo->Ah == 0) {
      if (cinfo->Ss == 0)
	entropy->pub.decode_mcu = decode_mcu_DC_first;
      else
	entropy->pub.decode_mcu = decode_mcu_AC_first;
    } else {
      if (cinfo->Ss == 0)
	entropy->pub.decode_mcu = decode_mcu_DC_refine;
      else
	entropy->pub.decode_mcu = decode_mcu_AC_refine;
    }

    for (ci = 0; ci < cinfo->comps_in_scan; ci++) {
      compptr = cinfo->cur_comp_info[ci];
      
      if (cinfo->Ss == 0) {
	if (cinfo->Ah == 0) {	
	  tbl = compptr->dc_tbl_no;
	  jpeg_make_d_derived_tbl(cinfo, TRUE, tbl,
				  & entropy->derived_tbls[tbl]);
	}
      } else {
	tbl = compptr->ac_tbl_no;
	jpeg_make_d_derived_tbl(cinfo, FALSE, tbl,
				& entropy->derived_tbls[tbl]);
	
	entropy->ac_derived_tbl = entropy->derived_tbls[tbl];
      }
      
      entropy->saved.last_dc_val[ci] = 0;
    }

    
    entropy->saved.EOBRUN = 0;
  } else {
    
    if (cinfo->Ss != 0 || cinfo->Ah != 0 || cinfo->Al != 0 ||
	((cinfo->is_baseline || cinfo->Se < DCTSIZE2) &&
	cinfo->Se != cinfo->lim_Se))
      WARNMS(cinfo, JWRN_NOT_SEQUENTIAL);

    
    
    if (cinfo->lim_Se != DCTSIZE2-1)
      entropy->pub.decode_mcu = decode_mcu_sub;
    else
      entropy->pub.decode_mcu = decode_mcu;

    for (ci = 0; ci < cinfo->comps_in_scan; ci++) {
      compptr = cinfo->cur_comp_info[ci];
      
      
      tbl = compptr->dc_tbl_no;
      jpeg_make_d_derived_tbl(cinfo, TRUE, tbl,
			      & entropy->dc_derived_tbls[tbl]);
      if (cinfo->lim_Se) {	
	tbl = compptr->ac_tbl_no;
	jpeg_make_d_derived_tbl(cinfo, FALSE, tbl,
				& entropy->ac_derived_tbls[tbl]);
      }
      
      entropy->saved.last_dc_val[ci] = 0;
    }

    
    for (blkn = 0; blkn < cinfo->blocks_in_MCU; blkn++) {
      ci = cinfo->MCU_membership[blkn];
      compptr = cinfo->cur_comp_info[ci];
      
      entropy->dc_cur_tbls[blkn] = entropy->dc_derived_tbls[compptr->dc_tbl_no];
      entropy->ac_cur_tbls[blkn] = entropy->ac_derived_tbls[compptr->ac_tbl_no];
      
      if (compptr->component_needed) {
	ci = compptr->DCT_v_scaled_size;
	i = compptr->DCT_h_scaled_size;
	switch (cinfo->lim_Se) {
	case (1*1-1):
	  entropy->coef_limit[blkn] = 1;
	  break;
	case (2*2-1):
	  if (ci <= 0 || ci > 2) ci = 2;
	  if (i <= 0 || i > 2) i = 2;
	  entropy->coef_limit[blkn] = 1 + jpeg_zigzag_order2[ci - 1][i - 1];
	  break;
	case (3*3-1):
	  if (ci <= 0 || ci > 3) ci = 3;
	  if (i <= 0 || i > 3) i = 3;
	  entropy->coef_limit[blkn] = 1 + jpeg_zigzag_order3[ci - 1][i - 1];
	  break;
	case (4*4-1):
	  if (ci <= 0 || ci > 4) ci = 4;
	  if (i <= 0 || i > 4) i = 4;
	  entropy->coef_limit[blkn] = 1 + jpeg_zigzag_order4[ci - 1][i - 1];
	  break;
	case (5*5-1):
	  if (ci <= 0 || ci > 5) ci = 5;
	  if (i <= 0 || i > 5) i = 5;
	  entropy->coef_limit[blkn] = 1 + jpeg_zigzag_order5[ci - 1][i - 1];
	  break;
	case (6*6-1):
	  if (ci <= 0 || ci > 6) ci = 6;
	  if (i <= 0 || i > 6) i = 6;
	  entropy->coef_limit[blkn] = 1 + jpeg_zigzag_order6[ci - 1][i - 1];
	  break;
	case (7*7-1):
	  if (ci <= 0 || ci > 7) ci = 7;
	  if (i <= 0 || i > 7) i = 7;
	  entropy->coef_limit[blkn] = 1 + jpeg_zigzag_order7[ci - 1][i - 1];
	  break;
	default:
	  if (ci <= 0 || ci > 8) ci = 8;
	  if (i <= 0 || i > 8) i = 8;
	  entropy->coef_limit[blkn] = 1 + jpeg_zigzag_order[ci - 1][i - 1];
	  break;
	}
      } else {
	entropy->coef_limit[blkn] = 0;
      }
    }
  }

  
  entropy->bitstate.bits_left = 0;
  entropy->bitstate.get_buffer = 0; 
  entropy->insufficient_data = FALSE;

  
  entropy->restarts_to_go = cinfo->restart_interval;
}

GLOBAL(void)
jinit_huff_decoder (j_decompress_ptr cinfo)
{
  huff_entropy_ptr entropy;
  int i;

  entropy = (huff_entropy_ptr)
    (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
				SIZEOF(huff_entropy_decoder));
  cinfo->entropy = (struct jpeg_entropy_decoder *) entropy;
  entropy->pub.start_pass = start_pass_huff_decoder;

  if (cinfo->progressive_mode) {
    
    int *coef_bit_ptr, ci;
    cinfo->coef_bits = (int (*)[DCTSIZE2])
      (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
				  cinfo->num_components*DCTSIZE2*SIZEOF(int));
    coef_bit_ptr = & cinfo->coef_bits[0][0];
    for (ci = 0; ci < cinfo->num_components; ci++)
      for (i = 0; i < DCTSIZE2; i++)
	*coef_bit_ptr++ = -1;

    
    for (i = 0; i < NUM_HUFF_TBLS; i++) {
      entropy->derived_tbls[i] = NULL;
    }
  } else {
    
    for (i = 0; i < NUM_HUFF_TBLS; i++) {
      entropy->dc_derived_tbls[i] = entropy->ac_derived_tbls[i] = NULL;
    }
  }
}
