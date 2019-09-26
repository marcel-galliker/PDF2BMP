

#ifdef _WIN32
#include <windows.h>
#endif 

#include "opj_config.h"
#include "opj_includes.h"

#ifdef _WIN32
#ifndef OPJ_STATIC
BOOL APIENTRY
DllMain(HANDLE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {

	OPJ_ARG_NOT_USED(lpReserved);
	OPJ_ARG_NOT_USED(hModule);

	switch (ul_reason_for_call) {
		case DLL_PROCESS_ATTACH :
			break;
		case DLL_PROCESS_DETACH :
			break;
		case DLL_THREAD_ATTACH :
		case DLL_THREAD_DETACH :
			break;
    }

    return TRUE;
}
#endif 
#endif 

const char* OPJ_CALLCONV opj_version(void) {
    return PACKAGE_VERSION;
}

opj_dinfo_t* OPJ_CALLCONV opj_create_decompress(OPJ_CODEC_FORMAT format) {
	opj_dinfo_t *dinfo = (opj_dinfo_t*)opj_calloc(1, sizeof(opj_dinfo_t));
	if(!dinfo) return NULL;
	dinfo->is_decompressor = OPJ_TRUE;
	switch(format) {
		case CODEC_J2K:
		case CODEC_JPT:
			
			dinfo->j2k_handle = (void*)j2k_create_decompress((opj_common_ptr)dinfo);
			if(!dinfo->j2k_handle) {
				opj_free(dinfo);
				return NULL;
			}
			break;
		case CODEC_JP2:
			
			dinfo->jp2_handle = (void*)jp2_create_decompress((opj_common_ptr)dinfo);
			if(!dinfo->jp2_handle) {
				opj_free(dinfo);
				return NULL;
			}
			break;
		case CODEC_UNKNOWN:
		default:
			opj_free(dinfo);
			return NULL;
	}

	dinfo->codec_format = format;

	return dinfo;
}

void OPJ_CALLCONV opj_destroy_decompress(opj_dinfo_t *dinfo) {
	if(dinfo) {
		
		switch(dinfo->codec_format) {
			case CODEC_J2K:
			case CODEC_JPT:
				j2k_destroy_decompress((opj_j2k_t*)dinfo->j2k_handle);
				break;
			case CODEC_JP2:
				jp2_destroy_decompress((opj_jp2_t*)dinfo->jp2_handle);
				break;
			case CODEC_UNKNOWN:
			default:
				break;
		}
		
		opj_free(dinfo);
	}
}

void OPJ_CALLCONV opj_set_default_decoder_parameters(opj_dparameters_t *parameters) {
	if(parameters) {
		memset(parameters, 0, sizeof(opj_dparameters_t));
		
		parameters->cp_layer = 0;
		parameters->cp_reduce = 0;
		parameters->cp_limit_decoding = NO_LIMITATION;

		parameters->decod_format = -1;
		parameters->cod_format = -1;
		parameters->flags = 0;		

#ifdef USE_JPWL
		parameters->jpwl_correct = OPJ_FALSE;
		parameters->jpwl_exp_comps = JPWL_EXPECTED_COMPONENTS;
		parameters->jpwl_max_tiles = JPWL_MAXIMUM_TILES;
#endif 

	}
}

void OPJ_CALLCONV opj_setup_decoder(opj_dinfo_t *dinfo, opj_dparameters_t *parameters) {
	if(dinfo && parameters) {
		switch(dinfo->codec_format) {
			case CODEC_J2K:
			case CODEC_JPT:
				j2k_setup_decoder((opj_j2k_t*)dinfo->j2k_handle, parameters);
				break;
			case CODEC_JP2:
				jp2_setup_decoder((opj_jp2_t*)dinfo->jp2_handle, parameters);
				break;
			case CODEC_UNKNOWN:
			default:
				break;
		}
	}
}

opj_image_t* OPJ_CALLCONV opj_decode(opj_dinfo_t *dinfo, opj_cio_t *cio) {
	return opj_decode_with_info(dinfo, cio, NULL);
}

opj_image_t* OPJ_CALLCONV opj_decode_with_info(opj_dinfo_t *dinfo, opj_cio_t *cio, opj_codestream_info_t *cstr_info) {
	if(dinfo && cio) {
		switch(dinfo->codec_format) {
			case CODEC_J2K:
				return j2k_decode((opj_j2k_t*)dinfo->j2k_handle, cio, cstr_info);
			case CODEC_JPT:
				return j2k_decode_jpt_stream((opj_j2k_t*)dinfo->j2k_handle, cio, cstr_info);
			case CODEC_JP2:
				return opj_jp2_decode((opj_jp2_t*)dinfo->jp2_handle, cio, cstr_info);
			case CODEC_UNKNOWN:
			default:
				break;
		}
	}
	return NULL;
}

opj_cinfo_t* OPJ_CALLCONV opj_create_compress(OPJ_CODEC_FORMAT format) {
	opj_cinfo_t *cinfo = (opj_cinfo_t*)opj_calloc(1, sizeof(opj_cinfo_t));
	if(!cinfo) return NULL;
	cinfo->is_decompressor = OPJ_FALSE;
	switch(format) {
		case CODEC_J2K:
			
			cinfo->j2k_handle = (void*)j2k_create_compress((opj_common_ptr)cinfo);
			if(!cinfo->j2k_handle) {
				opj_free(cinfo);
				return NULL;
			}
			break;
		case CODEC_JP2:
			
			cinfo->jp2_handle = (void*)jp2_create_compress((opj_common_ptr)cinfo);
			if(!cinfo->jp2_handle) {
				opj_free(cinfo);
				return NULL;
			}
			break;
		case CODEC_JPT:
		case CODEC_UNKNOWN:
		default:
			opj_free(cinfo);
			return NULL;
	}

	cinfo->codec_format = format;

	return cinfo;
}

void OPJ_CALLCONV opj_destroy_compress(opj_cinfo_t *cinfo) {
	if(cinfo) {
		
		switch(cinfo->codec_format) {
			case CODEC_J2K:
				j2k_destroy_compress((opj_j2k_t*)cinfo->j2k_handle);
				break;
			case CODEC_JP2:
				jp2_destroy_compress((opj_jp2_t*)cinfo->jp2_handle);
				break;
			case CODEC_JPT:
			case CODEC_UNKNOWN:
			default:
				break;
		}
		
		opj_free(cinfo);
	}
}

void OPJ_CALLCONV opj_set_default_encoder_parameters(opj_cparameters_t *parameters) {
	if(parameters) {
		memset(parameters, 0, sizeof(opj_cparameters_t));
		
		parameters->cp_cinema = OFF; 
		parameters->max_comp_size = 0;
		parameters->numresolution = 6;
		parameters->cp_rsiz = STD_RSIZ;
		parameters->cblockw_init = 64;
		parameters->cblockh_init = 64;
		parameters->prog_order = LRCP;
		parameters->roi_compno = -1;		
		parameters->subsampling_dx = 1;
		parameters->subsampling_dy = 1;
		parameters->tp_on = 0;
		parameters->decod_format = -1;
		parameters->cod_format = -1;
		parameters->tcp_rates[0] = 0;   
		parameters->tcp_numlayers = 0;
    parameters->cp_disto_alloc = 0;
		parameters->cp_fixed_alloc = 0;
		parameters->cp_fixed_quality = 0;
		parameters->jpip_on = OPJ_FALSE;

#ifdef USE_JPWL
		parameters->jpwl_epc_on = OPJ_FALSE;
		parameters->jpwl_hprot_MH = -1; 
		{
			int i;
			for (i = 0; i < JPWL_MAX_NO_TILESPECS; i++) {
				parameters->jpwl_hprot_TPH_tileno[i] = -1; 
				parameters->jpwl_hprot_TPH[i] = 0; 
			}
		};
		{
			int i;
			for (i = 0; i < JPWL_MAX_NO_PACKSPECS; i++) {
				parameters->jpwl_pprot_tileno[i] = -1; 
				parameters->jpwl_pprot_packno[i] = -1; 
				parameters->jpwl_pprot[i] = 0; 
			}
		};
		parameters->jpwl_sens_size = 0; 
		parameters->jpwl_sens_addr = 0; 
		parameters->jpwl_sens_range = 0; 
		parameters->jpwl_sens_MH = -1; 
		{
			int i;
			for (i = 0; i < JPWL_MAX_NO_TILESPECS; i++) {
				parameters->jpwl_sens_TPH_tileno[i] = -1; 
				parameters->jpwl_sens_TPH[i] = -1; 
			}
		};
#endif 

	}
}

void OPJ_CALLCONV opj_setup_encoder(opj_cinfo_t *cinfo, opj_cparameters_t *parameters, opj_image_t *image) {
	if(cinfo && parameters && image) {
		switch(cinfo->codec_format) {
			case CODEC_J2K:
				j2k_setup_encoder((opj_j2k_t*)cinfo->j2k_handle, parameters, image);
				break;
			case CODEC_JP2:
				jp2_setup_encoder((opj_jp2_t*)cinfo->jp2_handle, parameters, image);
				break;
			case CODEC_JPT:
			case CODEC_UNKNOWN:
			default:
				break;
		}
	}
}

opj_bool OPJ_CALLCONV opj_encode(opj_cinfo_t *cinfo, opj_cio_t *cio, opj_image_t *image, char *index) {
	if (index != NULL)
		opj_event_msg((opj_common_ptr)cinfo, EVT_WARNING, "Set index to NULL when calling the opj_encode function.\n"
		"To extract the index, use the opj_encode_with_info() function.\n"
		"No index will be generated during this encoding\n");
	return opj_encode_with_info(cinfo, cio, image, NULL);
}

opj_bool OPJ_CALLCONV opj_encode_with_info(opj_cinfo_t *cinfo, opj_cio_t *cio, opj_image_t *image, opj_codestream_info_t *cstr_info) {
	if(cinfo && cio && image) {
		switch(cinfo->codec_format) {
			case CODEC_J2K:
				return j2k_encode((opj_j2k_t*)cinfo->j2k_handle, cio, image, cstr_info);
			case CODEC_JP2:
				return opj_jp2_encode((opj_jp2_t*)cinfo->jp2_handle, cio, image, cstr_info);	    
			case CODEC_JPT:
			case CODEC_UNKNOWN:
			default:
				break;
		}
	}
	return OPJ_FALSE;
}

void OPJ_CALLCONV opj_destroy_cstr_info(opj_codestream_info_t *cstr_info) {
	if (cstr_info) {
		int tileno;
		for (tileno = 0; tileno < cstr_info->tw * cstr_info->th; tileno++) {
			opj_tile_info_t *tile_info = &cstr_info->tile[tileno];
			opj_free(tile_info->thresh);
			opj_free(tile_info->packet);
			opj_free(tile_info->tp);
			opj_free(tile_info->marker);
		}
		opj_free(cstr_info->tile);
		opj_free(cstr_info->marker);
		opj_free(cstr_info->numdecompos);
	}
}
