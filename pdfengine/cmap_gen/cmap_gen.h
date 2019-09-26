#ifndef CMAP_CNS_H

#include "pdf.h"
#include "pdf-internal.h"
#include "pdfengine-internal.h"

#define CMAP_CNS_H
#if defined(CMAP_GEN_LIBRARY)
#  define CMAP_GENDLL_EXPORT __declspec(dllexport)
#else
#  define CMAP_GENDLL_EXPORT __declspec(dllimport)
#endif

#endif 
