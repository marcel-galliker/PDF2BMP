

#ifndef  INDEXBOX_MANAGER_H_
# define INDEXBOX_MANAGER_H_

#include "openjpeg.h"
#include "j2k.h" 
#include "jp2.h"

#define JPIP_CIDX 0x63696478   
#define JPIP_CPTR 0x63707472   
#define JPIP_MANF 0x6d616e66   
#define JPIP_FAIX 0x66616978   
#define JPIP_MHIX 0x6d686978   
#define JPIP_TPIX 0x74706978   
#define JPIP_THIX 0x74686978   
#define JPIP_PPIX 0x70706978   
#define JPIP_PHIX 0x70686978   
#define JPIP_FIDX 0x66696478   
#define JPIP_FPTR 0x66707472   
#define JPIP_PRXY 0x70727879   
#define JPIP_IPTR 0x69707472   
#define JPIP_PHLD 0x70686c64   

int write_tpix( int coff, opj_codestream_info_t cstr_info, int j2klen, opj_cio_t *cio);

int write_thix( int coff, opj_codestream_info_t cstr_info, opj_cio_t *cio);

int write_ppix( int coff, opj_codestream_info_t cstr_info, opj_bool EPHused, int j2klen, opj_cio_t *cio);

int write_phix( int coff, opj_codestream_info_t cstr_info, opj_bool EPHused, int j2klen, opj_cio_t *cio);

void write_manf(int second, int v, opj_jp2_box_t *box, opj_cio_t *cio);

#endif      
