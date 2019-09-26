

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "os_types.h"

#include <stdlib.h>

#include "jbig2.h"
#include "jbig2_priv.h"

#ifdef OUTPUT_PBM
#include <stdio.h>
#include "jbig2_image.h"
#endif

static void
dump_page_info(Jbig2Ctx *ctx, Jbig2Segment *segment, Jbig2Page *page)
{
    if (page->x_resolution == 0) {
        jbig2_error(ctx, JBIG2_SEVERITY_INFO, segment->number,
            "page %d image is %dx%d (unknown res)", page->number,
            page->width, page->height);
    } else if (page->x_resolution == page->y_resolution) {
        jbig2_error(ctx, JBIG2_SEVERITY_INFO, segment->number,
            "page %d image is %dx%d (%d ppm)", page->number,
            page->width, page->height,
            page->x_resolution);
    } else {
        jbig2_error(ctx, JBIG2_SEVERITY_INFO, segment->number,
            "page %d image is %dx%d (%dx%d ppm)", page->number,
            page->width, page->height,
            page->x_resolution, page->y_resolution);
    }
    if (page->striped) {
        jbig2_error(ctx, JBIG2_SEVERITY_INFO, segment->number,
            "\tmaximum stripe size: %d", page->stripe_size);
    }
}

int
jbig2_page_info (Jbig2Ctx *ctx, Jbig2Segment *segment, const uint8_t *segment_data)
{
    Jbig2Page *page;

    
    page = &(ctx->pages[ctx->current_page]);
    if ((page->number != 0) &&
            ((page->state == JBIG2_PAGE_NEW) || (page->state == JBIG2_PAGE_FREE))) {
        page->state = JBIG2_PAGE_COMPLETE;
        jbig2_error(ctx, JBIG2_SEVERITY_WARNING, segment->number,
            "unexpected page info segment, marking previous page finished");
    }

    
    {
        int index, j;
        index = ctx->current_page;
        while (ctx->pages[index].state != JBIG2_PAGE_FREE) {
            index++;
            if (index >= ctx->max_page_index) {
                
		ctx->pages = jbig2_renew(ctx, ctx->pages, Jbig2Page,
                    (ctx->max_page_index <<= 2));
                for (j=index; j < ctx->max_page_index; j++) {
                    ctx->pages[j].state = JBIG2_PAGE_FREE;
                    ctx->pages[j].number = 0;
                    ctx->pages[j].image = NULL;

                }
            }
        }
        page = &(ctx->pages[index]);
        ctx->current_page = index;
        page->state = JBIG2_PAGE_NEW;
        page->number = segment->page_association;
    }

    
    if (segment->data_length < 19) {
        return jbig2_error(ctx, JBIG2_SEVERITY_FATAL, segment->number,
            "segment too short");
    }

    
    page->width = jbig2_get_uint32(segment_data);
    page->height = jbig2_get_uint32(segment_data + 4);

    page->x_resolution = jbig2_get_uint32(segment_data + 8);
    page->y_resolution = jbig2_get_uint32(segment_data + 12);
    page->flags = segment_data[16];

    
    {
        int16_t striping = jbig2_get_int16(segment_data +17);
        if (striping & 0x8000) {
            page->striped = TRUE;
            page->stripe_size = striping & 0x7FFF;
        } else {
            page->striped = FALSE;
            page->stripe_size = 0;	
        }
    }
    if (page->height == 0xFFFFFFFF && page->striped == FALSE) {
        jbig2_error(ctx, JBIG2_SEVERITY_WARNING, segment->number,
            "height is unspecified but page is not markes as striped");
        page->striped = TRUE;
    }
    page->end_row = 0;

    if (segment->data_length > 19) {
        jbig2_error(ctx, JBIG2_SEVERITY_WARNING, segment->number,
            "extra data in segment");
    }

    dump_page_info(ctx, segment, page);

    
    
    if (page->height == 0xFFFFFFFF) {
        page->image = jbig2_image_new(ctx, page->width, page->stripe_size);
    } else {
        page->image = jbig2_image_new(ctx, page->width, page->height);
    }
    if (page->image == NULL) {
        return jbig2_error(ctx, JBIG2_SEVERITY_WARNING, segment->number,
            "failed to allocate buffer for page image");
    } else {
	
	jbig2_image_clear(ctx, page->image, (page->flags & 4));
        jbig2_error(ctx, JBIG2_SEVERITY_DEBUG, segment->number,
            "allocated %dx%d page image (%d bytes)",
            page->image->width, page->image->height,
            page->image->stride*page->image->height);
    }

    return 0;
}

int
jbig2_end_of_stripe(Jbig2Ctx *ctx, Jbig2Segment *segment, const uint8_t *segment_data)
{
    Jbig2Page page = ctx->pages[ctx->current_page];
    int end_row;

    end_row = jbig2_get_int32(segment_data);
    if (end_row < page.end_row) {
	jbig2_error(ctx, JBIG2_SEVERITY_WARNING, segment->number,
	    "end of stripe segment with non-positive end row advance"
	    " (new end row %d vs current end row %d)",
	    end_row, page.end_row);
    } else {
	jbig2_error(ctx, JBIG2_SEVERITY_INFO, segment->number,
	    "end of stripe: advancing end row to %d", end_row);
    }

    page.end_row = end_row;

    return 0;
}

int
jbig2_complete_page (Jbig2Ctx *ctx)
{
    int code = 0;

    
    if (ctx->segment_index != ctx->n_segments) {
      Jbig2Segment *segment = ctx->segments[ctx->segment_index];
      
      if ((segment->data_length & 0xffffffff) == 0xffffffff) {
        jbig2_error(ctx, JBIG2_SEVERITY_WARNING, segment->number,
          "File has an invalid segment data length!"
          " Trying to decode using the available data.");
        segment->data_length = ctx->buf_wr_ix - ctx->buf_rd_ix;
        code = jbig2_parse_segment(ctx, segment, ctx->buf + ctx->buf_rd_ix);
        ctx->buf_rd_ix += segment->data_length;
        ctx->segment_index++;
      }
    }

    
    if (ctx->pages[ctx->current_page].image != NULL)
    {
        ctx->pages[ctx->current_page].state = JBIG2_PAGE_COMPLETE;
    }

    return code;
}

int
jbig2_end_of_page(Jbig2Ctx *ctx, Jbig2Segment *segment, const uint8_t *segment_data)
{
    uint32_t page_number = ctx->pages[ctx->current_page].number;

    if (segment->page_association != page_number) {
        jbig2_error(ctx, JBIG2_SEVERITY_WARNING, segment->number,
            "end of page marker for page %d doesn't match current page number %d",
            segment->page_association, page_number);
    }

    jbig2_error(ctx, JBIG2_SEVERITY_INFO, segment->number,
        "end of page %d", page_number);

    jbig2_complete_page(ctx);

#ifdef OUTPUT_PBM
    jbig2_image_write_pbm(ctx->pages[ctx->current_page].image, stdout);
#endif

    return 0;
}

int
jbig2_page_add_result(Jbig2Ctx *ctx, Jbig2Page *page, Jbig2Image *image,
		      int x, int y, Jbig2ComposeOp op)
{
    
    if (page->striped) {
	int new_height = y + image->height + page->end_row;
	if (page->image->height < new_height) {
	    jbig2_error(ctx, JBIG2_SEVERITY_DEBUG, -1,
		"growing page buffer to %d rows "
		"to accomodate new stripe", new_height);
	    jbig2_image_resize(ctx, page->image,
		page->image->width, new_height);
	}
    }

    jbig2_image_compose(ctx, page->image, image,
                        x, y + page->end_row, JBIG2_COMPOSE_OR);

    return 0;
}

Jbig2Image *jbig2_page_out(Jbig2Ctx *ctx)
{
    int index;

    
    for (index=0; index < ctx->max_page_index; index++) {
        if (ctx->pages[index].state == JBIG2_PAGE_COMPLETE) {
            Jbig2Image *img = ctx->pages[index].image;
            uint32_t page_number = ctx->pages[index].number;
            ctx->pages[index].state = JBIG2_PAGE_RETURNED;
            if (img != NULL) {
                jbig2_error(ctx, JBIG2_SEVERITY_DEBUG, -1,
                            "page %d returned to the client", page_number);
                return jbig2_image_clone(ctx, img);
            } else {
                jbig2_error(ctx, JBIG2_SEVERITY_WARNING, -1,
                            "page %d returned with no associated image",
                            page_number);
                ; 
            }
        }
    }

    
    return NULL;
}

int jbig2_release_page(Jbig2Ctx *ctx, Jbig2Image *image)
{
    int index;

    
    for (index = 0; index < ctx->max_page_index; index++) {
        if (ctx->pages[index].image == image) {
            jbig2_image_release(ctx, image);
            ctx->pages[index].state = JBIG2_PAGE_RELEASED;
            jbig2_error(ctx, JBIG2_SEVERITY_DEBUG, -1,
                "page %d released by the client", ctx->pages[index].number);
            return 0;
        }
    }

    
    jbig2_error(ctx, JBIG2_SEVERITY_WARNING, -1,
        "jbig2_release_page called on unknown page");
    return 1;
}
