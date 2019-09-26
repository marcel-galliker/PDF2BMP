

#ifndef __CIO_H
#define __CIO_H

int cio_numbytesleft(opj_cio_t *cio);

unsigned char *cio_getbp(opj_cio_t *cio);

unsigned int cio_write(opj_cio_t *cio, unsigned long long int v, int n);

unsigned int cio_read(opj_cio_t *cio, int n);

void cio_skip(opj_cio_t *cio, int n);

#endif 

