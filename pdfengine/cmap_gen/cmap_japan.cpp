#include "cmap_gen.h"
#include "../generated/cmap_japan.h"

static const pdf_cmap_table cmap_table[] =
{
	{"78-EUC-H",&cmap_78_EUC_H},
	{"78-EUC-V",&cmap_78_EUC_V},
	{"78-H",&cmap_78_H},
	{"78-RKSJ-H",&cmap_78_RKSJ_H},
	{"78-RKSJ-V",&cmap_78_RKSJ_V},
	{"78-V",&cmap_78_V},
	{"78ms-RKSJ-H",&cmap_78ms_RKSJ_H},
	{"78ms-RKSJ-V",&cmap_78ms_RKSJ_V},
	{"83pv-RKSJ-H",&cmap_83pv_RKSJ_H},
	{"90ms-RKSJ-H",&cmap_90ms_RKSJ_H},
	{"90ms-RKSJ-V",&cmap_90ms_RKSJ_V},
	{"90msp-RKSJ-H",&cmap_90msp_RKSJ_H},
	{"90msp-RKSJ-V",&cmap_90msp_RKSJ_V},
	{"90pv-RKSJ-H",&cmap_90pv_RKSJ_H},
	{"90pv-RKSJ-V",&cmap_90pv_RKSJ_V},
	{"Add-H",&cmap_Add_H},
	{"Add-RKSJ-H",&cmap_Add_RKSJ_H},
	{"Add-RKSJ-V",&cmap_Add_RKSJ_V},
	{"Add-V",&cmap_Add_V},
	{"Adobe-Japan1-0",&cmap_Adobe_Japan1_0},
	{"Adobe-Japan1-1",&cmap_Adobe_Japan1_1},
	{"Adobe-Japan1-2",&cmap_Adobe_Japan1_2},
	{"Adobe-Japan1-3",&cmap_Adobe_Japan1_3},
	{"Adobe-Japan1-4",&cmap_Adobe_Japan1_4},
	{"Adobe-Japan1-5",&cmap_Adobe_Japan1_5},
	{"Adobe-Japan1-6",&cmap_Adobe_Japan1_6},
	{"Adobe-Japan1-UCS2",&cmap_Adobe_Japan1_UCS2},
	{"Adobe-Japan2-0",&cmap_Adobe_Japan2_0},
	{"EUC-H",&cmap_EUC_H},
	{"EUC-V",&cmap_EUC_V},
	{"Ext-H",&cmap_Ext_H},
	{"Ext-RKSJ-H",&cmap_Ext_RKSJ_H},
	{"Ext-RKSJ-V",&cmap_Ext_RKSJ_V},
	{"Ext-V",&cmap_Ext_V},
	{"H",&cmap_H},
	{"Hankaku",&cmap_Hankaku},
	{"Hiragana",&cmap_Hiragana},
	{"Hojo-EUC-H",&cmap_Hojo_EUC_H},
	{"Hojo-EUC-V",&cmap_Hojo_EUC_V},
	{"Hojo-H",&cmap_Hojo_H},
	{"Hojo-V",&cmap_Hojo_V},
	{"Katakana",&cmap_Katakana},
	{"NWP-H",&cmap_NWP_H},
	{"NWP-V",&cmap_NWP_V},
	{"RKSJ-H",&cmap_RKSJ_H},
	{"RKSJ-V",&cmap_RKSJ_V},
	{"Roman",&cmap_Roman},
	{"UniHojo-UCS2-H",&cmap_UniHojo_UCS2_H},
	{"UniHojo-UCS2-V",&cmap_UniHojo_UCS2_V},
	{"UniHojo-UTF16-H",&cmap_UniHojo_UTF16_H},
	{"UniHojo-UTF16-V",&cmap_UniHojo_UTF16_V},
	{"UniJIS-UCS2-H",&cmap_UniJIS_UCS2_H},
	{"UniJIS-UCS2-HW-H",&cmap_UniJIS_UCS2_HW_H},
	{"UniJIS-UCS2-HW-V",&cmap_UniJIS_UCS2_HW_V},
	{"UniJIS-UCS2-V",&cmap_UniJIS_UCS2_V},
	{"UniJIS-UTF16-H",&cmap_UniJIS_UTF16_H},
	{"UniJIS-UTF16-V",&cmap_UniJIS_UTF16_V},
	{"UniJISPro-UCS2-HW-V",&cmap_UniJISPro_UCS2_HW_V},
	{"UniJISPro-UCS2-V",&cmap_UniJISPro_UCS2_V},
	{"V",&cmap_V},
	{"WP-Symbol",&cmap_WP_Symbol},
};

void
pdf_free_cmap_imp(base_context *, base_storable *)
{
}

extern "C" CMAP_GENDLL_EXPORT pdf_cmap_table* loadFonts(int *count)
{
	*count = sizeof(cmap_table)/sizeof(cmap_table[0]);

	return (pdf_cmap_table*) cmap_table;
}
