

#ifndef __AFGLOBAL_H__
#define __AFGLOBAL_H__

#include "aftypes.h"

FT_BEGIN_HEADER

  
  
  
  
  
  
  

  
  typedef struct AF_FaceGlobalsRec_*   AF_FaceGlobals;

  FT_LOCAL( FT_Error )
  af_face_globals_new( FT_Face          face,
                       AF_FaceGlobals  *aglobals );

  FT_LOCAL( FT_Error )
  af_face_globals_get_metrics( AF_FaceGlobals     globals,
                               FT_UInt            gindex,
                               FT_UInt            options,
                               AF_ScriptMetrics  *ametrics );

  FT_LOCAL( void )
  af_face_globals_free( AF_FaceGlobals  globals );

  FT_LOCAL_DEF( FT_Bool )
  af_face_globals_is_digit( AF_FaceGlobals  globals,
                            FT_UInt         gindex );

  

FT_END_HEADER

#endif 

