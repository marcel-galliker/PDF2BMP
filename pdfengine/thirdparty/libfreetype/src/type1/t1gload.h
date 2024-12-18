

#ifndef __T1GLOAD_H__
#define __T1GLOAD_H__

#include <ft2build.h>
#include "t1objs.h"

FT_BEGIN_HEADER

  FT_LOCAL( FT_Error )
  T1_Compute_Max_Advance( T1_Face  face,
                          FT_Pos*  max_advance );

  FT_LOCAL( FT_Error )
  T1_Get_Advances( FT_Face    face,
                   FT_UInt    first,
                   FT_UInt    count,
                   FT_Int32   load_flags,
                   FT_Fixed*  advances );

  FT_LOCAL( FT_Error )
  T1_Load_Glyph( FT_GlyphSlot  glyph,
                 FT_Size       size,
                 FT_UInt       glyph_index,
                 FT_Int32      load_flags );

FT_END_HEADER

#endif 

