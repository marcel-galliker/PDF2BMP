

#include "jinclude.h"
#include "jpeglib.h"
#include "jerror.h"

#ifndef HAVE_STDLIB_H		
extern void * malloc JPP((size_t size));
extern void free JPP((void *ptr));
#endif

typedef struct {
  struct jpeg_destination_mgr pub; 

  FILE * outfile;		
  JOCTET * buffer;		
} my_destination_mgr;

typedef my_destination_mgr * my_dest_ptr;

#define OUTPUT_BUF_SIZE  4096	

typedef struct {
  struct jpeg_destination_mgr pub; 

  unsigned char ** outbuffer;	
  unsigned long * outsize;
  unsigned char * newbuffer;	
  JOCTET * buffer;		
  size_t bufsize;
} my_mem_destination_mgr;

typedef my_mem_destination_mgr * my_mem_dest_ptr;

METHODDEF(void)
init_destination (j_compress_ptr cinfo)
{
  my_dest_ptr dest = (my_dest_ptr) cinfo->dest;

  
  dest->buffer = (JOCTET *)
      (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
				  OUTPUT_BUF_SIZE * SIZEOF(JOCTET));

  dest->pub.next_output_byte = dest->buffer;
  dest->pub.free_in_buffer = OUTPUT_BUF_SIZE;
}

METHODDEF(void)
init_mem_destination (j_compress_ptr cinfo)
{
  
}

METHODDEF(boolean)
empty_output_buffer (j_compress_ptr cinfo)
{
  my_dest_ptr dest = (my_dest_ptr) cinfo->dest;

  if (JFWRITE(dest->outfile, dest->buffer, OUTPUT_BUF_SIZE) !=
      (size_t) OUTPUT_BUF_SIZE)
    ERREXIT(cinfo, JERR_FILE_WRITE);

  dest->pub.next_output_byte = dest->buffer;
  dest->pub.free_in_buffer = OUTPUT_BUF_SIZE;

  return TRUE;
}

METHODDEF(boolean)
empty_mem_output_buffer (j_compress_ptr cinfo)
{
  size_t nextsize;
  JOCTET * nextbuffer;
  my_mem_dest_ptr dest = (my_mem_dest_ptr) cinfo->dest;

  
  nextsize = dest->bufsize * 2;
  nextbuffer = (JOCTET *) malloc(nextsize);

  if (nextbuffer == NULL)
    ERREXIT1(cinfo, JERR_OUT_OF_MEMORY, 10);

  MEMCOPY(nextbuffer, dest->buffer, dest->bufsize);

  if (dest->newbuffer != NULL)
    free(dest->newbuffer);

  dest->newbuffer = nextbuffer;

  dest->pub.next_output_byte = nextbuffer + dest->bufsize;
  dest->pub.free_in_buffer = dest->bufsize;

  dest->buffer = nextbuffer;
  dest->bufsize = nextsize;

  return TRUE;
}

METHODDEF(void)
term_destination (j_compress_ptr cinfo)
{
  my_dest_ptr dest = (my_dest_ptr) cinfo->dest;
  size_t datacount = OUTPUT_BUF_SIZE - dest->pub.free_in_buffer;

  
  if (datacount > 0) {
    if (JFWRITE(dest->outfile, dest->buffer, datacount) != datacount)
      ERREXIT(cinfo, JERR_FILE_WRITE);
  }
  fflush(dest->outfile);
  
  if (ferror(dest->outfile))
    ERREXIT(cinfo, JERR_FILE_WRITE);
}

METHODDEF(void)
term_mem_destination (j_compress_ptr cinfo)
{
  my_mem_dest_ptr dest = (my_mem_dest_ptr) cinfo->dest;

  *dest->outbuffer = dest->buffer;
  *dest->outsize = dest->bufsize - dest->pub.free_in_buffer;
}

GLOBAL(void)
jpeg_stdio_dest (j_compress_ptr cinfo, FILE * outfile)
{
  my_dest_ptr dest;

  
  if (cinfo->dest == NULL) {	
    cinfo->dest = (struct jpeg_destination_mgr *)
      (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_PERMANENT,
				  SIZEOF(my_destination_mgr));
  }

  dest = (my_dest_ptr) cinfo->dest;
  dest->pub.init_destination = init_destination;
  dest->pub.empty_output_buffer = empty_output_buffer;
  dest->pub.term_destination = term_destination;
  dest->outfile = outfile;
}

GLOBAL(void)
jpeg_mem_dest (j_compress_ptr cinfo,
	       unsigned char ** outbuffer, unsigned long * outsize)
{
  my_mem_dest_ptr dest;

  if (outbuffer == NULL || outsize == NULL)	
    ERREXIT(cinfo, JERR_BUFFER_SIZE);

  
  if (cinfo->dest == NULL) {	
    cinfo->dest = (struct jpeg_destination_mgr *)
      (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_PERMANENT,
				  SIZEOF(my_mem_destination_mgr));
  }

  dest = (my_mem_dest_ptr) cinfo->dest;
  dest->pub.init_destination = init_mem_destination;
  dest->pub.empty_output_buffer = empty_mem_output_buffer;
  dest->pub.term_destination = term_mem_destination;
  dest->outbuffer = outbuffer;
  dest->outsize = outsize;
  dest->newbuffer = NULL;

  if (*outbuffer == NULL || *outsize == 0) {
    
    dest->newbuffer = *outbuffer = (unsigned char *) malloc(OUTPUT_BUF_SIZE);
    if (dest->newbuffer == NULL)
      ERREXIT1(cinfo, JERR_OUT_OF_MEMORY, 10);
    *outsize = OUTPUT_BUF_SIZE;
  }

  dest->pub.next_output_byte = dest->buffer = *outbuffer;
  dest->pub.free_in_buffer = dest->bufsize = *outsize;
}
