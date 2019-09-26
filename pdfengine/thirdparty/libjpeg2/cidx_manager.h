

#ifndef  CIDX_MANAGER_H_
# define CIDX_MANAGER_H_

#include "openjpeg.h"

int write_cidx( int offset, opj_cio_t *cio, opj_image_t *image, opj_codestream_info_t cstr_info, int j2klen);

#endif      
