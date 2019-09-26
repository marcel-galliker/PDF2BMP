

#ifndef _TIFFPREDICT_
#define	_TIFFPREDICT_

typedef struct {
	int		predictor;	
	int		stride;		
	tsize_t		rowsize;	

 	TIFFCodeMethod  encoderow;	
 	TIFFCodeMethod  encodestrip;	
 	TIFFCodeMethod  encodetile;	 
 	TIFFPostMethod  encodepfunc;	
 
 	TIFFCodeMethod  decoderow;	
 	TIFFCodeMethod  decodestrip;	
 	TIFFCodeMethod  decodetile;	 
 	TIFFPostMethod  decodepfunc;	

	TIFFVGetMethod	vgetparent;	
	TIFFVSetMethod	vsetparent;	
	TIFFPrintMethod	printdir;	
	TIFFBoolMethod	setupdecode;	
	TIFFBoolMethod	setupencode;	
} TIFFPredictorState;

#if defined(__cplusplus)
extern "C" {
#endif
extern	int TIFFPredictorInit(TIFF*);
extern	int TIFFPredictorCleanup(TIFF*);
#if defined(__cplusplus)
}
#endif
#endif 

