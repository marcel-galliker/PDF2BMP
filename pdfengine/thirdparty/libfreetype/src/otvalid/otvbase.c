

#include "otvalid.h"
#include "otvcommn.h"

  
  
  
  
  
  
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_otvbase

  static void
  otv_BaseCoord_validate( FT_Bytes       table,
                          OTV_Validator  valid )
  {
    FT_Bytes  p = table;
    FT_UInt   BaseCoordFormat;

    OTV_NAME_ENTER( "BaseCoord" );

    OTV_LIMIT_CHECK( 4 );
    BaseCoordFormat = FT_NEXT_USHORT( p );
    p += 2;     

    OTV_TRACE(( " (format %d)\n", BaseCoordFormat ));

    switch ( BaseCoordFormat )
    {
    case 1:     
      break;

    case 2:     
      OTV_LIMIT_CHECK( 4 );   
      break;

    case 3:     
      OTV_LIMIT_CHECK( 2 );
      
      otv_Device_validate( table + FT_NEXT_USHORT( p ), valid );
      break;

    default:
      FT_INVALID_FORMAT;
    }

    OTV_EXIT;
  }

  static void
  otv_BaseTagList_validate( FT_Bytes       table,
                            OTV_Validator  valid )
  {
    FT_Bytes  p = table;
    FT_UInt   BaseTagCount;

    OTV_NAME_ENTER( "BaseTagList" );

    OTV_LIMIT_CHECK( 2 );

    BaseTagCount = FT_NEXT_USHORT( p );

    OTV_TRACE(( " (BaseTagCount = %d)\n", BaseTagCount ));

    OTV_LIMIT_CHECK( BaseTagCount * 4 );          

    OTV_EXIT;
  }

  static void
  otv_BaseValues_validate( FT_Bytes       table,
                           OTV_Validator  valid )
  {
    FT_Bytes  p = table;
    FT_UInt   BaseCoordCount;

    OTV_NAME_ENTER( "BaseValues" );

    OTV_LIMIT_CHECK( 4 );

    p             += 2;                     
    BaseCoordCount = FT_NEXT_USHORT( p );

    OTV_TRACE(( " (BaseCoordCount = %d)\n", BaseCoordCount ));

    OTV_LIMIT_CHECK( BaseCoordCount * 2 );

    
    for ( ; BaseCoordCount > 0; BaseCoordCount-- )
      otv_BaseCoord_validate( table + FT_NEXT_USHORT( p ), valid );

    OTV_EXIT;
  }

  static void
  otv_MinMax_validate( FT_Bytes       table,
                       OTV_Validator  valid )
  {
    FT_Bytes  p = table;
    FT_UInt   table_size;
    FT_UInt   FeatMinMaxCount;

    OTV_OPTIONAL_TABLE( MinCoord );
    OTV_OPTIONAL_TABLE( MaxCoord );

    OTV_NAME_ENTER( "MinMax" );

    OTV_LIMIT_CHECK( 6 );

    OTV_OPTIONAL_OFFSET( MinCoord );
    OTV_OPTIONAL_OFFSET( MaxCoord );
    FeatMinMaxCount = FT_NEXT_USHORT( p );

    OTV_TRACE(( " (FeatMinMaxCount = %d)\n", FeatMinMaxCount ));

    table_size = FeatMinMaxCount * 8 + 6;

    OTV_SIZE_CHECK( MinCoord );
    if ( MinCoord )
      otv_BaseCoord_validate( table + MinCoord, valid );

    OTV_SIZE_CHECK( MaxCoord );
    if ( MaxCoord )
      otv_BaseCoord_validate( table + MaxCoord, valid );

    OTV_LIMIT_CHECK( FeatMinMaxCount * 8 );

    
    for ( ; FeatMinMaxCount > 0; FeatMinMaxCount-- )
    {
      p += 4;                           

      OTV_OPTIONAL_OFFSET( MinCoord );
      OTV_OPTIONAL_OFFSET( MaxCoord );

      OTV_SIZE_CHECK( MinCoord );
      if ( MinCoord )
        otv_BaseCoord_validate( table + MinCoord, valid );

      OTV_SIZE_CHECK( MaxCoord );
      if ( MaxCoord )
        otv_BaseCoord_validate( table + MaxCoord, valid );
    }

    OTV_EXIT;
  }

  static void
  otv_BaseScript_validate( FT_Bytes       table,
                           OTV_Validator  valid )
  {
    FT_Bytes  p = table;
    FT_UInt   table_size;
    FT_UInt   BaseLangSysCount;

    OTV_OPTIONAL_TABLE( BaseValues    );
    OTV_OPTIONAL_TABLE( DefaultMinMax );

    OTV_NAME_ENTER( "BaseScript" );

    OTV_LIMIT_CHECK( 6 );
    OTV_OPTIONAL_OFFSET( BaseValues    );
    OTV_OPTIONAL_OFFSET( DefaultMinMax );
    BaseLangSysCount = FT_NEXT_USHORT( p );

    OTV_TRACE(( " (BaseLangSysCount = %d)\n", BaseLangSysCount ));

    table_size = BaseLangSysCount * 6 + 6;

    OTV_SIZE_CHECK( BaseValues );
    if ( BaseValues )
      otv_BaseValues_validate( table + BaseValues, valid );

    OTV_SIZE_CHECK( DefaultMinMax );
    if ( DefaultMinMax )
      otv_MinMax_validate( table + DefaultMinMax, valid );

    OTV_LIMIT_CHECK( BaseLangSysCount * 6 );

    
    for ( ; BaseLangSysCount > 0; BaseLangSysCount-- )
    {
      p += 4;       

      otv_MinMax_validate( table + FT_NEXT_USHORT( p ), valid );
    }

    OTV_EXIT;
  }

  static void
  otv_BaseScriptList_validate( FT_Bytes       table,
                               OTV_Validator  valid )
  {
    FT_Bytes  p = table;
    FT_UInt   BaseScriptCount;

    OTV_NAME_ENTER( "BaseScriptList" );

    OTV_LIMIT_CHECK( 2 );
    BaseScriptCount = FT_NEXT_USHORT( p );

    OTV_TRACE(( " (BaseScriptCount = %d)\n", BaseScriptCount ));

    OTV_LIMIT_CHECK( BaseScriptCount * 6 );

    
    for ( ; BaseScriptCount > 0; BaseScriptCount-- )
    {
      p += 4;       

      
      otv_BaseScript_validate( table + FT_NEXT_USHORT( p ), valid );
    }

    OTV_EXIT;
  }

  static void
  otv_Axis_validate( FT_Bytes       table,
                     OTV_Validator  valid )
  {
    FT_Bytes  p = table;
    FT_UInt   table_size;

    OTV_OPTIONAL_TABLE( BaseTagList );

    OTV_NAME_ENTER( "Axis" );

    OTV_LIMIT_CHECK( 4 );
    OTV_OPTIONAL_OFFSET( BaseTagList );

    table_size = 4;

    OTV_SIZE_CHECK( BaseTagList );
    if ( BaseTagList )
      otv_BaseTagList_validate( table + BaseTagList, valid );

    
    otv_BaseScriptList_validate( table + FT_NEXT_USHORT( p ), valid );

    OTV_EXIT;
  }

  FT_LOCAL_DEF( void )
  otv_BASE_validate( FT_Bytes      table,
                     FT_Validator  ftvalid )
  {
    OTV_ValidatorRec  validrec;
    OTV_Validator     valid = &validrec;
    FT_Bytes          p     = table;
    FT_UInt           table_size;

    OTV_OPTIONAL_TABLE( HorizAxis );
    OTV_OPTIONAL_TABLE( VertAxis  );

    valid->root = ftvalid;

    FT_TRACE3(( "validating BASE table\n" ));
    OTV_INIT;

    OTV_LIMIT_CHECK( 6 );

    if ( FT_NEXT_ULONG( p ) != 0x10000UL )      
      FT_INVALID_FORMAT;

    table_size = 6;

    OTV_OPTIONAL_OFFSET( HorizAxis );
    OTV_SIZE_CHECK( HorizAxis );
    if ( HorizAxis )
      otv_Axis_validate( table + HorizAxis, valid );

    OTV_OPTIONAL_OFFSET( VertAxis );
    OTV_SIZE_CHECK( VertAxis );
    if ( VertAxis )
      otv_Axis_validate( table + VertAxis, valid );

    FT_TRACE4(( "\n" ));
  }
