

#include "gxvmort.h"
#include "gxvfeat.h"

  
  
  
  
  
  
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_gxvmort

  static void
  gxv_mort_feature_validate( GXV_mort_feature  f,
                             GXV_Validator     valid )
  {
    if ( f->featureType >= gxv_feat_registry_length )
    {
      GXV_TRACE(( "featureType %d is out of registered range, "
                  "setting %d is unchecked\n",
                  f->featureType, f->featureSetting ));
      GXV_SET_ERR_IF_PARANOID( FT_INVALID_DATA );
    }
    else if ( !gxv_feat_registry[f->featureType].existence )
    {
      GXV_TRACE(( "featureType %d is within registered area "
                  "but undefined, setting %d is unchecked\n",
                  f->featureType, f->featureSetting ));
      GXV_SET_ERR_IF_PARANOID( FT_INVALID_DATA );
    }
    else
    {
      FT_Byte  nSettings_max;

      
      nSettings_max = gxv_feat_registry[f->featureType].nSettings;
      if ( gxv_feat_registry[f->featureType].exclusive )
        nSettings_max = (FT_Byte)( 2 * nSettings_max );

      GXV_TRACE(( "featureType %d is registered", f->featureType ));
      GXV_TRACE(( "setting %d", f->featureSetting ));

      if ( f->featureSetting > nSettings_max )
      {
        GXV_TRACE(( "out of defined range %d", nSettings_max ));
        GXV_SET_ERR_IF_PARANOID( FT_INVALID_DATA );
      }
      GXV_TRACE(( "\n" ));
    }

    
  }

  
  FT_LOCAL_DEF( void )
  gxv_mort_featurearray_validate( FT_Bytes       table,
                                  FT_Bytes       limit,
                                  FT_ULong       nFeatureFlags,
                                  GXV_Validator  valid )
  {
    FT_Bytes  p = table;
    FT_ULong  i;

    GXV_mort_featureRec  f = GXV_MORT_FEATURE_OFF;

    GXV_NAME_ENTER( "mort feature list" );
    for ( i = 0; i < nFeatureFlags; i++ )
    {
      GXV_LIMIT_CHECK( 2 + 2 + 4 + 4 );
      f.featureType    = FT_NEXT_USHORT( p );
      f.featureSetting = FT_NEXT_USHORT( p );
      f.enableFlags    = FT_NEXT_ULONG( p );
      f.disableFlags   = FT_NEXT_ULONG( p );

      gxv_mort_feature_validate( &f, valid );
    }

    if ( !IS_GXV_MORT_FEATURE_OFF( f ) )
      FT_INVALID_DATA;

    valid->subtable_length = p - table;
    GXV_EXIT;
  }

  FT_LOCAL_DEF( void )
  gxv_mort_coverage_validate( FT_UShort      coverage,
                              GXV_Validator  valid )
  {
    FT_UNUSED( valid );

    if ( coverage & 0x8000U )
      GXV_TRACE(( " this subtable is for vertical text only\n" ));
    else
      GXV_TRACE(( " this subtable is for horizontal text only\n" ));

    if ( coverage & 0x4000 )
      GXV_TRACE(( " this subtable is applied to glyph array "
                  "in descending order\n" ));
    else
      GXV_TRACE(( " this subtable is applied to glyph array "
                  "in ascending order\n" ));

    if ( coverage & 0x2000 )
      GXV_TRACE(( " this subtable is forcibly applied to "
                  "vertical/horizontal text\n" ));

    if ( coverage & 0x1FF8 )
      GXV_TRACE(( " coverage has non-zero bits in reserved area\n" ));
  }

  static void
  gxv_mort_subtables_validate( FT_Bytes       table,
                               FT_Bytes       limit,
                               FT_UShort      nSubtables,
                               GXV_Validator  valid )
  {
    FT_Bytes  p = table;

    GXV_Validate_Func fmt_funcs_table[] =
    {
      gxv_mort_subtable_type0_validate, 
      gxv_mort_subtable_type1_validate, 
      gxv_mort_subtable_type2_validate, 
      NULL,                             
      gxv_mort_subtable_type4_validate, 
      gxv_mort_subtable_type5_validate, 

    };

    GXV_Validate_Func  func;
    FT_UShort          i;

    GXV_NAME_ENTER( "subtables in a chain" );

    for ( i = 0; i < nSubtables; i++ )
    {
      FT_UShort  length;
      FT_UShort  coverage;
#ifdef GXV_LOAD_UNUSED_VARS
      FT_ULong   subFeatureFlags;
#endif
      FT_UInt    type;
      FT_UInt    rest;

      GXV_LIMIT_CHECK( 2 + 2 + 4 );
      length          = FT_NEXT_USHORT( p );
      coverage        = FT_NEXT_USHORT( p );
#ifdef GXV_LOAD_UNUSED_VARS
      subFeatureFlags = FT_NEXT_ULONG( p );
#else
      p += 4;
#endif

      GXV_TRACE(( "validating chain subtable %d/%d (%d bytes)\n",
                  i + 1, nSubtables, length ));
      type = coverage & 0x0007;
      rest = length - ( 2 + 2 + 4 );

      GXV_LIMIT_CHECK( rest );
      gxv_mort_coverage_validate( coverage, valid );

      if ( type > 5 )
        FT_INVALID_FORMAT;

      func = fmt_funcs_table[type];
      if ( func == NULL )
        GXV_TRACE(( "morx type %d is reserved\n", type ));

      func( p, p + rest, valid );

      p += rest;
      
    }

    valid->subtable_length = p - table;

    GXV_EXIT;
  }

  static void
  gxv_mort_chain_validate( FT_Bytes       table,
                           FT_Bytes       limit,
                           GXV_Validator  valid )
  {
    FT_Bytes   p = table;
#ifdef GXV_LOAD_UNUSED_VARS
    FT_ULong   defaultFlags;
#endif
    FT_ULong   chainLength;
    FT_UShort  nFeatureFlags;
    FT_UShort  nSubtables;

    GXV_NAME_ENTER( "mort chain header" );

    GXV_LIMIT_CHECK( 4 + 4 + 2 + 2 );
#ifdef GXV_LOAD_UNUSED_VARS
    defaultFlags  = FT_NEXT_ULONG( p );
#else
    p += 4;
#endif
    chainLength   = FT_NEXT_ULONG( p );
    nFeatureFlags = FT_NEXT_USHORT( p );
    nSubtables    = FT_NEXT_USHORT( p );

    gxv_mort_featurearray_validate( p, table + chainLength,
                                    nFeatureFlags, valid );
    p += valid->subtable_length;
    gxv_mort_subtables_validate( p, table + chainLength, nSubtables, valid );
    valid->subtable_length = chainLength;

    
    GXV_EXIT;
  }

  FT_LOCAL_DEF( void )
  gxv_mort_validate( FT_Bytes      table,
                     FT_Face       face,
                     FT_Validator  ftvalid )
  {
    GXV_ValidatorRec  validrec;
    GXV_Validator     valid = &validrec;
    FT_Bytes          p     = table;
    FT_Bytes          limit = 0;
    FT_ULong          version;
    FT_ULong          nChains;
    FT_ULong          i;

    valid->root = ftvalid;
    valid->face = face;
    limit       = valid->root->limit;

    FT_TRACE3(( "validating `mort' table\n" ));
    GXV_INIT;

    GXV_LIMIT_CHECK( 4 + 4 );
    version = FT_NEXT_ULONG( p );
    nChains = FT_NEXT_ULONG( p );

    if (version != 0x00010000UL)
      FT_INVALID_FORMAT;

    for ( i = 0; i < nChains; i++ )
    {
      GXV_TRACE(( "validating chain %d/%d\n", i + 1, nChains ));
      GXV_32BIT_ALIGNMENT_VALIDATE( p - table );
      gxv_mort_chain_validate( p, limit, valid );
      p += valid->subtable_length;
    }

    FT_TRACE4(( "\n" ));
  }
