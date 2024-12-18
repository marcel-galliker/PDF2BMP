

#ifndef __FTSTREAM_H__
#define __FTSTREAM_H__

#include <ft2build.h>
#include FT_SYSTEM_H
#include FT_INTERNAL_OBJECTS_H

FT_BEGIN_HEADER

  
  
  
  
  
  
  
  

#define FT_FRAME_OP_SHIFT         2
#define FT_FRAME_OP_SIGNED        1
#define FT_FRAME_OP_LITTLE        2
#define FT_FRAME_OP_COMMAND( x )  ( x >> FT_FRAME_OP_SHIFT )

#define FT_MAKE_FRAME_OP( command, little, sign ) \
          ( ( command << FT_FRAME_OP_SHIFT ) | ( little << 1 ) | sign )

#define FT_FRAME_OP_END    0
#define FT_FRAME_OP_START  1  
#define FT_FRAME_OP_BYTE   2  
#define FT_FRAME_OP_SHORT  3  
#define FT_FRAME_OP_LONG   4  
#define FT_FRAME_OP_OFF3   5  
#define FT_FRAME_OP_BYTES  6  

  typedef enum  FT_Frame_Op_
  {
    ft_frame_end       = 0,
    ft_frame_start     = FT_MAKE_FRAME_OP( FT_FRAME_OP_START, 0, 0 ),

    ft_frame_byte      = FT_MAKE_FRAME_OP( FT_FRAME_OP_BYTE,  0, 0 ),
    ft_frame_schar     = FT_MAKE_FRAME_OP( FT_FRAME_OP_BYTE,  0, 1 ),

    ft_frame_ushort_be = FT_MAKE_FRAME_OP( FT_FRAME_OP_SHORT, 0, 0 ),
    ft_frame_short_be  = FT_MAKE_FRAME_OP( FT_FRAME_OP_SHORT, 0, 1 ),
    ft_frame_ushort_le = FT_MAKE_FRAME_OP( FT_FRAME_OP_SHORT, 1, 0 ),
    ft_frame_short_le  = FT_MAKE_FRAME_OP( FT_FRAME_OP_SHORT, 1, 1 ),

    ft_frame_ulong_be  = FT_MAKE_FRAME_OP( FT_FRAME_OP_LONG, 0, 0 ),
    ft_frame_long_be   = FT_MAKE_FRAME_OP( FT_FRAME_OP_LONG, 0, 1 ),
    ft_frame_ulong_le  = FT_MAKE_FRAME_OP( FT_FRAME_OP_LONG, 1, 0 ),
    ft_frame_long_le   = FT_MAKE_FRAME_OP( FT_FRAME_OP_LONG, 1, 1 ),

    ft_frame_uoff3_be  = FT_MAKE_FRAME_OP( FT_FRAME_OP_OFF3, 0, 0 ),
    ft_frame_off3_be   = FT_MAKE_FRAME_OP( FT_FRAME_OP_OFF3, 0, 1 ),
    ft_frame_uoff3_le  = FT_MAKE_FRAME_OP( FT_FRAME_OP_OFF3, 1, 0 ),
    ft_frame_off3_le   = FT_MAKE_FRAME_OP( FT_FRAME_OP_OFF3, 1, 1 ),

    ft_frame_bytes     = FT_MAKE_FRAME_OP( FT_FRAME_OP_BYTES, 0, 0 ),
    ft_frame_skip      = FT_MAKE_FRAME_OP( FT_FRAME_OP_BYTES, 0, 1 )

  } FT_Frame_Op;

  typedef struct  FT_Frame_Field_
  {
    FT_Byte    value;
    FT_Byte    size;
    FT_UShort  offset;

  } FT_Frame_Field;

  
  
  
  
#define FT_FIELD_SIZE( f ) \
          (FT_Byte)sizeof ( ((FT_STRUCTURE*)0)->f )

#define FT_FIELD_SIZE_DELTA( f ) \
          (FT_Byte)sizeof ( ((FT_STRUCTURE*)0)->f[0] )

#define FT_FIELD_OFFSET( f ) \
          (FT_UShort)( offsetof( FT_STRUCTURE, f ) )

#define FT_FRAME_FIELD( frame_op, field ) \
          {                               \
            frame_op,                     \
            FT_FIELD_SIZE( field ),       \
            FT_FIELD_OFFSET( field )      \
          }

#define FT_MAKE_EMPTY_FIELD( frame_op )  { frame_op, 0, 0 }

#define FT_FRAME_START( size )   { ft_frame_start, 0, size }
#define FT_FRAME_END             { ft_frame_end, 0, 0 }

#define FT_FRAME_LONG( f )       FT_FRAME_FIELD( ft_frame_long_be, f )
#define FT_FRAME_ULONG( f )      FT_FRAME_FIELD( ft_frame_ulong_be, f )
#define FT_FRAME_SHORT( f )      FT_FRAME_FIELD( ft_frame_short_be, f )
#define FT_FRAME_USHORT( f )     FT_FRAME_FIELD( ft_frame_ushort_be, f )
#define FT_FRAME_OFF3( f )       FT_FRAME_FIELD( ft_frame_off3_be, f )
#define FT_FRAME_UOFF3( f )      FT_FRAME_FIELD( ft_frame_uoff3_be, f )
#define FT_FRAME_BYTE( f )       FT_FRAME_FIELD( ft_frame_byte, f )
#define FT_FRAME_CHAR( f )       FT_FRAME_FIELD( ft_frame_schar, f )

#define FT_FRAME_LONG_LE( f )    FT_FRAME_FIELD( ft_frame_long_le, f )
#define FT_FRAME_ULONG_LE( f )   FT_FRAME_FIELD( ft_frame_ulong_le, f )
#define FT_FRAME_SHORT_LE( f )   FT_FRAME_FIELD( ft_frame_short_le, f )
#define FT_FRAME_USHORT_LE( f )  FT_FRAME_FIELD( ft_frame_ushort_le, f )
#define FT_FRAME_OFF3_LE( f )    FT_FRAME_FIELD( ft_frame_off3_le, f )
#define FT_FRAME_UOFF3_LE( f )   FT_FRAME_FIELD( ft_frame_uoff3_le, f )

#define FT_FRAME_SKIP_LONG       { ft_frame_long_be, 0, 0 }
#define FT_FRAME_SKIP_SHORT      { ft_frame_short_be, 0, 0 }
#define FT_FRAME_SKIP_BYTE       { ft_frame_byte, 0, 0 }

#define FT_FRAME_BYTES( field, count ) \
          {                            \
            ft_frame_bytes,            \
            count,                     \
            FT_FIELD_OFFSET( field )   \
          }

#define FT_FRAME_SKIP_BYTES( count )  { ft_frame_skip, count, 0 }

  
  
  
  
  

#define FT_BYTE_( p, i )  ( ((const FT_Byte*)(p))[(i)] )
#define FT_INT8_( p, i )  ( ((const FT_Char*)(p))[(i)] )

#define FT_INT16( x )   ( (FT_Int16)(x)  )
#define FT_UINT16( x )  ( (FT_UInt16)(x) )
#define FT_INT32( x )   ( (FT_Int32)(x)  )
#define FT_UINT32( x )  ( (FT_UInt32)(x) )

#define FT_BYTE_I16( p, i, s )  ( FT_INT16(  FT_BYTE_( p, i ) ) << (s) )
#define FT_BYTE_U16( p, i, s )  ( FT_UINT16( FT_BYTE_( p, i ) ) << (s) )
#define FT_BYTE_I32( p, i, s )  ( FT_INT32(  FT_BYTE_( p, i ) ) << (s) )
#define FT_BYTE_U32( p, i, s )  ( FT_UINT32( FT_BYTE_( p, i ) ) << (s) )

#define FT_INT8_I16( p, i, s )  ( FT_INT16(  FT_INT8_( p, i ) ) << (s) )
#define FT_INT8_U16( p, i, s )  ( FT_UINT16( FT_INT8_( p, i ) ) << (s) )
#define FT_INT8_I32( p, i, s )  ( FT_INT32(  FT_INT8_( p, i ) ) << (s) )
#define FT_INT8_U32( p, i, s )  ( FT_UINT32( FT_INT8_( p, i ) ) << (s) )

#define FT_PEEK_SHORT( p )  FT_INT16( FT_INT8_I16( p, 0, 8) | \
                                      FT_BYTE_I16( p, 1, 0) )

#define FT_PEEK_USHORT( p )  FT_UINT16( FT_BYTE_U16( p, 0, 8 ) | \
                                        FT_BYTE_U16( p, 1, 0 ) )

#define FT_PEEK_LONG( p )  FT_INT32( FT_INT8_I32( p, 0, 24 ) | \
                                     FT_BYTE_I32( p, 1, 16 ) | \
                                     FT_BYTE_I32( p, 2,  8 ) | \
                                     FT_BYTE_I32( p, 3,  0 ) )

#define FT_PEEK_ULONG( p )  FT_UINT32( FT_BYTE_U32( p, 0, 24 ) | \
                                       FT_BYTE_U32( p, 1, 16 ) | \
                                       FT_BYTE_U32( p, 2,  8 ) | \
                                       FT_BYTE_U32( p, 3,  0 ) )

#define FT_PEEK_OFF3( p )  FT_INT32( FT_INT8_I32( p, 0, 16 ) | \
                                     FT_BYTE_I32( p, 1,  8 ) | \
                                     FT_BYTE_I32( p, 2,  0 ) )

#define FT_PEEK_UOFF3( p )  FT_UINT32( FT_BYTE_U32( p, 0, 16 ) | \
                                       FT_BYTE_U32( p, 1,  8 ) | \
                                       FT_BYTE_U32( p, 2,  0 ) )

#define FT_PEEK_SHORT_LE( p )  FT_INT16( FT_INT8_I16( p, 1, 8 ) | \
                                         FT_BYTE_I16( p, 0, 0 ) )

#define FT_PEEK_USHORT_LE( p )  FT_UINT16( FT_BYTE_U16( p, 1, 8 ) |  \
                                           FT_BYTE_U16( p, 0, 0 ) )

#define FT_PEEK_LONG_LE( p )  FT_INT32( FT_INT8_I32( p, 3, 24 ) | \
                                        FT_BYTE_I32( p, 2, 16 ) | \
                                        FT_BYTE_I32( p, 1,  8 ) | \
                                        FT_BYTE_I32( p, 0,  0 ) )

#define FT_PEEK_ULONG_LE( p )  FT_UINT32( FT_BYTE_U32( p, 3, 24 ) | \
                                          FT_BYTE_U32( p, 2, 16 ) | \
                                          FT_BYTE_U32( p, 1,  8 ) | \
                                          FT_BYTE_U32( p, 0,  0 ) )

#define FT_PEEK_OFF3_LE( p )  FT_INT32( FT_INT8_I32( p, 2, 16 ) | \
                                        FT_BYTE_I32( p, 1,  8 ) | \
                                        FT_BYTE_I32( p, 0,  0 ) )

#define FT_PEEK_UOFF3_LE( p )  FT_UINT32( FT_BYTE_U32( p, 2, 16 ) | \
                                          FT_BYTE_U32( p, 1,  8 ) | \
                                          FT_BYTE_U32( p, 0,  0 ) )

#define FT_NEXT_CHAR( buffer )       \
          ( (signed char)*buffer++ )

#define FT_NEXT_BYTE( buffer )         \
          ( (unsigned char)*buffer++ )

#define FT_NEXT_SHORT( buffer )                                   \
          ( (short)( buffer += 2, FT_PEEK_SHORT( buffer - 2 ) ) )

#define FT_NEXT_USHORT( buffer )                                            \
          ( (unsigned short)( buffer += 2, FT_PEEK_USHORT( buffer - 2 ) ) )

#define FT_NEXT_OFF3( buffer )                                  \
          ( (long)( buffer += 3, FT_PEEK_OFF3( buffer - 3 ) ) )

#define FT_NEXT_UOFF3( buffer )                                           \
          ( (unsigned long)( buffer += 3, FT_PEEK_UOFF3( buffer - 3 ) ) )

#define FT_NEXT_LONG( buffer )                                  \
          ( (long)( buffer += 4, FT_PEEK_LONG( buffer - 4 ) ) )

#define FT_NEXT_ULONG( buffer )                                           \
          ( (unsigned long)( buffer += 4, FT_PEEK_ULONG( buffer - 4 ) ) )

#define FT_NEXT_SHORT_LE( buffer )                                   \
          ( (short)( buffer += 2, FT_PEEK_SHORT_LE( buffer - 2 ) ) )

#define FT_NEXT_USHORT_LE( buffer )                                            \
          ( (unsigned short)( buffer += 2, FT_PEEK_USHORT_LE( buffer - 2 ) ) )

#define FT_NEXT_OFF3_LE( buffer )                                  \
          ( (long)( buffer += 3, FT_PEEK_OFF3_LE( buffer - 3 ) ) )

#define FT_NEXT_UOFF3_LE( buffer )                                           \
          ( (unsigned long)( buffer += 3, FT_PEEK_UOFF3_LE( buffer - 3 ) ) )

#define FT_NEXT_LONG_LE( buffer )                                  \
          ( (long)( buffer += 4, FT_PEEK_LONG_LE( buffer - 4 ) ) )

#define FT_NEXT_ULONG_LE( buffer )                                           \
          ( (unsigned long)( buffer += 4, FT_PEEK_ULONG_LE( buffer - 4 ) ) )

  
  
  
  
#if 0
#define FT_GET_MACRO( type )    FT_NEXT_ ## type ( stream->cursor )

#define FT_GET_CHAR()       FT_GET_MACRO( CHAR )
#define FT_GET_BYTE()       FT_GET_MACRO( BYTE )
#define FT_GET_SHORT()      FT_GET_MACRO( SHORT )
#define FT_GET_USHORT()     FT_GET_MACRO( USHORT )
#define FT_GET_OFF3()       FT_GET_MACRO( OFF3 )
#define FT_GET_UOFF3()      FT_GET_MACRO( UOFF3 )
#define FT_GET_LONG()       FT_GET_MACRO( LONG )
#define FT_GET_ULONG()      FT_GET_MACRO( ULONG )
#define FT_GET_TAG4()       FT_GET_MACRO( ULONG )

#define FT_GET_SHORT_LE()   FT_GET_MACRO( SHORT_LE )
#define FT_GET_USHORT_LE()  FT_GET_MACRO( USHORT_LE )
#define FT_GET_LONG_LE()    FT_GET_MACRO( LONG_LE )
#define FT_GET_ULONG_LE()   FT_GET_MACRO( ULONG_LE )

#else
#define FT_GET_MACRO( func, type )        ( (type)func( stream ) )

#define FT_GET_CHAR()       FT_GET_MACRO( FT_Stream_GetChar, FT_Char )
#define FT_GET_BYTE()       FT_GET_MACRO( FT_Stream_GetChar, FT_Byte )
#define FT_GET_SHORT()      FT_GET_MACRO( FT_Stream_GetUShort, FT_Short )
#define FT_GET_USHORT()     FT_GET_MACRO( FT_Stream_GetUShort, FT_UShort )
#define FT_GET_OFF3()       FT_GET_MACRO( FT_Stream_GetUOffset, FT_Long )
#define FT_GET_UOFF3()      FT_GET_MACRO( FT_Stream_GetUOffset, FT_ULong )
#define FT_GET_LONG()       FT_GET_MACRO( FT_Stream_GetULong, FT_Long )
#define FT_GET_ULONG()      FT_GET_MACRO( FT_Stream_GetULong, FT_ULong )
#define FT_GET_TAG4()       FT_GET_MACRO( FT_Stream_GetULong, FT_ULong )

#define FT_GET_SHORT_LE()   FT_GET_MACRO( FT_Stream_GetUShortLE, FT_Short )
#define FT_GET_USHORT_LE()  FT_GET_MACRO( FT_Stream_GetUShortLE, FT_UShort )
#define FT_GET_LONG_LE()    FT_GET_MACRO( FT_Stream_GetULongLE, FT_Long )
#define FT_GET_ULONG_LE()   FT_GET_MACRO( FT_Stream_GetULongLE, FT_ULong )
#endif

#define FT_READ_MACRO( func, type, var )        \
          ( var = (type)func( stream, &error ), \
            error != FT_Err_Ok )

#define FT_READ_BYTE( var )       FT_READ_MACRO( FT_Stream_ReadChar, FT_Byte, var )
#define FT_READ_CHAR( var )       FT_READ_MACRO( FT_Stream_ReadChar, FT_Char, var )
#define FT_READ_SHORT( var )      FT_READ_MACRO( FT_Stream_ReadUShort, FT_Short, var )
#define FT_READ_USHORT( var )     FT_READ_MACRO( FT_Stream_ReadUShort, FT_UShort, var )
#define FT_READ_OFF3( var )       FT_READ_MACRO( FT_Stream_ReadUOffset, FT_Long, var )
#define FT_READ_UOFF3( var )      FT_READ_MACRO( FT_Stream_ReadUOffset, FT_ULong, var )
#define FT_READ_LONG( var )       FT_READ_MACRO( FT_Stream_ReadULong, FT_Long, var )
#define FT_READ_ULONG( var )      FT_READ_MACRO( FT_Stream_ReadULong, FT_ULong, var )

#define FT_READ_SHORT_LE( var )   FT_READ_MACRO( FT_Stream_ReadUShortLE, FT_Short, var )
#define FT_READ_USHORT_LE( var )  FT_READ_MACRO( FT_Stream_ReadUShortLE, FT_UShort, var )
#define FT_READ_LONG_LE( var )    FT_READ_MACRO( FT_Stream_ReadULongLE, FT_Long, var )
#define FT_READ_ULONG_LE( var )   FT_READ_MACRO( FT_Stream_ReadULongLE, FT_ULong, var )

#ifndef FT_CONFIG_OPTION_NO_DEFAULT_SYSTEM

  
  FT_BASE( FT_Error )
  FT_Stream_Open( FT_Stream    stream,
                  const char*  filepathname );

#endif 

  
  FT_BASE( FT_Error )
  FT_Stream_New( FT_Library           library,
                 const FT_Open_Args*  args,
                 FT_Stream           *astream );

  
  FT_BASE( void )
  FT_Stream_Free( FT_Stream  stream,
                  FT_Int     external );

  
  FT_BASE( void )
  FT_Stream_OpenMemory( FT_Stream       stream,
                        const FT_Byte*  base,
                        FT_ULong        size );

  
  FT_BASE( void )
  FT_Stream_Close( FT_Stream  stream );

  
  FT_BASE( FT_Error )
  FT_Stream_Seek( FT_Stream  stream,
                  FT_ULong   pos );

  
  FT_BASE( FT_Error )
  FT_Stream_Skip( FT_Stream  stream,
                  FT_Long    distance );

  
  FT_BASE( FT_Long )
  FT_Stream_Pos( FT_Stream  stream );

  
  
  FT_BASE( FT_Error )
  FT_Stream_Read( FT_Stream  stream,
                  FT_Byte*   buffer,
                  FT_ULong   count );

  
  FT_BASE( FT_Error )
  FT_Stream_ReadAt( FT_Stream  stream,
                    FT_ULong   pos,
                    FT_Byte*   buffer,
                    FT_ULong   count );

  
  
  FT_BASE( FT_ULong )
  FT_Stream_TryRead( FT_Stream  stream,
                     FT_Byte*   buffer,
                     FT_ULong   count );

  
  
  
  
  
  
  
  
  FT_BASE( FT_Error )
  FT_Stream_EnterFrame( FT_Stream  stream,
                        FT_ULong   count );

  
  FT_BASE( void )
  FT_Stream_ExitFrame( FT_Stream  stream );

  
  
  
  
  
  
  
  
  
  FT_BASE( FT_Error )
  FT_Stream_ExtractFrame( FT_Stream  stream,
                          FT_ULong   count,
                          FT_Byte**  pbytes );

  
  FT_BASE( void )
  FT_Stream_ReleaseFrame( FT_Stream  stream,
                          FT_Byte**  pbytes );

  
  FT_BASE( FT_Char )
  FT_Stream_GetChar( FT_Stream  stream );

  
  FT_BASE( FT_UShort )
  FT_Stream_GetUShort( FT_Stream  stream );

  
  FT_BASE( FT_ULong )
  FT_Stream_GetUOffset( FT_Stream  stream );

  
  FT_BASE( FT_ULong )
  FT_Stream_GetULong( FT_Stream  stream );

  
  FT_BASE( FT_UShort )
  FT_Stream_GetUShortLE( FT_Stream  stream );

  
  FT_BASE( FT_ULong )
  FT_Stream_GetULongLE( FT_Stream  stream );

  
  FT_BASE( FT_Char )
  FT_Stream_ReadChar( FT_Stream  stream,
                      FT_Error*  error );

  
  FT_BASE( FT_UShort )
  FT_Stream_ReadUShort( FT_Stream  stream,
                        FT_Error*  error );

  
  FT_BASE( FT_ULong )
  FT_Stream_ReadUOffset( FT_Stream  stream,
                         FT_Error*  error );

  
  FT_BASE( FT_ULong )
  FT_Stream_ReadULong( FT_Stream  stream,
                       FT_Error*  error );

  
  FT_BASE( FT_UShort )
  FT_Stream_ReadUShortLE( FT_Stream  stream,
                          FT_Error*  error );

  
  FT_BASE( FT_ULong )
  FT_Stream_ReadULongLE( FT_Stream  stream,
                         FT_Error*  error );

  
  
  FT_BASE( FT_Error )
  FT_Stream_ReadFields( FT_Stream              stream,
                        const FT_Frame_Field*  fields,
                        void*                  structure );

#define FT_STREAM_POS()           \
          FT_Stream_Pos( stream )

#define FT_STREAM_SEEK( position )                           \
          FT_SET_ERROR( FT_Stream_Seek( stream, position ) )

#define FT_STREAM_SKIP( distance )                           \
          FT_SET_ERROR( FT_Stream_Skip( stream, distance ) )

#define FT_STREAM_READ( buffer, count )                   \
          FT_SET_ERROR( FT_Stream_Read( stream,           \
                                        (FT_Byte*)buffer, \
                                        count ) )

#define FT_STREAM_READ_AT( position, buffer, count )         \
          FT_SET_ERROR( FT_Stream_ReadAt( stream,            \
                                           position,         \
                                           (FT_Byte*)buffer, \
                                           count ) )

#define FT_STREAM_READ_FIELDS( fields, object )                          \
          FT_SET_ERROR( FT_Stream_ReadFields( stream, fields, object ) )

#define FT_FRAME_ENTER( size )                                       \
          FT_SET_ERROR(                                              \
            FT_DEBUG_INNER( FT_Stream_EnterFrame( stream, size ) ) )

#define FT_FRAME_EXIT()                 \
          FT_DEBUG_INNER( FT_Stream_ExitFrame( stream ) )

#define FT_FRAME_EXTRACT( size, bytes )                                       \
          FT_SET_ERROR(                                                       \
            FT_DEBUG_INNER( FT_Stream_ExtractFrame( stream, size,             \
                                                    (FT_Byte**)&(bytes) ) ) )

#define FT_FRAME_RELEASE( bytes )                                         \
          FT_DEBUG_INNER( FT_Stream_ReleaseFrame( stream,                 \
                                                  (FT_Byte**)&(bytes) ) )

FT_END_HEADER

#endif 

