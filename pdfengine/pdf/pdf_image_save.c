#include "pdf-internal.h"

void base_write_pixmap(base_context *ctx, base_pixmap *img, char *file, int rgb)
{
	char name[1024];
	base_pixmap *converted = NULL;

	if (!img)
		return;

	if (rgb && img->colorspace && img->colorspace != base_device_rgb)
	{
		converted = base_new_pixmap_with_bbox(ctx, base_device_rgb, base_pixmap_bbox(ctx, img));
		base_convert_pixmap(ctx, converted, img);
		img = converted;
	}

	if (img->n <= 4)
	{
		sprintf(name, "%s.png", file);
		printf("extracting image %s\n", name);
		base_write_png(ctx, img, name, 0);
	}
	else
	{
		sprintf(name, "%s.pam", file);
		printf("extracting image %s\n", name);
		base_write_pam(ctx, img, name, 0);
	}

	base_drop_pixmap(ctx, converted);
}
