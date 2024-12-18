

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  
  
  
  
  

#ifdef _STANDALONE_

#define FT_CONFIG_STANDARD_LIBRARY_H  <stdlib.h>

#include <string.h>           

#include "ftmisc.h"
#include "ftimage.h"

#else 

#include <ft2build.h>
#include "ftraster.h"
#include FT_INTERNAL_CALC_H   

#include "rastpic.h"

#endif 

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  
  
  
  
  
  
  

  

  
  

  
  
#define RASTER_GRAY_LINES  2048

  
  
  
  
  
  
  

  
  
  
  
  
  
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_raster

#ifdef _STANDALONE_

  
  
  
  
#define FT_UNUSED( x )  (x) = (x)

  
  
#ifndef FT_ERROR
#define FT_ERROR( x )  do { } while ( 0 )     
#endif

#ifndef FT_TRACE
#define FT_TRACE( x )   do { } while ( 0 )    
#define FT_TRACE1( x )  do { } while ( 0 )    
#define FT_TRACE6( x )  do { } while ( 0 )    
#endif

#define Raster_Err_None          0
#define Raster_Err_Not_Ini      -1
#define Raster_Err_Overflow     -2
#define Raster_Err_Neg_Height   -3
#define Raster_Err_Invalid      -4
#define Raster_Err_Unsupported  -5

#define ft_memset  memset

#define FT_DEFINE_RASTER_FUNCS( class_, glyph_format_, raster_new_, \
                                raster_reset_, raster_set_mode_,    \
                                raster_render_, raster_done_ )      \
          const FT_Raster_Funcs class_ =                            \
          {                                                         \
            glyph_format_,                                          \
            raster_new_,                                            \
            raster_reset_,                                          \
            raster_set_mode_,                                       \
            raster_render_,                                         \
            raster_done_                                            \
         };

#else 

#include FT_INTERNAL_OBJECTS_H
#include FT_INTERNAL_DEBUG_H        

#include "rasterrs.h"

#define Raster_Err_None         Raster_Err_Ok
#define Raster_Err_Not_Ini      Raster_Err_Raster_Uninitialized
#define Raster_Err_Overflow     Raster_Err_Raster_Overflow
#define Raster_Err_Neg_Height   Raster_Err_Raster_Negative_Height
#define Raster_Err_Invalid      Raster_Err_Invalid_Outline
#define Raster_Err_Unsupported  Raster_Err_Cannot_Render_Glyph

#endif 

#ifndef FT_MEM_SET
#define FT_MEM_SET( d, s, c )  ft_memset( d, s, c )
#endif

#ifndef FT_MEM_ZERO
#define FT_MEM_ZERO( dest, count )  FT_MEM_SET( dest, 0, count )
#endif

  
  
  
#define FMulDiv( a, b, c )  ( (a) * (b) / (c) )

  
  
  
#define SMulDiv  FT_MulDiv

  
  
  

#ifndef TRUE
#define TRUE   1
#endif

#ifndef FALSE
#define FALSE  0
#endif

#ifndef NULL
#define NULL  (void*)0
#endif

#ifndef SUCCESS
#define SUCCESS  0
#endif

#ifndef FAILURE
#define FAILURE  1
#endif

#define MaxBezier  32   
                        
                        

#define Pixel_Bits  6   

  
  
  
  
  
  
  

  typedef int             Int;
  typedef unsigned int    UInt;
  typedef short           Short;
  typedef unsigned short  UShort, *PUShort;
  typedef long            Long, *PLong;

  typedef unsigned char   Byte, *PByte;
  typedef char            Bool;

  typedef union  Alignment_
  {
    long    l;
    void*   p;
    void  (*f)(void);

  } Alignment, *PAlignment;

  typedef struct  TPoint_
  {
    Long  x;
    Long  y;

  } TPoint;

  
#define Flow_Up           0x8
#define Overshoot_Top     0x10
#define Overshoot_Bottom  0x20

  
  typedef enum  TStates_
  {
    Unknown_State,
    Ascending_State,
    Descending_State,
    Flat_State

  } TStates;

  typedef struct TProfile_  TProfile;
  typedef TProfile*         PProfile;

  struct  TProfile_
  {
    FT_F26Dot6  X;           
    PProfile    link;        
    PLong       offset;      
    unsigned    flags;       
                             
                             
                             
    long        height;      
    long        start;       

    unsigned    countL;      
                             

    PProfile    next;        
                             
  };

  typedef PProfile   TProfileList;
  typedef PProfile*  PProfileList;

  
  
  typedef struct  black_TBand_
  {
    Short  y_min;   
    Short  y_max;   

  } black_TBand;

#define AlignProfileSize \
  ( ( sizeof ( TProfile ) + sizeof ( Alignment ) - 1 ) / sizeof ( long ) )

#undef RAS_ARG
#undef RAS_ARGS
#undef RAS_VAR
#undef RAS_VARS

#ifdef FT_STATIC_RASTER

#define RAS_ARGS       
#define RAS_ARG        

#define RAS_VARS       
#define RAS_VAR        

#define FT_UNUSED_RASTER  do { } while ( 0 )

#else 

#define RAS_ARGS       black_PWorker  worker,
#define RAS_ARG        black_PWorker  worker

#define RAS_VARS       worker,
#define RAS_VAR        worker

#define FT_UNUSED_RASTER  FT_UNUSED( worker )

#endif 

  typedef struct black_TWorker_  black_TWorker, *black_PWorker;

  
  typedef void
  Function_Sweep_Init( RAS_ARGS Short*  min,
                                Short*  max );

  typedef void
  Function_Sweep_Span( RAS_ARGS Short       y,
                                FT_F26Dot6  x1,
                                FT_F26Dot6  x2,
                                PProfile    left,
                                PProfile    right );

  typedef void
  Function_Sweep_Step( RAS_ARG );

  
#undef FLOOR
#undef CEILING
#undef TRUNC
#undef SCALED

#define FLOOR( x )    ( (x) & -ras.precision )
#define CEILING( x )  ( ( (x) + ras.precision - 1 ) & -ras.precision )
#define TRUNC( x )    ( (signed long)(x) >> ras.precision_bits )
#define FRAC( x )     ( (x) & ( ras.precision - 1 ) )
#define SCALED( x )   ( ( (x) << ras.scale_shift ) - ras.precision_half )

#define IS_BOTTOM_OVERSHOOT( x )  ( CEILING( x ) - x >= ras.precision_half )
#define IS_TOP_OVERSHOOT( x )     ( x - FLOOR( x ) >= ras.precision_half )

  
  
  

  struct  black_TWorker_
  {
    Int         precision_bits;     
    Int         precision;
    Int         precision_half;
    Int         precision_shift;
    Int         precision_step;
    Int         precision_jitter;

    Int         scale_shift;        
                                    

    PLong       buff;               
    PLong       sizeBuff;           
    PLong       maxBuff;            
    PLong       top;                

    FT_Error    error;

    Int         numTurns;           

    TPoint*     arc;                

    UShort      bWidth;             
    PByte       bTarget;            
    PByte       gTarget;            

    Long        lastX, lastY;
    Long        minY, maxY;

    UShort      num_Profs;          

    Bool        fresh;              
                                    
    Bool        joint;              
                                    
                                    
    PProfile    cProfile;           
    PProfile    fProfile;           
    PProfile    gProfile;           
                                    

    TStates     state;              

    FT_Bitmap   target;             
    FT_Outline  outline;

    Long        traceOfs;           
    Long        traceG;             

    Short       traceIncr;          

    Short       gray_min_x;         
    Short       gray_max_x;         

    

    Function_Sweep_Init*  Proc_Sweep_Init;
    Function_Sweep_Span*  Proc_Sweep_Span;
    Function_Sweep_Span*  Proc_Sweep_Drop;
    Function_Sweep_Step*  Proc_Sweep_Step;

    Byte        dropOutControl;     

    Bool        second_pass;        
                                    
                                    
                                    
                                    
                                    

    TPoint      arcs[3 * MaxBezier + 1]; 

    black_TBand  band_stack[16];    
    Int          band_top;          

#ifdef FT_RASTER_OPTION_ANTI_ALIASING

    Byte*       grays;

    Byte        gray_lines[RASTER_GRAY_LINES];
                                
                                
                                
                                

    Short       gray_width;     
                                
                                

                       
                       

#endif 

  };

  typedef struct  black_TRaster_
  {
    char*          buffer;
    long           buffer_size;
    void*          memory;
    black_PWorker  worker;
    Byte           grays[5];
    Short          gray_width;

  } black_TRaster, *black_PRaster;

#ifdef FT_STATIC_RASTER

  static black_TWorker  cur_ras;
#define ras  cur_ras

#else 

#define ras  (*worker)

#endif 

#ifdef FT_RASTER_OPTION_ANTI_ALIASING

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  static const short  count_table[256] =
  {
    0x0000, 0x0001, 0x0001, 0x0002, 0x0010, 0x0011, 0x0011, 0x0012,
    0x0010, 0x0011, 0x0011, 0x0012, 0x0020, 0x0021, 0x0021, 0x0022,
    0x0100, 0x0101, 0x0101, 0x0102, 0x0110, 0x0111, 0x0111, 0x0112,
    0x0110, 0x0111, 0x0111, 0x0112, 0x0120, 0x0121, 0x0121, 0x0122,
    0x0100, 0x0101, 0x0101, 0x0102, 0x0110, 0x0111, 0x0111, 0x0112,
    0x0110, 0x0111, 0x0111, 0x0112, 0x0120, 0x0121, 0x0121, 0x0122,
    0x0200, 0x0201, 0x0201, 0x0202, 0x0210, 0x0211, 0x0211, 0x0212,
    0x0210, 0x0211, 0x0211, 0x0212, 0x0220, 0x0221, 0x0221, 0x0222,
    0x1000, 0x1001, 0x1001, 0x1002, 0x1010, 0x1011, 0x1011, 0x1012,
    0x1010, 0x1011, 0x1011, 0x1012, 0x1020, 0x1021, 0x1021, 0x1022,
    0x1100, 0x1101, 0x1101, 0x1102, 0x1110, 0x1111, 0x1111, 0x1112,
    0x1110, 0x1111, 0x1111, 0x1112, 0x1120, 0x1121, 0x1121, 0x1122,
    0x1100, 0x1101, 0x1101, 0x1102, 0x1110, 0x1111, 0x1111, 0x1112,
    0x1110, 0x1111, 0x1111, 0x1112, 0x1120, 0x1121, 0x1121, 0x1122,
    0x1200, 0x1201, 0x1201, 0x1202, 0x1210, 0x1211, 0x1211, 0x1212,
    0x1210, 0x1211, 0x1211, 0x1212, 0x1220, 0x1221, 0x1221, 0x1222,
    0x1000, 0x1001, 0x1001, 0x1002, 0x1010, 0x1011, 0x1011, 0x1012,
    0x1010, 0x1011, 0x1011, 0x1012, 0x1020, 0x1021, 0x1021, 0x1022,
    0x1100, 0x1101, 0x1101, 0x1102, 0x1110, 0x1111, 0x1111, 0x1112,
    0x1110, 0x1111, 0x1111, 0x1112, 0x1120, 0x1121, 0x1121, 0x1122,
    0x1100, 0x1101, 0x1101, 0x1102, 0x1110, 0x1111, 0x1111, 0x1112,
    0x1110, 0x1111, 0x1111, 0x1112, 0x1120, 0x1121, 0x1121, 0x1122,
    0x1200, 0x1201, 0x1201, 0x1202, 0x1210, 0x1211, 0x1211, 0x1212,
    0x1210, 0x1211, 0x1211, 0x1212, 0x1220, 0x1221, 0x1221, 0x1222,
    0x2000, 0x2001, 0x2001, 0x2002, 0x2010, 0x2011, 0x2011, 0x2012,
    0x2010, 0x2011, 0x2011, 0x2012, 0x2020, 0x2021, 0x2021, 0x2022,
    0x2100, 0x2101, 0x2101, 0x2102, 0x2110, 0x2111, 0x2111, 0x2112,
    0x2110, 0x2111, 0x2111, 0x2112, 0x2120, 0x2121, 0x2121, 0x2122,
    0x2100, 0x2101, 0x2101, 0x2102, 0x2110, 0x2111, 0x2111, 0x2112,
    0x2110, 0x2111, 0x2111, 0x2112, 0x2120, 0x2121, 0x2121, 0x2122,
    0x2200, 0x2201, 0x2201, 0x2202, 0x2210, 0x2211, 0x2211, 0x2212,
    0x2210, 0x2211, 0x2211, 0x2212, 0x2220, 0x2221, 0x2221, 0x2222
  };

#endif 

  
  
  
  
  
  
  

  
  
  
  
  
  
  
  
  
  
  
  
  static void
  Set_High_Precision( RAS_ARGS Int  High )
  {
    

    if ( High )
    {
      ras.precision_bits   = 12;
      ras.precision_step   = 256;
      ras.precision_jitter = 30;
    }
    else
    {
      ras.precision_bits   = 6;
      ras.precision_step   = 32;
      ras.precision_jitter = 2;
    }

    FT_TRACE6(( "Set_High_Precision(%s)\n", High ? "true" : "false" ));

    ras.precision       = 1 << ras.precision_bits;
    ras.precision_half  = ras.precision / 2;
    ras.precision_shift = ras.precision_bits - Pixel_Bits;
  }

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  static Bool
  New_Profile( RAS_ARGS TStates  aState,
                        Bool     overshoot )
  {
    if ( !ras.fProfile )
    {
      ras.cProfile  = (PProfile)ras.top;
      ras.fProfile  = ras.cProfile;
      ras.top      += AlignProfileSize;
    }

    if ( ras.top >= ras.maxBuff )
    {
      ras.error = Raster_Err_Overflow;
      return FAILURE;
    }

    ras.cProfile->flags  = 0;
    ras.cProfile->start  = 0;
    ras.cProfile->height = 0;
    ras.cProfile->offset = ras.top;
    ras.cProfile->link   = (PProfile)0;
    ras.cProfile->next   = (PProfile)0;
    ras.cProfile->flags  = ras.dropOutControl;

    switch ( aState )
    {
    case Ascending_State:
      ras.cProfile->flags |= Flow_Up;
      if ( overshoot )
        ras.cProfile->flags |= Overshoot_Bottom;

      FT_TRACE6(( "New ascending profile = %p\n", ras.cProfile ));
      break;

    case Descending_State:
      if ( overshoot )
        ras.cProfile->flags |= Overshoot_Top;
      FT_TRACE6(( "New descending profile = %p\n", ras.cProfile ));
      break;

    default:
      FT_ERROR(( "New_Profile: invalid profile direction\n" ));
      ras.error = Raster_Err_Invalid;
      return FAILURE;
    }

    if ( !ras.gProfile )
      ras.gProfile = ras.cProfile;

    ras.state = aState;
    ras.fresh = TRUE;
    ras.joint = FALSE;

    return SUCCESS;
  }

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  static Bool
  End_Profile( RAS_ARGS Bool  overshoot )
  {
    Long      h;
    PProfile  oldProfile;

    h = (Long)( ras.top - ras.cProfile->offset );

    if ( h < 0 )
    {
      FT_ERROR(( "End_Profile: negative height encountered\n" ));
      ras.error = Raster_Err_Neg_Height;
      return FAILURE;
    }

    if ( h > 0 )
    {
      FT_TRACE6(( "Ending profile %p, start = %ld, height = %ld\n",
                  ras.cProfile, ras.cProfile->start, h ));

      ras.cProfile->height = h;
      if ( overshoot )
      {
        if ( ras.cProfile->flags & Flow_Up )
          ras.cProfile->flags |= Overshoot_Top;
        else
          ras.cProfile->flags |= Overshoot_Bottom;
      }

      oldProfile   = ras.cProfile;
      ras.cProfile = (PProfile)ras.top;

      ras.top += AlignProfileSize;

      ras.cProfile->height = 0;
      ras.cProfile->offset = ras.top;

      oldProfile->next = ras.cProfile;
      ras.num_Profs++;
    }

    if ( ras.top >= ras.maxBuff )
    {
      FT_TRACE1(( "overflow in End_Profile\n" ));
      ras.error = Raster_Err_Overflow;
      return FAILURE;
    }

    ras.joint = FALSE;

    return SUCCESS;
  }

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  static Bool
  Insert_Y_Turn( RAS_ARGS Int  y )
  {
    PLong  y_turns;
    Int    y2, n;

    n       = ras.numTurns - 1;
    y_turns = ras.sizeBuff - ras.numTurns;

    
    while ( n >= 0 && y < y_turns[n] )
      n--;

    
    if ( n >= 0 && y > y_turns[n] )
      while ( n >= 0 )
      {
        y2 = (Int)y_turns[n];
        y_turns[n] = y;
        y = y2;
        n--;
      }

    if ( n < 0 )
    {
      ras.maxBuff--;
      if ( ras.maxBuff <= ras.top )
      {
        ras.error = Raster_Err_Overflow;
        return FAILURE;
      }
      ras.numTurns++;
      ras.sizeBuff[-ras.numTurns] = y;
    }

    return SUCCESS;
  }

  
  
  
  
  
  
  
  
  
  
  
  static Bool
  Finalize_Profile_Table( RAS_ARG )
  {
    Int       bottom, top;
    UShort    n;
    PProfile  p;

    n = ras.num_Profs;
    p = ras.fProfile;

    if ( n > 1 && p )
    {
      while ( n > 0 )
      {
        if ( n > 1 )
          p->link = (PProfile)( p->offset + p->height );
        else
          p->link = NULL;

        if ( p->flags & Flow_Up )
        {
          bottom = (Int)p->start;
          top    = (Int)( p->start + p->height - 1 );
        }
        else
        {
          bottom     = (Int)( p->start - p->height + 1 );
          top        = (Int)p->start;
          p->start   = bottom;
          p->offset += p->height - 1;
        }

        if ( Insert_Y_Turn( RAS_VARS bottom )  ||
             Insert_Y_Turn( RAS_VARS top + 1 ) )
          return FAILURE;

        p = p->link;
        n--;
      }
    }
    else
      ras.fProfile = NULL;

    return SUCCESS;
  }

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  static void
  Split_Conic( TPoint*  base )
  {
    Long  a, b;

    base[4].x = base[2].x;
    b = base[1].x;
    a = base[3].x = ( base[2].x + b ) / 2;
    b = base[1].x = ( base[0].x + b ) / 2;
    base[2].x = ( a + b ) / 2;

    base[4].y = base[2].y;
    b = base[1].y;
    a = base[3].y = ( base[2].y + b ) / 2;
    b = base[1].y = ( base[0].y + b ) / 2;
    base[2].y = ( a + b ) / 2;

    
    
  }

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  static void
  Split_Cubic( TPoint*  base )
  {
    Long  a, b, c, d;

    base[6].x = base[3].x;
    c = base[1].x;
    d = base[2].x;
    base[1].x = a = ( base[0].x + c + 1 ) >> 1;
    base[5].x = b = ( base[3].x + d + 1 ) >> 1;
    c = ( c + d + 1 ) >> 1;
    base[2].x = a = ( a + c + 1 ) >> 1;
    base[4].x = b = ( b + c + 1 ) >> 1;
    base[3].x = ( a + b + 1 ) >> 1;

    base[6].y = base[3].y;
    c = base[1].y;
    d = base[2].y;
    base[1].y = a = ( base[0].y + c + 1 ) >> 1;
    base[5].y = b = ( base[3].y + d + 1 ) >> 1;
    c = ( c + d + 1 ) >> 1;
    base[2].y = a = ( a + c + 1 ) >> 1;
    base[4].y = b = ( b + c + 1 ) >> 1;
    base[3].y = ( a + b + 1 ) >> 1;
  }

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  static Bool
  Line_Up( RAS_ARGS Long  x1,
                    Long  y1,
                    Long  x2,
                    Long  y2,
                    Long  miny,
                    Long  maxy )
  {
    Long   Dx, Dy;
    Int    e1, e2, f1, f2, size;     
    Long   Ix, Rx, Ax;

    PLong  top;

    Dx = x2 - x1;
    Dy = y2 - y1;

    if ( Dy <= 0 || y2 < miny || y1 > maxy )
      return SUCCESS;

    if ( y1 < miny )
    {
      
      
      x1 += SMulDiv( Dx, miny - y1, Dy );
      e1  = (Int)TRUNC( miny );
      f1  = 0;
    }
    else
    {
      e1 = (Int)TRUNC( y1 );
      f1 = (Int)FRAC( y1 );
    }

    if ( y2 > maxy )
    {
      
      e2  = (Int)TRUNC( maxy );
      f2  = 0;
    }
    else
    {
      e2 = (Int)TRUNC( y2 );
      f2 = (Int)FRAC( y2 );
    }

    if ( f1 > 0 )
    {
      if ( e1 == e2 )
        return SUCCESS;
      else
      {
        x1 += SMulDiv( Dx, ras.precision - f1, Dy );
        e1 += 1;
      }
    }
    else
      if ( ras.joint )
      {
        ras.top--;
        ras.joint = FALSE;
      }

    ras.joint = (char)( f2 == 0 );

    if ( ras.fresh )
    {
      ras.cProfile->start = e1;
      ras.fresh           = FALSE;
    }

    size = e2 - e1 + 1;
    if ( ras.top + size >= ras.maxBuff )
    {
      ras.error = Raster_Err_Overflow;
      return FAILURE;
    }

    if ( Dx > 0 )
    {
      Ix = SMulDiv( ras.precision, Dx, Dy);
      Rx = ( ras.precision * Dx ) % Dy;
      Dx = 1;
    }
    else
    {
      Ix = SMulDiv( ras.precision, -Dx, Dy) * -1;
      Rx =    ( ras.precision * -Dx ) % Dy;
      Dx = -1;
    }

    Ax  = -Dy;
    top = ras.top;

    while ( size > 0 )
    {
      *top++ = x1;

      x1 += Ix;
      Ax += Rx;
      if ( Ax >= 0 )
      {
        Ax -= Dy;
        x1 += Dx;
      }
      size--;
    }

    ras.top = top;
    return SUCCESS;
  }

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  static Bool
  Line_Down( RAS_ARGS Long  x1,
                      Long  y1,
                      Long  x2,
                      Long  y2,
                      Long  miny,
                      Long  maxy )
  {
    Bool  result, fresh;

    fresh  = ras.fresh;

    result = Line_Up( RAS_VARS x1, -y1, x2, -y2, -maxy, -miny );

    if ( fresh && !ras.fresh )
      ras.cProfile->start = -ras.cProfile->start;

    return result;
  }

  
  typedef void  (*TSplitter)( TPoint*  base );

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  static Bool
  Bezier_Up( RAS_ARGS Int        degree,
                      TSplitter  splitter,
                      Long       miny,
                      Long       maxy )
  {
    Long   y1, y2, e, e2, e0;
    Short  f1;

    TPoint*  arc;
    TPoint*  start_arc;

    PLong top;

    arc = ras.arc;
    y1  = arc[degree].y;
    y2  = arc[0].y;
    top = ras.top;

    if ( y2 < miny || y1 > maxy )
      goto Fin;

    e2 = FLOOR( y2 );

    if ( e2 > maxy )
      e2 = maxy;

    e0 = miny;

    if ( y1 < miny )
      e = miny;
    else
    {
      e  = CEILING( y1 );
      f1 = (Short)( FRAC( y1 ) );
      e0 = e;

      if ( f1 == 0 )
      {
        if ( ras.joint )
        {
          top--;
          ras.joint = FALSE;
        }

        *top++ = arc[degree].x;

        e += ras.precision;
      }
    }

    if ( ras.fresh )
    {
      ras.cProfile->start = TRUNC( e0 );
      ras.fresh = FALSE;
    }

    if ( e2 < e )
      goto Fin;

    if ( ( top + TRUNC( e2 - e ) + 1 ) >= ras.maxBuff )
    {
      ras.top   = top;
      ras.error = Raster_Err_Overflow;
      return FAILURE;
    }

    start_arc = arc;

    while ( arc >= start_arc && e <= e2 )
    {
      ras.joint = FALSE;

      y2 = arc[0].y;

      if ( y2 > e )
      {
        y1 = arc[degree].y;
        if ( y2 - y1 >= ras.precision_step )
        {
          splitter( arc );
          arc += degree;
        }
        else
        {
          *top++ = arc[degree].x + FMulDiv( arc[0].x - arc[degree].x,
                                            e - y1, y2 - y1 );
          arc -= degree;
          e   += ras.precision;
        }
      }
      else
      {
        if ( y2 == e )
        {
          ras.joint  = TRUE;
          *top++     = arc[0].x;

          e += ras.precision;
        }
        arc -= degree;
      }
    }

  Fin:
    ras.top  = top;
    ras.arc -= degree;
    return SUCCESS;
  }

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  static Bool
  Bezier_Down( RAS_ARGS Int        degree,
                        TSplitter  splitter,
                        Long       miny,
                        Long       maxy )
  {
    TPoint*  arc = ras.arc;
    Bool     result, fresh;

    arc[0].y = -arc[0].y;
    arc[1].y = -arc[1].y;
    arc[2].y = -arc[2].y;
    if ( degree > 2 )
      arc[3].y = -arc[3].y;

    fresh = ras.fresh;

    result = Bezier_Up( RAS_VARS degree, splitter, -maxy, -miny );

    if ( fresh && !ras.fresh )
      ras.cProfile->start = -ras.cProfile->start;

    arc[0].y = -arc[0].y;
    return result;
  }

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  static Bool
  Line_To( RAS_ARGS Long  x,
                    Long  y )
  {
    

    switch ( ras.state )
    {
    case Unknown_State:
      if ( y > ras.lastY )
      {
        if ( New_Profile( RAS_VARS Ascending_State,
                                   IS_BOTTOM_OVERSHOOT( ras.lastY ) ) )
          return FAILURE;
      }
      else
      {
        if ( y < ras.lastY )
          if ( New_Profile( RAS_VARS Descending_State,
                                     IS_TOP_OVERSHOOT( ras.lastY ) ) )
            return FAILURE;
      }
      break;

    case Ascending_State:
      if ( y < ras.lastY )
      {
        if ( End_Profile( RAS_VARS IS_TOP_OVERSHOOT( ras.lastY ) ) ||
             New_Profile( RAS_VARS Descending_State,
                                   IS_TOP_OVERSHOOT( ras.lastY ) ) )
          return FAILURE;
      }
      break;

    case Descending_State:
      if ( y > ras.lastY )
      {
        if ( End_Profile( RAS_VARS IS_BOTTOM_OVERSHOOT( ras.lastY ) ) ||
             New_Profile( RAS_VARS Ascending_State,
                                   IS_BOTTOM_OVERSHOOT( ras.lastY ) ) )
          return FAILURE;
      }
      break;

    default:
      ;
    }

    

    switch ( ras.state )
    {
    case Ascending_State:
      if ( Line_Up( RAS_VARS ras.lastX, ras.lastY,
                             x, y, ras.minY, ras.maxY ) )
        return FAILURE;
      break;

    case Descending_State:
      if ( Line_Down( RAS_VARS ras.lastX, ras.lastY,
                               x, y, ras.minY, ras.maxY ) )
        return FAILURE;
      break;

    default:
      ;
    }

    ras.lastX = x;
    ras.lastY = y;

    return SUCCESS;
  }

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  static Bool
  Conic_To( RAS_ARGS Long  cx,
                     Long  cy,
                     Long  x,
                     Long  y )
  {
    Long     y1, y2, y3, x3, ymin, ymax;
    TStates  state_bez;

    ras.arc      = ras.arcs;
    ras.arc[2].x = ras.lastX;
    ras.arc[2].y = ras.lastY;
    ras.arc[1].x = cx;
    ras.arc[1].y = cy;
    ras.arc[0].x = x;
    ras.arc[0].y = y;

    do
    {
      y1 = ras.arc[2].y;
      y2 = ras.arc[1].y;
      y3 = ras.arc[0].y;
      x3 = ras.arc[0].x;

      

      if ( y1 <= y3 )
      {
        ymin = y1;
        ymax = y3;
      }
      else
      {
        ymin = y3;
        ymax = y1;
      }

      if ( y2 < ymin || y2 > ymax )
      {
        
        Split_Conic( ras.arc );
        ras.arc += 2;
      }
      else if ( y1 == y3 )
      {
        
        ras.arc -= 2;
      }
      else
      {
        
        
        state_bez = y1 < y3 ? Ascending_State : Descending_State;
        if ( ras.state != state_bez )
        {
          Bool  o = state_bez == Ascending_State ? IS_BOTTOM_OVERSHOOT( y1 )
                                                 : IS_TOP_OVERSHOOT( y1 );

          
          if ( ras.state != Unknown_State &&
               End_Profile( RAS_VARS o )  )
            goto Fail;

          
          if ( New_Profile( RAS_VARS state_bez, o ) )
            goto Fail;
        }

        
        if ( state_bez == Ascending_State )
        {
          if ( Bezier_Up( RAS_VARS 2, Split_Conic, ras.minY, ras.maxY ) )
            goto Fail;
        }
        else
          if ( Bezier_Down( RAS_VARS 2, Split_Conic, ras.minY, ras.maxY ) )
            goto Fail;
      }

    } while ( ras.arc >= ras.arcs );

    ras.lastX = x3;
    ras.lastY = y3;

    return SUCCESS;

  Fail:
    return FAILURE;
  }

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  static Bool
  Cubic_To( RAS_ARGS Long  cx1,
                     Long  cy1,
                     Long  cx2,
                     Long  cy2,
                     Long  x,
                     Long  y )
  {
    Long     y1, y2, y3, y4, x4, ymin1, ymax1, ymin2, ymax2;
    TStates  state_bez;

    ras.arc      = ras.arcs;
    ras.arc[3].x = ras.lastX;
    ras.arc[3].y = ras.lastY;
    ras.arc[2].x = cx1;
    ras.arc[2].y = cy1;
    ras.arc[1].x = cx2;
    ras.arc[1].y = cy2;
    ras.arc[0].x = x;
    ras.arc[0].y = y;

    do
    {
      y1 = ras.arc[3].y;
      y2 = ras.arc[2].y;
      y3 = ras.arc[1].y;
      y4 = ras.arc[0].y;
      x4 = ras.arc[0].x;

      

      if ( y1 <= y4 )
      {
        ymin1 = y1;
        ymax1 = y4;
      }
      else
      {
        ymin1 = y4;
        ymax1 = y1;
      }

      if ( y2 <= y3 )
      {
        ymin2 = y2;
        ymax2 = y3;
      }
      else
      {
        ymin2 = y3;
        ymax2 = y2;
      }

      if ( ymin2 < ymin1 || ymax2 > ymax1 )
      {
        
        Split_Cubic( ras.arc );
        ras.arc += 3;
      }
      else if ( y1 == y4 )
      {
        
        ras.arc -= 3;
      }
      else
      {
        state_bez = ( y1 <= y4 ) ? Ascending_State : Descending_State;

        
        if ( ras.state != state_bez )
        {
          Bool  o = state_bez == Ascending_State ? IS_BOTTOM_OVERSHOOT( y1 )
                                                 : IS_TOP_OVERSHOOT( y1 );

          
          if ( ras.state != Unknown_State &&
               End_Profile( RAS_VARS o )  )
            goto Fail;

          if ( New_Profile( RAS_VARS state_bez, o ) )
            goto Fail;
        }

        
        if ( state_bez == Ascending_State )
        {
          if ( Bezier_Up( RAS_VARS 3, Split_Cubic, ras.minY, ras.maxY ) )
            goto Fail;
        }
        else
          if ( Bezier_Down( RAS_VARS 3, Split_Cubic, ras.minY, ras.maxY ) )
            goto Fail;
      }

    } while ( ras.arc >= ras.arcs );

    ras.lastX = x4;
    ras.lastY = y4;

    return SUCCESS;

  Fail:
    return FAILURE;
  }

#undef  SWAP_
#define SWAP_( x, y )  do                \
                       {                 \
                         Long  swap = x; \
                                         \
                                         \
                         x = y;          \
                         y = swap;       \
                       } while ( 0 )

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  static Bool
  Decompose_Curve( RAS_ARGS UShort  first,
                            UShort  last,
                            int     flipped )
  {
    FT_Vector   v_last;
    FT_Vector   v_control;
    FT_Vector   v_start;

    FT_Vector*  points;
    FT_Vector*  point;
    FT_Vector*  limit;
    char*       tags;

    unsigned    tag;       

    points = ras.outline.points;
    limit  = points + last;

    v_start.x = SCALED( points[first].x );
    v_start.y = SCALED( points[first].y );
    v_last.x  = SCALED( points[last].x );
    v_last.y  = SCALED( points[last].y );

    if ( flipped )
    {
      SWAP_( v_start.x, v_start.y );
      SWAP_( v_last.x, v_last.y );
    }

    v_control = v_start;

    point = points + first;
    tags  = ras.outline.tags + first;

    
    if ( tags[0] & FT_CURVE_TAG_HAS_SCANMODE )
      ras.dropOutControl = (Byte)tags[0] >> 5;

    tag = FT_CURVE_TAG( tags[0] );

    
    if ( tag == FT_CURVE_TAG_CUBIC )
      goto Invalid_Outline;

    
    if ( tag == FT_CURVE_TAG_CONIC )
    {
      
      if ( FT_CURVE_TAG( ras.outline.tags[last] ) == FT_CURVE_TAG_ON )
      {
        
        v_start = v_last;
        limit--;
      }
      else
      {
        
        
        
        v_start.x = ( v_start.x + v_last.x ) / 2;
        v_start.y = ( v_start.y + v_last.y ) / 2;

        v_last = v_start;
      }
      point--;
      tags--;
    }

    ras.lastX = v_start.x;
    ras.lastY = v_start.y;

    while ( point < limit )
    {
      point++;
      tags++;

      tag = FT_CURVE_TAG( tags[0] );

      switch ( tag )
      {
      case FT_CURVE_TAG_ON:  
        {
          Long  x, y;

          x = SCALED( point->x );
          y = SCALED( point->y );
          if ( flipped )
            SWAP_( x, y );

          if ( Line_To( RAS_VARS x, y ) )
            goto Fail;
          continue;
        }

      case FT_CURVE_TAG_CONIC:  
        v_control.x = SCALED( point[0].x );
        v_control.y = SCALED( point[0].y );

        if ( flipped )
          SWAP_( v_control.x, v_control.y );

      Do_Conic:
        if ( point < limit )
        {
          FT_Vector  v_middle;
          Long       x, y;

          point++;
          tags++;
          tag = FT_CURVE_TAG( tags[0] );

          x = SCALED( point[0].x );
          y = SCALED( point[0].y );

          if ( flipped )
            SWAP_( x, y );

          if ( tag == FT_CURVE_TAG_ON )
          {
            if ( Conic_To( RAS_VARS v_control.x, v_control.y, x, y ) )
              goto Fail;
            continue;
          }

          if ( tag != FT_CURVE_TAG_CONIC )
            goto Invalid_Outline;

          v_middle.x = ( v_control.x + x ) / 2;
          v_middle.y = ( v_control.y + y ) / 2;

          if ( Conic_To( RAS_VARS v_control.x, v_control.y,
                                  v_middle.x,  v_middle.y ) )
            goto Fail;

          v_control.x = x;
          v_control.y = y;

          goto Do_Conic;
        }

        if ( Conic_To( RAS_VARS v_control.x, v_control.y,
                                v_start.x,   v_start.y ) )
          goto Fail;

        goto Close;

      default:  
        {
          Long  x1, y1, x2, y2, x3, y3;

          if ( point + 1 > limit                             ||
               FT_CURVE_TAG( tags[1] ) != FT_CURVE_TAG_CUBIC )
            goto Invalid_Outline;

          point += 2;
          tags  += 2;

          x1 = SCALED( point[-2].x );
          y1 = SCALED( point[-2].y );
          x2 = SCALED( point[-1].x );
          y2 = SCALED( point[-1].y );

          if ( flipped )
          {
            SWAP_( x1, y1 );
            SWAP_( x2, y2 );
          }

          if ( point <= limit )
          {
            x3 = SCALED( point[0].x );
            y3 = SCALED( point[0].y );

            if ( flipped )
              SWAP_( x3, y3 );

            if ( Cubic_To( RAS_VARS x1, y1, x2, y2, x3, y3 ) )
              goto Fail;
            continue;
          }

          if ( Cubic_To( RAS_VARS x1, y1, x2, y2, v_start.x, v_start.y ) )
            goto Fail;
          goto Close;
        }
      }
    }

    
    if ( Line_To( RAS_VARS v_start.x, v_start.y ) )
      goto Fail;

  Close:
    return SUCCESS;

  Invalid_Outline:
    ras.error = Raster_Err_Invalid;

  Fail:
    return FAILURE;
  }

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  static Bool
  Convert_Glyph( RAS_ARGS int  flipped )
  {
    int       i;
    unsigned  start;

    PProfile  lastProfile;

    ras.fProfile = NULL;
    ras.joint    = FALSE;
    ras.fresh    = FALSE;

    ras.maxBuff  = ras.sizeBuff - AlignProfileSize;

    ras.numTurns = 0;

    ras.cProfile         = (PProfile)ras.top;
    ras.cProfile->offset = ras.top;
    ras.num_Profs        = 0;

    start = 0;

    for ( i = 0; i < ras.outline.n_contours; i++ )
    {
      Bool  o;

      ras.state    = Unknown_State;
      ras.gProfile = NULL;

      if ( Decompose_Curve( RAS_VARS (unsigned short)start,
                                     ras.outline.contours[i],
                                     flipped ) )
        return FAILURE;

      start = ras.outline.contours[i] + 1;

      
      if ( FRAC( ras.lastY ) == 0 &&
           ras.lastY >= ras.minY  &&
           ras.lastY <= ras.maxY  )
        if ( ras.gProfile                        &&
             ( ras.gProfile->flags & Flow_Up ) ==
               ( ras.cProfile->flags & Flow_Up ) )
          ras.top--;
        
        

      lastProfile = ras.cProfile;
      if ( ras.cProfile->flags & Flow_Up )
        o = IS_TOP_OVERSHOOT( ras.lastY );
      else
        o = IS_BOTTOM_OVERSHOOT( ras.lastY );
      if ( End_Profile( RAS_VARS o ) )
        return FAILURE;

      
      if ( ras.gProfile )
        lastProfile->next = ras.gProfile;
    }

    if ( Finalize_Profile_Table( RAS_VAR ) )
      return FAILURE;

    return (Bool)( ras.top < ras.maxBuff ? SUCCESS : FAILURE );
  }

  
  
  
  
  
  
  

  
  
  
  
  
  
  static void
  Init_Linked( TProfileList*  l )
  {
    *l = NULL;
  }

  
  
  
  
  
  
  static void
  InsNew( PProfileList  list,
          PProfile      profile )
  {
    PProfile  *old, current;
    Long       x;

    old     = list;
    current = *old;
    x       = profile->X;

    while ( current )
    {
      if ( x < current->X )
        break;
      old     = &current->link;
      current = *old;
    }

    profile->link = current;
    *old          = profile;
  }

  
  
  
  
  
  
  static void
  DelOld( PProfileList  list,
          PProfile      profile )
  {
    PProfile  *old, current;

    old     = list;
    current = *old;

    while ( current )
    {
      if ( current == profile )
      {
        *old = current->link;
        return;
      }

      old     = &current->link;
      current = *old;
    }

    
    
  }

  
  
  
  
  
  
  
  
  static void
  Sort( PProfileList  list )
  {
    PProfile  *old, current, next;

    
    current = *list;
    while ( current )
    {
      current->X       = *current->offset;
      current->offset += current->flags & Flow_Up ? 1 : -1;
      current->height--;
      current = current->link;
    }

    
    old     = list;
    current = *old;

    if ( !current )
      return;

    next = current->link;

    while ( next )
    {
      if ( current->X <= next->X )
      {
        old     = &current->link;
        current = *old;

        if ( !current )
          return;
      }
      else
      {
        *old          = next;
        current->link = next->link;
        next->link    = current;

        old     = list;
        current = *old;
      }

      next = current->link;
    }
  }

  
  
  
  
  
  
  
  

  static void
  Vertical_Sweep_Init( RAS_ARGS Short*  min,
                                Short*  max )
  {
    Long  pitch = ras.target.pitch;

    FT_UNUSED( max );

    ras.traceIncr = (Short)-pitch;
    ras.traceOfs  = -*min * pitch;
    if ( pitch > 0 )
      ras.traceOfs += ( ras.target.rows - 1 ) * pitch;

    ras.gray_min_x = 0;
    ras.gray_max_x = 0;
  }

  static void
  Vertical_Sweep_Span( RAS_ARGS Short       y,
                                FT_F26Dot6  x1,
                                FT_F26Dot6  x2,
                                PProfile    left,
                                PProfile    right )
  {
    Long   e1, e2;
    int    c1, c2;
    Byte   f1, f2;
    Byte*  target;

    FT_UNUSED( y );
    FT_UNUSED( left );
    FT_UNUSED( right );

    

    e1 = TRUNC( CEILING( x1 ) );

    if ( x2 - x1 - ras.precision <= ras.precision_jitter )
      e2 = e1;
    else
      e2 = TRUNC( FLOOR( x2 ) );

    if ( e2 >= 0 && e1 < ras.bWidth )
    {
      if ( e1 < 0 )
        e1 = 0;
      if ( e2 >= ras.bWidth )
        e2 = ras.bWidth - 1;

      c1 = (Short)( e1 >> 3 );
      c2 = (Short)( e2 >> 3 );

      f1 = (Byte)  ( 0xFF >> ( e1 & 7 ) );
      f2 = (Byte) ~( 0x7F >> ( e2 & 7 ) );

      if ( ras.gray_min_x > c1 )
        ras.gray_min_x = (short)c1;
      if ( ras.gray_max_x < c2 )
        ras.gray_max_x = (short)c2;

      target = ras.bTarget + ras.traceOfs + c1;
      c2 -= c1;

      if ( c2 > 0 )
      {
        target[0] |= f1;

        
        
        
        c2--;
        while ( c2 > 0 )
        {
          *(++target) = 0xFF;
          c2--;
        }
        target[1] |= f2;
      }
      else
        *target |= ( f1 & f2 );
    }
  }

  static void
  Vertical_Sweep_Drop( RAS_ARGS Short       y,
                                FT_F26Dot6  x1,
                                FT_F26Dot6  x2,
                                PProfile    left,
                                PProfile    right )
  {
    Long   e1, e2, pxl;
    Short  c1, f1;

    

    
    
    
    
    
    
    
    
    
    

    
    
    
    
    
    
    
    
    

    e1  = CEILING( x1 );
    e2  = FLOOR  ( x2 );
    pxl = e1;

    if ( e1 > e2 )
    {
      Int  dropOutControl = left->flags & 7;

      if ( e1 == e2 + ras.precision )
      {
        switch ( dropOutControl )
        {
        case 0: 
          pxl = e2;
          break;

        case 4: 
          pxl = FLOOR( ( x1 + x2 - 1 ) / 2 + ras.precision_half );
          break;

        case 1: 
        case 5: 

          

          
          
          
          
          
          
          
          
          
          
          
          
          
          
          
          
          
          
          
          
          
          
          

          
          if ( left->next == right                &&
               left->height <= 0                  &&
               !( left->flags & Overshoot_Top   &&
                  x2 - x1 >= ras.precision_half ) )
            return;

          
          if ( right->next == left                 &&
               left->start == y                    &&
               !( left->flags & Overshoot_Bottom &&
                  x2 - x1 >= ras.precision_half  ) )
            return;

          if ( dropOutControl == 1 )
            pxl = e2;
          else
            pxl = FLOOR( ( x1 + x2 - 1 ) / 2 + ras.precision_half );
          break;

        default: 
          return;  
        }

        
        
        
        if ( pxl < 0 )
          pxl = e1;
        else if ( TRUNC( pxl ) >= ras.bWidth )
          pxl = e2;

        
        e1 = pxl == e1 ? e2 : e1;

        e1 = TRUNC( e1 );

        c1 = (Short)( e1 >> 3 );
        f1 = (Short)( e1 &  7 );

        if ( e1 >= 0 && e1 < ras.bWidth                      &&
             ras.bTarget[ras.traceOfs + c1] & ( 0x80 >> f1 ) )
          return;
      }
      else
        return;
    }

    e1 = TRUNC( pxl );

    if ( e1 >= 0 && e1 < ras.bWidth )
    {
      c1 = (Short)( e1 >> 3 );
      f1 = (Short)( e1 & 7 );

      if ( ras.gray_min_x > c1 )
        ras.gray_min_x = c1;
      if ( ras.gray_max_x < c1 )
        ras.gray_max_x = c1;

      ras.bTarget[ras.traceOfs + c1] |= (char)( 0x80 >> f1 );
    }
  }

  static void
  Vertical_Sweep_Step( RAS_ARG )
  {
    ras.traceOfs += ras.traceIncr;
  }

  
  
  
  
  
  
  
  

  static void
  Horizontal_Sweep_Init( RAS_ARGS Short*  min,
                                  Short*  max )
  {
    
    FT_UNUSED_RASTER;
    FT_UNUSED( min );
    FT_UNUSED( max );
  }

  static void
  Horizontal_Sweep_Span( RAS_ARGS Short       y,
                                  FT_F26Dot6  x1,
                                  FT_F26Dot6  x2,
                                  PProfile    left,
                                  PProfile    right )
  {
    Long   e1, e2;
    PByte  bits;
    Byte   f1;

    FT_UNUSED( left );
    FT_UNUSED( right );

    if ( x2 - x1 < ras.precision )
    {
      e1 = CEILING( x1 );
      e2 = FLOOR  ( x2 );

      if ( e1 == e2 )
      {
        bits = ras.bTarget + ( y >> 3 );
        f1   = (Byte)( 0x80 >> ( y & 7 ) );

        e1 = TRUNC( e1 );

        if ( e1 >= 0 && e1 < ras.target.rows )
        {
          PByte  p;

          p = bits - e1 * ras.target.pitch;
          if ( ras.target.pitch > 0 )
            p += ( ras.target.rows - 1 ) * ras.target.pitch;

          p[0] |= f1;
        }
      }
    }
  }

  static void
  Horizontal_Sweep_Drop( RAS_ARGS Short       y,
                                  FT_F26Dot6  x1,
                                  FT_F26Dot6  x2,
                                  PProfile    left,
                                  PProfile    right )
  {
    Long   e1, e2, pxl;
    PByte  bits;
    Byte   f1;

    

    
    
    
    
    
    
    
    
    

    e1  = CEILING( x1 );
    e2  = FLOOR  ( x2 );
    pxl = e1;

    if ( e1 > e2 )
    {
      Int  dropOutControl = left->flags & 7;

      if ( e1 == e2 + ras.precision )
      {
        switch ( dropOutControl )
        {
        case 0: 
          pxl = e2;
          break;

        case 4: 
          pxl = FLOOR( ( x1 + x2 - 1 ) / 2 + ras.precision_half );
          break;

        case 1: 
        case 5: 
          

          
          if ( left->next == right                &&
               left->height <= 0                  &&
               !( left->flags & Overshoot_Top   &&
                  x2 - x1 >= ras.precision_half ) )
            return;

          
          if ( right->next == left                 &&
               left->start == y                    &&
               !( left->flags & Overshoot_Bottom &&
                  x2 - x1 >= ras.precision_half  ) )
            return;

          if ( dropOutControl == 1 )
            pxl = e2;
          else
            pxl = FLOOR( ( x1 + x2 - 1 ) / 2 + ras.precision_half );
          break;

        default: 
          return;  
        }

        
        
        
        if ( pxl < 0 )
          pxl = e1;
        else if ( TRUNC( pxl ) >= ras.target.rows )
          pxl = e2;

        
        e1 = pxl == e1 ? e2 : e1;

        e1 = TRUNC( e1 );

        bits = ras.bTarget + ( y >> 3 );
        f1   = (Byte)( 0x80 >> ( y & 7 ) );

        bits -= e1 * ras.target.pitch;
        if ( ras.target.pitch > 0 )
          bits += ( ras.target.rows - 1 ) * ras.target.pitch;

        if ( e1 >= 0              &&
             e1 < ras.target.rows &&
             *bits & f1           )
          return;
      }
      else
        return;
    }

    bits = ras.bTarget + ( y >> 3 );
    f1   = (Byte)( 0x80 >> ( y & 7 ) );

    e1 = TRUNC( pxl );

    if ( e1 >= 0 && e1 < ras.target.rows )
    {
      bits -= e1 * ras.target.pitch;
      if ( ras.target.pitch > 0 )
        bits += ( ras.target.rows - 1 ) * ras.target.pitch;

      bits[0] |= f1;
    }
  }

  static void
  Horizontal_Sweep_Step( RAS_ARG )
  {
    
    FT_UNUSED_RASTER;
  }

#ifdef FT_RASTER_OPTION_ANTI_ALIASING

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  static void
  Vertical_Gray_Sweep_Init( RAS_ARGS Short*  min,
                                     Short*  max )
  {
    Long  pitch, byte_len;

    *min = *min & -2;
    *max = ( *max + 3 ) & -2;

    ras.traceOfs  = 0;
    pitch         = ras.target.pitch;
    byte_len      = -pitch;
    ras.traceIncr = (Short)byte_len;
    ras.traceG    = ( *min / 2 ) * byte_len;

    if ( pitch > 0 )
    {
      ras.traceG += ( ras.target.rows - 1 ) * pitch;
      byte_len    = -byte_len;
    }

    ras.gray_min_x =  (Short)byte_len;
    ras.gray_max_x = -(Short)byte_len;
  }

  static void
  Vertical_Gray_Sweep_Step( RAS_ARG )
  {
    Int     c1, c2;
    PByte   pix, bit, bit2;
    short*  count = (short*)count_table;
    Byte*   grays;

    ras.traceOfs += ras.gray_width;

    if ( ras.traceOfs > ras.gray_width )
    {
      pix   = ras.gTarget + ras.traceG + ras.gray_min_x * 4;
      grays = ras.grays;

      if ( ras.gray_max_x >= 0 )
      {
        Long  last_pixel = ras.target.width - 1;
        Int   last_cell  = last_pixel >> 2;
        Int   last_bit   = last_pixel & 3;
        Bool  over       = 0;

        if ( ras.gray_max_x >= last_cell && last_bit != 3 )
        {
          ras.gray_max_x = last_cell - 1;
          over = 1;
        }

        if ( ras.gray_min_x < 0 )
          ras.gray_min_x = 0;

        bit  = ras.bTarget + ras.gray_min_x;
        bit2 = bit + ras.gray_width;

        c1 = ras.gray_max_x - ras.gray_min_x;

        while ( c1 >= 0 )
        {
          c2 = count[*bit] + count[*bit2];

          if ( c2 )
          {
            pix[0] = grays[(c2 >> 12) & 0x000F];
            pix[1] = grays[(c2 >> 8 ) & 0x000F];
            pix[2] = grays[(c2 >> 4 ) & 0x000F];
            pix[3] = grays[ c2        & 0x000F];

            *bit  = 0;
            *bit2 = 0;
          }

          bit++;
          bit2++;
          pix += 4;
          c1--;
        }

        if ( over )
        {
          c2 = count[*bit] + count[*bit2];
          if ( c2 )
          {
            switch ( last_bit )
            {
            case 2:
              pix[2] = grays[(c2 >> 4 ) & 0x000F];
            case 1:
              pix[1] = grays[(c2 >> 8 ) & 0x000F];
            default:
              pix[0] = grays[(c2 >> 12) & 0x000F];
            }

            *bit  = 0;
            *bit2 = 0;
          }
        }
      }

      ras.traceOfs = 0;
      ras.traceG  += ras.traceIncr;

      ras.gray_min_x =  32000;
      ras.gray_max_x = -32000;
    }
  }

  static void
  Horizontal_Gray_Sweep_Span( RAS_ARGS Short       y,
                                       FT_F26Dot6  x1,
                                       FT_F26Dot6  x2,
                                       PProfile    left,
                                       PProfile    right )
  {
    
    FT_UNUSED_RASTER;
    FT_UNUSED( y );
    FT_UNUSED( x1 );
    FT_UNUSED( x2 );
    FT_UNUSED( left );
    FT_UNUSED( right );
  }

  static void
  Horizontal_Gray_Sweep_Drop( RAS_ARGS Short       y,
                                       FT_F26Dot6  x1,
                                       FT_F26Dot6  x2,
                                       PProfile    left,
                                       PProfile    right )
  {
    Long   e1, e2;
    PByte  pixel;
    Byte   color;

    

    e1 = CEILING( x1 );
    e2 = FLOOR  ( x2 );

    if ( e1 > e2 )
    {
      Int  dropOutControl = left->flags & 7;

      if ( e1 == e2 + ras.precision )
      {
        switch ( dropOutControl )
        {
        case 0: 
          e1 = e2;
          break;

        case 4: 
          e1 = FLOOR( ( x1 + x2 - 1 ) / 2 + ras.precision_half );
          break;

        case 1: 
        case 5: 
          

          
          if ( left->next == right && left->height <= 0 )
            return;

          
          if ( right->next == left && left->start == y )
            return;

          if ( dropOutControl == 1 )
            e1 = e2;
          else
            e1 = FLOOR( ( x1 + x2 - 1 ) / 2 + ras.precision_half );

          break;

        default: 
          return;  
        }
      }
      else
        return;
    }

    if ( e1 >= 0 )
    {
      if ( x2 - x1 >= ras.precision_half )
        color = ras.grays[2];
      else
        color = ras.grays[1];

      e1 = TRUNC( e1 ) / 2;
      if ( e1 < ras.target.rows )
      {
        pixel = ras.gTarget - e1 * ras.target.pitch + y / 2;
        if ( ras.target.pitch > 0 )
          pixel += ( ras.target.rows - 1 ) * ras.target.pitch;

        if ( pixel[0] == ras.grays[0] )
          pixel[0] = color;
      }
    }
  }

#endif 

  
  
  
  
  

  static Bool
  Draw_Sweep( RAS_ARG )
  {
    Short         y, y_change, y_height;

    PProfile      P, Q, P_Left, P_Right;

    Short         min_Y, max_Y, top, bottom, dropouts;

    Long          x1, x2, xs, e1, e2;

    TProfileList  waiting;
    TProfileList  draw_left, draw_right;

    

    Init_Linked( &waiting );

    Init_Linked( &draw_left  );
    Init_Linked( &draw_right );

    

    P     = ras.fProfile;
    max_Y = (Short)TRUNC( ras.minY );
    min_Y = (Short)TRUNC( ras.maxY );

    while ( P )
    {
      Q = P->link;

      bottom = (Short)P->start;
      top    = (Short)( P->start + P->height - 1 );

      if ( min_Y > bottom )
        min_Y = bottom;
      if ( max_Y < top )
        max_Y = top;

      P->X = 0;
      InsNew( &waiting, P );

      P = Q;
    }

    
    if ( ras.numTurns == 0 )
    {
      ras.error = Raster_Err_Invalid;
      return FAILURE;
    }

    

    ras.Proc_Sweep_Init( RAS_VARS &min_Y, &max_Y );

    

    P = waiting;

    while ( P )
    {
      P->countL = (UShort)( P->start - min_Y );
      P = P->link;
    }

    

    y        = min_Y;
    y_height = 0;

    if ( ras.numTurns > 0                     &&
         ras.sizeBuff[-ras.numTurns] == min_Y )
      ras.numTurns--;

    while ( ras.numTurns > 0 )
    {
      

      P = waiting;

      while ( P )
      {
        Q = P->link;
        P->countL -= y_height;
        if ( P->countL == 0 )
        {
          DelOld( &waiting, P );

          if ( P->flags & Flow_Up )
            InsNew( &draw_left,  P );
          else
            InsNew( &draw_right, P );
        }

        P = Q;
      }

      

      Sort( &draw_left );
      Sort( &draw_right );

      y_change = (Short)ras.sizeBuff[-ras.numTurns--];
      y_height = (Short)( y_change - y );

      while ( y < y_change )
      {
        

        dropouts = 0;

        P_Left  = draw_left;
        P_Right = draw_right;

        while ( P_Left )
        {
          x1 = P_Left ->X;
          x2 = P_Right->X;

          if ( x1 > x2 )
          {
            xs = x1;
            x1 = x2;
            x2 = xs;
          }

          e1 = FLOOR( x1 );
          e2 = CEILING( x2 );

          if ( x2 - x1 <= ras.precision &&
               e1 != x1 && e2 != x2     )
          {
            if ( e1 > e2 || e2 == e1 + ras.precision )
            {
              Int  dropOutControl = P_Left->flags & 7;

              if ( dropOutControl != 2 )
              {
                

                P_Left ->X = x1;
                P_Right->X = x2;

                
                P_Left->countL = 1;
                dropouts++;
              }

              goto Skip_To_Next;
            }
          }

          ras.Proc_Sweep_Span( RAS_VARS y, x1, x2, P_Left, P_Right );

        Skip_To_Next:

          P_Left  = P_Left->link;
          P_Right = P_Right->link;
        }

        
        
        
        if ( dropouts > 0 )
          goto Scan_DropOuts;

      Next_Line:

        ras.Proc_Sweep_Step( RAS_VAR );

        y++;

        if ( y < y_change )
        {
          Sort( &draw_left  );
          Sort( &draw_right );
        }
      }

      

      P = draw_left;
      while ( P )
      {
        Q = P->link;
        if ( P->height == 0 )
          DelOld( &draw_left, P );
        P = Q;
      }

      P = draw_right;
      while ( P )
      {
        Q = P->link;
        if ( P->height == 0 )
          DelOld( &draw_right, P );
        P = Q;
      }
    }

    
    while ( y <= max_Y )
    {
      ras.Proc_Sweep_Step( RAS_VAR );
      y++;
    }

    return SUCCESS;

  Scan_DropOuts:

    P_Left  = draw_left;
    P_Right = draw_right;

    while ( P_Left )
    {
      if ( P_Left->countL )
      {
        P_Left->countL = 0;
#if 0
        dropouts--;  
#endif
        ras.Proc_Sweep_Drop( RAS_VARS y,
                                      P_Left->X,
                                      P_Right->X,
                                      P_Left,
                                      P_Right );
      }

      P_Left  = P_Left->link;
      P_Right = P_Right->link;
    }

    goto Next_Line;
  }

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  static int
  Render_Single_Pass( RAS_ARGS Bool  flipped )
  {
    Short  i, j, k;

    while ( ras.band_top >= 0 )
    {
      ras.maxY = (Long)ras.band_stack[ras.band_top].y_max * ras.precision;
      ras.minY = (Long)ras.band_stack[ras.band_top].y_min * ras.precision;

      ras.top = ras.buff;

      ras.error = Raster_Err_None;

      if ( Convert_Glyph( RAS_VARS flipped ) )
      {
        if ( ras.error != Raster_Err_Overflow )
          return FAILURE;

        ras.error = Raster_Err_None;

        

#ifdef DEBUG_RASTER
        ClearBand( RAS_VARS TRUNC( ras.minY ), TRUNC( ras.maxY ) );
#endif

        i = ras.band_stack[ras.band_top].y_min;
        j = ras.band_stack[ras.band_top].y_max;

        k = (Short)( ( i + j ) / 2 );

        if ( ras.band_top >= 7 || k < i )
        {
          ras.band_top = 0;
          ras.error    = Raster_Err_Invalid;

          return ras.error;
        }

        ras.band_stack[ras.band_top + 1].y_min = k;
        ras.band_stack[ras.band_top + 1].y_max = j;

        ras.band_stack[ras.band_top].y_max = (Short)( k - 1 );

        ras.band_top++;
      }
      else
      {
        if ( ras.fProfile )
          if ( Draw_Sweep( RAS_VAR ) )
             return ras.error;
        ras.band_top--;
      }
    }

    return SUCCESS;
  }

  
  
  
  
  
  
  
  
  
  
  
  FT_LOCAL_DEF( FT_Error )
  Render_Glyph( RAS_ARG )
  {
    FT_Error  error;

    Set_High_Precision( RAS_VARS ras.outline.flags &
                                 FT_OUTLINE_HIGH_PRECISION );
    ras.scale_shift = ras.precision_shift;

    if ( ras.outline.flags & FT_OUTLINE_IGNORE_DROPOUTS )
      ras.dropOutControl = 2;
    else
    {
      if ( ras.outline.flags & FT_OUTLINE_SMART_DROPOUTS )
        ras.dropOutControl = 4;
      else
        ras.dropOutControl = 0;

      if ( !( ras.outline.flags & FT_OUTLINE_INCLUDE_STUBS ) )
        ras.dropOutControl += 1;
    }

    ras.second_pass = (FT_Byte)( !( ras.outline.flags &
                                    FT_OUTLINE_SINGLE_PASS ) );

    
    ras.Proc_Sweep_Init = Vertical_Sweep_Init;
    ras.Proc_Sweep_Span = Vertical_Sweep_Span;
    ras.Proc_Sweep_Drop = Vertical_Sweep_Drop;
    ras.Proc_Sweep_Step = Vertical_Sweep_Step;

    ras.band_top            = 0;
    ras.band_stack[0].y_min = 0;
    ras.band_stack[0].y_max = (short)( ras.target.rows - 1 );

    ras.bWidth  = (unsigned short)ras.target.width;
    ras.bTarget = (Byte*)ras.target.buffer;

    if ( ( error = Render_Single_Pass( RAS_VARS 0 ) ) != 0 )
      return error;

    
    if ( ras.second_pass && ras.dropOutControl != 2 )
    {
      ras.Proc_Sweep_Init = Horizontal_Sweep_Init;
      ras.Proc_Sweep_Span = Horizontal_Sweep_Span;
      ras.Proc_Sweep_Drop = Horizontal_Sweep_Drop;
      ras.Proc_Sweep_Step = Horizontal_Sweep_Step;

      ras.band_top            = 0;
      ras.band_stack[0].y_min = 0;
      ras.band_stack[0].y_max = (short)( ras.target.width - 1 );

      if ( ( error = Render_Single_Pass( RAS_VARS 1 ) ) != 0 )
        return error;
    }

    return Raster_Err_None;
  }

#ifdef FT_RASTER_OPTION_ANTI_ALIASING

  
  
  
  
  
  
  
  
  
  
  
  FT_LOCAL_DEF( FT_Error )
  Render_Gray_Glyph( RAS_ARG )
  {
    Long      pixel_width;
    FT_Error  error;

    Set_High_Precision( RAS_VARS ras.outline.flags &
                                 FT_OUTLINE_HIGH_PRECISION );
    ras.scale_shift = ras.precision_shift + 1;

    if ( ras.outline.flags & FT_OUTLINE_IGNORE_DROPOUTS )
      ras.dropOutControl = 2;
    else
    {
      if ( ras.outline.flags & FT_OUTLINE_SMART_DROPOUTS )
        ras.dropOutControl = 4;
      else
        ras.dropOutControl = 0;

      if ( !( ras.outline.flags & FT_OUTLINE_INCLUDE_STUBS ) )
        ras.dropOutControl += 1;
    }

    ras.second_pass = !( ras.outline.flags & FT_OUTLINE_SINGLE_PASS );

    

    ras.band_top            = 0;
    ras.band_stack[0].y_min = 0;
    ras.band_stack[0].y_max = 2 * ras.target.rows - 1;

    ras.bWidth  = ras.gray_width;
    pixel_width = 2 * ( ( ras.target.width + 3 ) >> 2 );

    if ( ras.bWidth > pixel_width )
      ras.bWidth = pixel_width;

    ras.bWidth  = ras.bWidth * 8;
    ras.bTarget = (Byte*)ras.gray_lines;
    ras.gTarget = (Byte*)ras.target.buffer;

    ras.Proc_Sweep_Init = Vertical_Gray_Sweep_Init;
    ras.Proc_Sweep_Span = Vertical_Sweep_Span;
    ras.Proc_Sweep_Drop = Vertical_Sweep_Drop;
    ras.Proc_Sweep_Step = Vertical_Gray_Sweep_Step;

    error = Render_Single_Pass( RAS_VARS 0 );
    if ( error )
      return error;

    
    if ( ras.second_pass && ras.dropOutControl != 2 )
    {
      ras.Proc_Sweep_Init = Horizontal_Sweep_Init;
      ras.Proc_Sweep_Span = Horizontal_Gray_Sweep_Span;
      ras.Proc_Sweep_Drop = Horizontal_Gray_Sweep_Drop;
      ras.Proc_Sweep_Step = Horizontal_Sweep_Step;

      ras.band_top            = 0;
      ras.band_stack[0].y_min = 0;
      ras.band_stack[0].y_max = ras.target.width * 2 - 1;

      error = Render_Single_Pass( RAS_VARS 1 );
      if ( error )
        return error;
    }

    return Raster_Err_None;
  }

#else 

  FT_LOCAL_DEF( FT_Error )
  Render_Gray_Glyph( RAS_ARG )
  {
    FT_UNUSED_RASTER;

    return Raster_Err_Unsupported;
  }

#endif 

  static void
  ft_black_init( black_PRaster  raster )
  {
#ifdef FT_RASTER_OPTION_ANTI_ALIASING
    FT_UInt  n;

    
    for ( n = 0; n < 5; n++ )
      raster->grays[n] = n * 255 / 4;

    raster->gray_width = RASTER_GRAY_LINES / 2;
#else
    FT_UNUSED( raster );
#endif
  }

  
  

#ifdef _STANDALONE_

  static int
  ft_black_new( void*       memory,
                FT_Raster  *araster )
  {
     static black_TRaster  the_raster;
     FT_UNUSED( memory );

     *araster = (FT_Raster)&the_raster;
     FT_MEM_ZERO( &the_raster, sizeof ( the_raster ) );
     ft_black_init( &the_raster );

     return 0;
  }

  static void
  ft_black_done( FT_Raster  raster )
  {
    
    FT_UNUSED( raster );
  }

#else 

  static int
  ft_black_new( FT_Memory       memory,
                black_PRaster  *araster )
  {
    FT_Error       error;
    black_PRaster  raster = NULL;

    *araster = 0;
    if ( !FT_NEW( raster ) )
    {
      raster->memory = memory;
      ft_black_init( raster );

      *araster = raster;
    }

    return error;
  }

  static void
  ft_black_done( black_PRaster  raster )
  {
    FT_Memory  memory = (FT_Memory)raster->memory;

    FT_FREE( raster );
  }

#endif 

  static void
  ft_black_reset( black_PRaster  raster,
                  char*          pool_base,
                  long           pool_size )
  {
    if ( raster )
    {
      if ( pool_base && pool_size >= (long)sizeof ( black_TWorker ) + 2048 )
      {
        black_PWorker  worker = (black_PWorker)pool_base;

        raster->buffer      = pool_base + ( ( sizeof ( *worker ) + 7 ) & ~7 );
        raster->buffer_size = pool_base + pool_size - (char*)raster->buffer;
        raster->worker      = worker;
      }
      else
      {
        raster->buffer      = NULL;
        raster->buffer_size = 0;
        raster->worker      = NULL;
      }
    }
  }

  static void
  ft_black_set_mode( black_PRaster  raster,
                     unsigned long  mode,
                     const char*    palette )
  {
#ifdef FT_RASTER_OPTION_ANTI_ALIASING

    if ( mode == FT_MAKE_TAG( 'p', 'a', 'l', '5' ) )
    {
      
      raster->grays[0] = palette[0];
      raster->grays[1] = palette[1];
      raster->grays[2] = palette[2];
      raster->grays[3] = palette[3];
      raster->grays[4] = palette[4];
    }

#else

    FT_UNUSED( raster );
    FT_UNUSED( mode );
    FT_UNUSED( palette );

#endif
  }

  static int
  ft_black_render( black_PRaster            raster,
                   const FT_Raster_Params*  params )
  {
    const FT_Outline*  outline    = (const FT_Outline*)params->source;
    const FT_Bitmap*   target_map = params->target;
    black_PWorker      worker;

    if ( !raster || !raster->buffer || !raster->buffer_size )
      return Raster_Err_Not_Ini;

    if ( !outline )
      return Raster_Err_Invalid;

    
    if ( outline->n_points == 0 || outline->n_contours <= 0 )
      return Raster_Err_None;

    if ( !outline->contours || !outline->points )
      return Raster_Err_Invalid;

    if ( outline->n_points !=
           outline->contours[outline->n_contours - 1] + 1 )
      return Raster_Err_Invalid;

    worker = raster->worker;

    
    if ( params->flags & FT_RASTER_FLAG_DIRECT )
      return Raster_Err_Unsupported;

    if ( !target_map )
      return Raster_Err_Invalid;

    
    if ( !target_map->width || !target_map->rows )
      return Raster_Err_None;

    if ( !target_map->buffer )
      return Raster_Err_Invalid;

    ras.outline = *outline;
    ras.target  = *target_map;

    worker->buff       = (PLong) raster->buffer;
    worker->sizeBuff   = worker->buff +
                           raster->buffer_size / sizeof ( Long );
#ifdef FT_RASTER_OPTION_ANTI_ALIASING
    worker->grays      = raster->grays;
    worker->gray_width = raster->gray_width;

    FT_MEM_ZERO( worker->gray_lines, worker->gray_width * 2 );
#endif

    return ( params->flags & FT_RASTER_FLAG_AA )
           ? Render_Gray_Glyph( RAS_VAR )
           : Render_Glyph( RAS_VAR );
  }

  FT_DEFINE_RASTER_FUNCS( ft_standard_raster,
    FT_GLYPH_FORMAT_OUTLINE,
    (FT_Raster_New_Func)     ft_black_new,
    (FT_Raster_Reset_Func)   ft_black_reset,
    (FT_Raster_Set_Mode_Func)ft_black_set_mode,
    (FT_Raster_Render_Func)  ft_black_render,
    (FT_Raster_Done_Func)    ft_black_done
  )

