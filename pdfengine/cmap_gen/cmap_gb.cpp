#include "cmap_gen.h"
#include "../generated/cmap_gb.h"

static const pdf_cmap_table cmap_table[] =
{
	{"Adobe-GB1-0",&cmap_Adobe_GB1_0},
	{"Adobe-GB1-1",&cmap_Adobe_GB1_1},
	{"Adobe-GB1-2",&cmap_Adobe_GB1_2},
	{"Adobe-GB1-3",&cmap_Adobe_GB1_3},
	{"Adobe-GB1-4",&cmap_Adobe_GB1_4},
	{"Adobe-GB1-5",&cmap_Adobe_GB1_5},
	{"Adobe-GB1-UCS2",&cmap_Adobe_GB1_UCS2},
	{"GB-EUC-H",&cmap_GB_EUC_H},
	{"GB-EUC-V",&cmap_GB_EUC_V},
	{"GB-H",&cmap_GB_H},
	{"GB-V",&cmap_GB_V},
	{"GBK-EUC-H",&cmap_GBK_EUC_H},
	{"GBK-EUC-V",&cmap_GBK_EUC_V},
	{"GBK2K-H",&cmap_GBK2K_H},
	{"GBK2K-V",&cmap_GBK2K_V},
	{"GBKp-EUC-H",&cmap_GBKp_EUC_H},
	{"GBKp-EUC-V",&cmap_GBKp_EUC_V},
	{"GBT-EUC-H",&cmap_GBT_EUC_H},
	{"GBT-EUC-V",&cmap_GBT_EUC_V},
	{"GBT-H",&cmap_GBT_H},
	{"GBT-V",&cmap_GBT_V},
	{"GBTpc-EUC-H",&cmap_GBTpc_EUC_H},
	{"GBTpc-EUC-V",&cmap_GBTpc_EUC_V},
	{"GBpc-EUC-H",&cmap_GBpc_EUC_H},
	{"GBpc-EUC-V",&cmap_GBpc_EUC_V},
	{"UniGB-UCS2-H",&cmap_UniGB_UCS2_H},
	{"UniGB-UCS2-V",&cmap_UniGB_UCS2_V},
	{"UniGB-UTF16-H",&cmap_UniGB_UTF16_H},
	{"UniGB-UTF16-V",&cmap_UniGB_UTF16_V},
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
