

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "opj_includes.h"

int write_tilemhix( int coff, opj_codestream_info_t cstr_info, int tileno, opj_cio_t *cio);

int write_thix( int coff, opj_codestream_info_t cstr_info, opj_cio_t *cio)
{
  int len, lenp, i;
  int tileno;
  opj_jp2_box_t *box;

  lenp = 0;
  box = (opj_jp2_box_t *)opj_calloc( cstr_info.tw*cstr_info.th, sizeof(opj_jp2_box_t));

  for ( i = 0; i < 2 ; i++ ){
    if (i)
      cio_seek( cio, lenp);

    lenp = cio_tell( cio);
    cio_skip( cio, 4);              
    cio_write( cio, JPIP_THIX, 4);  
    write_manf( i, cstr_info.tw*cstr_info.th, box, cio);
    
    for (tileno = 0; tileno < cstr_info.tw*cstr_info.th; tileno++){
      box[tileno].length = write_tilemhix( coff, cstr_info, tileno, cio);
      box[tileno].type = JPIP_MHIX;
    }
 
    len = cio_tell( cio)-lenp;
    cio_seek( cio, lenp);
    cio_write( cio, len, 4);        
    cio_seek( cio, lenp+len);
  }

  opj_free(box);

  return len;
}

int write_tilemhix( int coff, opj_codestream_info_t cstr_info, int tileno, opj_cio_t *cio)
{
  int i;
  opj_tile_info_t tile;
  opj_tp_info_t tp;
  int len, lenp;
  opj_marker_info_t *marker;

  lenp = cio_tell( cio);
  cio_skip( cio, 4);                               
  cio_write( cio, JPIP_MHIX, 4);                   

  tile = cstr_info.tile[tileno];
  tp = tile.tp[0];

  cio_write( cio, tp.tp_end_header-tp.tp_start_pos+1, 8);   

  marker = cstr_info.tile[tileno].marker;

  for( i=0; i<cstr_info.tile[tileno].marknum; i++){             
    cio_write( cio, marker[i].type, 2);
    cio_write( cio, 0, 2);
    cio_write( cio, marker[i].pos-coff, 8);
    cio_write( cio, marker[i].len, 2);
  }
     
  

  len = cio_tell( cio) - lenp;
  cio_seek( cio, lenp);
  cio_write( cio, len, 4);        
  cio_seek( cio, lenp+len);

  return len;
}
