

#include <ft2build.h>
#include FT_STROKER_H
#include FT_TRIGONOMETRY_H
#include FT_OUTLINE_H
#include FT_INTERNAL_MEMORY_H
#include FT_INTERNAL_DEBUG_H
#include FT_INTERNAL_OBJECTS_H

  

  FT_EXPORT_DEF( FT_StrokerBorder )
  FT_Outline_GetInsideBorder( FT_Outline*  outline )
  {
    FT_Orientation  o = FT_Outline_Get_Orientation( outline );

    return o == FT_ORIENTATION_TRUETYPE ? FT_STROKER_BORDER_RIGHT
                                        : FT_STROKER_BORDER_LEFT;
  }

  

  FT_EXPORT_DEF( FT_StrokerBorder )
  FT_Outline_GetOutsideBorder( FT_Outline*  outline )
  {
    FT_Orientation  o = FT_Outline_Get_Orientation( outline );

    return o == FT_ORIENTATION_TRUETYPE ? FT_STROKER_BORDER_LEFT
                                        : FT_STROKER_BORDER_RIGHT;
  }

  
  
  
  
  
  
  

#define FT_SMALL_CONIC_THRESHOLD  ( FT_ANGLE_PI / 6 )
#define FT_SMALL_CUBIC_THRESHOLD  ( FT_ANGLE_PI / 8 )

#define FT_EPSILON  2

#define FT_IS_SMALL( x )  ( (x) > -FT_EPSILON && (x) < FT_EPSILON )

  static FT_Pos
  ft_pos_abs( FT_Pos  x )
  {
    return x >= 0 ? x : -x;
  }

  static void
  ft_conic_split( FT_Vector*  base )
  {
    FT_Pos  a, b;

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

  static FT_Bool
  ft_conic_is_small_enough( FT_Vector*  base,
                            FT_Angle   *angle_in,
                            FT_Angle   *angle_out )
  {
    FT_Vector  d1, d2;
    FT_Angle   theta;
    FT_Int     close1, close2;

    d1.x = base[1].x - base[2].x;
    d1.y = base[1].y - base[2].y;
    d2.x = base[0].x - base[1].x;
    d2.y = base[0].y - base[1].y;

    close1 = FT_IS_SMALL( d1.x ) && FT_IS_SMALL( d1.y );
    close2 = FT_IS_SMALL( d2.x ) && FT_IS_SMALL( d2.y );

    if ( close1 )
    {
      if ( close2 )
      {
        
        
      }
      else
      {
        *angle_in  =
        *angle_out = FT_Atan2( d2.x, d2.y );
      }
    }
    else 
    {
      if ( close2 )
      {
        *angle_in  =
        *angle_out = FT_Atan2( d1.x, d1.y );
      }
      else
      {
        *angle_in  = FT_Atan2( d1.x, d1.y );
        *angle_out = FT_Atan2( d2.x, d2.y );
      }
    }

    theta = ft_pos_abs( FT_Angle_Diff( *angle_in, *angle_out ) );

    return FT_BOOL( theta < FT_SMALL_CONIC_THRESHOLD );
  }

  static void
  ft_cubic_split( FT_Vector*  base )
  {
    FT_Pos  a, b, c, d;

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

  
  
  
  static FT_Angle
  ft_angle_mean( FT_Angle  angle1,
                 FT_Angle  angle2 )
  {
    return angle1 + FT_Angle_Diff( angle1, angle2 ) / 2;
  }

  static FT_Bool
  ft_cubic_is_small_enough( FT_Vector*  base,
                            FT_Angle   *angle_in,
                            FT_Angle   *angle_mid,
                            FT_Angle   *angle_out )
  {
    FT_Vector  d1, d2, d3;
    FT_Angle   theta1, theta2;
    FT_Int     close1, close2, close3;

    d1.x = base[2].x - base[3].x;
    d1.y = base[2].y - base[3].y;
    d2.x = base[1].x - base[2].x;
    d2.y = base[1].y - base[2].y;
    d3.x = base[0].x - base[1].x;
    d3.y = base[0].y - base[1].y;

    close1 = FT_IS_SMALL( d1.x ) && FT_IS_SMALL( d1.y );
    close2 = FT_IS_SMALL( d2.x ) && FT_IS_SMALL( d2.y );
    close3 = FT_IS_SMALL( d3.x ) && FT_IS_SMALL( d3.y );

    if ( close1 )
    {
      if ( close2 )
      {
        if ( close3 )
        {
          
          
        }
        else 
        {
          *angle_in  =
          *angle_mid =
          *angle_out = FT_Atan2( d3.x, d3.y );
        }
      }
      else 
      {
        if ( close3 )
        {
          *angle_in  =
          *angle_mid =
          *angle_out = FT_Atan2( d2.x, d2.y );
        }
        else 
        {
          *angle_in  =
          *angle_mid = FT_Atan2( d2.x, d2.y );
          *angle_out = FT_Atan2( d3.x, d3.y );
        }
      }
    }
    else 
    {
      if ( close2 )
      {
        if ( close3 )
        {
          *angle_in  =
          *angle_mid =
          *angle_out = FT_Atan2( d1.x, d1.y );
        }
        else 
        {
          *angle_in  = FT_Atan2( d1.x, d1.y );
          *angle_out = FT_Atan2( d3.x, d3.y );
          *angle_mid = ft_angle_mean( *angle_in, *angle_out );
        }
      }
      else 
      {
        if ( close3 )
        {
          *angle_in  = FT_Atan2( d1.x, d1.y );
          *angle_mid =
          *angle_out = FT_Atan2( d2.x, d2.y );
        }
        else 
        {
          *angle_in  = FT_Atan2( d1.x, d1.y );
          *angle_mid = FT_Atan2( d2.x, d2.y );
          *angle_out = FT_Atan2( d3.x, d3.y );
        }
      }
    }

    theta1 = ft_pos_abs( FT_Angle_Diff( *angle_in,  *angle_mid ) );
    theta2 = ft_pos_abs( FT_Angle_Diff( *angle_mid, *angle_out ) );

    return FT_BOOL( theta1 < FT_SMALL_CUBIC_THRESHOLD &&
                    theta2 < FT_SMALL_CUBIC_THRESHOLD );
  }

  
  
  
  
  
  
  

  typedef enum  FT_StrokeTags_
  {
    FT_STROKE_TAG_ON    = 1,   
    FT_STROKE_TAG_CUBIC = 2,   
    FT_STROKE_TAG_BEGIN = 4,   
    FT_STROKE_TAG_END   = 8    

  } FT_StrokeTags;

#define  FT_STROKE_TAG_BEGIN_END  ( FT_STROKE_TAG_BEGIN | FT_STROKE_TAG_END )

  typedef struct  FT_StrokeBorderRec_
  {
    FT_UInt     num_points;
    FT_UInt     max_points;
    FT_Vector*  points;
    FT_Byte*    tags;
    FT_Bool     movable;  
    FT_Int      start;    
    FT_Memory   memory;
    FT_Bool     valid;

  } FT_StrokeBorderRec, *FT_StrokeBorder;

  static FT_Error
  ft_stroke_border_grow( FT_StrokeBorder  border,
                         FT_UInt          new_points )
  {
    FT_UInt   old_max = border->max_points;
    FT_UInt   new_max = border->num_points + new_points;
    FT_Error  error   = FT_Err_Ok;

    if ( new_max > old_max )
    {
      FT_UInt    cur_max = old_max;
      FT_Memory  memory  = border->memory;

      while ( cur_max < new_max )
        cur_max += ( cur_max >> 1 ) + 16;

      if ( FT_RENEW_ARRAY( border->points, old_max, cur_max ) ||
           FT_RENEW_ARRAY( border->tags,   old_max, cur_max ) )
        goto Exit;

      border->max_points = cur_max;
    }

  Exit:
    return error;
  }

  static void
  ft_stroke_border_close( FT_StrokeBorder  border,
                          FT_Bool          reverse )
  {
    FT_UInt  start = border->start;
    FT_UInt  count = border->num_points;

    FT_ASSERT( border->start >= 0 );

    
    if ( count <= start + 1U )
      border->num_points = start;
    else
    {
      
      
      border->num_points    = --count;
      border->points[start] = border->points[count];

      if ( reverse )
      {
        
        {
          FT_Vector*  vec1 = border->points + start + 1;
          FT_Vector*  vec2 = border->points + count - 1;

          for ( ; vec1 < vec2; vec1++, vec2-- )
          {
            FT_Vector  tmp;

            tmp   = *vec1;
            *vec1 = *vec2;
            *vec2 = tmp;
          }
        }

        
        {
          FT_Byte*  tag1 = border->tags + start + 1;
          FT_Byte*  tag2 = border->tags + count - 1;

          for ( ; tag1 < tag2; tag1++, tag2-- )
          {
            FT_Byte  tmp;

            tmp   = *tag1;
            *tag1 = *tag2;
            *tag2 = tmp;
          }
        }
      }

      border->tags[start    ] |= FT_STROKE_TAG_BEGIN;
      border->tags[count - 1] |= FT_STROKE_TAG_END;
    }

    border->start   = -1;
    border->movable = FALSE;
  }

  static FT_Error
  ft_stroke_border_lineto( FT_StrokeBorder  border,
                           FT_Vector*       to,
                           FT_Bool          movable )
  {
    FT_Error  error = FT_Err_Ok;

    FT_ASSERT( border->start >= 0 );

    if ( border->movable )
    {
      
      border->points[border->num_points - 1] = *to;
    }
    else
    {
      
      if ( border->num_points > 0                                          &&
           FT_IS_SMALL( border->points[border->num_points - 1].x - to->x ) &&
           FT_IS_SMALL( border->points[border->num_points - 1].y - to->y ) )
        return error;

      
      error = ft_stroke_border_grow( border, 1 );
      if ( !error )
      {
        FT_Vector*  vec = border->points + border->num_points;
        FT_Byte*    tag = border->tags   + border->num_points;

        vec[0] = *to;
        tag[0] = FT_STROKE_TAG_ON;

        border->num_points += 1;
      }
    }
    border->movable = movable;
    return error;
  }

  static FT_Error
  ft_stroke_border_conicto( FT_StrokeBorder  border,
                            FT_Vector*       control,
                            FT_Vector*       to )
  {
    FT_Error  error;

    FT_ASSERT( border->start >= 0 );

    error = ft_stroke_border_grow( border, 2 );
    if ( !error )
    {
      FT_Vector*  vec = border->points + border->num_points;
      FT_Byte*    tag = border->tags   + border->num_points;

      vec[0] = *control;
      vec[1] = *to;

      tag[0] = 0;
      tag[1] = FT_STROKE_TAG_ON;

      border->num_points += 2;
    }

    border->movable = FALSE;

    return error;
  }

  static FT_Error
  ft_stroke_border_cubicto( FT_StrokeBorder  border,
                            FT_Vector*       control1,
                            FT_Vector*       control2,
                            FT_Vector*       to )
  {
    FT_Error  error;

    FT_ASSERT( border->start >= 0 );

    error = ft_stroke_border_grow( border, 3 );
    if ( !error )
    {
      FT_Vector*  vec = border->points + border->num_points;
      FT_Byte*    tag = border->tags   + border->num_points;

      vec[0] = *control1;
      vec[1] = *control2;
      vec[2] = *to;

      tag[0] = FT_STROKE_TAG_CUBIC;
      tag[1] = FT_STROKE_TAG_CUBIC;
      tag[2] = FT_STROKE_TAG_ON;

      border->num_points += 3;
    }

    border->movable = FALSE;

    return error;
  }

#define FT_ARC_CUBIC_ANGLE  ( FT_ANGLE_PI / 2 )

  static FT_Error
  ft_stroke_border_arcto( FT_StrokeBorder  border,
                          FT_Vector*       center,
                          FT_Fixed         radius,
                          FT_Angle         angle_start,
                          FT_Angle         angle_diff )
  {
    FT_Angle   total, angle, step, rotate, next, theta;
    FT_Vector  a, b, a2, b2;
    FT_Fixed   length;
    FT_Error   error = FT_Err_Ok;

    
    FT_Vector_From_Polar( &a, radius, angle_start );
    a.x += center->x;
    a.y += center->y;

    total  = angle_diff;
    angle  = angle_start;
    rotate = ( angle_diff >= 0 ) ? FT_ANGLE_PI2 : -FT_ANGLE_PI2;

    while ( total != 0 )
    {
      step = total;
      if ( step > FT_ARC_CUBIC_ANGLE )
        step = FT_ARC_CUBIC_ANGLE;

      else if ( step < -FT_ARC_CUBIC_ANGLE )
        step = -FT_ARC_CUBIC_ANGLE;

      next  = angle + step;
      theta = step;
      if ( theta < 0 )
        theta = -theta;

      theta >>= 1;

      
      FT_Vector_From_Polar( &b, radius, next );
      b.x += center->x;
      b.y += center->y;

      
      length = FT_MulDiv( radius, FT_Sin( theta ) * 4,
                          ( 0x10000L + FT_Cos( theta ) ) * 3 );

      FT_Vector_From_Polar( &a2, length, angle + rotate );
      a2.x += a.x;
      a2.y += a.y;

      FT_Vector_From_Polar( &b2, length, next - rotate );
      b2.x += b.x;
      b2.y += b.y;

      
      error = ft_stroke_border_cubicto( border, &a2, &b2, &b );
      if ( error )
        break;

      
      a      = b;
      total -= step;
      angle  = next;
    }

    return error;
  }

  static FT_Error
  ft_stroke_border_moveto( FT_StrokeBorder  border,
                           FT_Vector*       to )
  {
    
    if ( border->start >= 0 )
      ft_stroke_border_close( border, FALSE );

    border->start = border->num_points;
    border->movable = FALSE;

    return ft_stroke_border_lineto( border, to, FALSE );
  }

  static void
  ft_stroke_border_init( FT_StrokeBorder  border,
                         FT_Memory        memory )
  {
    border->memory = memory;
    border->points = NULL;
    border->tags   = NULL;

    border->num_points = 0;
    border->max_points = 0;
    border->start      = -1;
    border->valid      = FALSE;
  }

  static void
  ft_stroke_border_reset( FT_StrokeBorder  border )
  {
    border->num_points = 0;
    border->start      = -1;
    border->valid      = FALSE;
  }

  static void
  ft_stroke_border_done( FT_StrokeBorder  border )
  {
    FT_Memory  memory = border->memory;

    FT_FREE( border->points );
    FT_FREE( border->tags );

    border->num_points = 0;
    border->max_points = 0;
    border->start      = -1;
    border->valid      = FALSE;
  }

  static FT_Error
  ft_stroke_border_get_counts( FT_StrokeBorder  border,
                               FT_UInt         *anum_points,
                               FT_UInt         *anum_contours )
  {
    FT_Error  error        = FT_Err_Ok;
    FT_UInt   num_points   = 0;
    FT_UInt   num_contours = 0;

    FT_UInt     count      = border->num_points;
    FT_Vector*  point      = border->points;
    FT_Byte*    tags       = border->tags;
    FT_Int      in_contour = 0;

    for ( ; count > 0; count--, num_points++, point++, tags++ )
    {
      if ( tags[0] & FT_STROKE_TAG_BEGIN )
      {
        if ( in_contour != 0 )
          goto Fail;

        in_contour = 1;
      }
      else if ( in_contour == 0 )
        goto Fail;

      if ( tags[0] & FT_STROKE_TAG_END )
      {
        in_contour = 0;
        num_contours++;
      }
    }

    if ( in_contour != 0 )
      goto Fail;

    border->valid = TRUE;

  Exit:
    *anum_points   = num_points;
    *anum_contours = num_contours;
    return error;

  Fail:
    num_points   = 0;
    num_contours = 0;
    goto Exit;
  }

  static void
  ft_stroke_border_export( FT_StrokeBorder  border,
                           FT_Outline*      outline )
  {
    
    FT_ARRAY_COPY( outline->points + outline->n_points,
                   border->points,
                   border->num_points );

    
    {
      FT_UInt   count = border->num_points;
      FT_Byte*  read  = border->tags;
      FT_Byte*  write = (FT_Byte*)outline->tags + outline->n_points;

      for ( ; count > 0; count--, read++, write++ )
      {
        if ( *read & FT_STROKE_TAG_ON )
          *write = FT_CURVE_TAG_ON;
        else if ( *read & FT_STROKE_TAG_CUBIC )
          *write = FT_CURVE_TAG_CUBIC;
        else
          *write = FT_CURVE_TAG_CONIC;
      }
    }

    
    {
      FT_UInt    count = border->num_points;
      FT_Byte*   tags  = border->tags;
      FT_Short*  write = outline->contours + outline->n_contours;
      FT_Short   idx   = (FT_Short)outline->n_points;

      for ( ; count > 0; count--, tags++, idx++ )
      {
        if ( *tags & FT_STROKE_TAG_END )
        {
          *write++ = idx;
          outline->n_contours++;
        }
      }
    }

    outline->n_points = (short)( outline->n_points + border->num_points );

    FT_ASSERT( FT_Outline_Check( outline ) == 0 );
  }

  
  
  
  
  
  
  

#define FT_SIDE_TO_ROTATE( s )   ( FT_ANGLE_PI2 - (s) * FT_ANGLE_PI )

  typedef struct  FT_StrokerRec_
  {
    FT_Angle             angle_in;             
    FT_Angle             angle_out;            
    FT_Vector            center;               
    FT_Fixed             line_length;          
    FT_Bool              first_point;          
    FT_Bool              subpath_open;         
    FT_Angle             subpath_angle;        
    FT_Vector            subpath_start;        
    FT_Fixed             subpath_line_length;  
    FT_Bool              handle_wide_strokes;  

    FT_Stroker_LineCap   line_cap;
    FT_Stroker_LineJoin  line_join;
    FT_Stroker_LineJoin  line_join_saved;
    FT_Fixed             miter_limit;
    FT_Fixed             radius;

    FT_StrokeBorderRec   borders[2];
    FT_Library           library;

  } FT_StrokerRec;

  

  FT_EXPORT_DEF( FT_Error )
  FT_Stroker_New( FT_Library   library,
                  FT_Stroker  *astroker )
  {
    FT_Error    error;
    FT_Memory   memory;
    FT_Stroker  stroker = NULL;

    if ( !library )
      return FT_Err_Invalid_Argument;

    memory = library->memory;

    if ( !FT_NEW( stroker ) )
    {
      stroker->library = library;

      ft_stroke_border_init( &stroker->borders[0], memory );
      ft_stroke_border_init( &stroker->borders[1], memory );
    }

    *astroker = stroker;

    return error;
  }

  

  FT_EXPORT_DEF( void )
  FT_Stroker_Set( FT_Stroker           stroker,
                  FT_Fixed             radius,
                  FT_Stroker_LineCap   line_cap,
                  FT_Stroker_LineJoin  line_join,
                  FT_Fixed             miter_limit )
  {
    stroker->radius      = radius;
    stroker->line_cap    = line_cap;
    stroker->line_join   = line_join;
    stroker->miter_limit = miter_limit;

    
    if ( stroker->miter_limit < 0x10000 )
      stroker->miter_limit = 0x10000;

    
    
    stroker->line_join_saved = line_join;

    FT_Stroker_Rewind( stroker );
  }

  

  FT_EXPORT_DEF( void )
  FT_Stroker_Rewind( FT_Stroker  stroker )
  {
    if ( stroker )
    {
      ft_stroke_border_reset( &stroker->borders[0] );
      ft_stroke_border_reset( &stroker->borders[1] );
    }
  }

  

  FT_EXPORT_DEF( void )
  FT_Stroker_Done( FT_Stroker  stroker )
  {
    if ( stroker )
    {
      FT_Memory  memory = stroker->library->memory;

      ft_stroke_border_done( &stroker->borders[0] );
      ft_stroke_border_done( &stroker->borders[1] );

      stroker->library = NULL;
      FT_FREE( stroker );
    }
  }

  
  static FT_Error
  ft_stroker_arcto( FT_Stroker  stroker,
                    FT_Int      side )
  {
    FT_Angle         total, rotate;
    FT_Fixed         radius = stroker->radius;
    FT_Error         error  = FT_Err_Ok;
    FT_StrokeBorder  border = stroker->borders + side;

    rotate = FT_SIDE_TO_ROTATE( side );

    total = FT_Angle_Diff( stroker->angle_in, stroker->angle_out );
    if ( total == FT_ANGLE_PI )
      total = -rotate * 2;

    error = ft_stroke_border_arcto( border,
                                    &stroker->center,
                                    radius,
                                    stroker->angle_in + rotate,
                                    total );
    border->movable = FALSE;
    return error;
  }

  
  static FT_Error
  ft_stroker_cap( FT_Stroker  stroker,
                  FT_Angle    angle,
                  FT_Int      side )
  {
    FT_Error  error = FT_Err_Ok;

    if ( stroker->line_cap == FT_STROKER_LINECAP_ROUND )
    {
      
      stroker->angle_in  = angle;
      stroker->angle_out = angle + FT_ANGLE_PI;

      error = ft_stroker_arcto( stroker, side );
    }
    else if ( stroker->line_cap == FT_STROKER_LINECAP_SQUARE )
    {
      
      FT_Vector        delta, delta2;
      FT_Angle         rotate = FT_SIDE_TO_ROTATE( side );
      FT_Fixed         radius = stroker->radius;
      FT_StrokeBorder  border = stroker->borders + side;

      FT_Vector_From_Polar( &delta2, radius, angle + rotate );
      FT_Vector_From_Polar( &delta,  radius, angle );

      delta.x += stroker->center.x + delta2.x;
      delta.y += stroker->center.y + delta2.y;

      error = ft_stroke_border_lineto( border, &delta, FALSE );
      if ( error )
        goto Exit;

      FT_Vector_From_Polar( &delta2, radius, angle - rotate );
      FT_Vector_From_Polar( &delta,  radius, angle );

      delta.x += delta2.x + stroker->center.x;
      delta.y += delta2.y + stroker->center.y;

      error = ft_stroke_border_lineto( border, &delta, FALSE );
    }
    else if ( stroker->line_cap == FT_STROKER_LINECAP_BUTT )
    {
      
      FT_Vector        delta;
      FT_Angle         rotate = FT_SIDE_TO_ROTATE( side );
      FT_Fixed         radius = stroker->radius;
      FT_StrokeBorder  border = stroker->borders + side;

      FT_Vector_From_Polar( &delta, radius, angle + rotate );

      delta.x += stroker->center.x;
      delta.y += stroker->center.y;

      error = ft_stroke_border_lineto( border, &delta, FALSE );
      if ( error )
        goto Exit;

      FT_Vector_From_Polar( &delta, radius, angle - rotate );

      delta.x += stroker->center.x;
      delta.y += stroker->center.y;

      error = ft_stroke_border_lineto( border, &delta, FALSE );
    }

  Exit:
    return error;
  }

  
  static FT_Error
  ft_stroker_inside( FT_Stroker  stroker,
                     FT_Int      side,
                     FT_Fixed    line_length )
  {
    FT_StrokeBorder  border = stroker->borders + side;
    FT_Angle         phi, theta, rotate;
    FT_Fixed         length, thcos;
    FT_Vector        delta;
    FT_Error         error = FT_Err_Ok;
    FT_Bool          intersect;          

    rotate = FT_SIDE_TO_ROTATE( side );

    theta = FT_Angle_Diff( stroker->angle_in, stroker->angle_out ) / 2;

    
    
    if ( !border->movable || line_length == 0 )
      intersect = FALSE;
    else
    {
      
      FT_Fixed  min_length = ft_pos_abs( FT_MulFix( stroker->radius,
                                                    FT_Tan( theta ) ) );

      intersect = FT_BOOL( stroker->line_length >= min_length &&
                           line_length          >= min_length );
    }

    if ( !intersect )
    {
      FT_Vector_From_Polar( &delta, stroker->radius,
                            stroker->angle_out + rotate );
      delta.x += stroker->center.x;
      delta.y += stroker->center.y;

      border->movable = FALSE;
    }
    else
    {
      
      phi = stroker->angle_in + theta;

      thcos = FT_Cos( theta );

      length = FT_DivFix( stroker->radius, thcos );

      FT_Vector_From_Polar( &delta, length, phi + rotate );
      delta.x += stroker->center.x;
      delta.y += stroker->center.y;
    }

    error = ft_stroke_border_lineto( border, &delta, FALSE );

    return error;
  }

  
  static FT_Error
  ft_stroker_outside( FT_Stroker  stroker,
                      FT_Int      side,
                      FT_Fixed    line_length )
  {
    FT_StrokeBorder  border = stroker->borders + side;
    FT_Error         error;
    FT_Angle         rotate;

    if ( stroker->line_join == FT_STROKER_LINEJOIN_ROUND )
      error = ft_stroker_arcto( stroker, side );
    else
    {
      
      FT_Fixed  sigma = 0, radius = stroker->radius;
      FT_Angle  theta = 0, phi = 0;
      FT_Fixed  thcos = 0;
      FT_Bool   bevel, fixed_bevel;

      rotate = FT_SIDE_TO_ROTATE( side );

      bevel =
        FT_BOOL( stroker->line_join == FT_STROKER_LINEJOIN_BEVEL );

      fixed_bevel =
        FT_BOOL( stroker->line_join != FT_STROKER_LINEJOIN_MITER_VARIABLE );

      if ( !bevel )
      {
        theta = FT_Angle_Diff( stroker->angle_in, stroker->angle_out );

        if ( theta == FT_ANGLE_PI )
        {
          theta = rotate;
          phi   = stroker->angle_in;
        }
        else
        {
          theta /= 2;
          phi    = stroker->angle_in + theta + rotate;
        }

        thcos = FT_Cos( theta );
        sigma = FT_MulFix( stroker->miter_limit, thcos );

        
        if ( sigma < 0x10000L )
        {
          
          
          if ( fixed_bevel || ft_pos_abs( theta ) > 57 )
            bevel = TRUE;
        }
      }

      if ( bevel )  
      {
        if ( fixed_bevel )
        {
          
          FT_Vector  delta;

          
          FT_Vector_From_Polar( &delta,
                                radius,
                                stroker->angle_out + rotate );
          delta.x += stroker->center.x;
          delta.y += stroker->center.y;

          border->movable = FALSE;
          error = ft_stroke_border_lineto( border, &delta, FALSE );
        }
        else 
        {
          
          FT_Vector  middle, delta;
          FT_Fixed   length;

          
          FT_Vector_From_Polar( &middle,
                                FT_MulFix( radius, stroker->miter_limit ),
                                phi );
          middle.x += stroker->center.x;
          middle.y += stroker->center.y;

          
          length = FT_MulFix( radius,
                              FT_DivFix( 0x10000L - sigma,
                                         ft_pos_abs( FT_Sin( theta ) ) ) );

          FT_Vector_From_Polar( &delta, length, phi + rotate );
          delta.x += middle.x;
          delta.y += middle.y;

          error = ft_stroke_border_lineto( border, &delta, FALSE );
          if ( error )
            goto Exit;

          
          FT_Vector_From_Polar( &delta, length, phi - rotate );
          delta.x += middle.x;
          delta.y += middle.y;

          error = ft_stroke_border_lineto( border, &delta, FALSE );
          if ( error )
            goto Exit;

          
          
          if ( line_length == 0 )
          {
            FT_Vector_From_Polar( &delta,
                                  radius,
                                  stroker->angle_out + rotate );

            delta.x += stroker->center.x;
            delta.y += stroker->center.y;

            error = ft_stroke_border_lineto( border, &delta, FALSE );
          }
        }
      }
      else 
      {
        FT_Fixed   length;
        FT_Vector  delta;

        length = FT_DivFix( stroker->radius, thcos );

        FT_Vector_From_Polar( &delta, length, phi );
        delta.x += stroker->center.x;
        delta.y += stroker->center.y;

        error = ft_stroke_border_lineto( border, &delta, FALSE );
        if ( error )
          goto Exit;

        
        
        if ( line_length == 0 )
        {
          FT_Vector_From_Polar( &delta,
                                stroker->radius,
                                stroker->angle_out + rotate );
          delta.x += stroker->center.x;
          delta.y += stroker->center.y;

          error = ft_stroke_border_lineto( border, &delta, FALSE );
        }
      }
    }

  Exit:
    return error;
  }

  static FT_Error
  ft_stroker_process_corner( FT_Stroker  stroker,
                             FT_Fixed    line_length )
  {
    FT_Error  error = FT_Err_Ok;
    FT_Angle  turn;
    FT_Int    inside_side;

    turn = FT_Angle_Diff( stroker->angle_in, stroker->angle_out );

    
    if ( turn == 0 )
      goto Exit;

    
    inside_side = 0;

    
    if ( turn < 0 )
      inside_side = 1;

    
    error = ft_stroker_inside( stroker, inside_side, line_length );
    if ( error )
      goto Exit;

    
    error = ft_stroker_outside( stroker, 1 - inside_side, line_length );

  Exit:
    return error;
  }

  
  
  static FT_Error
  ft_stroker_subpath_start( FT_Stroker  stroker,
                            FT_Angle    start_angle,
                            FT_Fixed    line_length )
  {
    FT_Vector        delta;
    FT_Vector        point;
    FT_Error         error;
    FT_StrokeBorder  border;

    FT_Vector_From_Polar( &delta, stroker->radius,
                          start_angle + FT_ANGLE_PI2 );

    point.x = stroker->center.x + delta.x;
    point.y = stroker->center.y + delta.y;

    border = stroker->borders;
    error = ft_stroke_border_moveto( border, &point );
    if ( error )
      goto Exit;

    point.x = stroker->center.x - delta.x;
    point.y = stroker->center.y - delta.y;

    border++;
    error = ft_stroke_border_moveto( border, &point );

    
    
    stroker->subpath_angle       = start_angle;
    stroker->first_point         = FALSE;
    stroker->subpath_line_length = line_length;

  Exit:
    return error;
  }

  

  FT_EXPORT_DEF( FT_Error )
  FT_Stroker_LineTo( FT_Stroker  stroker,
                     FT_Vector*  to )
  {
    FT_Error         error = FT_Err_Ok;
    FT_StrokeBorder  border;
    FT_Vector        delta;
    FT_Angle         angle;
    FT_Int           side;
    FT_Fixed         line_length;

    delta.x = to->x - stroker->center.x;
    delta.y = to->y - stroker->center.y;

    
    if ( delta.x == 0 && delta.y == 0 )
       goto Exit;

    
    line_length = FT_Vector_Length( &delta );

    angle = FT_Atan2( delta.x, delta.y );
    FT_Vector_From_Polar( &delta, stroker->radius, angle + FT_ANGLE_PI2 );

    
    if ( stroker->first_point )
    {
      
      
      
      error = ft_stroker_subpath_start( stroker, angle, line_length );
      if ( error )
        goto Exit;
    }
    else
    {
      
      stroker->angle_out = angle;
      error = ft_stroker_process_corner( stroker, line_length );
      if ( error )
        goto Exit;
    }

    
    for ( border = stroker->borders, side = 1; side >= 0; side--, border++ )
    {
      FT_Vector  point;

      point.x = to->x + delta.x;
      point.y = to->y + delta.y;

      
      error = ft_stroke_border_lineto( border, &point, TRUE );
      if ( error )
        goto Exit;

      delta.x = -delta.x;
      delta.y = -delta.y;
    }

    stroker->angle_in    = angle;
    stroker->center      = *to;
    stroker->line_length = line_length;

  Exit:
    return error;
  }

  

  FT_EXPORT_DEF( FT_Error )
  FT_Stroker_ConicTo( FT_Stroker  stroker,
                      FT_Vector*  control,
                      FT_Vector*  to )
  {
    FT_Error    error = FT_Err_Ok;
    FT_Vector   bez_stack[34];
    FT_Vector*  arc;
    FT_Vector*  limit = bez_stack + 30;
    FT_Bool     first_arc = TRUE;

    
    
    if ( FT_IS_SMALL( stroker->center.x - control->x ) &&
         FT_IS_SMALL( stroker->center.y - control->y ) &&
         FT_IS_SMALL( control->x        - to->x      ) &&
         FT_IS_SMALL( control->y        - to->y      ) )
    {
       stroker->center = *to;
       goto Exit;
    }

    arc    = bez_stack;
    arc[0] = *to;
    arc[1] = *control;
    arc[2] = stroker->center;

    while ( arc >= bez_stack )
    {
      FT_Angle  angle_in, angle_out;

      
      angle_in = angle_out = stroker->angle_in;

      if ( arc < limit                                             &&
           !ft_conic_is_small_enough( arc, &angle_in, &angle_out ) )
      {
        if ( stroker->first_point )
          stroker->angle_in = angle_in;

        ft_conic_split( arc );
        arc += 2;
        continue;
      }

      if ( first_arc )
      {
        first_arc = FALSE;

        
        if ( stroker->first_point )
          error = ft_stroker_subpath_start( stroker, angle_in, 0 );
        else
        {
          stroker->angle_out = angle_in;
          error = ft_stroker_process_corner( stroker, 0 );
        }
      }
      else if ( ft_pos_abs( FT_Angle_Diff( stroker->angle_in, angle_in ) ) >
                  FT_SMALL_CONIC_THRESHOLD / 4                             )
      {
        
        
        stroker->center    = arc[2];
        stroker->angle_out = angle_in;
        stroker->line_join = FT_STROKER_LINEJOIN_ROUND;

        error = ft_stroker_process_corner( stroker, 0 );

        
        stroker->line_join = stroker->line_join_saved;
      }

      if ( error )
        goto Exit;

      
      
      {
        FT_Vector        ctrl, end;
        FT_Angle         theta, phi, rotate, alpha0 = 0;
        FT_Fixed         length;
        FT_StrokeBorder  border;
        FT_Int           side;

        theta  = FT_Angle_Diff( angle_in, angle_out ) / 2;
        phi    = angle_in + theta;
        length = FT_DivFix( stroker->radius, FT_Cos( theta ) );

        
        if ( stroker->handle_wide_strokes )
          alpha0 = FT_Atan2( arc[0].x - arc[2].x, arc[0].y - arc[2].y );

        for ( border = stroker->borders, side = 0;
              side <= 1;
              side++, border++ )
        {
          rotate = FT_SIDE_TO_ROTATE( side );

          
          FT_Vector_From_Polar( &ctrl, length, phi + rotate );
          ctrl.x += arc[1].x;
          ctrl.y += arc[1].y;

          
          FT_Vector_From_Polar( &end, stroker->radius, angle_out + rotate );
          end.x += arc[0].x;
          end.y += arc[0].y;

          if ( stroker->handle_wide_strokes )
          {
            FT_Vector  start;
            FT_Angle   alpha1;

            
            
            start = border->points[border->num_points - 1];

            alpha1 = FT_Atan2( end.x - start.x, end.y - start.y );

            
            
            if ( ft_pos_abs( FT_Angle_Diff( alpha0, alpha1 ) ) >
                   FT_ANGLE_PI / 2                             )
            {
              FT_Angle   beta, gamma;
              FT_Vector  bvec, delta;
              FT_Fixed   blen, sinA, sinB, alen;

              
              beta  = FT_Atan2( arc[2].x - start.x, arc[2].y - start.y );
              gamma = FT_Atan2( arc[0].x - end.x,   arc[0].y - end.y );

              bvec.x = end.x - start.x;
              bvec.y = end.y - start.y;

              blen = FT_Vector_Length( &bvec );

              sinA = ft_pos_abs( FT_Sin( alpha1 - gamma ) );
              sinB = ft_pos_abs( FT_Sin( beta - gamma ) );

              alen = FT_DivFix( FT_MulFix( blen, sinA ), sinB );

              FT_Vector_From_Polar( &delta, alen, beta );
              delta.x += start.x;
              delta.y += start.y;

              
              border->movable = FALSE;
              error = ft_stroke_border_lineto( border, &delta, FALSE );
              if ( error )
                goto Exit;
              error = ft_stroke_border_lineto( border, &end, FALSE );
              if ( error )
                goto Exit;
              error = ft_stroke_border_conicto( border, &ctrl, &start );
              if ( error )
                goto Exit;
              
              error = ft_stroke_border_lineto( border, &end, FALSE );
              if ( error )
                goto Exit;

              continue;
            }

            
          }

          
          error = ft_stroke_border_conicto( border, &ctrl, &end );
          if ( error )
            goto Exit;
        }
      }

      arc -= 2;

      stroker->angle_in = angle_out;
    }

    stroker->center = *to;

  Exit:
    return error;
  }

  

  FT_EXPORT_DEF( FT_Error )
  FT_Stroker_CubicTo( FT_Stroker  stroker,
                      FT_Vector*  control1,
                      FT_Vector*  control2,
                      FT_Vector*  to )
  {
    FT_Error    error = FT_Err_Ok;
    FT_Vector   bez_stack[37];
    FT_Vector*  arc;
    FT_Vector*  limit = bez_stack + 32;
    FT_Bool     first_arc = TRUE;

    
    
    if ( FT_IS_SMALL( stroker->center.x - control1->x ) &&
         FT_IS_SMALL( stroker->center.y - control1->y ) &&
         FT_IS_SMALL( control1->x       - control2->x ) &&
         FT_IS_SMALL( control1->y       - control2->y ) &&
         FT_IS_SMALL( control2->x       - to->x       ) &&
         FT_IS_SMALL( control2->y       - to->y       ) )
    {
       stroker->center = *to;
       goto Exit;
    }

    arc    = bez_stack;
    arc[0] = *to;
    arc[1] = *control2;
    arc[2] = *control1;
    arc[3] = stroker->center;

    while ( arc >= bez_stack )
    {
      FT_Angle  angle_in, angle_mid, angle_out;

      
      angle_in = angle_out = angle_mid = stroker->angle_in;

      if ( arc < limit                                         &&
           !ft_cubic_is_small_enough( arc, &angle_in,
                                      &angle_mid, &angle_out ) )
      {
        if ( stroker->first_point )
          stroker->angle_in = angle_in;

        ft_cubic_split( arc );
        arc += 3;
        continue;
      }

      if ( first_arc )
      {
        first_arc = FALSE;

        
        if ( stroker->first_point )
          error = ft_stroker_subpath_start( stroker, angle_in, 0 );
        else
        {
          stroker->angle_out = angle_in;
          error = ft_stroker_process_corner( stroker, 0 );
        }
      }
      else if ( ft_pos_abs( FT_Angle_Diff( stroker->angle_in, angle_in ) ) >
                  FT_SMALL_CUBIC_THRESHOLD / 4                             )
      {
        
        
        stroker->center    = arc[3];
        stroker->angle_out = angle_in;
        stroker->line_join = FT_STROKER_LINEJOIN_ROUND;

        error = ft_stroker_process_corner( stroker, 0 );

        
        stroker->line_join = stroker->line_join_saved;
      }

      if ( error )
        goto Exit;

      
      
      {
        FT_Vector        ctrl1, ctrl2, end;
        FT_Angle         theta1, phi1, theta2, phi2, rotate, alpha0 = 0;
        FT_Fixed         length1, length2;
        FT_StrokeBorder  border;
        FT_Int           side;

        theta1  = FT_Angle_Diff( angle_in,  angle_mid ) / 2;
        theta2  = FT_Angle_Diff( angle_mid, angle_out ) / 2;
        phi1    = ft_angle_mean( angle_in,  angle_mid );
        phi2    = ft_angle_mean( angle_mid, angle_out );
        length1 = FT_DivFix( stroker->radius, FT_Cos( theta1 ) );
        length2 = FT_DivFix( stroker->radius, FT_Cos( theta2 ) );

        
        if ( stroker->handle_wide_strokes )
          alpha0 = FT_Atan2( arc[0].x - arc[3].x, arc[0].y - arc[3].y );

        for ( border = stroker->borders, side = 0;
              side <= 1;
              side++, border++ )
        {
          rotate = FT_SIDE_TO_ROTATE( side );

          
          FT_Vector_From_Polar( &ctrl1, length1, phi1 + rotate );
          ctrl1.x += arc[2].x;
          ctrl1.y += arc[2].y;

          FT_Vector_From_Polar( &ctrl2, length2, phi2 + rotate );
          ctrl2.x += arc[1].x;
          ctrl2.y += arc[1].y;

          
          FT_Vector_From_Polar( &end, stroker->radius, angle_out + rotate );
          end.x += arc[0].x;
          end.y += arc[0].y;

          if ( stroker->handle_wide_strokes )
          {
            FT_Vector  start;
            FT_Angle   alpha1;

            
            
            start = border->points[border->num_points - 1];

            alpha1 = FT_Atan2( end.x - start.x, end.y - start.y );

            
            
            if ( ft_pos_abs( FT_Angle_Diff( alpha0, alpha1 ) ) >
                   FT_ANGLE_PI / 2                             )
            {
              FT_Angle   beta, gamma;
              FT_Vector  bvec, delta;
              FT_Fixed   blen, sinA, sinB, alen;

              
              beta  = FT_Atan2( arc[3].x - start.x, arc[3].y - start.y );
              gamma = FT_Atan2( arc[0].x - end.x,   arc[0].y - end.y );

              bvec.x = end.x - start.x;
              bvec.y = end.y - start.y;

              blen = FT_Vector_Length( &bvec );

              sinA = ft_pos_abs( FT_Sin( alpha1 - gamma ) );
              sinB = ft_pos_abs( FT_Sin( beta - gamma ) );

              alen = FT_DivFix( FT_MulFix( blen, sinA ), sinB );

              FT_Vector_From_Polar( &delta, alen, beta );
              delta.x += start.x;
              delta.y += start.y;

              
              border->movable = FALSE;
              error = ft_stroke_border_lineto( border, &delta, FALSE );
              if ( error )
                goto Exit;
              error = ft_stroke_border_lineto( border, &end, FALSE );
              if ( error )
                goto Exit;
              error = ft_stroke_border_cubicto( border,
                                                &ctrl2,
                                                &ctrl1,
                                                &start );
              if ( error )
                goto Exit;
              
              error = ft_stroke_border_lineto( border, &end, FALSE );
              if ( error )
                goto Exit;

              continue;
            }

            
          }

          
          error = ft_stroke_border_cubicto( border, &ctrl1, &ctrl2, &end );
          if ( error )
            goto Exit;
        }
      }

      arc -= 3;

      stroker->angle_in = angle_out;
    }

    stroker->center = *to;

  Exit:
    return error;
  }

  

  FT_EXPORT_DEF( FT_Error )
  FT_Stroker_BeginSubPath( FT_Stroker  stroker,
                           FT_Vector*  to,
                           FT_Bool     open )
  {
    
    
    
    
    stroker->first_point  = TRUE;
    stroker->center       = *to;
    stroker->subpath_open = open;

    
    
    
    
    
    stroker->handle_wide_strokes =
      FT_BOOL( stroker->line_join != FT_STROKER_LINEJOIN_ROUND  ||
               ( stroker->subpath_open                        &&
                 stroker->line_cap == FT_STROKER_LINECAP_BUTT ) );

    
    stroker->subpath_start = *to;

    stroker->angle_in = 0;

    return FT_Err_Ok;
  }

  static FT_Error
  ft_stroker_add_reverse_left( FT_Stroker  stroker,
                               FT_Bool     open )
  {
    FT_StrokeBorder  right = stroker->borders + 0;
    FT_StrokeBorder  left  = stroker->borders + 1;
    FT_Int           new_points;
    FT_Error         error = FT_Err_Ok;

    FT_ASSERT( left->start >= 0 );

    new_points = left->num_points - left->start;
    if ( new_points > 0 )
    {
      error = ft_stroke_border_grow( right, (FT_UInt)new_points );
      if ( error )
        goto Exit;

      {
        FT_Vector*  dst_point = right->points + right->num_points;
        FT_Byte*    dst_tag   = right->tags   + right->num_points;
        FT_Vector*  src_point = left->points  + left->num_points - 1;
        FT_Byte*    src_tag   = left->tags    + left->num_points - 1;

        while ( src_point >= left->points + left->start )
        {
          *dst_point = *src_point;
          *dst_tag   = *src_tag;

          if ( open )
            dst_tag[0] &= ~FT_STROKE_TAG_BEGIN_END;
          else
          {
            FT_Byte  ttag =
                       (FT_Byte)( dst_tag[0] & FT_STROKE_TAG_BEGIN_END );

            
            if ( ttag == FT_STROKE_TAG_BEGIN ||
                 ttag == FT_STROKE_TAG_END   )
              dst_tag[0] ^= FT_STROKE_TAG_BEGIN_END;
          }

          src_point--;
          src_tag--;
          dst_point++;
          dst_tag++;
        }
      }

      left->num_points   = left->start;
      right->num_points += new_points;

      right->movable = FALSE;
      left->movable  = FALSE;
    }

  Exit:
    return error;
  }

  

  
  FT_EXPORT_DEF( FT_Error )
  FT_Stroker_EndSubPath( FT_Stroker  stroker )
  {
    FT_Error  error = FT_Err_Ok;

    if ( stroker->subpath_open )
    {
      FT_StrokeBorder  right = stroker->borders;

      
      
      
      error = ft_stroker_cap( stroker, stroker->angle_in, 0 );
      if ( error )
        goto Exit;

      
      error = ft_stroker_add_reverse_left( stroker, TRUE );
      if ( error )
        goto Exit;

      
      stroker->center = stroker->subpath_start;
      error = ft_stroker_cap( stroker,
                              stroker->subpath_angle + FT_ANGLE_PI, 0 );
      if ( error )
        goto Exit;

      
      
      ft_stroke_border_close( right, FALSE );
    }
    else
    {
      FT_Angle  turn;
      FT_Int    inside_side;

      
      if ( stroker->center.x != stroker->subpath_start.x ||
           stroker->center.y != stroker->subpath_start.y )
      {
         error = FT_Stroker_LineTo( stroker, &stroker->subpath_start );
         if ( error )
           goto Exit;
      }

      
      stroker->angle_out = stroker->subpath_angle;
      turn               = FT_Angle_Diff( stroker->angle_in,
                                          stroker->angle_out );

      
      if ( turn != 0 )
      {
        
        inside_side = 0;

        
        if ( turn < 0 )
          inside_side = 1;

        error = ft_stroker_inside( stroker,
                                   inside_side,
                                   stroker->subpath_line_length );
        if ( error )
          goto Exit;

        
        error = ft_stroker_outside( stroker,
                                    1 - inside_side,
                                    stroker->subpath_line_length );
        if ( error )
          goto Exit;
      }

      
      ft_stroke_border_close( stroker->borders + 0, FALSE );
      ft_stroke_border_close( stroker->borders + 1, TRUE );
    }

  Exit:
    return error;
  }

  

  FT_EXPORT_DEF( FT_Error )
  FT_Stroker_GetBorderCounts( FT_Stroker        stroker,
                              FT_StrokerBorder  border,
                              FT_UInt          *anum_points,
                              FT_UInt          *anum_contours )
  {
    FT_UInt   num_points = 0, num_contours = 0;
    FT_Error  error;

    if ( !stroker || border > 1 )
    {
      error = FT_Err_Invalid_Argument;
      goto Exit;
    }

    error = ft_stroke_border_get_counts( stroker->borders + border,
                                         &num_points, &num_contours );
  Exit:
    if ( anum_points )
      *anum_points = num_points;

    if ( anum_contours )
      *anum_contours = num_contours;

    return error;
  }

  

  FT_EXPORT_DEF( FT_Error )
  FT_Stroker_GetCounts( FT_Stroker  stroker,
                        FT_UInt    *anum_points,
                        FT_UInt    *anum_contours )
  {
    FT_UInt   count1, count2, num_points   = 0;
    FT_UInt   count3, count4, num_contours = 0;
    FT_Error  error;

    error = ft_stroke_border_get_counts( stroker->borders + 0,
                                         &count1, &count2 );
    if ( error )
      goto Exit;

    error = ft_stroke_border_get_counts( stroker->borders + 1,
                                         &count3, &count4 );
    if ( error )
      goto Exit;

    num_points   = count1 + count3;
    num_contours = count2 + count4;

  Exit:
    *anum_points   = num_points;
    *anum_contours = num_contours;
    return error;
  }

  

  FT_EXPORT_DEF( void )
  FT_Stroker_ExportBorder( FT_Stroker        stroker,
                           FT_StrokerBorder  border,
                           FT_Outline*       outline )
  {
    if ( border == FT_STROKER_BORDER_LEFT  ||
         border == FT_STROKER_BORDER_RIGHT )
    {
      FT_StrokeBorder  sborder = & stroker->borders[border];

      if ( sborder->valid )
        ft_stroke_border_export( sborder, outline );
    }
  }

  

  FT_EXPORT_DEF( void )
  FT_Stroker_Export( FT_Stroker   stroker,
                     FT_Outline*  outline )
  {
    FT_Stroker_ExportBorder( stroker, FT_STROKER_BORDER_LEFT, outline );
    FT_Stroker_ExportBorder( stroker, FT_STROKER_BORDER_RIGHT, outline );
  }

  

  
  FT_EXPORT_DEF( FT_Error )
  FT_Stroker_ParseOutline( FT_Stroker   stroker,
                           FT_Outline*  outline,
                           FT_Bool      opened )
  {
    FT_Vector   v_last;
    FT_Vector   v_control;
    FT_Vector   v_start;

    FT_Vector*  point;
    FT_Vector*  limit;
    char*       tags;

    FT_Error    error;

    FT_Int      n;         
    FT_UInt     first;     
    FT_Int      tag;       

    if ( !outline || !stroker )
      return FT_Err_Invalid_Argument;

    FT_Stroker_Rewind( stroker );

    first = 0;

    for ( n = 0; n < outline->n_contours; n++ )
    {
      FT_UInt  last;  

      last  = outline->contours[n];
      limit = outline->points + last;

      
      if ( last <= first )
      {
        first = last + 1;
        continue;
      }

      v_start = outline->points[first];
      v_last  = outline->points[last];

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
        }
        point--;
        tags--;
      }

      error = FT_Stroker_BeginSubPath( stroker, &v_start, opened );
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

            vec.x = point->x;
            vec.y = point->y;

            error = FT_Stroker_LineTo( stroker, &vec );
            if ( error )
              goto Exit;
            continue;
          }

        case FT_CURVE_TAG_CONIC:  
          v_control.x = point->x;
          v_control.y = point->y;

        Do_Conic:
          if ( point < limit )
          {
            FT_Vector  vec;
            FT_Vector  v_middle;

            point++;
            tags++;
            tag = FT_CURVE_TAG( tags[0] );

            vec = point[0];

            if ( tag == FT_CURVE_TAG_ON )
            {
              error = FT_Stroker_ConicTo( stroker, &v_control, &vec );
              if ( error )
                goto Exit;
              continue;
            }

            if ( tag != FT_CURVE_TAG_CONIC )
              goto Invalid_Outline;

            v_middle.x = ( v_control.x + vec.x ) / 2;
            v_middle.y = ( v_control.y + vec.y ) / 2;

            error = FT_Stroker_ConicTo( stroker, &v_control, &v_middle );
            if ( error )
              goto Exit;

            v_control = vec;
            goto Do_Conic;
          }

          error = FT_Stroker_ConicTo( stroker, &v_control, &v_start );
          goto Close;

        default:  
          {
            FT_Vector  vec1, vec2;

            if ( point + 1 > limit                             ||
                 FT_CURVE_TAG( tags[1] ) != FT_CURVE_TAG_CUBIC )
              goto Invalid_Outline;

            point += 2;
            tags  += 2;

            vec1 = point[-2];
            vec2 = point[-1];

            if ( point <= limit )
            {
              FT_Vector  vec;

              vec = point[0];

              error = FT_Stroker_CubicTo( stroker, &vec1, &vec2, &vec );
              if ( error )
                goto Exit;
              continue;
            }

            error = FT_Stroker_CubicTo( stroker, &vec1, &vec2, &v_start );
            goto Close;
          }
        }
      }

    Close:
      if ( error )
        goto Exit;

      
      if ( !stroker->first_point )
      {
        error = FT_Stroker_EndSubPath( stroker );
        if ( error )
          goto Exit;
      }

      first = last + 1;
    }

    return FT_Err_Ok;

  Exit:
    return error;

  Invalid_Outline:
    return FT_Err_Invalid_Outline;
  }

  
  
  
#ifndef FT_CONFIG_OPTION_PIC
  extern const FT_Glyph_Class  ft_outline_glyph_class;
#endif
#include "basepic.h"

  

  FT_EXPORT_DEF( FT_Error )
  FT_Glyph_Stroke( FT_Glyph    *pglyph,
                   FT_Stroker   stroker,
                   FT_Bool      destroy )
  {
    FT_Error    error   = FT_Err_Invalid_Argument;
    FT_Glyph    glyph   = NULL;
    FT_Library  library = stroker->library;

    FT_UNUSED( library );

    if ( pglyph == NULL )
      goto Exit;

    glyph = *pglyph;
    if ( glyph == NULL || glyph->clazz != FT_OUTLINE_GLYPH_CLASS_GET )
      goto Exit;

    {
      FT_Glyph  copy;

      error = FT_Glyph_Copy( glyph, &copy );
      if ( error )
        goto Exit;

      glyph = copy;
    }

    {
      FT_OutlineGlyph  oglyph  = (FT_OutlineGlyph)glyph;
      FT_Outline*      outline = &oglyph->outline;
      FT_UInt          num_points, num_contours;

      error = FT_Stroker_ParseOutline( stroker, outline, FALSE );
      if ( error )
        goto Fail;

      (void)FT_Stroker_GetCounts( stroker, &num_points, &num_contours );

      FT_Outline_Done( glyph->library, outline );

      error = FT_Outline_New( glyph->library,
                              num_points, num_contours, outline );
      if ( error )
        goto Fail;

      outline->n_points   = 0;
      outline->n_contours = 0;

      FT_Stroker_Export( stroker, outline );
    }

    if ( destroy )
      FT_Done_Glyph( *pglyph );

    *pglyph = glyph;
    goto Exit;

  Fail:
    FT_Done_Glyph( glyph );
    glyph = NULL;

    if ( !destroy )
      *pglyph = NULL;

  Exit:
    return error;
  }

  

  FT_EXPORT_DEF( FT_Error )
  FT_Glyph_StrokeBorder( FT_Glyph    *pglyph,
                         FT_Stroker   stroker,
                         FT_Bool      inside,
                         FT_Bool      destroy )
  {
    FT_Error    error   = FT_Err_Invalid_Argument;
    FT_Glyph    glyph   = NULL;
    FT_Library  library = stroker->library;

    FT_UNUSED( library );

    if ( pglyph == NULL )
      goto Exit;

    glyph = *pglyph;
    if ( glyph == NULL || glyph->clazz != FT_OUTLINE_GLYPH_CLASS_GET )
      goto Exit;

    {
      FT_Glyph  copy;

      error = FT_Glyph_Copy( glyph, &copy );
      if ( error )
        goto Exit;

      glyph = copy;
    }

    {
      FT_OutlineGlyph   oglyph  = (FT_OutlineGlyph)glyph;
      FT_StrokerBorder  border;
      FT_Outline*       outline = &oglyph->outline;
      FT_UInt           num_points, num_contours;

      border = FT_Outline_GetOutsideBorder( outline );
      if ( inside )
      {
        if ( border == FT_STROKER_BORDER_LEFT )
          border = FT_STROKER_BORDER_RIGHT;
        else
          border = FT_STROKER_BORDER_LEFT;
      }

      error = FT_Stroker_ParseOutline( stroker, outline, FALSE );
      if ( error )
        goto Fail;

      (void)FT_Stroker_GetBorderCounts( stroker, border,
                                        &num_points, &num_contours );

      FT_Outline_Done( glyph->library, outline );

      error = FT_Outline_New( glyph->library,
                              num_points,
                              num_contours,
                              outline );
      if ( error )
        goto Fail;

      outline->n_points   = 0;
      outline->n_contours = 0;

      FT_Stroker_ExportBorder( stroker, border, outline );
    }

    if ( destroy )
      FT_Done_Glyph( *pglyph );

    *pglyph = glyph;
    goto Exit;

  Fail:
    FT_Done_Glyph( glyph );
    glyph = NULL;

    if ( !destroy )
      *pglyph = NULL;

  Exit:
    return error;
  }

