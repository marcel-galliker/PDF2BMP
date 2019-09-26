#include "cmap_gen.h"
#include "../generated/cmap_cns.h"

static const pdf_cmap_table cmap_table[] =
{
	{"Adobe-CNS1-0",&cmap_Adobe_CNS1_0},
	{"Adobe-CNS1-1",&cmap_Adobe_CNS1_1},
	{"Adobe-CNS1-2",&cmap_Adobe_CNS1_2},
	{"Adobe-CNS1-3",&cmap_Adobe_CNS1_3},
	{"Adobe-CNS1-4",&cmap_Adobe_CNS1_4},
	{"Adobe-CNS1-5",&cmap_Adobe_CNS1_5},
	{"Adobe-CNS1-6",&cmap_Adobe_CNS1_6},
	{"Adobe-CNS1-UCS2",&cmap_Adobe_CNS1_UCS2},
	{"B5-H",&cmap_B5_H},
	{"B5-V",&cmap_B5_V},
	{"B5pc-H",&cmap_B5pc_H},
	{"B5pc-V",&cmap_B5pc_V},
	{"CNS-EUC-H",&cmap_CNS_EUC_H},
	{"CNS-EUC-V",&cmap_CNS_EUC_V},
	{"CNS1-H",&cmap_CNS1_H},
	{"CNS1-V",&cmap_CNS1_V},
	{"CNS2-H",&cmap_CNS2_H},
	{"CNS2-V",&cmap_CNS2_V},
	{"ETHK-B5-H",&cmap_ETHK_B5_H},
	{"ETHK-B5-V",&cmap_ETHK_B5_V},
	{"ETen-B5-H",&cmap_ETen_B5_H},
	{"ETen-B5-V",&cmap_ETen_B5_V},
	{"ETenms-B5-H",&cmap_ETenms_B5_H},
	{"ETenms-B5-V",&cmap_ETenms_B5_V},
	{"HKdla-B5-H",&cmap_HKdla_B5_H},
	{"HKdla-B5-V",&cmap_HKdla_B5_V},
	{"HKdlb-B5-H",&cmap_HKdlb_B5_H},
	{"HKdlb-B5-V",&cmap_HKdlb_B5_V},
	{"HKgccs-B5-H",&cmap_HKgccs_B5_H},
	{"HKgccs-B5-V",&cmap_HKgccs_B5_V},
	{"HKm314-B5-H",&cmap_HKm314_B5_H},
	{"HKm314-B5-V",&cmap_HKm314_B5_V},
	{"HKm471-B5-H",&cmap_HKm471_B5_H},
	{"HKm471-B5-V",&cmap_HKm471_B5_V},
	{"HKscs-B5-H",&cmap_HKscs_B5_H},
	{"HKscs-B5-V",&cmap_HKscs_B5_V},
	{"UniCNS-UCS2-H",&cmap_UniCNS_UCS2_H},
	{"UniCNS-UCS2-V",&cmap_UniCNS_UCS2_V},
	{"UniCNS-UTF16-H",&cmap_UniCNS_UTF16_H},
	{"UniCNS-UTF16-V",&cmap_UniCNS_UTF16_V},
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
