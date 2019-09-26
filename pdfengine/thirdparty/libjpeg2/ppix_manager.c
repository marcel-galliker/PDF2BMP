

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "opj_includes.h"

int write_ppixfaix( int coff, int compno, opj_codestream_info_t cstr_info, opj_bool EPHused, int j2klen, opj_cio_t *cio);

int write_ppix( int coff, opj_codestream_info_t cstr_info, opj_bool EPHused, int j2klen, opj_cio_t *cio)
{
  int len, lenp, compno, i;
  opj_jp2_box_t *box;

  

  lenp = -1;
  box = (opj_jp2_box_t *)opj_calloc( cstr_info.numcomps, sizeof(opj_jp2_box_t));
  
  for (i=0;i<2;i++){
    if (i) cio_seek( cio, lenp);
    
    lenp = cio_tell( cio);
    cio_skip( cio, 4);              
    cio_write( cio, JPIP_PPIX, 4);  

    write_manf( i, cstr_info.numcomps, box, cio);
    
    for (compno=0; compno<cstr_info.numcomps; compno++){
      box[compno].length = write_ppixfaix( coff, compno, cstr_info, EPHused, j2klen, cio);
      box[compno].type = JPIP_FAIX;
    }
   
    len = cio_tell( cio)-lenp;
    cio_seek( cio, lenp);
    cio_write( cio, len, 4);        
    cio_seek( cio, lenp+len);
  }
  
  opj_free(box);

  return len;
}

int write_ppixfaix( int coff, int compno, opj_codestream_info_t cstr_info, opj_bool EPHused, int j2klen, opj_cio_t *cio)
{
  int len, lenp, tileno, version, i, nmax, size_of_coding; 
  opj_tile_info_t *tile_Idx;
  opj_packet_info_t packet;
  int resno, precno, layno, num_packet;
  int numOfres, numOfprec, numOflayers;
  packet.end_pos = packet.end_ph_pos = packet.start_pos = -1;
  (void)EPHused; 

  if( j2klen > pow( 2, 32)){
    size_of_coding =  8;
    version = 1;
  }
  else{
    size_of_coding = 4;
    version = 0;
  }
  
  lenp = cio_tell( cio);
  cio_skip( cio, 4);              
  cio_write( cio, JPIP_FAIX, 4);   
  cio_write( cio, version, 1);     

  nmax = 0;
  for( i=0; i<=cstr_info.numdecompos[compno]; i++)
    nmax += cstr_info.tile[0].ph[i] * cstr_info.tile[0].pw[i] * cstr_info.numlayers;
  
  cio_write( cio, nmax, size_of_coding); 
  cio_write( cio, cstr_info.tw*cstr_info.th, size_of_coding);      

  for( tileno=0; tileno<cstr_info.tw*cstr_info.th; tileno++){
    tile_Idx = &cstr_info.tile[ tileno];
 
    num_packet=0;
    numOfres = cstr_info.numdecompos[compno] + 1;
  
    for( resno=0; resno<numOfres ; resno++){
      numOfprec = tile_Idx->pw[resno]*tile_Idx->ph[resno];
      for( precno=0; precno<numOfprec; precno++){
	numOflayers = cstr_info.numlayers;
	for( layno=0; layno<numOflayers; layno++){

	  switch ( cstr_info.prog){
	  case LRCP:
	    packet = tile_Idx->packet[ ((layno*numOfres+resno)*cstr_info.numcomps+compno)*numOfprec+precno];
	    break;
	  case RLCP:
	    packet = tile_Idx->packet[ ((resno*numOflayers+layno)*cstr_info.numcomps+compno)*numOfprec+precno];
	    break;
	  case RPCL:
	    packet = tile_Idx->packet[ ((resno*numOfprec+precno)*cstr_info.numcomps+compno)*numOflayers+layno];
	    break;
	  case PCRL:
	    packet = tile_Idx->packet[ ((precno*cstr_info.numcomps+compno)*numOfres+resno)*numOflayers + layno];
	    break;
	  case CPRL:
	    packet = tile_Idx->packet[ ((compno*numOfprec+precno)*numOfres+resno)*numOflayers + layno];
	    break;
	  default:
	    fprintf( stderr, "failed to ppix indexing\n");
	  }

	  cio_write( cio, packet.start_pos-coff, size_of_coding);             
	  cio_write( cio, packet.end_pos-packet.start_pos+1, size_of_coding); 
	  
	  num_packet++;
	}
      }
    }
  
    while( num_packet < nmax){     
      cio_write( cio, 0, size_of_coding); 
      cio_write( cio, 0, size_of_coding); 
      num_packet++;
    }   
  }

  len = cio_tell( cio)-lenp;
  cio_seek( cio, lenp);
  cio_write( cio, len, 4);        
  cio_seek( cio, lenp+len);

  return len;
}
