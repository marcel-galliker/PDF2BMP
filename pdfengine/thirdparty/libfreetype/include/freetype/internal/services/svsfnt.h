

#ifndef __SVSFNT_H__
#define __SVSFNT_H__

#include FT_INTERNAL_SERVICE_H
#include FT_TRUETYPE_TABLES_H

FT_BEGIN_HEADER

  

#define FT_SERVICE_ID_SFNT_TABLE  "sfnt-table"

  
  typedef FT_Error
  (*FT_SFNT_TableLoadFunc)( FT_Face    face,
                            FT_ULong   tag,
                            FT_Long    offset,
                            FT_Byte*   buffer,
                            FT_ULong*  length );

  
  typedef void*
  (*FT_SFNT_TableGetFunc)( FT_Face      face,
                           FT_Sfnt_Tag  tag );

  
  typedef FT_Error
  (*FT_SFNT_TableInfoFunc)( FT_Face    face,
                            FT_UInt    idx,
                            FT_ULong  *tag,
                            FT_ULong  *offset,
                            FT_ULong  *length );

  FT_DEFINE_SERVICE( SFNT_Table )
  {
    FT_SFNT_TableLoadFunc  load_table;
    FT_SFNT_TableGetFunc   get_table;
    FT_SFNT_TableInfoFunc  table_info;
  };

#ifndef FT_CONFIG_OPTION_PIC

#define FT_DEFINE_SERVICE_SFNT_TABLEREC( class_, load_, get_, info_ )  \
  static const FT_Service_SFNT_TableRec  class_ =                      \
  {                                                                    \
    load_, get_, info_                                                 \
  };

#else 

#define FT_DEFINE_SERVICE_SFNT_TABLEREC( class_, load_, get_, info_ ) \
  void                                                                \
  FT_Init_Class_ ## class_( FT_Service_SFNT_TableRec*  clazz )        \
  {                                                                   \
    clazz->load_table = load_;                                        \
    clazz->get_table  = get_;                                         \
    clazz->table_info = info_;                                        \
  }

#endif 

  

FT_END_HEADER

#endif 

