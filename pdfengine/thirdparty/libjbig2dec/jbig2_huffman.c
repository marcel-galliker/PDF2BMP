

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "os_types.h"

#include <stdlib.h>
#include <string.h>

#ifdef JBIG2_DEBUG
#include <stdio.h>
#endif

#include "jbig2.h"
#include "jbig2_priv.h"
#include "jbig2_huffman.h"
#include "jbig2_hufftab.h"

#define JBIG2_HUFFMAN_FLAGS_ISOOB 1
#define JBIG2_HUFFMAN_FLAGS_ISLOW 2
#define JBIG2_HUFFMAN_FLAGS_ISEXT 4

struct _Jbig2HuffmanState {
  
  uint32_t this_word;
  uint32_t next_word;
  int offset_bits;
  int offset;

  Jbig2WordStream *ws;
};

Jbig2HuffmanState *
jbig2_huffman_new (Jbig2Ctx *ctx, Jbig2WordStream *ws)
{
  Jbig2HuffmanState *result = NULL;

  result = jbig2_new(ctx, Jbig2HuffmanState, 1);

  if (result != NULL) {
      result->offset = 0;
      result->offset_bits = 0;
      result->this_word = ws->get_next_word (ws, 0);
      result->next_word = ws->get_next_word (ws, 4);
      result->ws = ws;
  } else {
      jbig2_error(ctx, JBIG2_SEVERITY_FATAL, -1,
          "failed to allocate new huffman coding state");
  }

  return result;
}

void
jbig2_huffman_free (Jbig2Ctx *ctx, Jbig2HuffmanState *hs)
{
  if (hs != NULL) free(hs);
  return;
}

#ifdef JBIG2_DEBUG

void jbig2_dump_huffman_state(Jbig2HuffmanState *hs) {
  fprintf(stderr, "huffman state %08x %08x offset %d.%d\n",
	hs->this_word, hs->next_word, hs->offset, hs->offset_bits);
}

void jbig2_dump_huffman_binary(Jbig2HuffmanState *hs)
{
  const uint32_t word = hs->this_word;
  int i;

  fprintf(stderr, "huffman binary ");
  for (i = 31; i >= 0; i--)
    fprintf(stderr, ((word >> i) & 1) ? "1" : "0");
  fprintf(stderr, "\n");
}

void jbig2_dump_huffman_table(const Jbig2HuffmanTable *table)
{
    int i;
    int table_size = (1 << table->log_table_size);
    fprintf(stderr, "huffman table %p (log_table_size=%d, %d entries, entryies=%p):\n", table, table->log_table_size, table_size, table->entries);
    for (i = 0; i < table_size; i++) {
        fprintf(stderr, "%6d: PREFLEN=%d, RANGELEN=%d, ", i, table->entries[i].PREFLEN, table->entries[i].RANGELEN);
        if ( table->entries[i].flags & JBIG2_HUFFMAN_FLAGS_ISEXT ) {
            fprintf(stderr, "ext=%p", table->entries[i].u.ext_table);
        } else {
            fprintf(stderr, "RANGELOW=%d", table->entries[i].u.RANGELOW);
        }
        if ( table->entries[i].flags ) {
            int need_comma = 0;
            fprintf(stderr, ", flags=0x%x(", table->entries[i].flags);
            if ( table->entries[i].flags & JBIG2_HUFFMAN_FLAGS_ISOOB ) {
                fprintf(stderr, "OOB");
                need_comma = 1;
            }
            if ( table->entries[i].flags & JBIG2_HUFFMAN_FLAGS_ISLOW ) {
                if ( need_comma )
                    fprintf(stderr, ",");
                fprintf(stderr, "LOW");
                need_comma = 1;
            }
            if ( table->entries[i].flags & JBIG2_HUFFMAN_FLAGS_ISEXT ) {
                if ( need_comma )
                    fprintf(stderr, ",");
                fprintf(stderr, "EXT");
            }
            fprintf(stderr, ")");
        }
        fprintf(stderr, "\n");
    }
    fprintf(stderr, "\n");
}

#endif 

void
jbig2_huffman_skip(Jbig2HuffmanState *hs)
{
  int bits = hs->offset_bits & 7;

  if (bits) {
    bits = 8 - bits;
    hs->offset_bits += bits;
    hs->this_word = (hs->this_word << bits) |
	(hs->next_word >> (32 - hs->offset_bits));
  }

  if (hs->offset_bits >= 32) {
    Jbig2WordStream *ws = hs->ws;
    hs->this_word = hs->next_word;
    hs->offset += 4;
    hs->next_word = ws->get_next_word (ws, hs->offset + 4);
    hs->offset_bits -= 32;
    if (hs->offset_bits) {
      hs->this_word = (hs->this_word << hs->offset_bits) |
	(hs->next_word >> (32 - hs->offset_bits));
    }
  }
}

void jbig2_huffman_advance(Jbig2HuffmanState *hs, int offset)
{
  Jbig2WordStream *ws = hs->ws;

  hs->offset += offset & ~3;
  hs->offset_bits += (offset & 3) << 3;
  if (hs->offset_bits >= 32) {
    hs->offset += 4;
    hs->offset_bits -= 32;
  }
  hs->this_word = ws->get_next_word (ws, hs->offset);
  hs->next_word = ws->get_next_word (ws, hs->offset + 4);
  if (hs->offset_bits > 0)
    hs->this_word = (hs->this_word << hs->offset_bits) |
	(hs->next_word >> (32 - hs->offset_bits));
}

int
jbig2_huffman_offset(Jbig2HuffmanState *hs)
{
  return hs->offset + (hs->offset_bits >> 3);
}

int32_t
jbig2_huffman_get_bits (Jbig2HuffmanState *hs, const int bits)
{
  uint32_t this_word = hs->this_word;
  int32_t result;

  result = this_word >> (32 - bits);
  hs->offset_bits += bits;
  if (hs->offset_bits >= 32) {
    hs->offset += 4;
    hs->offset_bits -= 32;
    hs->this_word = hs->next_word;
    hs->next_word = hs->ws->get_next_word(hs->ws, hs->offset + 4);
    if (hs->offset_bits) {
      hs->this_word = (hs->this_word << hs->offset_bits) |
	(hs->next_word >> (32 - hs->offset_bits));
    } else {
      hs->this_word = (hs->this_word << hs->offset_bits);
    }
  } else {
    hs->this_word = (this_word << bits) |
	(hs->next_word >> (32 - hs->offset_bits));
  }

  return result;
}

int32_t
jbig2_huffman_get (Jbig2HuffmanState *hs,
		   const Jbig2HuffmanTable *table, bool *oob)
{
  Jbig2HuffmanEntry *entry;
  byte flags;
  int offset_bits = hs->offset_bits;
  uint32_t this_word = hs->this_word;
  uint32_t next_word;
  int RANGELEN;
  int32_t result;

  for (;;)
    {
      int log_table_size = table->log_table_size;
      int PREFLEN;

      entry = &table->entries[this_word >> (32 - log_table_size)];
      flags = entry->flags;
      PREFLEN = entry->PREFLEN;

      next_word = hs->next_word;
      offset_bits += PREFLEN;
      if (offset_bits >= 32)
	{
	  Jbig2WordStream *ws = hs->ws;
	  this_word = next_word;
	  hs->offset += 4;
	  next_word = ws->get_next_word (ws, hs->offset + 4);
	  offset_bits -= 32;
	  hs->next_word = next_word;
	  PREFLEN = offset_bits;
	}
      if (PREFLEN)
	this_word = (this_word << PREFLEN) |
	  (next_word >> (32 - offset_bits));
      if (flags & JBIG2_HUFFMAN_FLAGS_ISEXT)
	{
	  table = entry->u.ext_table;
	}
      else
	break;
    }
  result = entry->u.RANGELOW;
  RANGELEN = entry->RANGELEN;
  if (RANGELEN > 0)
    {
      int32_t HTOFFSET;

      HTOFFSET = this_word >> (32 - RANGELEN);
      if (flags & JBIG2_HUFFMAN_FLAGS_ISLOW)
	result -= HTOFFSET;
      else
	result += HTOFFSET;

      offset_bits += RANGELEN;
      if (offset_bits >= 32)
	{
	  Jbig2WordStream *ws = hs->ws;
	  this_word = next_word;
	  hs->offset += 4;
	  next_word = ws->get_next_word (ws, hs->offset + 4);
	  offset_bits -= 32;
	  hs->next_word = next_word;
	  RANGELEN = offset_bits;
	}
if (RANGELEN)
      this_word = (this_word << RANGELEN) |
	(next_word >> (32 - offset_bits));
    }

  hs->this_word = this_word;
  hs->offset_bits = offset_bits;

  if (oob != NULL)
    *oob = (flags & JBIG2_HUFFMAN_FLAGS_ISOOB);

  return result;
}

#define LOG_TABLE_SIZE_MAX 16

Jbig2HuffmanTable *
jbig2_build_huffman_table (Jbig2Ctx *ctx, const Jbig2HuffmanParams *params)
{
  int *LENCOUNT;
  int LENMAX = -1;
  const int lencountcount = 256;
  const Jbig2HuffmanLine *lines = params->lines;
  int n_lines = params->n_lines;
  int i, j;
  int max_j;
  int log_table_size = 0;
  Jbig2HuffmanTable *result;
  Jbig2HuffmanEntry *entries;
  int CURLEN;
  int firstcode = 0;
  int CURCODE;
  int CURTEMP;

  LENCOUNT = jbig2_new(ctx, int, lencountcount);
  if (LENCOUNT == NULL) {
    jbig2_error(ctx, JBIG2_SEVERITY_FATAL, -1,
      "couldn't allocate storage for huffman histogram");
    return NULL;
  }
  memset(LENCOUNT, 0, sizeof(int) * lencountcount);

  
  for (i = 0; i < params->n_lines; i++)
    {
      int PREFLEN = lines[i].PREFLEN;
      int lts;

      if (PREFLEN > LENMAX)
		{
			for (j = LENMAX + 1; j < PREFLEN + 1; j++)
				LENCOUNT[j] = 0;
			LENMAX = PREFLEN;
		}
      LENCOUNT[PREFLEN]++;

      lts = PREFLEN + lines[i].RANGELEN;
      if (lts > LOG_TABLE_SIZE_MAX)
		lts = PREFLEN;
      if (lts <= LOG_TABLE_SIZE_MAX && log_table_size < lts)
		log_table_size = lts;
    }
  jbig2_error(ctx, JBIG2_SEVERITY_DEBUG, -1,
	"constructing huffman table log size %d", log_table_size);
  max_j = 1 << log_table_size;

  result = jbig2_new(ctx, Jbig2HuffmanTable, 1);
  if (result == NULL)
  {
    jbig2_error(ctx, JBIG2_SEVERITY_FATAL, -1,
        "couldn't allocate result storage in jbig2_build_huffman_table");
    return NULL;
  }
  result->log_table_size = log_table_size;
  entries = jbig2_new(ctx, Jbig2HuffmanEntry, max_j);
  if (entries == NULL)
  {
    jbig2_error(ctx, JBIG2_SEVERITY_FATAL, -1,
        "couldn't allocate entries storage in jbig2_build_huffman_table");
    return NULL;
  }
  result->entries = entries;

  LENCOUNT[0] = 0;

  for (CURLEN = 1; CURLEN <= LENMAX; CURLEN++)
    {
      int shift = log_table_size - CURLEN;

      
      firstcode = (firstcode + LENCOUNT[CURLEN - 1]) << 1;
      CURCODE = firstcode;
      
      for (CURTEMP = 0; CURTEMP < n_lines; CURTEMP++)
	{
	  int PREFLEN = lines[CURTEMP].PREFLEN;
	  if (PREFLEN == CURLEN)
	    {
	      int RANGELEN = lines[CURTEMP].RANGELEN;
	      int start_j = CURCODE << shift;
	      int end_j = (CURCODE + 1) << shift;
	      byte eflags = 0;

	      if (end_j > max_j) {
		jbig2_error(ctx, JBIG2_SEVERITY_FATAL, -1,
		  "ran off the end of the entries table! (%d >= %d)",
		  end_j, max_j);
		jbig2_free(ctx->allocator, result->entries);
		jbig2_free(ctx->allocator, result);
		jbig2_free(ctx->allocator, LENCOUNT);
		return NULL;
	      }
	      
	      if (params->HTOOB && CURTEMP == n_lines - 1)
		eflags |= JBIG2_HUFFMAN_FLAGS_ISOOB;
	      if (CURTEMP == n_lines - (params->HTOOB ? 3 : 2))
		eflags |= JBIG2_HUFFMAN_FLAGS_ISLOW;
	      if (PREFLEN + RANGELEN > LOG_TABLE_SIZE_MAX) {
		  for (j = start_j; j < end_j; j++) {
		      entries[j].u.RANGELOW = lines[CURTEMP].RANGELOW;
		      entries[j].PREFLEN = PREFLEN;
		      entries[j].RANGELEN = RANGELEN;
		      entries[j].flags = eflags;
		    }
	      } else {
		  for (j = start_j; j < end_j; j++) {
		      int32_t HTOFFSET = (j >> (shift - RANGELEN)) &
			((1 << RANGELEN) - 1);
		      if (eflags & JBIG2_HUFFMAN_FLAGS_ISLOW)
			entries[j].u.RANGELOW = lines[CURTEMP].RANGELOW -
			  HTOFFSET;
		      else
			entries[j].u.RANGELOW = lines[CURTEMP].RANGELOW +
			  HTOFFSET;
		      entries[j].PREFLEN = PREFLEN + RANGELEN;
		      entries[j].RANGELEN = 0;
		      entries[j].flags = eflags;
		    }
		}
	      CURCODE++;
	    }
	}
    }

  jbig2_free(ctx->allocator, LENCOUNT);

  return result;
}

void
jbig2_release_huffman_table (Jbig2Ctx *ctx, Jbig2HuffmanTable *table)
{
  if (table != NULL) {
      jbig2_free(ctx->allocator, table->entries);
      jbig2_free(ctx->allocator, table);
  }
  return;
}

static uint32_t
jbig2_table_read_bits(const byte *data, size_t *bitoffset, const int bitlen)
{
    uint32_t result = 0;
    uint32_t byte_offset = *bitoffset / 8;
    const int endbit = (*bitoffset & 7) + bitlen;
    const int n_proc_bytes = (endbit + 7) / 8;
    const int rshift = n_proc_bytes * 8 - endbit;
    int i;
    for (i = n_proc_bytes - 1; i >= 0; i--) {
        uint32_t d = data[byte_offset++];
        const int nshift = i * 8 - rshift;
        if (nshift > 0)
            d <<= nshift;
        else if (nshift < 0)
            d >>= -nshift;
        result |= d;
    }
    result &= ~(-1 << bitlen);
    *bitoffset += bitlen;
    return result;
}

int
jbig2_table(Jbig2Ctx *ctx, Jbig2Segment *segment, const byte *segment_data)
{
    Jbig2HuffmanParams *params = NULL;
    Jbig2HuffmanLine *line = NULL;

    segment->result = NULL;
    if (segment->data_length < 10)
        goto too_short;

    {
        
        const int code_table_flags = segment_data[0];
        const int HTOOB = code_table_flags & 0x01; 
        
        const int HTPS  = (code_table_flags >> 1 & 0x07) + 1;
        
        const int HTRS  = (code_table_flags >> 4 & 0x07) + 1;
        
        const int32_t HTLOW  = jbig2_get_int32(segment_data + 1);
        
        const int32_t HTHIGH = jbig2_get_int32(segment_data + 5);
        
        const size_t lines_max = (segment->data_length * 8 - HTPS * (HTOOB ? 3 : 2)) /
                                                        (HTPS + HTRS) + (HTOOB ? 3 : 2);
        
        const byte *lines_data = segment_data + 9;
        const size_t lines_data_bitlen = (segment->data_length - 9) * 8;    
        
        size_t boffset = 0;
        
        int32_t CURRANGELOW = HTLOW;
        int NTEMP = 0;

#ifdef JBIG2_DEBUG
        jbig2_error(ctx, JBIG2_SEVERITY_DEBUG, segment->number, 
            "DECODING USER TABLE... Flags: %d, HTOOB: %d, HTPS: %d, HTRS: %d, HTLOW: %d, HTHIGH: %d", 
            code_table_flags, HTOOB, HTPS, HTRS, HTLOW, HTHIGH);
#endif

        
        params = jbig2_new(ctx, Jbig2HuffmanParams, 1);
        if (params == NULL) {
            jbig2_error(ctx, JBIG2_SEVERITY_FATAL, segment->number,
                            "Could not allocate Huffman Table Parameter");
            goto error_exit;
        }
        line = jbig2_new(ctx, Jbig2HuffmanLine, lines_max);
        if (line == NULL) {
            jbig2_error(ctx, JBIG2_SEVERITY_FATAL, segment->number,
                            "Could not allocate Huffman Table Lines");
            goto error_exit;
        }
        
        while (CURRANGELOW < HTHIGH) {
            
            if (boffset + HTPS >= lines_data_bitlen)
                goto too_short;
            line[NTEMP].PREFLEN  = jbig2_table_read_bits(lines_data, &boffset, HTPS);
            
            if (boffset + HTRS >= lines_data_bitlen)
                goto too_short;
            line[NTEMP].RANGELEN = jbig2_table_read_bits(lines_data, &boffset, HTRS);
            
            line[NTEMP].RANGELOW = CURRANGELOW;
            CURRANGELOW += (1 << line[NTEMP].RANGELEN);
            NTEMP++;
        }
        
        if (boffset + HTPS >= lines_data_bitlen)
            goto too_short;
        line[NTEMP].PREFLEN  = jbig2_table_read_bits(lines_data, &boffset, HTPS);
        line[NTEMP].RANGELEN = 32;
        line[NTEMP].RANGELOW = HTLOW - 1;
        NTEMP++;
        
        if (boffset + HTPS >= lines_data_bitlen)
            goto too_short;
        line[NTEMP].PREFLEN  = jbig2_table_read_bits(lines_data, &boffset, HTPS);
        line[NTEMP].RANGELEN = 32;
        line[NTEMP].RANGELOW = HTHIGH;
        NTEMP++;
        
        if (HTOOB) {
            
            if (boffset + HTPS >= lines_data_bitlen)
                goto too_short;
            line[NTEMP].PREFLEN  = jbig2_table_read_bits(lines_data, &boffset, HTPS);
            line[NTEMP].RANGELEN = 0;
            line[NTEMP].RANGELOW = 0;
            NTEMP++;
        }
        if (NTEMP != lines_max) {
            Jbig2HuffmanLine *new_line = jbig2_renew(ctx, line,
                Jbig2HuffmanLine, NTEMP);
            if ( new_line == NULL ) {
                jbig2_error(ctx, JBIG2_SEVERITY_FATAL, segment->number,
                                "Could not reallocate Huffman Table Lines");
                goto error_exit;
            }
            line = new_line;
        }
        params->HTOOB   = HTOOB;
        params->n_lines = NTEMP;
        params->lines   = line;
        segment->result = params;

#ifdef JBIG2_DEBUG
        {
            int i;
            for (i = 0; i < NTEMP; i++) {
                jbig2_error(ctx, JBIG2_SEVERITY_DEBUG, segment->number, 
                    "Line: %d, PREFLEN: %d, RANGELEN: %d, RANGELOW: %d", 
                    i, params->lines[i].PREFLEN, params->lines[i].RANGELEN, params->lines[i].RANGELOW);
            }
        }
#endif
    }
    return 0;

too_short:
    jbig2_error(ctx, JBIG2_SEVERITY_FATAL, segment->number, "Segment too short");
error_exit:
    if (line != NULL) {
        jbig2_free(ctx->allocator, line);
    }
    if (params != NULL) {
        jbig2_free(ctx->allocator, params);
    }
    return -1;
}

void
jbig2_table_free(Jbig2Ctx *ctx, Jbig2HuffmanParams *params)
{
    if (params != NULL) {
        if (params->lines != NULL)
            jbig2_free(ctx->allocator, (void *)params->lines);
        jbig2_free(ctx->allocator, params);
    }
}

const Jbig2HuffmanParams *
jbig2_find_table(Jbig2Ctx *ctx, Jbig2Segment *segment, int index)
{
    int i, table_index = 0;

    for (i = 0; i < segment->referred_to_segment_count; i++) {
        const Jbig2Segment * const rsegment =
                jbig2_find_segment(ctx, segment->referred_to_segments[i]);
        if (rsegment && (rsegment->flags & 63) == 53) {
            if (table_index == index)
                return (const Jbig2HuffmanParams *)rsegment->result;
            ++table_index;
        }
    }
    return NULL;
}

#ifdef TEST
#include <stdio.h>

const byte	test_stream[] = { 0xe9, 0xcb, 0xf4, 0x00 };
const byte	test_tabindex[] = { 4, 2, 2, 1 };

static uint32_t
test_get_word (Jbig2WordStream *self, int offset)
{
	
	if (offset+3 > sizeof(test_stream))
		return 0;
	else
		return ( (test_stream[offset] << 24) |
				 (test_stream[offset+1] << 16) |
				 (test_stream[offset+2] << 8) |
				 (test_stream[offset+3]) );
}

int
main (int argc, char **argv)
{
  Jbig2Ctx *ctx;
  Jbig2HuffmanTable *tables[5];
  Jbig2HuffmanState *hs;
  Jbig2WordStream ws;
  bool oob;
  int32_t code;

  ctx = jbig2_ctx_new(NULL, 0, NULL, NULL, NULL);

  tables[0] = NULL;
  tables[1] = jbig2_build_huffman_table (ctx, &jbig2_huffman_params_A);
  tables[2] = jbig2_build_huffman_table (ctx, &jbig2_huffman_params_B);
  tables[3] = NULL;
  tables[4] = jbig2_build_huffman_table (ctx, &jbig2_huffman_params_D);
  ws.get_next_word = test_get_word;
  hs = jbig2_huffman_new (ctx, &ws);

  printf("testing jbig2 huffmann decoding...");
  printf("\t(should be 8 5 (oob) 8)\n");

  {
	int i;
	int sequence_length = sizeof(test_tabindex);

	for (i = 0; i < sequence_length; i++) {
		code = jbig2_huffman_get (hs, tables[test_tabindex[i]], &oob);
		if (oob) printf("(oob) ");
		else printf("%d ", code);
	}
  }

  printf("\n");

  jbig2_ctx_free(ctx);

  return 0;
}
#endif

#ifdef TEST2
#include <stdio.h>

const int32_t test_output_A[] = {
    
    0,      
    1,      
    14,     
    15,     
    
    16,     
    17,     
    270,    
    271,    
    
    272,    
    273,    
    65806,  
    65807,  
    
    65808,  
    65809,  
};
const byte test_input_A[] = {
    
       0x00,     0x5c,     0xf8,     0x02,
    
       0x01,     0xbf,     0xaf,     0xfc,
    
       0x00,     0x01,     0x80,     0x00,
    
       0x77,     0xff,     0xf6,     0xff,
    
       0xff,     0xe0,     0x00,     0x00,
    
       0x00,     0x1c,     0x00,     0x00,
    
       0x00,     0x04, 
};

const int32_t test_output_B[] = {
    
    0,      
    
    1,      
    
    2,      
    
    3,      
    4,      
    9,      
    10,     
    
    11,     
    12,     
    73,     
    74,     
    
    75,     
    76,     
    
     
};
const byte test_input_B[] = {
    
       0x5b,     0x87,     0x1e,     0xdd,
    
       0xfc,     0x07,     0x81,     0xf7,
    
       0xde,     0xff,     0xe0,     0x00,
    
       0x00,     0x00,     0x0f,     0x80,
    
       0x00,     0x00,     0x00,     0x7f,
};

const int32_t test_output_C[] = {
    
    -256,   
    -255,   
    -2,     
    -1,     
    
    0,      
    
    1,      
    
    2,      
    
    3,      
    4,      
    9,      
    10,     
    
    11,     
    12,     
    73,     
    74,     
    
    -257,   
    -258,   
    
    75,     
    76,     
    
     
};
const byte test_input_C[] = {
    
       0xfe,     0x00,     0xfe,     0x01,
    
       0xfe,     0xfe,     0xfe,     0xff,
    
       0x5b,     0x87,     0x1e,     0xdd,
    
       0xfc,     0x07,     0x81,     0xf7,
    
       0xde,     0xff,     0xfc,     0x00,
    
       0x00,     0x00,     0x03,     0xfc,
    
       0x00,     0x00,     0x00,     0x07,
    
       0xf0,     0x00,     0x00,     0x00,
    
       0x07,     0xe0,     0x00,     0x00,
    
       0x00,     0x1f,     0x80,
};

const int32_t test_output_D[] = {
    
    1,      
    
    2,      
    
    3,      
    
    4,      
    5,      
    10,     
    11,     
    
    12,     
    13,     
    74,     
    75,     
    
    76,     
    77,     
};
const byte test_input_D[] = {
    
       0x5b,     0x87,     0x1e,     0xdd,
    
       0xfc,     0x07,     0x81,     0xf7,
    
       0xde,     0xff,     0xe0,     0x00,
    
       0x00,     0x00,     0x1f,     0x00,
    
       0x00,     0x00,     0x01,
};

const int32_t test_output_E[] = {
    
    -255,   
    -254,   
    -1,     
    0,      
    
    1,      
    
    2,      
    
    3,      
    
    4,      
    5,      
    10,     
    11,     
    
    12,     
    13,     
    74,     
    75,     
    
    -256,   
    -257,   
    
    76,     
    77,     
};
const byte test_input_E[] = {
    
       0xfc,     0x01,     0xf8,     0x07,
    
       0xf7,     0xf7,     0xef,     0xf5,
    
       0xb8,     0x71,     0xed,     0xdf,
    
       0xc0,     0x78,     0x1f,     0x7d,
    
       0xef,     0xff,     0x80,     0x00,
    
       0x00,     0x00,     0x7f,     0x00,
    
       0x00,     0x00,     0x01,     0xf8,
    
       0x00,     0x00,     0x00,     0x03,
    
       0xe0,     0x00,     0x00,     0x00,
    
       0x10,
};

const int32_t test_output_F[] = {
    
    -2048,  
    -2047,  
    -1026,  
    -1025,  
    
    -1024,  
    -1023,  
    -514,   
    -513,   
    
    -512,   
    -511,   
    -258,   
    -257,   
    
    -256,   
    -255,   
    -130,   
    -129,   
    
    -128,   
    -127,   
    -66,    
    -65,    
    
    -64,    
    -63,    
    -34,    
    -33,    
    
    -32,    
    -31,    
    -2,     
    -1,     
    
    0,      
    1,      
    126,    
    127,    
    
    128,    
    129,    
    254,    
    255,    
    
    256,    
    257,    
    510,    
    511,    
    
    512,    
    513,    
    1022,   
    1023,   
    
    1024,   
    1025,   
    2046,   
    2047,   
    
    -2049,  
    -2050,  
    
    2048,   
    2049,   
};
const byte test_input_F[] = {
    
       0xe0,     0x01,     0xc0,     0x07,
    
       0x9f,     0xf7,     0x3f,     0xf8,
    
       0x00,     0x40,     0x06,     0x3f,
    
       0xd1,     0xff,     0x90,     0x09,
    
       0x01,     0x9f,     0xe9,     0xff,
    
       0xa0,     0x14,     0x06,     0xbf,
    
       0x57,     0xfe,     0x81,     0xd0,
    
       0x7b,     0xf7,     0x7f,     0xf0,
    
       0x3c,     0x1f,     0x7b,     0xdf,
    
       0xb0,     0x58,     0x6f,     0xd7,
    
       0xf0,     0x00,     0x04,     0xfc,
    
       0x7f,     0x40,     0x10,     0x15,
    
       0xf9,     0x7f,     0x60,     0x0c,
    
       0x05,     0xff,     0x3f,     0xfc,
    
       0x00,     0x60,     0x07,     0x3f,
    
       0xd9,     0xff,     0xd0,     0x03,
    
       0x40,     0x1d,     0xff,     0xb7,
    
       0xff,     0xf8,     0x00,     0x00,
    
       0x00,     0x03,     0xe0,     0x00,
    
       0x00,     0x00,     0x1f,     0xc0,
    
       0x00,     0x00,     0x00,     0x3f,
    
       0x00,     0x00,     0x00,     0x01,
};

const int32_t test_output_G[] = {
    
    -1024,  
    -1023,  
    -514,   
    -513,   
    
    -512,   
    -511,   
    -258,   
    -257,   
    
    -256,   
    -255,   
    -130,   
    -129,   
    
    -128,   
    -127,   
    -66,    
    -65,    
    
    -64,    
    -63,    
    -34,    
    -33,    
    
    -32,    
    -31,    
    -2,     
    -1,     
    
    0,      
    1,      
    30,     
    31,     
    
    32,     
    33,     
    62,     
    63,     
    
    64,     
    65,     
    126,    
    127,    
    
    128,    
    129,    
    254,    
    255,    
    
    256,    
    257,    
    510,    
    511,    
    
    512,    
    513,    
    1022,   
    1023,   
    
    1024,   
    1025,   
    2046,   
    2047,   
    
    -1025,  
    -1026,  
    
    2048,   
    2049,   
};
const byte test_input_G[] = {
    
       0x80,     0x04,     0x00,     0x63,
    
       0xfd,     0x1f,     0xf0,     0x00,
    
       0x00,     0x47,     0xf0,     0xff,
    
       0x90,     0x12,     0x06,     0x7f,
    
       0x4f,     0xfd,     0x01,     0xa0,
    
       0x75,     0xf6,     0xbf,     0xd8,
    
       0x36,     0x1d,     0xfb,     0x7f,
    
       0xa0,     0x50,     0x6b,     0xd5,
    
       0xfb,     0x05,     0x86,     0xfd,
    
       0x7f,     0xe0,     0x38,     0x1e,
    
       0x7b,     0x9f,     0xe8,     0x1d,
    
       0x07,     0xbf,     0x77,     0xfc,
    
       0x01,     0x80,     0x73,     0xf6,
    
       0x7f,     0x20,     0x04,     0x04,
    
       0xff,     0x1f,     0xf4,     0x00,
    
       0x40,     0x15,     0xfe,     0x5f,
    
       0xf6,     0x00,     0x30,     0x05,
    
       0xff,     0xcf,     0xff,     0xf0,
    
       0x00,     0x00,     0x00,     0x07,
    
       0x80,     0x00,     0x00,     0x00,
    
       0x7e,     0x00,     0x00,     0x00,
    
       0x01,     0xf0,     0x00,     0x00,
    
       0x00,     0x10,
};

const int32_t test_output_H[] = {
    
    -15,    
    -14,    
    -9,     
    -8,     
    
    -7,     
    -6,     
    
    -5,     
    -4,     
    
    -3,     
    
    -2,     
    
    -1,     
    
    0,      
    1,      
    
    2,      
    
    3,      
    
    4,      
    5,      
    18,     
    19,     
    
    20,     
    21,     
    
    22,     
    23,     
    36,     
    37,     
    
    38,     
    39,     
    68,     
    69,     
    
    70,     
    71,     
    132,    
    133,    
    
    134,    
    135,    
    260,    
    261,    
    
    262,    
    263,    
    388,    
    389,    
    
    390,    
    391,    
    644,    
    645,    
    
    646,    
    647,    
    1668,   
    1669,   
    
    -16,    
    -17,    
    
    1670,   
    1671,   
    
     
};
const byte test_input_H[] = {
    
       0xfc,     0x1f,     0x87,     0xf3,
    
       0x7e,     0x7f,     0xe3,     0xf9,
    
       0xfd,     0x7e,     0xff,     0xbf,
    
       0x28,     0x1d,     0x75,     0x02,
    
       0x0c,     0xe9,     0xfd,     0xbb,
    
       0xd8,     0x58,     0xdf,     0x5f,
    
       0xe0,     0x30,     0x39,     0xec,
    
       0xfe,     0xc0,     0xd8,     0x3b,
    
       0xfb,     0x7f,     0xf0,     0x07,
    
       0x00,     0xf3,     0xf7,     0x3f,
    
       0xf8,     0x03,     0xc0,     0x3e,
    
       0x7e,     0xf3,     0xff,     0xd0,
    
       0x0f,     0xa0,     0x3f,     0x7f,
    
       0xbe,     0xff,     0xfa,     0x00,
    
       0x7a,     0x00,     0xfb,     0xff,
    
       0x7b,     0xff,     0xff,     0x80,
    
       0x00,     0x00,     0x00,     0x3f,
    
       0xc0,     0x00,     0x00,     0x00,
    
       0x3f,     0xf0,     0x00,     0x00,
    
       0x00,     0x0f,     0xf8,     0x00,
    
       0x00,     0x00,     0x0a,
};

const int32_t test_output_I[] = {
    
    -31,    
    -30,    
    -17,    
    -16,    
    
    -15,    
    -14,    
    -13,    
    -12,    
    
    -11,    
    -10,    
    -9,     
    -8,     
    
    -7,     
    -6,     
    
    -5,     
    -4,     
    
    -3,     
    -2,     
    
    -1,     
    0,      
    
    1,      
    2,      
    
    3,      
    4,      
    
    5,      
    6,      
    
    7,      
    8,      
    37,     
    38,     
    
    39,     
    40,     
    41,     
    42,     
    
    43,     
    44,     
    73,     
    74,     
    
    75,     
    76,     
    137,    
    138,    
    
    139,    
    140,    
    265,    
    266,    
    
    267,    
    268,    
    521,    
    522,    
    
    523,    
    524,    
    777,    
    778,    
    
    779,    
    780,    
    1289,   
    1290,   
    
    1291,   
    1292,   
    3337,   
    3338,   
    
    -32,    
    -33,    
    
    3339,   
    3340,   
    
     
};
const byte test_input_I[] = {
    
       0xfc,     0x0f,     0xc1,     0xfc,
    
       0xef,     0xcf,     0xfe,     0x1f,
    
       0xc7,     0xf9,     0x7f,     0x3f,
    
       0xd3,     0xf5,     0xfd,     0xbf,
    
       0x7f,     0xeb,     0xfb,     0xf8,
    
       0xf9,     0xa5,     0x51,     0x59,
    
       0xf4,     0xd7,     0xa7,     0x58,
    
       0x08,     0x19,     0xe9,     0xfe,
    
       0xce,     0xde,     0xee,     0xfb,
    
       0x05,     0x86,     0xfd,     0x7f,
    
       0xc0,     0x30,     0x1c,     0xfb,
    
       0x3f,     0xd8,     0x0d,     0x81,
    
       0xdf,     0xed,     0xff,     0xe0,
    
       0x07,     0x00,     0x79,     0xfd,
    
       0xcf,     0xff,     0x00,     0x3c,
    
       0x01,     0xf3,     0xfb,     0xcf,
    
       0xff,     0xa0,     0x0f,     0xa0,
    
       0x1f,     0xbf,     0xef,     0xbf,
    
       0xff,     0x40,     0x07,     0xa0,
    
       0x07,     0xdf,     0xfd,     0xef,
    
       0xff,     0xff,     0x00,     0x00,
    
       0x00,     0x00,     0x7f,     0x80,
    
       0x00,     0x00,     0x00,     0x7f,
    
       0xe0,     0x00,     0x00,     0x00,
    
       0x1f,     0xf0,     0x00,     0x00,
    
       0x00,     0x10,
};

const int32_t test_output_J[] = {
    
    -21,    
    -20,    
    -7,     
    -6,     
    
    -5,     
    
    -4,     
    
    -3,     
    
    -2,     
    -1,     
    0,      
    1,      
    
    2,      
    
    3,      
    
    4,      
    
    5,      
    
    6,      
    7,      
    68,     
    69,     
    
    70,     
    71,     
    100,    
    101,    
    
    102,    
    103,    
    132,    
    133,    
    
    134,    
    135,    
    196,    
    197,    
    
    198,    
    199,    
    324,    
    325,    
    
    326,    
    327,    
    580,    
    581,    
    
    582,    
    583,    
    1092,   
    1093,   
    
    1094,   
    1095,   
    2116,   
    2117,   
    
    2118,   
    2119,   
    4164,   
    4165,   
    
    -22,    
    -23,    
    
    4166,   
    4167,   
    
     
};
const byte test_input_J[] = {
    
       0xf4,     0x1e,     0x87,     0xd7,
    
       0x7a,     0xff,     0xcf,     0x78,
    
       0x01,     0x23,     0xce,     0xdf,
    
       0x3f,     0x50,     0x10,     0x5f,
    
       0x9f,     0xf4,     0x0d,     0x07,
    
       0x5e,     0xd7,     0xf7,     0x06,
    
       0xe1,     0xdf,     0xdb,     0xff,
    
       0x80,     0x38,     0x07,     0x8f,
    
       0xb8,     0xff,     0x90,     0x1c,
    
       0x81,     0xe7,     0xf7,     0x3f,
    
       0xfa,     0x00,     0xe8,     0x07,
    
       0xaf,     0xee,     0xbf,     0xfb,
    
       0x00,     0x76,     0x01,     0xef,
    
       0xfd,     0xdf,     0xff,     0xc0,
    
       0x03,     0xc0,     0x07,     0xcf,
    
       0xfb,     0xcf,     0xff,     0xe8,
    
       0x00,     0xfa,     0x00,     0x7e,
    
       0xff,     0xef,     0xbf,     0xff,
    
       0xf8,     0x00,     0x00,     0x00,
    
       0x03,     0xf8,     0x00,     0x00,
    
       0x00,     0x07,     0xfc,     0x00,
    
       0x00,     0x00,     0x03,     0xfc,
    
       0x00,     0x00,     0x00,     0x06,
};

const int32_t test_output_K[] = {
    
    1,      
    
    2,      
    3,      
    
    4,      
    
    5,      
    6,      
    
    7,      
    8,      
    
    9,      
    10,     
    11,     
    12,     
    
    13,     
    14,     
    15,     
    16,     
    
    17,     
    18,     
    19,     
    20,     
    
    21,     
    22,     
    27,     
    28,     
    
    29,     
    30,     
    43,     
    44,     
    
    45,     
    46,     
    75,     
    76,     
    
    77,     
    78,     
    139,    
    140,    
    
    141,    
    142,    
};
const byte test_input_K[] = {
    
       0x4b,     0x9a,     0xdf,     0x1c,
    
       0xf4,     0xeb,     0xdb,     0xbf,
    
       0x87,     0x8f,     0x97,     0x9f,
    
       0xa3,     0xd3,     0xea,     0xf5,
    
       0xfb,     0x1e,     0xcf,     0xbd,
    
       0xef,     0xfc,     0x0f,     0x83,
    
       0xf3,     0xbe,     0x7f,     0xd0,
    
       0x7d,     0x0f,     0xdf,     0x7d,
    
       0xff,     0xe0,     0x3f,     0x03,
    
       0xfb,     0xef,     0xdf,     0xff,
    
       0x00,     0x00,     0x00,     0x00,
    
       0xfe,     0x00,     0x00,     0x00,
    
       0x02,
};

const int32_t test_output_L[] = {
    
    1,      
    
    2,      
    
    3,      
    4,      
    
    5,      
    
    6,      
    7,      
    
    8,      
    9,      
    
    10,     
    
    11,     
    12,     
    
    13,     
    14,     
    15,     
    16,     
    
    17,     
    18,     
    23,     
    24,     
    
    25,     
    26,     
    39,     
    40,     
    
    41,     
    42,     
    71,     
    72,     
    
    73,     
    74,     
};
const byte test_input_L[] = {
    
       0x59,     0xbc,     0xeb,     0xbf,
    
       0x1e,     0x7d,     0x7b,     0x7b,
    
       0xfc,     0x3e,     0x3f,     0x2f,
    
       0x9f,     0xd1,     0xf4,     0xfd,
    
       0xdf,     0x7f,     0xe0,     0xfc,
    
       0x3f,     0xbb,     0xf7,     0xff,
    
       0x03,     0xf8,     0x3f,     0xde,
    
       0xfe,     0xff,     0xf8,     0x00,
    
       0x00,     0x00,     0x07,     0xf8,
    
       0x00,     0x00,     0x00,     0x08,
};

const int32_t test_output_M[] = {
    
    1,      
    
    2,      
    
    3,      
    
    4,      
    
    5,      
    6,      
    
    7,      
    8,      
    13,     
    14,     
    
    15,     
    16,     
    
    17,     
    18,     
    19,     
    20,     
    
    21,     
    22,     
    27,     
    28,     
    
    29,     
    30,     
    43,     
    44,     
    
    45,     
    46,     
    75,     
    76,     
    
    77,     
    78,     
    139,    
    140,    
    
    141,    
    142,    
};
const byte test_input_M[] = {
    
       0x4c,     0xe6,     0xb7,     0x45,
    
       0x37,     0x5f,     0xd3,     0xaf,
    
       0x67,     0x6f,     0x77,     0x7f,
    
       0x83,     0xc3,     0xe6,     0xf3,
    
       0xfa,     0x1e,     0x8f,     0xbd,
    
       0xef,     0xfc,     0x0f,     0x83,
    
       0xf7,     0xbe,     0xff,     0xe0,
    
       0x3f,     0x03,     0xfb,     0xef,
    
       0xdf,     0xff,     0x00,     0x00,
    
       0x00,     0x00,     0xfe,     0x00,
    
       0x00,     0x00,     0x02,
};

const int32_t test_output_N[] = {
    
    -2,     
    
    -1,     
    
    0,      
    
    1,      
    
    2,      
};
const byte test_input_N[] = {
    
       0x95,     0xb8,
};

const int32_t test_output_O[] = {
    
    -24,    
    -23,    
    -10,    
    -9,     
    
    -8,     
    -7,     
    -6,     
    -5,     
    
    -4,     
    -3,     
    
    -2,     
    
    -1,     
    
    0,      
    
    1,      
    
    2,      
    
    3,      
    4,      
    
    5,      
    6,      
    7,      
    8,      
    
    9,      
    10,     
    23,     
    24,     
    
    -25,    
    -26,    
    
    25,     
    26,     
};
const byte test_input_O[] = {
    
       0xf8,     0x1f,     0x07,     0xe7,
    
       0x7c,     0xff,     0x0f,     0x1f,
    
       0x2f,     0x3e,     0x39,     0xc8,
    
       0xbb,     0xd7,     0x7e,     0x9e,
    
       0xbe,     0xde,     0xff,     0x43,
    
       0xe8,     0xfd,     0xef,     0xbf,
    
       0xf8,     0x00,     0x00,     0x00,
    
       0x03,     0xf0,     0x00,     0x00,
    
       0x00,     0x0f,     0xf0,     0x00,
    
       0x00,     0x00,     0x0f,     0xe0,
    
       0x00,     0x00,     0x00,     0x20,
};

typedef struct test_huffmancodes {
    const char *name;
    const Jbig2HuffmanParams *params;
    const byte *input;
    const size_t input_len;
    const int32_t *output;
    const size_t output_len;
} test_huffmancodes_t;

#define countof(x) (sizeof((x)) / sizeof((x)[0]))

#define DEF_TEST_HUFFMANCODES(x) { \
    #x, \
    &jbig2_huffman_params_##x, \
    test_input_##x, countof(test_input_##x), \
    test_output_##x, countof(test_output_##x), \
}

test_huffmancodes_t   tests[] = {
    DEF_TEST_HUFFMANCODES(A),
    DEF_TEST_HUFFMANCODES(B),
    DEF_TEST_HUFFMANCODES(C),
    DEF_TEST_HUFFMANCODES(D),
    DEF_TEST_HUFFMANCODES(E),
    DEF_TEST_HUFFMANCODES(F),
    DEF_TEST_HUFFMANCODES(G),
    DEF_TEST_HUFFMANCODES(H),
    DEF_TEST_HUFFMANCODES(I),
    DEF_TEST_HUFFMANCODES(J),
    DEF_TEST_HUFFMANCODES(K),
    DEF_TEST_HUFFMANCODES(L),
    DEF_TEST_HUFFMANCODES(M),
    DEF_TEST_HUFFMANCODES(N),
    DEF_TEST_HUFFMANCODES(O),
};

typedef struct test_stream {
    Jbig2WordStream ws;
    test_huffmancodes_t *h;
} test_stream_t;

static uint32_t
test_get_word(Jbig2WordStream *self, int offset)
{
    uint32_t val = 0;
    test_stream_t *st = (test_stream_t *)self;
    if (st != NULL) {
        if (st->h != NULL) {
            if (offset   < st->h->input_len) {
                val |= (st->h->input[offset]   << 24);
            }
            if (offset+1 < st->h->input_len) {
                val |= (st->h->input[offset+1] << 16);
            }
            if (offset+2 < st->h->input_len) {
                val |= (st->h->input[offset+2] << 8);
            }
            if (offset+3 < st->h->input_len) {
                val |=  st->h->input[offset+3];
            }
        }
    }
    return val;
}

int
main (int argc, char **argv)
{
    Jbig2Ctx *ctx = jbig2_ctx_new(NULL, 0, NULL, NULL, NULL);
    int i;

    for (i = 0; i < countof(tests); i++) {
        Jbig2HuffmanTable *table;
        Jbig2HuffmanState *hs;
        test_stream_t st;
        int32_t code;
        bool oob;
        int j;

        st.ws.get_next_word = test_get_word;
        st.h = &tests[i];
        printf("testing Standard Huffman table %s: ", st.h->name);
        table = jbig2_build_huffman_table(ctx, st.h->params);
        if (table == NULL) {
            printf("jbig2_build_huffman_table() returned NULL!\n");
        } else {
            
            hs = jbig2_huffman_new(ctx, &st.ws);
            if ( hs == NULL ) {
                printf("jbig2_huffman_new() returned NULL!\n");
            } else {
                for (j = 0; j < st.h->output_len; j++) {
                    printf("%d...", st.h->output[j]);
                    code = jbig2_huffman_get(hs, table, &oob);
                    if (code == st.h->output[j] && !oob) {
                        printf("ok, ");
                    } else {
                        int need_comma = 0;
                        printf("NG(");
                        if (code != st.h->output[j]) {
                            printf("%d", code);
                            need_comma = 1;
                        }
                        if (oob) {
                            if (need_comma)
                                printf(",");
                            printf("OOB");
                        }
                        printf("), ");
                    }
                }
                if (st.h->params->HTOOB) {
                    printf("OOB...");
                    code = jbig2_huffman_get(hs, table, &oob);
                    if (oob) {
                        printf("ok");
                    } else {
                        printf("NG(%d)", code);
                    }
                }
                printf("\n");
                jbig2_huffman_free(ctx, hs);
            }
            jbig2_release_huffman_table(ctx, table);
        }
    }
    jbig2_ctx_free(ctx);
    return 0;
}
#endif
