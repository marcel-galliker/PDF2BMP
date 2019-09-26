

#include "tiffiop.h"

static const char TIFFVersion[] = TIFFLIB_VERSION_STR;

const char*
TIFFGetVersion(void)
{
	return (TIFFVersion);
}

