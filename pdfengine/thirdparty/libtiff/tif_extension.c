

#include "tiffiop.h"

int TIFFGetTagListCount( TIFF *tif )

{
    TIFFDirectory* td = &tif->tif_dir;
    
    return td->td_customValueCount;
}

ttag_t TIFFGetTagListEntry( TIFF *tif, int tag_index )

{
    TIFFDirectory* td = &tif->tif_dir;

    if( tag_index < 0 || tag_index >= td->td_customValueCount )
        return (ttag_t) -1;
    else
        return td->td_customValues[tag_index].info->field_tag;
}

TIFFTagMethods *TIFFAccessTagMethods( TIFF *tif )

{
    return &(tif->tif_tagmethods);
}

void *TIFFGetClientInfo( TIFF *tif, const char *name )

{
    TIFFClientInfoLink *link = tif->tif_clientinfo;

    while( link != NULL && strcmp(link->name,name) != 0 )
        link = link->next;

    if( link != NULL )
        return link->data;
    else
        return NULL;
}

void TIFFSetClientInfo( TIFF *tif, void *data, const char *name )

{
    TIFFClientInfoLink *link = tif->tif_clientinfo;

    
    while( link != NULL && strcmp(link->name,name) != 0 )
        link = link->next;

    if( link != NULL )
    {
        link->data = data;
        return;
    }

    

    link = (TIFFClientInfoLink *) _TIFFmalloc(sizeof(TIFFClientInfoLink));
    assert (link != NULL);
    link->next = tif->tif_clientinfo;
    link->name = (char *) _TIFFmalloc(strlen(name)+1);
    assert (link->name != NULL);
    strcpy(link->name, name);
    link->data = data;

    tif->tif_clientinfo = link;
}

