

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  
  
  
  
  
  
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_smooth

#ifdef _STANDALONE_

  

#ifdef FT_DEBUG_LEVEL_TRACE
#include <stdio.h>
#include <stdarg.h>
#endif

#include <stddef.h>
#include <string.h>
#include <setjmp.h>
#include <limits.h>
#define FT_UINT_MAX  UINT_MAX
#define FT_INT_MAX   INT_MAX

#define ft_memset   memset

#define ft_setjmp   setjmp
#define ft_longjmp  longjmp
#define ft_jmp_buf  jmp_buf

typedef ptrdiff_t  FT_PtrDist;

#define ErrRaster_Invalid_Mode      -2
#define ErrRaster_Invalid_Outline   -1
#define ErrRaster_Invalid_Argument  -3
#define ErrRaster_Memory_Overflow   -4

#define FT_BEGIN_HEADER
#define FT_END_HEADER

#include "ftimage.h"
#include "ftgrays.h"

  
  
  
  
#define FT_UNUSED( x )  (x) = (x)

  

#ifdef FT_DEBUG_LEVEL_TRACE

  void
  FT_Message( const char*  fmt,
              ... )
  {
    va_list  ap;

    va_start( ap, fmt );
    vfprintf( stderr, fmt, ap );
    va_end( ap );
  }

  
#ifndef FT_TRACE5
#define FT_TRACE5( varformat )  FT_Message varformat
#endif
#ifndef FT_TRACE7
#define FT_TRACE7( varformat )  FT_Message varformat
#endif
#ifndef FT_ERROR
#define FT_ERROR( varformat )   FT_Message varformat
#endif

#else 

#define FT_TRACE5( x )  do { } while ( 0 )     
#define FT_TRACE7( x )  do { } while ( 0 )     
#define FT_ERROR( x )   do { } while ( 0 )     

#endif 

#define FT_DEFINE_OUTLINE_FUNCS( class_,               \
                                 move_to_, line_to_,   \
                                 conic_to_, cubic_to_, \
                                 shift_, delta_ )      \
          static const FT_Outline_Funcs class_ =       \
          {                                            \
            move_to_,                                  \
            line_to_,                                  \
            conic_to_,                                 \
            cubic_to_,                                 \
            shift_,                                    \
            delta_                                     \
         };

#define FT_DEFINE_RASTER_FUNCS( class_, glyph_format_,            \
                                raster_new_, raster_reset_,       \
                                raster_set_mode_, raster_render_, \
                                raster_done_ )                    \
          const FT_Raster_Funcs class_ =                          \
          {                                                       \
            glyph_format_,                                        \
            raster_new_,                                          \
            raster_reset_,                                        \
            raster_set_mode_,                                     \
            raster_render_,                                       \
            raster_done_                                          \
         };

#else 

#include <ft2build.h>
#include "ftgrays.h"
#include FT_INTERNAL_OBJECTS_H
#include FT_INTERNAL_DEBUG_H
#include FT_OUTLINE_H

#include "ftsmerrs.h"

#include "ftspic.h"

#define ErrRaster_Invalid_Mode      Smooth_Err_Cannot_Render_Glyph
#define ErrRaster_Invalid_Outline   Smooth_Err_Invalid_Outline
#define ErrRaster_Memory_Overflow   Smooth_Err_Out_Of_Memory
#define ErrRaster_Invalid_Argument  Smooth_Err_Invalid_Argument

#endif 

#ifndef FT_MEM_SET
#define FT_MEM_SET( d, s, c )  ft_memset( d, s, c )
#endif

#ifndef FT_MEM_ZERO
#define FT_MEM_ZERO( dest, count )  FT_MEM_SET( dest, 0, count )
#endif

  

#undef RAS_ARG
#undef RAS_ARG_
#undef RAS_VAR
#undef RAS_VAR_

#ifndef FT_STATIC_RASTER

#define RAS_ARG   gray_PWorker  worker
#define RAS_ARG_  gray_PWorker  worker,

#define RAS_VAR   worker
#define RAS_VAR_  worker,

#else 

#define RAS_ARG   
#define RAS_ARG_  
#define RAS_VAR   
#define RAS_VAR_  

#endif 

  
#define PIXEL_BITS  8

#undef FLOOR
#undef CEILING
#undef TRUNC
#undef SCALED

#define ONE_PIXEL       ( 1L << PIXEL_BITS )
#define PIXEL_MASK      ( -1L << PIXEL_BITS )
#define TRUNC( x )      ( (TCoord)( (x) >> PIXEL_BITS ) )
#define SUBPIXELS( x )  ( (TPos)(x) << PIXEL_BITS )
#define FLOOR( x )      ( (x) & -ONE_PIXEL )
#define CEILING( x )    ( ( (x) + ONE_PIXEL - 1 ) & -ONE_PIXEL )
#define ROUND( x )      ( ( (x) + ONE_PIXEL / 2 ) & -ONE_PIXEL )

#if PIXEL_BITS >= 6
#define UPSCALE( x )    ( (x) << ( PIXEL_BITS - 6 ) )
#define DOWNSCALE( x )  ( (x) >> ( PIXEL_BITS - 6 ) )
#else
#define UPSCALE( x )    ( (x) >> ( 6 - PIXEL_BITS ) )
#define DOWNSCALE( x )  ( (x) << ( 6 - PIXEL_BITS ) )
#endif

  
  
  
  

  
  
  

  typedef long  TCoord;   
  typedef long  TPos;     

  
  
  

#if PIXEL_BITS <= 7

  typedef int  TArea;

#else 

  
#if FT_UINT_MAX == 0xFFFFU
  typedef long  TArea;
#else
  typedef int   TArea;
#endif

#endif 

  
#define FT_MAX_GRAY_SPANS  32

  typedef struct TCell_*  PCell;

  typedef struct  TCell_
  {
    TPos    x;     
    TCoord  cover; 
    TArea   area;
    PCell   next;

  } TCell;

  typedef struct  gray_TWorker_
  {
    TCoord  ex, ey;
    TPos    min_ex, max_ex;
    TPos    min_ey, max_ey;
    TPos    count_ex, count_ey;

    TArea   area;
    TCoord  cover;
    int     invalid;

    PCell       cells;
    FT_PtrDist  max_cells;
    FT_PtrDist  num_cells;

    TCoord  cx, cy;
    TPos    x,  y;

    TPos    last_ey;

    FT_Vector   bez_stack[32 * 3 + 1];
    int         lev_stack[32];

    FT_Outline  outline;
    FT_Bitmap   target;
    FT_BBox     clip_box;

    FT_Span     gray_spans[FT_MAX_GRAY_SPANS];
    int         num_gray_spans;

    FT_Raster_Span_Func  render_span;
    void*                render_span_data;
    int                  span_y;

    int  band_size;
    int  band_shoot;

    ft_jmp_buf  jump_buffer;

    void*       buffer;
    long        buffer_size;

    PCell*     ycells;
    TPos       ycount;

  } gray_TWorker, *gray_PWorker;

#ifndef FT_STATIC_RASTER
#define ras  (*worker)
#else
  static gray_TWorker  ras;
#endif

  typedef struct gray_TRaster_
  {
    void*         buffer;
    long          buffer_size;
    int           band_size;
    void*         memory;
    gray_PWorker  worker;

  } gray_TRaster, *gray_PRaster;

  
  
  
  
  static void
  gray_init_cells( RAS_ARG_ void*  buffer,
                   long            byte_size )
  {
    ras.buffer      = buffer;
    ras.buffer_size = byte_size;

    ras.ycells      = (PCell*) buffer;
    ras.cells       = NULL;
    ras.max_cells   = 0;
    ras.num_cells   = 0;
    ras.area        = 0;
    ras.cover       = 0;
    ras.invalid     = 1;
  }

  
  
  
  
  static void
  gray_compute_cbox( RAS_ARG )
  {
    FT_Outline*  outline = &ras.outline;
    FT_Vector*   vec     = outline->points;
    FT_Vector*   limit   = vec + outline->n_points;

    if ( outline->n_points <= 0 )
    {
      ras.min_ex = ras.max_ex = 0;
      ras.min_ey = ras.max_ey = 0;
      return;
    }

    ras.min_ex = ras.max_ex = vec->x;
    ras.min_ey = ras.max_ey = vec->y;

    vec++;

    for ( ; vec < limit; vec++ )
    {
      TPos  x = vec->x;
      TPos  y = vec->y;

      if ( x < ras.min_ex ) ras.min_ex = x;
      if ( x > ras.max_ex ) ras.max_ex = x;
      if ( y < ras.min_ey ) ras.min_ey = y;
      if ( y > ras.max_ey ) ras.max_ey = y;
    }

    
    ras.min_ex = ras.min_ex >> 6;
    ras.min_ey = ras.min_ey >> 6;
    ras.max_ex = ( ras.max_ex + 63 ) >> 6;
    ras.max_ey = ( ras.max_ey + 63 ) >> 6;
  }

  
  
  
  
  static PCell
  gray_find_cell( RAS_ARG )
  {
    PCell  *pcell, cell;
    TPos    x = ras.ex;

    if ( x > ras.count_ex )
      x = ras.count_ex;

    pcell = &ras.ycells[ras.ey];
    for (;;)
    {
      cell = *pcell;
      if ( cell == NULL || cell->x > x )
        break;

      if ( cell->x == x )
        goto Exit;

      pcell = &cell->next;
    }

    if ( ras.num_cells >= ras.max_cells )
      ft_longjmp( ras.jump_buffer, 1 );

    cell        = ras.cells + ras.num_cells++;
    cell->x     = x;
    cell->area  = 0;
    cell->cover = 0;

    cell->next  = *pcell;
    *pcell      = cell;

  Exit:
    return cell;
  }

  static void
  gray_record_cell( RAS_ARG )
  {
    if ( !ras.invalid && ( ras.area | ras.cover ) )
    {
      PCell  cell = gray_find_cell( RAS_VAR );

      cell->area  += ras.area;
      cell->cover += ras.cover;
    }
  }

  
  
  
  
  static void
  gray_set_cell( RAS_ARG_ TCoord  ex,
                          TCoord  ey )
  {
    
    
    
    
    
    
    
    
    

    
    
    ey -= ras.min_ey;

    if ( ex > ras.max_ex )
      ex = ras.max_ex;

    ex -= ras.min_ex;
    if ( ex < 0 )
      ex = -1;

    
    if ( ex != ras.ex || ey != ras.ey )
    {
      
      if ( !ras.invalid )
        gray_record_cell( RAS_VAR );

      ras.area  = 0;
      ras.cover = 0;
    }

    ras.ex      = ex;
    ras.ey      = ey;
    ras.invalid = ( (unsigned)ey >= (unsigned)ras.count_ey ||
                              ex >= ras.count_ex           );
  }

  
  
  
  
  static void
  gray_start_cell( RAS_ARG_ TCoord  ex,
                            TCoord  ey )
  {
    if ( ex > ras.max_ex )
      ex = (TCoord)( ras.max_ex );

    if ( ex < ras.min_ex )
      ex = (TCoord)( ras.min_ex - 1 );

    ras.area    = 0;
    ras.cover   = 0;
    ras.ex      = ex - ras.min_ex;
    ras.ey      = ey - ras.min_ey;
    ras.last_ey = SUBPIXELS( ey );
    ras.invalid = 0;

    gray_set_cell( RAS_VAR_ ex, ey );
  }

  
  
  
  
  static void
  gray_render_scanline( RAS_ARG_ TCoord  ey,
                                 TPos    x1,
                                 TCoord  y1,
                                 TPos    x2,
                                 TCoord  y2 )
  {
    TCoord  ex1, ex2, fx1, fx2, delta, mod, lift, rem;
    long    p, first, dx;
    int     incr;

    dx = x2 - x1;

    ex1 = TRUNC( x1 );
    ex2 = TRUNC( x2 );
    fx1 = (TCoord)( x1 - SUBPIXELS( ex1 ) );
    fx2 = (TCoord)( x2 - SUBPIXELS( ex2 ) );

    
    if ( y1 == y2 )
    {
      gray_set_cell( RAS_VAR_ ex2, ey );
      return;
    }

    
    
    if ( ex1 == ex2 )
    {
      delta      = y2 - y1;
      ras.area  += (TArea)(( fx1 + fx2 ) * delta);
      ras.cover += delta;
      return;
    }

    
    
    
    p     = ( ONE_PIXEL - fx1 ) * ( y2 - y1 );
    first = ONE_PIXEL;
    incr  = 1;

    if ( dx < 0 )
    {
      p     = fx1 * ( y2 - y1 );
      first = 0;
      incr  = -1;
      dx    = -dx;
    }

    delta = (TCoord)( p / dx );
    mod   = (TCoord)( p % dx );
    if ( mod < 0 )
    {
      delta--;
      mod += (TCoord)dx;
    }

    ras.area  += (TArea)(( fx1 + first ) * delta);
    ras.cover += delta;

    ex1 += incr;
    gray_set_cell( RAS_VAR_ ex1, ey );
    y1  += delta;

    if ( ex1 != ex2 )
    {
      p    = ONE_PIXEL * ( y2 - y1 + delta );
      lift = (TCoord)( p / dx );
      rem  = (TCoord)( p % dx );
      if ( rem < 0 )
      {
        lift--;
        rem += (TCoord)dx;
      }

      mod -= (int)dx;

      while ( ex1 != ex2 )
      {
        delta = lift;
        mod  += rem;
        if ( mod >= 0 )
        {
          mod -= (TCoord)dx;
          delta++;
        }

        ras.area  += (TArea)(ONE_PIXEL * delta);
        ras.cover += delta;
        y1        += delta;
        ex1       += incr;
        gray_set_cell( RAS_VAR_ ex1, ey );
      }
    }

    delta      = y2 - y1;
    ras.area  += (TArea)(( fx2 + ONE_PIXEL - first ) * delta);
    ras.cover += delta;
  }

  
  
  
  
  static void
  gray_render_line( RAS_ARG_ TPos  to_x,
                             TPos  to_y )
  {
    TCoord  ey1, ey2, fy1, fy2, mod;
    TPos    dx, dy, x, x2;
    long    p, first;
    int     delta, rem, lift, incr;

    ey1 = TRUNC( ras.last_ey );
    ey2 = TRUNC( to_y );     
    fy1 = (TCoord)( ras.y - ras.last_ey );
    fy2 = (TCoord)( to_y - SUBPIXELS( ey2 ) );

    dx = to_x - ras.x;
    dy = to_y - ras.y;

    
    

    
    {
      TCoord  min, max;

      min = ey1;
      max = ey2;
      if ( ey1 > ey2 )
      {
        min = ey2;
        max = ey1;
      }
      if ( min >= ras.max_ey || max < ras.min_ey )
        goto End;
    }

    
    if ( ey1 == ey2 )
    {
      gray_render_scanline( RAS_VAR_ ey1, ras.x, fy1, to_x, fy2 );
      goto End;
    }

    
    incr = 1;

    if ( dx == 0 )
    {
      TCoord  ex     = TRUNC( ras.x );
      TCoord  two_fx = (TCoord)( ( ras.x - SUBPIXELS( ex ) ) << 1 );
      TArea   area;

      first = ONE_PIXEL;
      if ( dy < 0 )
      {
        first = 0;
        incr  = -1;
      }

      delta      = (int)( first - fy1 );
      ras.area  += (TArea)two_fx * delta;
      ras.cover += delta;
      ey1       += incr;

      gray_set_cell( RAS_VAR_ ex, ey1 );

      delta = (int)( first + first - ONE_PIXEL );
      area  = (TArea)two_fx * delta;
      while ( ey1 != ey2 )
      {
        ras.area  += area;
        ras.cover += delta;
        ey1       += incr;

        gray_set_cell( RAS_VAR_ ex, ey1 );
      }

      delta      = (int)( fy2 - ONE_PIXEL + first );
      ras.area  += (TArea)two_fx * delta;
      ras.cover += delta;

      goto End;
    }

    
    p     = ( ONE_PIXEL - fy1 ) * dx;
    first = ONE_PIXEL;
    incr  = 1;

    if ( dy < 0 )
    {
      p     = fy1 * dx;
      first = 0;
      incr  = -1;
      dy    = -dy;
    }

    delta = (int)( p / dy );
    mod   = (int)( p % dy );
    if ( mod < 0 )
    {
      delta--;
      mod += (TCoord)dy;
    }

    x = ras.x + delta;
    gray_render_scanline( RAS_VAR_ ey1, ras.x, fy1, x, (TCoord)first );

    ey1 += incr;
    gray_set_cell( RAS_VAR_ TRUNC( x ), ey1 );

    if ( ey1 != ey2 )
    {
      p     = ONE_PIXEL * dx;
      lift  = (int)( p / dy );
      rem   = (int)( p % dy );
      if ( rem < 0 )
      {
        lift--;
        rem += (int)dy;
      }
      mod -= (int)dy;

      while ( ey1 != ey2 )
      {
        delta = lift;
        mod  += rem;
        if ( mod >= 0 )
        {
          mod -= (int)dy;
          delta++;
        }

        x2 = x + delta;
        gray_render_scanline( RAS_VAR_ ey1, x,
                                       (TCoord)( ONE_PIXEL - first ), x2,
                                       (TCoord)first );
        x = x2;

        ey1 += incr;
        gray_set_cell( RAS_VAR_ TRUNC( x ), ey1 );
      }
    }

    gray_render_scanline( RAS_VAR_ ey1, x,
                                   (TCoord)( ONE_PIXEL - first ), to_x,
                                   fy2 );

  End:
    ras.x       = to_x;
    ras.y       = to_y;
    ras.last_ey = SUBPIXELS( ey2 );
  }

  static void
  gray_split_conic( FT_Vector*  base )
  {
    TPos  a, b;

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
  gray_render_conic( RAS_ARG_ const FT_Vector*  control,
                              const FT_Vector*  to )
  {
    TPos        dx, dy;
    TPos        min, max, y;
    int         top, level;
    int*        levels;
    FT_Vector*  arc;

    levels = ras.lev_stack;

    arc      = ras.bez_stack;
    arc[0].x = UPSCALE( to->x );
    arc[0].y = UPSCALE( to->y );
    arc[1].x = UPSCALE( control->x );
    arc[1].y = UPSCALE( control->y );
    arc[2].x = ras.x;
    arc[2].y = ras.y;
    top      = 0;

    dx = FT_ABS( arc[2].x + arc[0].x - 2 * arc[1].x );
    dy = FT_ABS( arc[2].y + arc[0].y - 2 * arc[1].y );
    if ( dx < dy )
      dx = dy;

    if ( dx < ONE_PIXEL / 4 )
      goto Draw;

    
    min = max = arc[0].y;

    y = arc[1].y;
    if ( y < min ) min = y;
    if ( y > max ) max = y;

    y = arc[2].y;
    if ( y < min ) min = y;
    if ( y > max ) max = y;

    if ( TRUNC( min ) >= ras.max_ey || TRUNC( max ) < ras.min_ey )
      goto Draw;

    level = 0;
    do
    {
      dx >>= 2;
      level++;
    } while ( dx > ONE_PIXEL / 4 );

    levels[0] = level;

    do
    {
      level = levels[top];
      if ( level > 0 )
      {
        gray_split_conic( arc );
        arc += 2;
        top++;
        levels[top] = levels[top - 1] = level - 1;
        continue;
      }

    Draw:
      gray_render_line( RAS_VAR_ arc[0].x, arc[0].y );
      top--;
      arc -= 2;

    } while ( top >= 0 );
  }

  static void
  gray_split_cubic( FT_Vector*  base )
  {
    TPos  a, b, c, d;

    base[6].x = base[3].x;
    c = base[1].x;
    d = base[2].x;
    base[1].x = a = ( base[0].x + c ) / 2;
    base[5].x = b = ( base[3].x + d ) / 2;
    c = ( c + d ) / 2;
    base[2].x = a = ( a + c ) / 2;
    base[4].x = b = ( b + c ) / 2;
    base[3].x = ( a + b ) / 2;

    base[6].y = base[3].y;
    c = base[1].y;
    d = base[2].y;
    base[1].y = a = ( base[0].y + c ) / 2;
    base[5].y = b = ( base[3].y + d ) / 2;
    c = ( c + d ) / 2;
    base[2].y = a = ( a + c ) / 2;
    base[4].y = b = ( b + c ) / 2;
    base[3].y = ( a + b ) / 2;
  }

  static void
  gray_render_cubic( RAS_ARG_ const FT_Vector*  control1,
                              const FT_Vector*  control2,
                              const FT_Vector*  to )
  {
    FT_Vector*  arc;
    TPos        min, max, y;

    arc      = ras.bez_stack;
    arc[0].x = UPSCALE( to->x );
    arc[0].y = UPSCALE( to->y );
    arc[1].x = UPSCALE( control2->x );
    arc[1].y = UPSCALE( control2->y );
    arc[2].x = UPSCALE( control1->x );
    arc[2].y = UPSCALE( control1->y );
    arc[3].x = ras.x;
    arc[3].y = ras.y;

    
    min = max = arc[0].y;

    y = arc[1].y;
    if ( y < min )
      min = y;
    if ( y > max )
      max = y;

    y = arc[2].y;
    if ( y < min )
      min = y;
    if ( y > max )
      max = y;

    y = arc[3].y;
    if ( y < min )
      min = y;
    if ( y > max )
      max = y;

    if ( TRUNC( min ) >= ras.max_ey || TRUNC( max ) < ras.min_ey )
      goto Draw;

    for (;;)
    {
      
      
      
      

      {
        TPos  dx, dy, dx_, dy_;
        TPos  dx1, dy1, dx2, dy2;
        TPos  L, s, s_limit;

        
        dx = arc[3].x - arc[0].x;
        dy = arc[3].y - arc[0].y;

        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        

        dx_ = FT_ABS( dx );
        dy_ = FT_ABS( dy );

        
        
        
        
        L = ( dx_ > dy_ ? 236 * dx_ +  97 * dy_
                        :  97 * dx_ + 236 * dy_ ) >> 8;

        
        if ( L > 32767 )
          goto Split;

        
        s_limit = L * (TPos)( ONE_PIXEL / 6 );

        
        dx1 = arc[1].x - arc[0].x;
        dy1 = arc[1].y - arc[0].y;
        s = FT_ABS( dy * dx1 - dx * dy1 );

        if ( s > s_limit )
          goto Split;

        
        dx2 = arc[2].x - arc[0].x;
        dy2 = arc[2].y - arc[0].y;
        s = FT_ABS( dy * dx2 - dx * dy2 );

        if ( s > s_limit )
          goto Split;

        
        if ( dy * dy1 + dx * dx1 < 0                                     ||
             dy * dy2 + dx * dx2 < 0                                     ||
             dy * (arc[3].y - arc[1].y) + dx * (arc[3].x - arc[1].x) < 0 ||
             dy * (arc[3].y - arc[2].y) + dx * (arc[3].x - arc[2].x) < 0 )
          goto Split;

        
        goto Draw;
      }

    Split:
      gray_split_cubic( arc );
      arc += 3;
      continue;

    Draw:
      gray_render_line( RAS_VAR_ arc[0].x, arc[0].y );

      if ( arc == ras.bez_stack )
        return;

      arc -= 3;
    }
  }

  static int
  gray_move_to( const FT_Vector*  to,
                gray_PWorker      worker )
  {
    TPos  x, y;

    
    gray_record_cell( RAS_VAR );

    
    x = UPSCALE( to->x );
    y = UPSCALE( to->y );

    gray_start_cell( RAS_VAR_ TRUNC( x ), TRUNC( y ) );

    worker->x = x;
    worker->y = y;
    return 0;
  }

  static int
  gray_line_to( const FT_Vector*  to,
                gray_PWorker      worker )
  {
    gray_render_line( RAS_VAR_ UPSCALE( to->x ), UPSCALE( to->y ) );
    return 0;
  }

  static int
  gray_conic_to( const FT_Vector*  control,
                 const FT_Vector*  to,
                 gray_PWorker      worker )
  {
    gray_render_conic( RAS_VAR_ control, to );
    return 0;
  }

  static int
  gray_cubic_to( const FT_Vector*  control1,
                 const FT_Vector*  control2,
                 const FT_Vector*  to,
                 gray_PWorker      worker )
  {
    gray_render_cubic( RAS_VAR_ control1, control2, to );
    return 0;
  }

  static void
  gray_render_span( int             y,
                    int             count,
                    const FT_Span*  spans,
                    gray_PWorker    worker )
  {
    unsigned char*  p;
    FT_Bitmap*      map = &worker->target;

    
    p = (unsigned char*)map->buffer - y * map->pitch;
    if ( map->pitch >= 0 )
      p += (unsigned)( ( map->rows - 1 ) * map->pitch );

    for ( ; count > 0; count--, spans++ )
    {
      unsigned char  coverage = spans->coverage;

      if ( coverage )
      {
        
        if ( spans->len >= 8 )
          FT_MEM_SET( p + spans->x, (unsigned char)coverage, spans->len );
        else
        {
          unsigned char*  q = p + spans->x;

          switch ( spans->len )
          {
          case 7: *q++ = (unsigned char)coverage;
          case 6: *q++ = (unsigned char)coverage;
          case 5: *q++ = (unsigned char)coverage;
          case 4: *q++ = (unsigned char)coverage;
          case 3: *q++ = (unsigned char)coverage;
          case 2: *q++ = (unsigned char)coverage;
          case 1: *q   = (unsigned char)coverage;
          default:
            ;
          }
        }
      }
    }
  }

  static void
  gray_hline( RAS_ARG_ TCoord  x,
                       TCoord  y,
                       TPos    area,
                       TCoord  acount )
  {
    FT_Span*  span;
    int       count;
    int       coverage;

    
    
    
    
    
    coverage = (int)( area >> ( PIXEL_BITS * 2 + 1 - 8 ) );
                                                    
    if ( coverage < 0 )
      coverage = -coverage;

    if ( ras.outline.flags & FT_OUTLINE_EVEN_ODD_FILL )
    {
      coverage &= 511;

      if ( coverage > 256 )
        coverage = 512 - coverage;
      else if ( coverage == 256 )
        coverage = 255;
    }
    else
    {
      
      if ( coverage >= 256 )
        coverage = 255;
    }

    y += (TCoord)ras.min_ey;
    x += (TCoord)ras.min_ex;

    
    if ( x >= 32767 )
      x = 32767;

    
    if ( y >= FT_INT_MAX )
      y = FT_INT_MAX;

    if ( coverage )
    {
      
      count = ras.num_gray_spans;
      span  = ras.gray_spans + count - 1;
      if ( count > 0                          &&
           ras.span_y == y                    &&
           (int)span->x + span->len == (int)x &&
           span->coverage == coverage         )
      {
        span->len = (unsigned short)( span->len + acount );
        return;
      }

      if ( ras.span_y != y || count >= FT_MAX_GRAY_SPANS )
      {
        if ( ras.render_span && count > 0 )
          ras.render_span( ras.span_y, count, ras.gray_spans,
                           ras.render_span_data );

#ifdef FT_DEBUG_LEVEL_TRACE

        if ( count > 0 )
        {
          int  n;

          FT_TRACE7(( "y = %3d ", ras.span_y ));
          span = ras.gray_spans;
          for ( n = 0; n < count; n++, span++ )
            FT_TRACE7(( "[%d..%d]:%02x ",
                        span->x, span->x + span->len - 1, span->coverage ));
          FT_TRACE7(( "\n" ));
        }

#endif 

        ras.num_gray_spans = 0;
        ras.span_y         = (int)y;

        count = 0;
        span  = ras.gray_spans;
      }
      else
        span++;

      
      span->x        = (short)x;
      span->len      = (unsigned short)acount;
      span->coverage = (unsigned char)coverage;

      ras.num_gray_spans++;
    }
  }

#ifdef FT_DEBUG_LEVEL_TRACE

  
  
  static void
  gray_dump_cells( RAS_ARG )
  {
    int  yindex;

    for ( yindex = 0; yindex < ras.ycount; yindex++ )
    {
      PCell  cell;

      printf( "%3d:", yindex );

      for ( cell = ras.ycells[yindex]; cell != NULL; cell = cell->next )
        printf( " (%3ld, c:%4ld, a:%6d)", cell->x, cell->cover, cell->area );
      printf( "\n" );
    }
  }

#endif 

  static void
  gray_sweep( RAS_ARG_ const FT_Bitmap*  target )
  {
    int  yindex;

    FT_UNUSED( target );

    if ( ras.num_cells == 0 )
      return;

    ras.num_gray_spans = 0;

    FT_TRACE7(( "gray_sweep: start\n" ));

    for ( yindex = 0; yindex < ras.ycount; yindex++ )
    {
      PCell   cell  = ras.ycells[yindex];
      TCoord  cover = 0;
      TCoord  x     = 0;

      for ( ; cell != NULL; cell = cell->next )
      {
        TPos  area;

        if ( cell->x > x && cover != 0 )
          gray_hline( RAS_VAR_ x, yindex, cover * ( ONE_PIXEL * 2 ),
                      cell->x - x );

        cover += cell->cover;
        area   = cover * ( ONE_PIXEL * 2 ) - cell->area;

        if ( area != 0 && cell->x >= 0 )
          gray_hline( RAS_VAR_ cell->x, yindex, area, 1 );

        x = cell->x + 1;
      }

      if ( cover != 0 )
        gray_hline( RAS_VAR_ x, yindex, cover * ( ONE_PIXEL * 2 ),
                    ras.count_ex - x );
    }

    if ( ras.render_span && ras.num_gray_spans > 0 )
      ras.render_span( ras.span_y, ras.num_gray_spans,
                       ras.gray_spans, ras.render_span_data );

    FT_TRACE7(( "gray_sweep: end\n" ));
  }

#ifdef _STANDALONE_

  
  
  
  
  
  

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  static int
  FT_Outline_Decompose( const FT_Outline*        outline,
                        const FT_Outline_Funcs*  func_interface,
                        void*                    user )
  {
#undef SCALED
#define SCALED( x )  ( ( (x) << shift ) - delta )

    FT_Vector   v_last;
    FT_Vector   v_control;
    FT_Vector   v_start;

    FT_Vector*  point;
    FT_Vector*  limit;
    char*       tags;

    int         error;

    int   n;         
    int   first;     
    char  tag;       

    int   shift;
    TPos  delta;

    if ( !outline || !func_interface )
      return ErrRaster_Invalid_Argument;

    shift = func_interface->shift;
    delta = func_interface->delta;
    first = 0;

    for ( n = 0; n < outline->n_contours; n++ )
    {
      int  last;  

      FT_TRACE5(( "FT_Outline_Decompose: Outline %d\n", n ));

      last  = outline->contours[n];
      if ( last < 0 )
        goto Invalid_Outline;
      limit = outline->points + last;

      v_start   = outline->points[first];
      v_start.x = SCALED( v_start.x );
      v_start.y = SCALED( v_start.y );

      v_last   = outline->points[last];
      v_last.x = SCALED( v_last.x );
      v_last.y = SCALED( v_last.y );

      v_control = v_start;

      point = outline->points + first;
      tags  = outline->tags   + first;
      tag   = FT_CURVE_TAG( tags[0] );

      
      if ( tag == FT_CURVE_TAG_CUBIC )
        goto Invalid_Outline;

      
      if ( tag == FT_CURVE_TAG_CONIC )
      {
        
        if ( FT_CURVE_TAG( outline->tags[last] ) == FT_CURVE_TAG_ON )
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

      FT_TRACE5(( "  move to (%.2f, %.2f)\n",
                  v_start.x / 64.0, v_start.y / 64.0 ));
      error = func_interface->move_to( &v_start, user );
      if ( error )
        goto Exit;

      while ( point < limit )
      {
        point++;
        tags++;

        tag = FT_CURVE_TAG( tags[0] );
        switch ( tag )
        {
        case FT_CURVE_TAG_ON:  
          {
            FT_Vector  vec;

            vec.x = SCALED( point->x );
            vec.y = SCALED( point->y );

            FT_TRACE5(( "  line to (%.2f, %.2f)\n",
                        vec.x / 64.0, vec.y / 64.0 ));
            error = func_interface->line_to( &vec, user );
            if ( error )
              goto Exit;
            continue;
          }

        case FT_CURVE_TAG_CONIC:  
          v_control.x = SCALED( point->x );
          v_control.y = SCALED( point->y );

        Do_Conic:
          if ( point < limit )
          {
            FT_Vector  vec;
            FT_Vector  v_middle;

            point++;
            tags++;
            tag = FT_CURVE_TAG( tags[0] );

            vec.x = SCALED( point->x );
            vec.y = SCALED( point->y );

            if ( tag == FT_CURVE_TAG_ON )
            {
              FT_TRACE5(( "  conic to (%.2f, %.2f)"
                          " with control (%.2f, %.2f)\n",
                          vec.x / 64.0, vec.y / 64.0,
                          v_control.x / 64.0, v_control.y / 64.0 ));
              error = func_interface->conic_to( &v_control, &vec, user );
              if ( error )
                goto Exit;
              continue;
            }

            if ( tag != FT_CURVE_TAG_CONIC )
              goto Invalid_Outline;

            v_middle.x = ( v_control.x + vec.x ) / 2;
            v_middle.y = ( v_control.y + vec.y ) / 2;

            FT_TRACE5(( "  conic to (%.2f, %.2f)"
                        " with control (%.2f, %.2f)\n",
                        v_middle.x / 64.0, v_middle.y / 64.0,
                        v_control.x / 64.0, v_control.y / 64.0 ));
            error = func_interface->conic_to( &v_control, &v_middle, user );
            if ( error )
              goto Exit;

            v_control = vec;
            goto Do_Conic;
          }

          FT_TRACE5(( "  conic to (%.2f, %.2f)"
                      " with control (%.2f, %.2f)\n",
                      v_start.x / 64.0, v_start.y / 64.0,
                      v_control.x / 64.0, v_control.y / 64.0 ));
          error = func_interface->conic_to( &v_control, &v_start, user );
          goto Close;

        default:  
          {
            FT_Vector  vec1, vec2;

            if ( point + 1 > limit                             ||
                 FT_CURVE_TAG( tags[1] ) != FT_CURVE_TAG_CUBIC )
              goto Invalid_Outline;

            point += 2;
            tags  += 2;

            vec1.x = SCALED( point[-2].x );
            vec1.y = SCALED( point[-2].y );

            vec2.x = SCALED( point[-1].x );
            vec2.y = SCALED( point[-1].y );

            if ( point <= limit )
            {
              FT_Vector  vec;

              vec.x = SCALED( point->x );
              vec.y = SCALED( point->y );

              FT_TRACE5(( "  cubic to (%.2f, %.2f)"
                          " with controls (%.2f, %.2f) and (%.2f, %.2f)\n",
                          vec.x / 64.0, vec.y / 64.0,
                          vec1.x / 64.0, vec1.y / 64.0,
                          vec2.x / 64.0, vec2.y / 64.0 ));
              error = func_interface->cubic_to( &vec1, &vec2, &vec, user );
              if ( error )
                goto Exit;
              continue;
            }

            FT_TRACE5(( "  cubic to (%.2f, %.2f)"
                        " with controls (%.2f, %.2f) and (%.2f, %.2f)\n",
                        v_start.x / 64.0, v_start.y / 64.0,
                        vec1.x / 64.0, vec1.y / 64.0,
                        vec2.x / 64.0, vec2.y / 64.0 ));
            error = func_interface->cubic_to( &vec1, &vec2, &v_start, user );
            goto Close;
          }
        }
      }

      
      FT_TRACE5(( "  line to (%.2f, %.2f)\n",
                  v_start.x / 64.0, v_start.y / 64.0 ));
      error = func_interface->line_to( &v_start, user );

   Close:
      if ( error )
        goto Exit;

      first = last + 1;
    }

    FT_TRACE5(( "FT_Outline_Decompose: Done\n", n ));
    return 0;

  Exit:
    FT_TRACE5(( "FT_Outline_Decompose: Error %d\n", error ));
    return error;

  Invalid_Outline:
    return ErrRaster_Invalid_Outline;
  }

#endif 

  typedef struct  gray_TBand_
  {
    TPos  min, max;

  } gray_TBand;

    FT_DEFINE_OUTLINE_FUNCS(func_interface,
      (FT_Outline_MoveTo_Func) gray_move_to,
      (FT_Outline_LineTo_Func) gray_line_to,
      (FT_Outline_ConicTo_Func)gray_conic_to,
      (FT_Outline_CubicTo_Func)gray_cubic_to,
      0,
      0
    )

  static int
  gray_convert_glyph_inner( RAS_ARG )
  {

    volatile int  error = 0;

#ifdef FT_CONFIG_OPTION_PIC
      FT_Outline_Funcs func_interface;
      Init_Class_func_interface(&func_interface);
#endif

    if ( ft_setjmp( ras.jump_buffer ) == 0 )
    {
      error = FT_Outline_Decompose( &ras.outline, &func_interface, &ras );
      gray_record_cell( RAS_VAR );
    }
    else
      error = ErrRaster_Memory_Overflow;

    return error;
  }

  static int
  gray_convert_glyph( RAS_ARG )
  {
    gray_TBand            bands[40];
    gray_TBand* volatile  band;
    int volatile          n, num_bands;
    TPos volatile         min, max, max_y;
    FT_BBox*              clip;

    
    gray_compute_cbox( RAS_VAR );

    
    clip = &ras.clip_box;

    if ( ras.max_ex <= clip->xMin || ras.min_ex >= clip->xMax ||
         ras.max_ey <= clip->yMin || ras.min_ey >= clip->yMax )
      return 0;

    if ( ras.min_ex < clip->xMin ) ras.min_ex = clip->xMin;
    if ( ras.min_ey < clip->yMin ) ras.min_ey = clip->yMin;

    if ( ras.max_ex > clip->xMax ) ras.max_ex = clip->xMax;
    if ( ras.max_ey > clip->yMax ) ras.max_ey = clip->yMax;

    ras.count_ex = ras.max_ex - ras.min_ex;
    ras.count_ey = ras.max_ey - ras.min_ey;

    
    num_bands = (int)( ( ras.max_ey - ras.min_ey ) / ras.band_size );
    if ( num_bands == 0 )
      num_bands = 1;
    if ( num_bands >= 39 )
      num_bands = 39;

    ras.band_shoot = 0;

    min   = ras.min_ey;
    max_y = ras.max_ey;

    for ( n = 0; n < num_bands; n++, min = max )
    {
      max = min + ras.band_size;
      if ( n == num_bands - 1 || max > max_y )
        max = max_y;

      bands[0].min = min;
      bands[0].max = max;
      band         = bands;

      while ( band >= bands )
      {
        TPos  bottom, top, middle;
        int   error;

        {
          PCell  cells_max;
          int    yindex;
          long   cell_start, cell_end, cell_mod;

          ras.ycells = (PCell*)ras.buffer;
          ras.ycount = band->max - band->min;

          cell_start = sizeof ( PCell ) * ras.ycount;
          cell_mod   = cell_start % sizeof ( TCell );
          if ( cell_mod > 0 )
            cell_start += sizeof ( TCell ) - cell_mod;

          cell_end  = ras.buffer_size;
          cell_end -= cell_end % sizeof ( TCell );

          cells_max = (PCell)( (char*)ras.buffer + cell_end );
          ras.cells = (PCell)( (char*)ras.buffer + cell_start );
          if ( ras.cells >= cells_max )
            goto ReduceBands;

          ras.max_cells = cells_max - ras.cells;
          if ( ras.max_cells < 2 )
            goto ReduceBands;

          for ( yindex = 0; yindex < ras.ycount; yindex++ )
            ras.ycells[yindex] = NULL;
        }

        ras.num_cells = 0;
        ras.invalid   = 1;
        ras.min_ey    = band->min;
        ras.max_ey    = band->max;
        ras.count_ey  = band->max - band->min;

        error = gray_convert_glyph_inner( RAS_VAR );

        if ( !error )
        {
          gray_sweep( RAS_VAR_ &ras.target );
          band--;
          continue;
        }
        else if ( error != ErrRaster_Memory_Overflow )
          return 1;

      ReduceBands:
        
        bottom = band->min;
        top    = band->max;
        middle = bottom + ( ( top - bottom ) >> 1 );

        
        
        if ( middle == bottom )
        {
#ifdef FT_DEBUG_LEVEL_TRACE
          FT_TRACE7(( "gray_convert_glyph: rotten glyph\n" ));
#endif
          return 1;
        }

        if ( bottom-top >= ras.band_size )
          ras.band_shoot++;

        band[1].min = bottom;
        band[1].max = middle;
        band[0].min = middle;
        band[0].max = top;
        band++;
      }
    }

    if ( ras.band_shoot > 8 && ras.band_size > 16 )
      ras.band_size = ras.band_size / 2;

    return 0;
  }

  static int
  gray_raster_render( gray_PRaster             raster,
                      const FT_Raster_Params*  params )
  {
    const FT_Outline*  outline    = (const FT_Outline*)params->source;
    const FT_Bitmap*   target_map = params->target;
    gray_PWorker       worker;

    if ( !raster || !raster->buffer || !raster->buffer_size )
      return ErrRaster_Invalid_Argument;

    if ( !outline )
      return ErrRaster_Invalid_Outline;

    
    if ( outline->n_points == 0 || outline->n_contours <= 0 )
      return 0;

    if ( !outline->contours || !outline->points )
      return ErrRaster_Invalid_Outline;

    if ( outline->n_points !=
           outline->contours[outline->n_contours - 1] + 1 )
      return ErrRaster_Invalid_Outline;

    worker = raster->worker;

    
    if ( !( params->flags & FT_RASTER_FLAG_DIRECT ) )
    {
      if ( !target_map )
        return ErrRaster_Invalid_Argument;

      
      if ( !target_map->width || !target_map->rows )
        return 0;

      if ( !target_map->buffer )
        return ErrRaster_Invalid_Argument;
    }

    
    if ( !( params->flags & FT_RASTER_FLAG_AA ) )
      return ErrRaster_Invalid_Mode;

    
    if ( !( params->flags & FT_RASTER_FLAG_DIRECT ) )
    {
      
      ras.clip_box.xMin = 0;
      ras.clip_box.yMin = 0;
      ras.clip_box.xMax = target_map->width;
      ras.clip_box.yMax = target_map->rows;
    }
    else if ( params->flags & FT_RASTER_FLAG_CLIP )
      ras.clip_box = params->clip_box;
    else
    {
      ras.clip_box.xMin = -32768L;
      ras.clip_box.yMin = -32768L;
      ras.clip_box.xMax =  32767L;
      ras.clip_box.yMax =  32767L;
    }

    gray_init_cells( RAS_VAR_ raster->buffer, raster->buffer_size );

    ras.outline        = *outline;
    ras.num_cells      = 0;
    ras.invalid        = 1;
    ras.band_size      = raster->band_size;
    ras.num_gray_spans = 0;

    if ( params->flags & FT_RASTER_FLAG_DIRECT )
    {
      ras.render_span      = (FT_Raster_Span_Func)params->gray_spans;
      ras.render_span_data = params->user;
    }
    else
    {
      ras.target           = *target_map;
      ras.render_span      = (FT_Raster_Span_Func)gray_render_span;
      ras.render_span_data = &ras;
    }

    return gray_convert_glyph( RAS_VAR );
  }

  
  

#ifdef _STANDALONE_

  static int
  gray_raster_new( void*       memory,
                   FT_Raster*  araster )
  {
    static gray_TRaster  the_raster;

    FT_UNUSED( memory );

    *araster = (FT_Raster)&the_raster;
    FT_MEM_ZERO( &the_raster, sizeof ( the_raster ) );

    return 0;
  }

  static void
  gray_raster_done( FT_Raster  raster )
  {
    
    FT_UNUSED( raster );
  }

#else 

  static int
  gray_raster_new( FT_Memory   memory,
                   FT_Raster*  araster )
  {
    FT_Error      error;
    gray_PRaster  raster = NULL;

    *araster = 0;
    if ( !FT_ALLOC( raster, sizeof ( gray_TRaster ) ) )
    {
      raster->memory = memory;
      *araster       = (FT_Raster)raster;
    }

    return error;
  }

  static void
  gray_raster_done( FT_Raster  raster )
  {
    FT_Memory  memory = (FT_Memory)((gray_PRaster)raster)->memory;

    FT_FREE( raster );
  }

#endif 

  static void
  gray_raster_reset( FT_Raster  raster,
                     char*      pool_base,
                     long       pool_size )
  {
    gray_PRaster  rast = (gray_PRaster)raster;

    if ( raster )
    {
      if ( pool_base && pool_size >= (long)sizeof ( gray_TWorker ) + 2048 )
      {
        gray_PWorker  worker = (gray_PWorker)pool_base;

        rast->worker      = worker;
        rast->buffer      = pool_base +
                              ( ( sizeof ( gray_TWorker ) +
                                  sizeof ( TCell ) - 1 )  &
                                ~( sizeof ( TCell ) - 1 ) );
        rast->buffer_size = (long)( ( pool_base + pool_size ) -
                                    (char*)rast->buffer ) &
                                      ~( sizeof ( TCell ) - 1 );
        rast->band_size   = (int)( rast->buffer_size /
                                     ( sizeof ( TCell ) * 8 ) );
      }
      else
      {
        rast->buffer      = NULL;
        rast->buffer_size = 0;
        rast->worker      = NULL;
      }
    }
  }

  FT_DEFINE_RASTER_FUNCS(ft_grays_raster,
    FT_GLYPH_FORMAT_OUTLINE,

    (FT_Raster_New_Func)     gray_raster_new,
    (FT_Raster_Reset_Func)   gray_raster_reset,
    (FT_Raster_Set_Mode_Func)0,
    (FT_Raster_Render_Func)  gray_raster_render,
    (FT_Raster_Done_Func)    gray_raster_done
  )

