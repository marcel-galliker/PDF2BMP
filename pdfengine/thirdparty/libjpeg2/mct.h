

#ifndef __MCT_H
#define __MCT_H

void mct_encode(int *c0, int *c1, int *c2, int n);

void mct_decode(int *c0, int *c1, int *c2, int n);

double mct_getnorm(int compno);

void mct_encode_real(int *c0, int *c1, int *c2, int n);

void mct_decode_real(float* c0, float* c1, float* c2, int n);

double mct_getnorm_real(int compno);

#endif 
