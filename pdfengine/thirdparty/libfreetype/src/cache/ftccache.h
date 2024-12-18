

#ifndef __FTCCACHE_H__
#define __FTCCACHE_H__

#include "ftcmru.h"

FT_BEGIN_HEADER

#define _FTC_FACE_ID_HASH( i )                                                \
          ((FT_PtrDist)(( (FT_PtrDist)(i) >> 3 ) ^ ( (FT_PtrDist)(i) << 7 )))

  
  typedef struct FTC_CacheRec_*  FTC_Cache;

  
  typedef const struct FTC_CacheClassRec_*  FTC_CacheClass;

  
  
  
  
  
  
  

  
  
  
  
  
  
  
  
  
  
  

  
  typedef struct  FTC_NodeRec_
  {
    FTC_MruNodeRec  mru;          
    FTC_Node        link;         
    FT_PtrDist      hash;         
    FT_UShort       cache_index;  
    FT_Short        ref_count;    

  } FTC_NodeRec;

#define FTC_NODE( x )    ( (FTC_Node)(x) )
#define FTC_NODE_P( x )  ( (FTC_Node*)(x) )

#define FTC_NODE__NEXT( x )  FTC_NODE( (x)->mru.next )
#define FTC_NODE__PREV( x )  FTC_NODE( (x)->mru.prev )

#ifdef FTC_INLINE
#define FTC_NODE__TOP_FOR_HASH( cache, hash )                     \
        ( ( cache )->buckets +                                    \
            ( ( ( ( hash ) &   ( cache )->mask ) < ( cache )->p ) \
              ? ( ( hash ) & ( ( cache )->mask * 2 + 1 ) )        \
              : ( ( hash ) &   ( cache )->mask ) ) )
#else
  FT_LOCAL( FTC_Node* )
  ftc_get_top_node_for_hash( FTC_Cache   cache,
                             FT_PtrDist  hash );
#define FTC_NODE__TOP_FOR_HASH( cache, hash )            \
        ftc_get_top_node_for_hash( ( cache ), ( hash ) )
#endif

#ifdef FT_CONFIG_OPTION_OLD_INTERNALS
  FT_BASE( void )
  ftc_node_destroy( FTC_Node     node,
                    FTC_Manager  manager );
#endif

  
  
  
  
  
  
  

  
  typedef FT_Error
  (*FTC_Node_NewFunc)( FTC_Node    *pnode,
                       FT_Pointer   query,
                       FTC_Cache    cache );

  typedef FT_Offset
  (*FTC_Node_WeightFunc)( FTC_Node   node,
                          FTC_Cache  cache );

  
  typedef FT_Bool
  (*FTC_Node_CompareFunc)( FTC_Node    node,
                           FT_Pointer  key,
                           FTC_Cache   cache,
                           FT_Bool*    list_changed );

  typedef void
  (*FTC_Node_FreeFunc)( FTC_Node   node,
                        FTC_Cache  cache );

  typedef FT_Error
  (*FTC_Cache_InitFunc)( FTC_Cache  cache );

  typedef void
  (*FTC_Cache_DoneFunc)( FTC_Cache  cache );

  typedef struct  FTC_CacheClassRec_
  {
    FTC_Node_NewFunc      node_new;
    FTC_Node_WeightFunc   node_weight;
    FTC_Node_CompareFunc  node_compare;
    FTC_Node_CompareFunc  node_remove_faceid;
    FTC_Node_FreeFunc     node_free;

    FT_Offset             cache_size;
    FTC_Cache_InitFunc    cache_init;
    FTC_Cache_DoneFunc    cache_done;

  } FTC_CacheClassRec;

  
  typedef struct  FTC_CacheRec_
  {
    FT_UFast           p;
    FT_UFast           mask;
    FT_Long            slack;
    FTC_Node*          buckets;

    FTC_CacheClassRec  clazz;       

    FTC_Manager        manager;
    FT_Memory          memory;
    FT_UInt            index;       

    FTC_CacheClass     org_class;   

  } FTC_CacheRec;

#define FTC_CACHE( x )    ( (FTC_Cache)(x) )
#define FTC_CACHE_P( x )  ( (FTC_Cache*)(x) )

  
  FT_LOCAL( FT_Error )
  FTC_Cache_Init( FTC_Cache  cache );

  
  FT_LOCAL( void )
  FTC_Cache_Done( FTC_Cache  cache );

  

#ifndef FTC_INLINE
  FT_LOCAL( FT_Error )
  FTC_Cache_Lookup( FTC_Cache   cache,
                    FT_PtrDist  hash,
                    FT_Pointer  query,
                    FTC_Node   *anode );
#endif

  FT_LOCAL( FT_Error )
  FTC_Cache_NewNode( FTC_Cache   cache,
                     FT_PtrDist  hash,
                     FT_Pointer  query,
                     FTC_Node   *anode );

  
  FT_LOCAL( void )
  FTC_Cache_RemoveFaceID( FTC_Cache   cache,
                          FTC_FaceID  face_id );

#ifdef FTC_INLINE

#define FTC_CACHE_LOOKUP_CMP( cache, nodecmp, hash, query, node, error ) \
  FT_BEGIN_STMNT                                                         \
    FTC_Node             *_bucket, *_pnode, _node;                       \
    FTC_Cache             _cache   = FTC_CACHE(cache);                   \
    FT_PtrDist            _hash    = (FT_PtrDist)(hash);                 \
    FTC_Node_CompareFunc  _nodcomp = (FTC_Node_CompareFunc)(nodecmp);    \
    FT_Bool               _list_changed = FALSE;                         \
                                                                         \
                                                                         \
    error = FTC_Err_Ok;                                                  \
    node  = NULL;                                                        \
                                                                         \
          \
    _bucket = _pnode = FTC_NODE__TOP_FOR_HASH( _cache, _hash );          \
                                                                         \
      \
      \
    for (;;)                                                             \
    {                                                                    \
      _node = *_pnode;                                                   \
      if ( _node == NULL )                                               \
        goto _NewNode;                                                   \
                                                                         \
      if ( _node->hash == _hash                             &&           \
           _nodcomp( _node, query, _cache, &_list_changed ) )            \
        break;                                                           \
                                                                         \
      _pnode = &_node->link;                                             \
    }                                                                    \
                                                                         \
    if ( _list_changed )                                                 \
    {                                                                    \
                    \
      _bucket = _pnode = FTC_NODE__TOP_FOR_HASH( _cache, _hash );        \
                                                                         \
                     \
      while ( *_pnode != _node )                                         \
      {                                                                  \
        if ( *_pnode == NULL )                                           \
        {                                                                \
          FT_ERROR(( "FTC_CACHE_LOOKUP_CMP: oops!!! node missing\n" ));  \
          goto _NewNode;                                                 \
        }                                                                \
        else                                                             \
          _pnode = &((*_pnode)->link);                                   \
      }                                                                  \
    }                                                                    \
                                                                         \
               \
    if ( _node != *_bucket )                                             \
    {                                                                    \
      *_pnode     = _node->link;                                         \
      _node->link = *_bucket;                                            \
      *_bucket    = _node;                                               \
    }                                                                    \
                                                                         \
                                                    \
    {                                                                    \
      FTC_Manager  _manager = _cache->manager;                           \
      void*        _nl      = &_manager->nodes_list;                     \
                                                                         \
                                                                         \
      if ( _node != _manager->nodes_list )                               \
        FTC_MruNode_Up( (FTC_MruNode*)_nl,                               \
                        (FTC_MruNode)_node );                            \
    }                                                                    \
    goto _Ok;                                                            \
                                                                         \
  _NewNode:                                                              \
    error = FTC_Cache_NewNode( _cache, _hash, query, &_node );           \
                                                                         \
  _Ok:                                                                   \
    node = _node;                                                        \
  FT_END_STMNT

#else 

#define FTC_CACHE_LOOKUP_CMP( cache, nodecmp, hash, query, node, error ) \
  FT_BEGIN_STMNT                                                         \
    error = FTC_Cache_Lookup( FTC_CACHE( cache ), hash, query,           \
                              (FTC_Node*)&(node) );                      \
  FT_END_STMNT

#endif 

  
#define FTC_CACHE_TRYLOOP( cache )                           \
  {                                                          \
    FTC_Manager  _try_manager = FTC_CACHE( cache )->manager; \
    FT_UInt      _try_count   = 4;                           \
                                                             \
                                                             \
    for (;;)                                                 \
    {                                                        \
      FT_UInt  _try_done;

#define FTC_CACHE_TRYLOOP_END( list_changed )                     \
      if ( !error || error != FTC_Err_Out_Of_Memory )             \
        break;                                                    \
                                                                  \
      _try_done = FTC_Manager_FlushN( _try_manager, _try_count ); \
      if ( _try_done > 0 && ( list_changed ) )                    \
        *(FT_Bool*)( list_changed ) = TRUE;                       \
                                                                  \
      if ( _try_done == 0 )                                       \
        break;                                                    \
                                                                  \
      if ( _try_done == _try_count )                              \
      {                                                           \
        _try_count *= 2;                                          \
        if ( _try_count < _try_done              ||               \
            _try_count > _try_manager->num_nodes )                \
          _try_count = _try_manager->num_nodes;                   \
      }                                                           \
    }                                                             \
  }

 

FT_END_HEADER

#endif 

