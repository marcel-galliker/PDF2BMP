

#ifndef __DWT_H
#define __DWT_H

void dwt_encode(opj_tcd_tilecomp_t * tilec);

void dwt_decode(opj_tcd_tilecomp_t* tilec, int numres);

int dwt_getgain(int orient);

double dwt_getnorm(int level, int orient);

void dwt_encode_real(opj_tcd_tilecomp_t * tilec);

void dwt_decode_real(opj_tcd_tilecomp_t* tilec, int numres);

int dwt_getgain_real(int orient);

double dwt_getnorm_real(int level, int orient);

void dwt_calc_explicit_stepsizes(opj_tccp_t * tccp, int prec);

#endif 
