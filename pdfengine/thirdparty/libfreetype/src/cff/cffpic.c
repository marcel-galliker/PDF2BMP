

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_INTERNAL_OBJECTS_H
#include "cffcmap.h"
#include "cffpic.h"
#include "cfferrs.h"

#ifdef FT_CONFIG_OPTION_PIC

  
  FT_Error
  FT_Create_Class_cff_services(
    FT_Library           library,
    FT_ServiceDescRec**  output_class );

  void
  FT_Destroy_Class_cff_services(
    FT_Library          library,
    FT_ServiceDescRec*  clazz );

  void
  FT_Init_Class_cff_service_ps_info(
    FT_Library             library,
    FT_Service_PsInfoRec*  clazz );

  void
  FT_Init_Class_cff_service_glyph_dict(
    FT_Library                library,
    FT_Service_GlyphDictRec*  clazz );

  void
  FT_Init_Class_cff_service_ps_name(
    FT_Library                 library,
    FT_Service_PsFontNameRec*  clazz );

  void
  FT_Init_Class_cff_service_get_cmap_info(
    FT_Library              library,
    FT_Service_TTCMapsRec*  clazz );

  void
  FT_Init_Class_cff_service_cid_info(
    FT_Library          library,
    FT_Service_CIDRec*  clazz );

  
  FT_Error
  FT_Create_Class_cff_field_handlers(
    FT_Library           library,
    CFF_Field_Handler**  output_class );

  void
  FT_Destroy_Class_cff_field_handlers(
    FT_Library          library,
    CFF_Field_Handler*  clazz );

  void
  cff_driver_class_pic_free( FT_Library  library )
  {
    FT_PIC_Container*  pic_container = &library->pic_container;
    FT_Memory  memory = library->memory;

    if ( pic_container->cff )
    {
      CffModulePIC*  container = ( CffModulePIC* )pic_container->cff;

      if ( container->cff_services )
        FT_Destroy_Class_cff_services( library,
                                       container->cff_services );
      container->cff_services = NULL;
      if ( container->cff_field_handlers )
        FT_Destroy_Class_cff_field_handlers(
          library, container->cff_field_handlers );
      container->cff_field_handlers = NULL;
      FT_FREE( container );
      pic_container->cff = NULL;
    }
  }

  FT_Error
  cff_driver_class_pic_init( FT_Library  library )
  {
    FT_PIC_Container*  pic_container = &library->pic_container;
    FT_Error           error         = CFF_Err_Ok;
    CffModulePIC*      container;
    FT_Memory          memory        = library->memory;

    
    if ( FT_ALLOC ( container, sizeof ( *container ) ) )
      return error;
    FT_MEM_SET( container, 0, sizeof ( *container ) );
    pic_container->cff = container;

    
    error = FT_Create_Class_cff_services( library,
                                          &container->cff_services );
    if ( error )
      goto Exit;
    error = FT_Create_Class_cff_field_handlers(
              library, &container->cff_field_handlers );
    if ( error )
      goto Exit;
    FT_Init_Class_cff_service_ps_info(
      library, &container->cff_service_ps_info );
    FT_Init_Class_cff_service_glyph_dict(
      library, &container->cff_service_glyph_dict );
    FT_Init_Class_cff_service_ps_name(
      library, &container->cff_service_ps_name );
    FT_Init_Class_cff_service_get_cmap_info(
      library, &container->cff_service_get_cmap_info );
    FT_Init_Class_cff_service_cid_info(
      library, &container->cff_service_cid_info );
    FT_Init_Class_cff_cmap_encoding_class_rec(
      library, &container->cff_cmap_encoding_class_rec );
    FT_Init_Class_cff_cmap_unicode_class_rec(
      library, &container->cff_cmap_unicode_class_rec );
Exit:
    if ( error )
      cff_driver_class_pic_free( library );
    return error;
  }

#endif 

