

#include <stdio.h>
#include <stdlib.h>
#include "opj_includes.h"

void write_cptr(int coff, int clen, opj_cio_t *cio);

int write_mainmhix( int coff, opj_codestream_info_t cstr_info, opj_cio_t *cio);

opj_bool check_EPHuse( int coff, opj_marker_info_t *markers, int marknum, opj_cio_t *cio);

int write_cidx( int offset, opj_cio_t *cio, opj_image_t *image, opj_codestream_info_t cstr_info, int j2klen)
{
  int len, i, lenp;
  opj_jp2_box_t *box;
  int num_box = 0;
  opj_bool  EPHused;
  (void)image; 

  lenp = -1;
  box = (opj_jp2_box_t *)opj_calloc( 32, sizeof(opj_jp2_box_t));

  for (i=0;i<2;i++){
  
    if(i)
      cio_seek( cio, lenp);

    lenp = cio_tell( cio);

    cio_skip( cio, 4);              
    cio_write( cio, JPIP_CIDX, 4);  
    write_cptr( offset, cstr_info.codestream_size, cio);

    write_manf( i, num_box, box, cio);
    
    num_box = 0;
    box[num_box].length = write_mainmhix( offset, cstr_info, cio);
    box[num_box].type = JPIP_MHIX;
    num_box++;

    box[num_box].length = write_tpix( offset, cstr_info, j2klen, cio);
    box[num_box].type = JPIP_TPIX;
    num_box++;
      
    box[num_box].length = write_thix( offset, cstr_info, cio);
    box[num_box].type = JPIP_THIX;
    num_box++;

    EPHused = check_EPHuse( offset, cstr_info.marker, cstr_info.marknum, cio);
      
    box[num_box].length = write_ppix( offset, cstr_info, EPHused, j2klen, cio);
    box[num_box].type = JPIP_PPIX;
    num_box++;
    
    box[num_box].length = write_phix( offset, cstr_info, EPHused, j2klen, cio);
    box[num_box].type = JPIP_PHIX;
    num_box++;
      
    len = cio_tell( cio)-lenp;
    cio_seek( cio, lenp);
    cio_write( cio, len, 4);        
    cio_seek( cio, lenp+len);
  }

  opj_free( box);
  
  return len;
}

void write_cptr(int coff, int clen, opj_cio_t *cio)
{
  int len, lenp;

  lenp = cio_tell( cio);
  cio_skip( cio, 4);               
  cio_write( cio, JPIP_CPTR, 4);   
  cio_write( cio, 0, 2);           
  cio_write( cio, 0, 2);           
  cio_write( cio, coff, 8);    
  cio_write( cio, clen, 8);    
  len = cio_tell( cio) - lenp;
  cio_seek( cio, lenp);
  cio_write( cio, len, 4);         
  cio_seek( cio, lenp+len);
}

void write_manf(int second, int v, opj_jp2_box_t *box, opj_cio_t *cio)
{
  int len, lenp, i;
  
  lenp = cio_tell( cio); 
  cio_skip( cio, 4);                         
  cio_write( cio, JPIP_MANF,4);              

  if (second){                          
    for( i=0; i<v; i++){
      cio_write( cio, box[i].length, 4);   
      cio_write( cio, box[i].type, 4); 
    }
  }

  len = cio_tell( cio) - lenp;
  cio_seek( cio, lenp);
  cio_write( cio, len, 4);                   
  cio_seek( cio, lenp+len);
}

int write_mainmhix( int coff, opj_codestream_info_t cstr_info, opj_cio_t *cio)
{
  int i;
  int len, lenp;
  
  lenp = cio_tell( cio);
  cio_skip( cio, 4);                               
  cio_write( cio, JPIP_MHIX, 4);                   

  cio_write( cio, cstr_info.main_head_end-cstr_info.main_head_start+1, 8);        

  for(i = 1; i < cstr_info.marknum; i++){    
    cio_write( cio, cstr_info.marker[i].type, 2);
    cio_write( cio, 0, 2);
    cio_write( cio, cstr_info.marker[i].pos-coff, 8);
    cio_write( cio, cstr_info.marker[i].len, 2);
  }

  len = cio_tell( cio) - lenp;
  cio_seek( cio, lenp);
  cio_write( cio, len, 4);        
  cio_seek( cio, lenp+len);
  
  return len;
}

opj_bool check_EPHuse( int coff, opj_marker_info_t *markers, int marknum, opj_cio_t *cio)
{
  opj_bool EPHused = OPJ_FALSE;
  int i=0;
  int org_pos;
  unsigned int Scod;

  for(i = 0; i < marknum; i++){
    if( markers[i].type == J2K_MS_COD){
      org_pos = cio_tell( cio);
      cio_seek( cio, coff+markers[i].pos+2);
      
      Scod = cio_read( cio, 1);
      if( ((Scod >> 2) & 1))
	EPHused = OPJ_TRUE;
      cio_seek( cio, org_pos);

      break;
    }
  }    
  return EPHused;
}
