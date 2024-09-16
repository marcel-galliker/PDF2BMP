

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "os_types.h"

#include <stddef.h> 

#include "jbig2.h"
#include "jbig2_priv.h"
#include "jbig2_huffman.h"
#include "jbig2_symbol_dict.h"
#include "jbig2_metadata.h"

Jbig2Segment *
jbig2_parse_segment_header (Jbig2Ctx *ctx, uint8_t *buf, size_t buf_size,
			    size_t *p_header_size)
{
  Jbig2Segment *result;
  uint8_t rtscarf;
  uint32_t rtscarf_long;
  uint32_t *referred_to_segments;
  int referred_to_segment_count;
  int referred_to_segment_size;
  int pa_size;
  int offset;

  
  if (buf_size < 11)
    return NULL;

  result = jbig2_new(ctx, Jbig2Segment, 1);
  if (result == NULL)
  {
      jbig2_error(ctx, JBIG2_SEVERITY_FATAL, -1,
          "failed to allocate segment in jbig2_parse_segment_header");
      return result;
  }

  
  result->number = jbig2_get_uint32(buf);

  
  result->flags = buf[4];

  
  rtscarf = buf[5];
  if ((rtscarf & 0xe0) == 0xe0)
    {
      rtscarf_long = jbig2_get_uint32(buf + 5);
      referred_to_segment_count = rtscarf_long & 0x1fffffff;
      offset = 5 + 4 + (referred_to_segment_count + 1) / 8;
    }
  else
    {
      referred_to_segment_count = (rtscarf >> 5);
      offset = 5 + 1;
    }
  result->referred_to_segment_count = referred_to_segment_count;

  
  referred_to_segment_size = result->number <= 256 ? 1:
        result->number <= 65536 ? 2 : 4;  
  pa_size = result->flags & 0x40 ? 4 : 1; 
  if (offset + referred_to_segment_count*referred_to_segment_size + pa_size + 4 > (int)buf_size)
    {
      jbig2_error(ctx, JBIG2_SEVERITY_DEBUG, result->number,
        "jbig2_parse_segment_header() called with insufficient data", -1);
      jbig2_free (ctx->allocator, result);
      return NULL;
    }

  
  if (referred_to_segment_count)
    {
      int i;

      referred_to_segments = jbig2_new(ctx, uint32_t,
        referred_to_segment_count * referred_to_segment_size);
      if (referred_to_segments == NULL)
      {
          jbig2_error(ctx, JBIG2_SEVERITY_FATAL, -1,
              "could not allocate referred_to_segments "
              "in jbig2_parse_segment_header");
          return NULL;
      }

      for (i = 0; i < referred_to_segment_count; i++) {
        referred_to_segments[i] =
          (referred_to_segment_size == 1) ? buf[offset] :
          (referred_to_segment_size == 2) ? jbig2_get_uint16(buf+offset) :
            jbig2_get_uint32(buf + offset);
        offset += referred_to_segment_size;
        jbig2_error(ctx, JBIG2_SEVERITY_DEBUG, result->number,
            "segment %d refers to segment %d",
            result->number, referred_to_segments[i]);
      }
      result->referred_to_segments = referred_to_segments;
    }
  else 
    {
      result->referred_to_segments = NULL;
    }

  
  if (result->flags & 0x40) {
	result->page_association = jbig2_get_uint32(buf + offset);
	offset += 4;
  } else {
	result->page_association = buf[offset++];
  }
  jbig2_error(ctx, JBIG2_SEVERITY_DEBUG, result->number,
  	"segment %d is associated with page %d",
  	result->number, result->page_association);

  
  result->data_length = jbig2_get_uint32(buf + offset);
  *p_header_size = offset + 4;

  
  result->result = NULL;

  return result;
}

void
jbig2_free_segment (Jbig2Ctx *ctx, Jbig2Segment *segment)
{
  if (segment->referred_to_segments != NULL) {
    jbig2_free(ctx->allocator, segment->referred_to_segments);
  }
  
    switch (segment->flags & 63) {
	case 0:  
	  if (segment->result != NULL)
	    jbig2_sd_release(ctx, (Jbig2SymbolDict*)segment->result);
	  break;
	case 4:  
	case 40: 
	  if (segment->result != NULL)
	    jbig2_image_release(ctx, (Jbig2Image*)segment->result);
	  break;
	case 53: 
	  if (segment->result != NULL)
		jbig2_table_free(ctx, (Jbig2HuffmanParams*)segment->result);
	  break;
	case 62:
	  if (segment->result != NULL)
	    jbig2_metadata_free(ctx, (Jbig2Metadata*)segment->result);
	  break;
	default:
	  
	  break;
    }
  jbig2_free (ctx->allocator, segment);
}

Jbig2Segment *
jbig2_find_segment(Jbig2Ctx *ctx, uint32_t number)
{
    int index, index_max = ctx->segment_index - 1;
    const Jbig2Ctx *global_ctx = ctx->global_ctx;

    
    for (index = index_max; index >= 0; index--)
        if (ctx->segments[index]->number == number)
            return (ctx->segments[index]);

    if (global_ctx)
	for (index = global_ctx->segment_index - 1; index >= 0; index--)
	    if (global_ctx->segments[index]->number == number)
		return (global_ctx->segments[index]);

    
    return NULL;
}

void
jbig2_get_region_segment_info(Jbig2RegionSegmentInfo *info,
			      const uint8_t *segment_data)
{
  
  info->width = jbig2_get_int32(segment_data);
  info->height = jbig2_get_int32(segment_data + 4);
  info->x = jbig2_get_int32(segment_data + 8);
  info->y = jbig2_get_int32(segment_data + 12);
  info->flags = segment_data[16];
  info->op = (Jbig2ComposeOp)(info->flags & 0x7);
}

int jbig2_parse_extension_segment(Jbig2Ctx *ctx, Jbig2Segment *segment,
                            const uint8_t *segment_data)
{
    uint32_t type = jbig2_get_uint32(segment_data);
    bool reserved = type & 0x20000000;
    
    bool necessary = type & 0x80000000;

    if (necessary && !reserved) {
        jbig2_error(ctx, JBIG2_SEVERITY_WARNING, segment->number,
            "extension segment is marked 'necessary' but "
            "not 'reservered' contrary to spec");
    }

    switch (type) {
        case 0x20000000: return jbig2_comment_ascii(ctx, segment, segment_data);
        case 0x20000002: return jbig2_comment_unicode(ctx, segment, segment_data);
        default:
            if (necessary) {
                return jbig2_error(ctx, JBIG2_SEVERITY_FATAL, segment->number,
                    "unhandled necessary extension segment type 0x%08x", type);
            } else {
                return jbig2_error(ctx, JBIG2_SEVERITY_WARNING, segment->number,
                    "unhandled extension segment");
            }
    }

    return 0;
}

int jbig2_parse_segment (Jbig2Ctx *ctx, Jbig2Segment *segment,
			 const uint8_t *segment_data)
{
  jbig2_error(ctx, JBIG2_SEVERITY_INFO, segment->number,
	      "Segment %d, flags=%x, type=%d, data_length=%d",
	      segment->number, segment->flags, segment->flags & 63,
	      segment->data_length);
  switch (segment->flags & 63)
    {
    case 0:
      return jbig2_symbol_dictionary(ctx, segment, segment_data);
    case 4: 
    case 6: 
    case 7: 
      return jbig2_text_region(ctx, segment, segment_data);
    case 16:
      return jbig2_pattern_dictionary(ctx, segment, segment_data);
    case 20: 
    case 22: 
    case 23: 
      return jbig2_halftone_region(ctx, segment, segment_data);
    case 36:
      return jbig2_error(ctx, JBIG2_SEVERITY_WARNING, segment->number,
        "unhandled segment type 'intermediate generic region'");
    case 38: 
    case 39: 
      return jbig2_immediate_generic_region(ctx, segment, segment_data);
    case 40: 
    case 42: 
    case 43: 
      return jbig2_refinement_region(ctx, segment, segment_data);
    case 48:
      return jbig2_page_info(ctx, segment, segment_data);
    case 49:
      return jbig2_end_of_page(ctx, segment, segment_data);
    case 50:
      return jbig2_end_of_stripe(ctx, segment, segment_data);
    case 51:
      ctx->state = JBIG2_FILE_EOF;
      return jbig2_error(ctx, JBIG2_SEVERITY_INFO, segment->number,
        "end of file");
    case 52:
      return jbig2_error(ctx, JBIG2_SEVERITY_WARNING, segment->number,
        "unhandled segment type 'profile'");
    case 53: 
      return jbig2_table(ctx, segment, segment_data);
    case 62:
      return jbig2_parse_extension_segment(ctx, segment, segment_data);
    default:
        jbig2_error(ctx, JBIG2_SEVERITY_WARNING, segment->number,
          "unknown segment type %d", segment->flags & 63);
    }
  return 0;
}
