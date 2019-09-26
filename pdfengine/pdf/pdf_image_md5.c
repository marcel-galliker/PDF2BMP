#include "pdf-internal.h"

void base_md5_pixmap(base_pixmap *pix, unsigned char digest[16])
{
	base_md5 md5;

	base_md5_init(&md5);
	if (pix)
		base_md5_update(&md5, pix->samples, pix->w * pix->h * pix->n);
	base_md5_final(&md5, digest);
}
