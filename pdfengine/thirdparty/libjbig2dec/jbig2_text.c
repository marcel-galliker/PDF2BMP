

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "os_types.h"

#include <stddef.h>
#include <string.h> 

#include "jbig2.h"
#include "jbig2_priv.h"
#include "jbig2_arith.h"
#include "jbig2_arith_int.h"
#include "jbig2_arith_iaid.h"
#include "jbig2_huffman.h"
#include "jbig2_generic.h"
#include "jbig2_symbol_dict.h"
#include "jbig2_text.h"

int
jbig2_decode_text_region(Jbig2Ctx *ctx, Jbig2Segment *segment,
                             const Jbig2TextRegionParams *params,
                             const Jbig2SymbolDict * const *dicts, const int n_dicts,
                             Jbig2Image *image,
                             const byte *data, const size_t size,
			     Jbig2ArithCx *GR_stats, Jbig2ArithState *as, Jbig2WordStream *ws)
{
    
    uint32_t NINSTANCES;
    uint32_t ID;
    int32_t STRIPT;
    int32_t FIRSTS;
    int32_t DT;
    int32_t DFS;
    int32_t IDS;
    int32_t CURS;
    int32_t CURT;
    int S,T;
    int x,y;
    bool first_symbol;
    uint32_t index, SBNUMSYMS;
    Jbig2Image *IB = NULL;
    Jbig2HuffmanState *hs = NULL;
    Jbig2HuffmanTable *SBSYMCODES = NULL;
    int code = 0;
    int RI;

    SBNUMSYMS = 0;
    for (index = 0; index < (uint32_t)n_dicts; index++) {
        SBNUMSYMS += dicts[index]->n_symbols;
    }
    jbig2_error(ctx, JBIG2_SEVERITY_DEBUG, segment->number,
        "symbol list contains %d glyphs in %d dictionaries", SBNUMSYMS, n_dicts);

    if (params->SBHUFF) {
	Jbig2HuffmanTable *runcodes = NULL;
	Jbig2HuffmanParams runcodeparams;
	Jbig2HuffmanLine runcodelengths[35];
	Jbig2HuffmanLine *symcodelengths = NULL;
	Jbig2HuffmanParams symcodeparams;
	int err, len, range, r;

	jbig2_error(ctx, JBIG2_SEVERITY_DEBUG, segment->number,
	  "huffman coded text region");
	hs = jbig2_huffman_new(ctx, ws);
    if (hs == NULL)
    {
        jbig2_error(ctx, JBIG2_SEVERITY_WARNING, segment->number,
            "failed to allocate storage for text region");
        return -1;
    }

	
	

	
	for (index = 0; index < 35; index++) {
	  runcodelengths[index].PREFLEN = jbig2_huffman_get_bits(hs, 4);
	  runcodelengths[index].RANGELEN = 0;
	  runcodelengths[index].RANGELOW = index;
	  jbig2_error(ctx, JBIG2_SEVERITY_DEBUG, segment->number,
	    "  read runcode%d length %d", index, runcodelengths[index].PREFLEN);
	}
	runcodeparams.HTOOB = 0;
	runcodeparams.lines = runcodelengths;
	runcodeparams.n_lines = 35;
	runcodes = jbig2_build_huffman_table(ctx, &runcodeparams);
	if (runcodes == NULL) {
	  jbig2_error(ctx, JBIG2_SEVERITY_FATAL, segment->number,
	    "error constructing symbol id runcode table!");
	  code = -1;
      goto cleanup1;
	}

	
	symcodelengths = jbig2_new(ctx, Jbig2HuffmanLine, SBNUMSYMS);
	if (symcodelengths == NULL) {
	  jbig2_error(ctx, JBIG2_SEVERITY_FATAL, segment->number,
	    "memory allocation failure reading symbol ID huffman table!");
	  code = -1;
      goto cleanup1;
	}
	index = 0;
	while (index < SBNUMSYMS) {
	  code = jbig2_huffman_get(hs, runcodes, &err);
	  if (err != 0 || code < 0 || code >= 35) {
	    jbig2_error(ctx, JBIG2_SEVERITY_FATAL, segment->number,
	      "error reading symbol ID huffman table!");
	    code = err ? err : -1;
        goto cleanup1;
	  }

	  if (code < 32) {
	    len = code;
	    range = 1;
	  } else {
	    if (code == 32) {
	      len = symcodelengths[index-1].PREFLEN;
	      if (index < 1) {
		jbig2_error(ctx, JBIG2_SEVERITY_FATAL, segment->number,
	 	  "error decoding symbol id table: run length with no antecedent!");
	        code = -1;
            goto cleanup1;
	      }
	    } else {
	      len = 0; 
	    }
	    if (code == 32) range = jbig2_huffman_get_bits(hs, 2) + 3;
	    else if (code == 33) range = jbig2_huffman_get_bits(hs, 3) + 3;
	    else if (code == 34) range = jbig2_huffman_get_bits(hs, 7) + 11;
	  }
	  jbig2_error(ctx, JBIG2_SEVERITY_DEBUG, segment->number,
	    "  read runcode%d at index %d (length %d range %d)", code, index, len, range);
	  if (index+range > SBNUMSYMS) {
	    jbig2_error(ctx, JBIG2_SEVERITY_WARNING, segment->number,
	      "runlength extends %d entries beyond the end of symbol id table!",
		index+range - SBNUMSYMS);
	    range = SBNUMSYMS - index;
	  }
	  for (r = 0; r < range; r++) {
	    symcodelengths[index+r].PREFLEN = len;
	    symcodelengths[index+r].RANGELEN = 0;
	    symcodelengths[index+r].RANGELOW = index + r;
	  }
	  index += r;
	}

	if (index < SBNUMSYMS) {
	  jbig2_error(ctx, JBIG2_SEVERITY_WARNING, segment->number,
	    "runlength codes do not cover the available symbol set");
	}
	symcodeparams.HTOOB = 0;
	symcodeparams.lines = symcodelengths;
	symcodeparams.n_lines = SBNUMSYMS;

	
	jbig2_huffman_skip(hs);

	
	SBSYMCODES = jbig2_build_huffman_table(ctx, &symcodeparams);

cleanup1:
	jbig2_free(ctx->allocator, symcodelengths);
	jbig2_release_huffman_table(ctx, runcodes);

	if (SBSYMCODES == NULL) {
	    jbig2_error(ctx, JBIG2_SEVERITY_FATAL, segment->number,
            "could not construct Symbol ID huffman table!");
        jbig2_huffman_free(ctx, hs);
	    return ((code != 0) ? code : -1);
	}
    }

    
    jbig2_image_clear(ctx, image, params->SBDEFPIXEL);

    
    if (params->SBHUFF) {
        STRIPT = jbig2_huffman_get(hs, params->SBHUFFDT, &code);
    } else {
        code = jbig2_arith_int_decode(params->IADT, as, &STRIPT);
        if (code < 0) goto cleanup2;
    }

    
    STRIPT *= -(params->SBSTRIPS);
    FIRSTS = 0;
    NINSTANCES = 0;

    
    while (NINSTANCES < params->SBNUMINSTANCES) {
        
        if (params->SBHUFF) {
            DT = jbig2_huffman_get(hs, params->SBHUFFDT, &code);
        } else {
            code = jbig2_arith_int_decode(params->IADT, as, &DT);
            if (code < 0) goto cleanup2;
        }
        DT *= params->SBSTRIPS;
        STRIPT += DT;

	first_symbol = TRUE;
	
	for (;;) {
	    
	    if (first_symbol) {
		
		if (params->SBHUFF) {
		    DFS = jbig2_huffman_get(hs, params->SBHUFFFS, &code);
		} else {
		    code = jbig2_arith_int_decode(params->IAFS, as, &DFS);
            if (code < 0) goto cleanup2;
		}
		FIRSTS += DFS;
		CURS = FIRSTS;
		first_symbol = FALSE;

	    } else {
		
		if (params->SBHUFF) {
		    IDS = jbig2_huffman_get(hs, params->SBHUFFDS, &code);
		} else {
		    code = jbig2_arith_int_decode(params->IADS, as, &IDS);
		}
		if (code) {
		    break;
		}
		CURS += IDS + params->SBDSOFFSET;
	    }

	    
	    if (params->SBSTRIPS == 1) {
		CURT = 0;
	    } else if (params->SBHUFF) {
		CURT = jbig2_huffman_get_bits(hs, params->LOGSBSTRIPS);
	    } else {
		code = jbig2_arith_int_decode(params->IAIT, as, &CURT);
        if (code < 0) goto cleanup2;
	    }
	    T = STRIPT + CURT;

	    
	    if (params->SBHUFF) {
		ID = jbig2_huffman_get(hs, SBSYMCODES, &code);
	    } else {
		code = jbig2_arith_iaid_decode(params->IAID, as, (int *)&ID);
        if (code < 0) goto cleanup2;
	    }
	    if (ID >= SBNUMSYMS) {
		return jbig2_error(ctx, JBIG2_SEVERITY_FATAL, segment->number,
                    "symbol id out of range! (%d/%d)", ID, SBNUMSYMS);
	    }

	    
	    {
		uint32_t id = ID;

		index = 0;
		while ((int)id >= dicts[index]->n_symbols)
		    id -= dicts[index++]->n_symbols;
		IB = jbig2_image_clone(ctx, dicts[index]->glyphs[id]);
	    }
	    if (params->SBREFINE) {
	      if (params->SBHUFF) {
		RI = jbig2_huffman_get_bits(hs, 1);
	      } else {
		code = jbig2_arith_int_decode(params->IARI, as, &RI);
        if (code < 0) goto cleanup2;
	      }
	    } else {
		RI = 0;
	    }
	    if (RI) {
		Jbig2RefinementRegionParams rparams;
		Jbig2Image *IBO;
		int32_t RDW, RDH, RDX, RDY;
		Jbig2Image *refimage;
		int BMSIZE = 0;
		int code1 = 0;
		int code2 = 0;
		int code3 = 0;
		int code4 = 0;
		int code5 = 0;

		
		if (!params->SBHUFF) {
		  code1 = jbig2_arith_int_decode(params->IARDW, as, &RDW);
		  code2 = jbig2_arith_int_decode(params->IARDH, as, &RDH);
		  code3 = jbig2_arith_int_decode(params->IARDX, as, &RDX);
		  code4 = jbig2_arith_int_decode(params->IARDY, as, &RDY);
		} else {
		  RDW = jbig2_huffman_get(hs, params->SBHUFFRDW, &code1);
		  RDH = jbig2_huffman_get(hs, params->SBHUFFRDH, &code2);
		  RDX = jbig2_huffman_get(hs, params->SBHUFFRDX, &code3);
		  RDY = jbig2_huffman_get(hs, params->SBHUFFRDY, &code4);
		  BMSIZE = jbig2_huffman_get(hs, params->SBHUFFRSIZE, &code5);
		  jbig2_huffman_skip(hs);
		}

		if ((code1 < 0) || (code2 < 0) || (code3 < 0) || (code4 < 0) || (code5 < 0))
		{
		    code = jbig2_error(ctx, JBIG2_SEVERITY_FATAL, segment->number,
		        "failed to decode data");
		    goto cleanup2;
		}

		
		IBO = IB;
		refimage = jbig2_image_new(ctx, IBO->width + RDW,
						IBO->height + RDH);
		if (refimage == NULL) {
		  jbig2_image_release(ctx, IBO);
		  if (params->SBHUFF) {
		    jbig2_release_huffman_table(ctx, SBSYMCODES);
		  }
		  return jbig2_error(ctx, JBIG2_SEVERITY_FATAL, 
			segment->number,
			"couldn't allocate reference image");
	        }

		
		rparams.GRTEMPLATE = params->SBRTEMPLATE;
		rparams.reference = IBO;
		rparams.DX = (RDW >> 1) + RDX;
		rparams.DY = (RDH >> 1) + RDY;
		rparams.TPGRON = 0;
		memcpy(rparams.grat, params->sbrat, 4);
		jbig2_decode_refinement_region(ctx, segment,
		    &rparams, as, refimage, GR_stats);
		IB = refimage;

		jbig2_image_release(ctx, IBO);

		
		if (params->SBHUFF) {
		  jbig2_huffman_advance(hs, BMSIZE);
		}

	    }

	    
	    if ((!params->TRANSPOSED) && (params->REFCORNER > 1)) {
		CURS += IB->width - 1;
	    } else if ((params->TRANSPOSED) && !(params->REFCORNER & 1)) {
		CURS += IB->height - 1;
	    }

	    
	    S = CURS;

	    
	    if (!params->TRANSPOSED) {
		switch (params->REFCORNER) {
		case JBIG2_CORNER_TOPLEFT: x = S; y = T; break;
		case JBIG2_CORNER_TOPRIGHT: x = S - IB->width + 1; y = T; break;
		case JBIG2_CORNER_BOTTOMLEFT: x = S; y = T - IB->height + 1; break;
		case JBIG2_CORNER_BOTTOMRIGHT: x = S - IB->width + 1; y = T - IB->height + 1; break;
		}
	    } else { 
		switch (params->REFCORNER) {
		case JBIG2_CORNER_TOPLEFT: x = T; y = S; break;
		case JBIG2_CORNER_TOPRIGHT: x = T - IB->width + 1; y = S; break;
		case JBIG2_CORNER_BOTTOMLEFT: x = T; y = S - IB->height + 1; break;
		case JBIG2_CORNER_BOTTOMRIGHT: x = T - IB->width + 1; y = S - IB->height + 1; break;
		}
	    }

	    
#ifdef JBIG2_DEBUG
	    jbig2_error(ctx, JBIG2_SEVERITY_DEBUG, segment->number,
			"composing glyph id %d: %dx%d @ (%d,%d) symbol %d/%d",
			ID, IB->width, IB->height, x, y, NINSTANCES + 1,
			params->SBNUMINSTANCES);
#endif
	    jbig2_image_compose(ctx, image, IB, x, y, params->SBCOMBOP);

	    
	    if ((!params->TRANSPOSED) && (params->REFCORNER < 2)) {
		CURS += IB->width -1 ;
	    } else if ((params->TRANSPOSED) && (params->REFCORNER & 1)) {
		CURS += IB->height - 1;
	    }

	    
	    NINSTANCES++;

	    jbig2_image_release(ctx, IB);
	}
        
    }
    

cleanup2:
    if (params->SBHUFF) {
      jbig2_release_huffman_table(ctx, SBSYMCODES);
    }
    jbig2_huffman_free(ctx, hs);

    return code;
}

int
jbig2_text_region(Jbig2Ctx *ctx, Jbig2Segment *segment, const byte *segment_data)
{
    int offset = 0;
    Jbig2RegionSegmentInfo region_info;
    Jbig2TextRegionParams params;
    Jbig2Image *image = NULL;
    Jbig2SymbolDict **dicts = NULL;
    int n_dicts = 0;
    uint16_t flags = 0;
    uint16_t huffman_flags = 0;
    Jbig2ArithCx *GR_stats = NULL;
    int code = 0;
    Jbig2WordStream *ws = NULL;
    Jbig2ArithState *as = NULL;
    int table_index = 0;
    const Jbig2HuffmanParams *huffman_params = NULL;

    
    if (segment->data_length < 17)
        goto too_short;
    jbig2_get_region_segment_info(&region_info, segment_data);
    offset += 17;

    
    flags = jbig2_get_uint16(segment_data + offset);
    offset += 2;

    jbig2_error(ctx, JBIG2_SEVERITY_DEBUG, segment->number,
	"text region header flags 0x%04x", flags);

    
    memset(&params, 0, sizeof(Jbig2TextRegionParams));

    params.SBHUFF = flags & 0x0001;
    params.SBREFINE = flags & 0x0002;
    params.LOGSBSTRIPS = (flags & 0x000c) >> 2;
    params.SBSTRIPS = 1 << params.LOGSBSTRIPS;
    params.REFCORNER = (Jbig2RefCorner)((flags & 0x0030) >> 4);
    params.TRANSPOSED = flags & 0x0040;
    params.SBCOMBOP = (Jbig2ComposeOp)((flags & 0x0180) >> 7);
    params.SBDEFPIXEL = flags & 0x0200;
    
    params.SBDSOFFSET = (flags & 0x7C00) >> 10;
    if (params.SBDSOFFSET > 0x0f) params.SBDSOFFSET -= 0x20;
    params.SBRTEMPLATE = flags & 0x8000;

    if (params.SBDSOFFSET) {
      jbig2_error(ctx, JBIG2_SEVERITY_DEBUG, segment->number,
	"text region has SBDSOFFSET %d", params.SBDSOFFSET);
    }

    if (params.SBHUFF)	
      {
        
        huffman_flags = jbig2_get_uint16(segment_data + offset);
        offset += 2;

        if (huffman_flags & 0x8000)
            jbig2_error(ctx, JBIG2_SEVERITY_WARNING, segment->number,
                "reserved bit 15 of text region huffman flags is not zero");
      }
    else	
      {
        
        if ((params.SBREFINE) && !(params.SBRTEMPLATE))
          {
            params.sbrat[0] = segment_data[offset];
            params.sbrat[1] = segment_data[offset + 1];
            params.sbrat[2] = segment_data[offset + 2];
            params.sbrat[3] = segment_data[offset + 3];
            offset += 4;
	  }
      }

    
    params.SBNUMINSTANCES = jbig2_get_uint32(segment_data + offset);
    offset += 4;

    if (params.SBHUFF) {
        
        

        
	switch (huffman_flags & 0x0003) {
	  case 0: 
	    params.SBHUFFFS = jbig2_build_huffman_table(ctx,
			&jbig2_huffman_params_F);
	    break;
	  case 1: 
	    params.SBHUFFFS = jbig2_build_huffman_table(ctx,
			&jbig2_huffman_params_G);
	    break;
	  case 3: 
            huffman_params = jbig2_find_table(ctx, segment, table_index);
            if (huffman_params == NULL) {
                code = jbig2_error(ctx, JBIG2_SEVERITY_FATAL, segment->number,
                    "Custom FS huffman table not found (%d)", table_index);
                goto cleanup1;
            }
            params.SBHUFFFS = jbig2_build_huffman_table(ctx, huffman_params);
            ++table_index;
            break;
	  case 2: 
	  default:
	    code = jbig2_error(ctx, JBIG2_SEVERITY_FATAL, segment->number,
            "text region specified invalid FS huffman table");
        goto cleanup1;
	    break;
	}
    if (params.SBHUFFFS == NULL)
    {
        code = jbig2_error(ctx, JBIG2_SEVERITY_FATAL, segment->number,
            "failed to allocate text region specified FS huffman table");
        goto cleanup1;
    }

	switch ((huffman_flags & 0x000c) >> 2) {
	  case 0: 
	    params.SBHUFFDS = jbig2_build_huffman_table(ctx,
			&jbig2_huffman_params_H);
	    break;
	  case 1: 
	    params.SBHUFFDS = jbig2_build_huffman_table(ctx,
			&jbig2_huffman_params_I);
	    break;
	  case 2: 
	    params.SBHUFFDS = jbig2_build_huffman_table(ctx,
			&jbig2_huffman_params_J);
	    break;
	  case 3: 
            huffman_params = jbig2_find_table(ctx, segment, table_index);
            if (huffman_params == NULL) {
                code = jbig2_error(ctx, JBIG2_SEVERITY_FATAL, segment->number,
                    "Custom DS huffman table not found (%d)", table_index);
                goto cleanup1;
            }
            params.SBHUFFDS = jbig2_build_huffman_table(ctx, huffman_params);
            ++table_index;
            break;
	}
    if (params.SBHUFFDS == NULL)
    {
        code = jbig2_error(ctx, JBIG2_SEVERITY_FATAL, segment->number,
            "failed to allocate text region specified DS huffman table");
        goto cleanup1;
    }

	switch ((huffman_flags & 0x0030) >> 4) {
	  case 0: 
	    params.SBHUFFDT = jbig2_build_huffman_table(ctx,
			&jbig2_huffman_params_K);
	    break;
	  case 1: 
	    params.SBHUFFDT = jbig2_build_huffman_table(ctx,
			&jbig2_huffman_params_L);
	    break;
	  case 2: 
	    params.SBHUFFDT = jbig2_build_huffman_table(ctx,
			&jbig2_huffman_params_M);
	    break;
	  case 3: 
            huffman_params = jbig2_find_table(ctx, segment, table_index);
            if (huffman_params == NULL) {
                code = jbig2_error(ctx, JBIG2_SEVERITY_FATAL, segment->number,
                    "Custom DT huffman table not found (%d)", table_index);
                goto cleanup1;
            }
            params.SBHUFFDT = jbig2_build_huffman_table(ctx, huffman_params);
            ++table_index;
            break;
	}
    if (params.SBHUFFDT == NULL)
    {
        code = jbig2_error(ctx, JBIG2_SEVERITY_FATAL, segment->number,
            "failed to allocate text region specified DT huffman table");
        goto cleanup1;
    }

	switch ((huffman_flags & 0x00c0) >> 6) {
	  case 0: 
	    params.SBHUFFRDW = jbig2_build_huffman_table(ctx,
			&jbig2_huffman_params_N);
	    break;
	  case 1: 
	    params.SBHUFFRDW = jbig2_build_huffman_table(ctx,
			&jbig2_huffman_params_O);
	    break;
	  case 3: 
            huffman_params = jbig2_find_table(ctx, segment, table_index);
            if (huffman_params == NULL) {
                code = jbig2_error(ctx, JBIG2_SEVERITY_FATAL, segment->number,
                    "Custom RDW huffman table not found (%d)", table_index);
                goto cleanup1;
            }
            params.SBHUFFRDW = jbig2_build_huffman_table(ctx, huffman_params);
            ++table_index;
	    break;
	  case 2: 
	  default:
	    code = jbig2_error(ctx, JBIG2_SEVERITY_FATAL, segment->number,
            "text region specified invalid RDW huffman table");
        goto cleanup1;
	    break;
	}
    if (params.SBHUFFRDW == NULL)
    {
        code = jbig2_error(ctx, JBIG2_SEVERITY_FATAL, segment->number,
            "failed to allocate text region specified RDW huffman table");
        goto cleanup1;
    }

	switch ((huffman_flags & 0x0300) >> 8) {
	  case 0: 
	    params.SBHUFFRDH = jbig2_build_huffman_table(ctx,
			&jbig2_huffman_params_N);
	    break;
	  case 1: 
	    params.SBHUFFRDH = jbig2_build_huffman_table(ctx,
			&jbig2_huffman_params_O);
	    break;
	  case 3: 
            huffman_params = jbig2_find_table(ctx, segment, table_index);
            if (huffman_params == NULL) {
                code = jbig2_error(ctx, JBIG2_SEVERITY_FATAL, segment->number,
                    "Custom RDH huffman table not found (%d)", table_index);
                goto cleanup1;
            }
            params.SBHUFFRDH = jbig2_build_huffman_table(ctx, huffman_params);
            ++table_index;
	    break;
	  case 2: 
	  default:
	    code = jbig2_error(ctx, JBIG2_SEVERITY_FATAL, segment->number,
            "text region specified invalid RDH huffman table");
        goto cleanup1;
	    break;
	}
    if (params.SBHUFFRDH == NULL)
    {
        code = jbig2_error(ctx, JBIG2_SEVERITY_FATAL, segment->number,
            "failed to allocate text region specified RDH huffman table");
        goto cleanup1;
    }

    switch ((huffman_flags & 0x0c00) >> 10) {
	  case 0: 
	    params.SBHUFFRDX = jbig2_build_huffman_table(ctx,
			&jbig2_huffman_params_N);
	    break;
	  case 1: 
	    params.SBHUFFRDX = jbig2_build_huffman_table(ctx,
			&jbig2_huffman_params_O);
	    break;
	  case 3: 
            huffman_params = jbig2_find_table(ctx, segment, table_index);
            if (huffman_params == NULL) {
                code = jbig2_error(ctx, JBIG2_SEVERITY_FATAL, segment->number,
                    "Custom RDX huffman table not found (%d)", table_index);
                goto cleanup1;
            }
            params.SBHUFFRDX = jbig2_build_huffman_table(ctx, huffman_params);
            ++table_index;
	    break;
	  case 2: 
	  default:
	    code = jbig2_error(ctx, JBIG2_SEVERITY_FATAL, segment->number,
            "text region specified invalid RDX huffman table");
        goto cleanup1;
	    break;
	}
    if (params.SBHUFFRDX == NULL)
    {
        code = jbig2_error(ctx, JBIG2_SEVERITY_FATAL, segment->number,
            "failed to allocate text region specified RDX huffman table");
        goto cleanup1;
    }

	switch ((huffman_flags & 0x3000) >> 12) {
	  case 0: 
	    params.SBHUFFRDY = jbig2_build_huffman_table(ctx,
			&jbig2_huffman_params_N);
	    break;
	  case 1: 
	    params.SBHUFFRDY = jbig2_build_huffman_table(ctx,
			&jbig2_huffman_params_O);
	    break;
	  case 3: 
            huffman_params = jbig2_find_table(ctx, segment, table_index);
            if (huffman_params == NULL) {
                code = jbig2_error(ctx, JBIG2_SEVERITY_FATAL, segment->number,
                    "Custom RDY huffman table not found (%d)", table_index);
                goto cleanup1;
            }
            params.SBHUFFRDY = jbig2_build_huffman_table(ctx, huffman_params);
            ++table_index;
	    break;
	  case 2: 
	  default:
	    code = jbig2_error(ctx, JBIG2_SEVERITY_FATAL, segment->number,
            "text region specified invalid RDY huffman table");
        goto cleanup1;
	    break;
	}
    if (params.SBHUFFRDY == NULL)
    {
        code = jbig2_error(ctx, JBIG2_SEVERITY_FATAL, segment->number,
            "failed to allocate text region specified RDY huffman table");
        goto cleanup1;
    }

	switch ((huffman_flags & 0x4000) >> 14) {
	  case 0: 
	    params.SBHUFFRSIZE = jbig2_build_huffman_table(ctx,
			&jbig2_huffman_params_A);
	    break;
	  case 1: 
            huffman_params = jbig2_find_table(ctx, segment, table_index);
            if (huffman_params == NULL) {
                code = jbig2_error(ctx, JBIG2_SEVERITY_FATAL, segment->number,
                    "Custom RSIZE huffman table not found (%d)", table_index);
                goto cleanup1;
            }
            params.SBHUFFRSIZE = jbig2_build_huffman_table(ctx, huffman_params);
            ++table_index;
	    break;
	}
    if (params.SBHUFFRSIZE == NULL)
    {
        code = jbig2_error(ctx, JBIG2_SEVERITY_FATAL, segment->number,
            "failed to allocate text region specified RSIZE huffman table");
        goto cleanup1;
    }

        if (huffman_flags & 0x8000) {
	  jbig2_error(ctx, JBIG2_SEVERITY_WARNING, segment->number,
 	    "text region huffman flags bit 15 is set, contrary to spec");
	}

        
        
    }

    jbig2_error(ctx, JBIG2_SEVERITY_INFO, segment->number,
        "text region: %d x %d @ (%d,%d) %d symbols",
        region_info.width, region_info.height,
        region_info.x, region_info.y, params.SBNUMINSTANCES);

    
    n_dicts = jbig2_sd_count_referred(ctx, segment);
    if (n_dicts != 0) {
        dicts = jbig2_sd_list_referred(ctx, segment);
    } else {
        code = jbig2_error(ctx, JBIG2_SEVERITY_FATAL, segment->number,
            "text region refers to no symbol dictionaries!");
        goto cleanup1;
    }
    if (dicts == NULL) {
        code = jbig2_error(ctx, JBIG2_SEVERITY_FATAL, segment->number,
            "unable to retrive symbol dictionaries! previous parsing error?");
        goto cleanup1;
    } else {
	int index;
	if (dicts[0] == NULL) {
        code =jbig2_error(ctx, JBIG2_SEVERITY_WARNING, segment->number,
            "unable to find first referenced symbol dictionary!");
        goto cleanup1;
	}
	for (index = 1; index < n_dicts; index++)
	    if (dicts[index] == NULL) {
		jbig2_error(ctx, JBIG2_SEVERITY_WARNING, segment->number,
			"unable to find all referenced symbol dictionaries!");
	    n_dicts = index;
	}
    }

    
    if (!params.SBHUFF && params.SBREFINE) {
	int stats_size = params.SBRTEMPLATE ? 1 << 10 : 1 << 13;
	GR_stats = jbig2_new(ctx, Jbig2ArithCx, stats_size);
    if (GR_stats == NULL)
    {
        code = jbig2_error(ctx, JBIG2_SEVERITY_FATAL, segment->number,
            "could not allocate GR_stats");
        goto cleanup1;
    }
	memset(GR_stats, 0, stats_size);
    }

    image = jbig2_image_new(ctx, region_info.width, region_info.height);
    if (image == NULL) {
        code =jbig2_error(ctx, JBIG2_SEVERITY_FATAL, segment->number,
            "couldn't allocate text region image");
        goto cleanup1;
    }

    ws = jbig2_word_stream_buf_new(ctx, segment_data + offset, segment->data_length - offset);
    if (ws == NULL)
    {
        code = jbig2_error(ctx, JBIG2_SEVERITY_FATAL, segment->number,
            "couldn't allocate text region image");
        jbig2_image_release(ctx, image);
        goto cleanup2;
    }
    if (!params.SBHUFF) {
	int SBSYMCODELEN, index;
        int SBNUMSYMS = 0;
	for (index = 0; index < n_dicts; index++) {
	    SBNUMSYMS += dicts[index]->n_symbols;
	}

	as = jbig2_arith_new(ctx, ws);
    params.IADT = jbig2_arith_int_ctx_new(ctx);
    params.IAFS = jbig2_arith_int_ctx_new(ctx);
    params.IADS = jbig2_arith_int_ctx_new(ctx);
    params.IAIT = jbig2_arith_int_ctx_new(ctx);
    if ((as == NULL) || (params.IADT == NULL) || (params.IAFS == NULL) ||
        (params.IADS == NULL) || (params.IAIT == NULL))
    {
        code = jbig2_error(ctx, JBIG2_SEVERITY_FATAL, segment->number,
            "couldn't allocate text region image data");
        goto cleanup3;
    }

	
	for (SBSYMCODELEN = 0; (1 << SBSYMCODELEN) < SBNUMSYMS; SBSYMCODELEN++);
    params.IAID = jbig2_arith_iaid_ctx_new(ctx, SBSYMCODELEN);
	params.IARI = jbig2_arith_int_ctx_new(ctx);
	params.IARDW = jbig2_arith_int_ctx_new(ctx);
	params.IARDH = jbig2_arith_int_ctx_new(ctx);
	params.IARDX = jbig2_arith_int_ctx_new(ctx);
	params.IARDY = jbig2_arith_int_ctx_new(ctx);
    if ((params.IAID == NULL) || (params.IARI == NULL) ||
        (params.IARDW == NULL) || (params.IARDH == NULL) ||
        (params.IARDX == NULL) || (params.IARDY == NULL))
    {
        code = jbig2_error(ctx, JBIG2_SEVERITY_FATAL, segment->number,
            "couldn't allocate text region image data");
        goto cleanup4;
    }
    }

    code = jbig2_decode_text_region(ctx, segment, &params,
        (const Jbig2SymbolDict * const *)dicts, n_dicts, image,
        segment_data + offset, segment->data_length - offset,
		GR_stats, as, as ? NULL : ws);
    if (code < 0)
    {
        jbig2_error(ctx, JBIG2_SEVERITY_FATAL, segment->number,
            "failed to decode text region image data");
        jbig2_image_release(ctx, image);
        goto cleanup4;
    }

    if ((segment->flags & 63) == 4) {
        
        segment->result = image;
    } else {
        
        jbig2_error(ctx, JBIG2_SEVERITY_DEBUG, segment->number,
            "composing %dx%d decoded text region onto page at (%d, %d)",
            region_info.width, region_info.height, region_info.x, region_info.y);
        jbig2_page_add_result(ctx, &ctx->pages[ctx->current_page], image,
            region_info.x, region_info.y, region_info.op);
        jbig2_image_release(ctx, image);
    }

cleanup4:
    if (!params.SBHUFF) {
        jbig2_arith_iaid_ctx_free(ctx, params.IAID);
        jbig2_arith_int_ctx_free(ctx, params.IARI);
        jbig2_arith_int_ctx_free(ctx, params.IARDW);
        jbig2_arith_int_ctx_free(ctx, params.IARDH);
        jbig2_arith_int_ctx_free(ctx, params.IARDX);
        jbig2_arith_int_ctx_free(ctx, params.IARDY);
    }

cleanup3:
    if (!params.SBHUFF) {
        jbig2_arith_int_ctx_free(ctx, params.IADT);
        jbig2_arith_int_ctx_free(ctx, params.IAFS);
        jbig2_arith_int_ctx_free(ctx, params.IADS);
        jbig2_arith_int_ctx_free(ctx, params.IAIT);
        jbig2_free(ctx->allocator, as);
    }
    jbig2_word_stream_buf_free(ctx, ws);

cleanup2:
    if (!params.SBHUFF && params.SBREFINE) {
        jbig2_free(ctx->allocator, GR_stats);
    }

cleanup1:
    if (params.SBHUFF) {
        jbig2_release_huffman_table(ctx, params.SBHUFFFS);
        jbig2_release_huffman_table(ctx, params.SBHUFFDS);
        jbig2_release_huffman_table(ctx, params.SBHUFFDT);
        jbig2_release_huffman_table(ctx, params.SBHUFFRDX);
        jbig2_release_huffman_table(ctx, params.SBHUFFRDY);
        jbig2_release_huffman_table(ctx, params.SBHUFFRDW);
        jbig2_release_huffman_table(ctx, params.SBHUFFRDH);
        jbig2_release_huffman_table(ctx, params.SBHUFFRSIZE);
    }
    jbig2_free(ctx->allocator, dicts);

    return code;

too_short:
    return jbig2_error(ctx, JBIG2_SEVERITY_FATAL, segment->number,
        "Segment too short");
}
