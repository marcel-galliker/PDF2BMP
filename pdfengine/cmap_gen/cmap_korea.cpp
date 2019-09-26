#include "cmap_gen.h"
#include "../generated/cmap_korea.h"

static const pdf_cmap_table cmap_table[] =
{
	{"Adobe-Korea1-0",&cmap_Adobe_Korea1_0},
	{"Adobe-Korea1-1",&cmap_Adobe_Korea1_1},
	{"Adobe-Korea1-2",&cmap_Adobe_Korea1_2},
	{"Adobe-Korea1-UCS2",&cmap_Adobe_Korea1_UCS2},
	{"KSC-EUC-H",&cmap_KSC_EUC_H},
	{"KSC-EUC-V",&cmap_KSC_EUC_V},
	{"KSC-H",&cmap_KSC_H},
	{"KSC-Johab-H",&cmap_KSC_Johab_H},
	{"KSC-Johab-V",&cmap_KSC_Johab_V},
	{"KSC-V",&cmap_KSC_V},
	{"KSCms-UHC-H",&cmap_KSCms_UHC_H},
	{"KSCms-UHC-HW-H",&cmap_KSCms_UHC_HW_H},
	{"KSCms-UHC-HW-V",&cmap_KSCms_UHC_HW_V},
	{"KSCms-UHC-V",&cmap_KSCms_UHC_V},
	{"KSCpc-EUC-H",&cmap_KSCpc_EUC_H},
	{"KSCpc-EUC-V",&cmap_KSCpc_EUC_V},
	{"UniKS-UCS2-H",&cmap_UniKS_UCS2_H},
	{"UniKS-UCS2-V",&cmap_UniKS_UCS2_V},
	{"UniKS-UTF16-H",&cmap_UniKS_UTF16_H},
	{"UniKS-UTF16-V",&cmap_UniKS_UTF16_V},
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
