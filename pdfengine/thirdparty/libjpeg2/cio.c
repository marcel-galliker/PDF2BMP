

#include "opj_includes.h"

opj_cio_t* OPJ_CALLCONV opj_cio_open(opj_common_ptr cinfo, unsigned char *buffer, int length) {
	opj_cp_t *cp = NULL;
	opj_cio_t *cio = (opj_cio_t*)opj_malloc(sizeof(opj_cio_t));
	if(!cio) return NULL;
	cio->cinfo = cinfo;
	if(buffer && length) {
		
		cio->openmode = OPJ_STREAM_READ;
		cio->buffer = buffer;
		cio->length = length;
	}
	else if(!buffer && !length && cinfo) {
		
		cio->openmode = OPJ_STREAM_WRITE;
		switch(cinfo->codec_format) {
			case CODEC_J2K:
				cp = ((opj_j2k_t*)cinfo->j2k_handle)->cp;
				break;
			case CODEC_JP2:
				cp = ((opj_jp2_t*)cinfo->jp2_handle)->j2k->cp;
				break;
			default:
				opj_free(cio);
				return NULL;
		}
		cio->length = (unsigned int) (0.1625 * cp->img_size + 2000); 
		cio->buffer = (unsigned char *)opj_malloc(cio->length);
		if(!cio->buffer) {
			opj_event_msg(cio->cinfo, EVT_ERROR, "Error allocating memory for compressed bitstream\n");
			opj_free(cio);
			return NULL;
		}
	}
	else {
		opj_free(cio);
		return NULL;
	}

	
	cio->start = cio->buffer;
	cio->end = cio->buffer + cio->length;
	cio->bp = cio->buffer;

	return cio;
}

void OPJ_CALLCONV opj_cio_close(opj_cio_t *cio) {
	if(cio) {
		if(cio->openmode == OPJ_STREAM_WRITE) {
			
			opj_free(cio->buffer);
		}
		
		opj_free(cio);
	}
}

int OPJ_CALLCONV cio_tell(opj_cio_t *cio) {
	return cio->bp - cio->start;
}

void OPJ_CALLCONV cio_seek(opj_cio_t *cio, int pos) {
	cio->bp = cio->start + pos;
}

int cio_numbytesleft(opj_cio_t *cio) {
	return cio->end - cio->bp;
}

unsigned char *cio_getbp(opj_cio_t *cio) {
	return cio->bp;
}

opj_bool cio_byteout(opj_cio_t *cio, unsigned char v) {
	if (cio->bp >= cio->end) {
		opj_event_msg(cio->cinfo, EVT_ERROR, "write error\n");
		return OPJ_FALSE;
	}
	*cio->bp++ = v;
	return OPJ_TRUE;
}

unsigned char cio_bytein(opj_cio_t *cio) {
	if (cio->bp >= cio->end) {
		opj_event_msg(cio->cinfo, EVT_ERROR, "read error: passed the end of the codestream (start = %d, current = %d, end = %d\n", cio->start, cio->bp, cio->end);
		return 0;
	}
	return *cio->bp++;
}

unsigned int cio_write(opj_cio_t *cio, unsigned long long int v, int n) {
	int i;
	for (i = n - 1; i >= 0; i--) {
		if( !cio_byteout(cio, (unsigned char) ((v >> (i << 3)) & 0xff)) )
			return 0;
	}
	return n;
}

unsigned int cio_read(opj_cio_t *cio, int n) {
	int i;
	unsigned int v;
	v = 0;
	for (i = n - 1; i >= 0; i--) {
		v += cio_bytein(cio) << (i << 3);
	}
	return v;
}

void cio_skip(opj_cio_t *cio, int n) {
	cio->bp += n;
}

