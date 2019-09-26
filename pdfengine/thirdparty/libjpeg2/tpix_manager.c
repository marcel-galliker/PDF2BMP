

#include <math.h>
#include "opj_includes.h"

#define MAX(a,b) ((a)>(b)?(a):(b))

int write_tpixfaix( int coff, int compno, opj_codestream_info_t cstr_info, int j2klen, opj_cio_t *cio);

int write_tpix( int coff, opj_codestream_info_t cstr_info, int j2klen, opj_cio_t *cio)
{
  int len, lenp;
  lenp = cio_tell( cio);
  cio_skip( cio, 4);              
  cio_write( cio, JPIP_TPIX, 4);  
  
  write_tpixfaix( coff, 0, cstr_info, j2klen, cio);

  len = cio_tell( cio)-lenp;
  cio_seek( cio, lenp);
  cio_write( cio, len, 4);        
  cio_seek( cio, lenp+len);

  return len;
}

int get_num_max_tile_parts( opj_codestream_info_t cstr_info);

int write_tpixfaix( int coff, int compno, opj_codestream_info_t cstr_info, int j2klen, opj_cio_t *cio)
{
  int len, lenp;
  int i, j;
  int Aux;
  int num_max_tile_parts;
  int size_of_coding; 
  opj_tp_info_t tp;
  int version;

  num_max_tile_parts = get_num_max_tile_parts( cstr_info);

  if( j2klen > pow( 2, 32)){
    size_of_coding =  8;
    version = num_max_tile_parts == 1 ? 1:3;
  }
  else{
    size_of_coding = 4;
    version = num_max_tile_parts == 1 ? 0:2;
  }

  lenp = cio_tell( cio);
  cio_skip( cio, 4);              
  cio_write( cio, JPIP_FAIX, 4);   
  cio_write( cio, version, 1);     

  cio_write( cio, num_max_tile_parts, size_of_coding);                      
  cio_write( cio, cstr_info.tw*cstr_info.th, size_of_coding);                               
  for (i = 0; i < cstr_info.tw*cstr_info.th; i++){
    for (j = 0; j < cstr_info.tile[i].num_tps; j++){
      tp = cstr_info.tile[i].tp[j];
      cio_write( cio, tp.tp_start_pos-coff, size_of_coding); 
      cio_write( cio, tp.tp_end_pos-tp.tp_start_pos+1, size_of_coding);    
      if (version & 0x02){
	if( cstr_info.tile[i].num_tps == 1 && cstr_info.numdecompos[compno] > 1)
	  Aux = cstr_info.numdecompos[compno] + 1;
	else
	  Aux = j + 1;
		  
	cio_write( cio, Aux,4);
	 
	
      }
      
    }
    
    while (j < num_max_tile_parts){
      cio_write( cio, 0, size_of_coding); 
      cio_write( cio, 0, size_of_coding); 
      if (version & 0x02)
	cio_write( cio, 0,4);                  
      j++;
    }
  }
  
  len = cio_tell( cio)-lenp;
  cio_seek( cio, lenp);
  cio_write( cio, len, 4);        
  cio_seek( cio, lenp+len);

  return len;

}

int get_num_max_tile_parts( opj_codestream_info_t cstr_info)
{
  int num_max_tp = 0, i;

  for( i=0; i<cstr_info.tw*cstr_info.th; i++)
    num_max_tp = MAX( cstr_info.tile[i].num_tps, num_max_tp);
  
  return num_max_tp;
}
