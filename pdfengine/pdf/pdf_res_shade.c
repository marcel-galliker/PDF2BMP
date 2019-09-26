#include "pdf-internal.h"

base_shade *
base_keep_shade(base_context *ctx, base_shade *shade)
{
	return (base_shade *)base_keep_storable(ctx, &shade->storable);
}

void
base_free_shade_imp(base_context *ctx, base_storable *shade_)
{
	base_shade *shade = (base_shade *)shade_;

	if (shade->colorspace)
		base_drop_colorspace(ctx, shade->colorspace);
	base_free(ctx, shade->mesh);
	base_free(ctx, shade);
}

void
base_drop_shade(base_context *ctx, base_shade *shade)
{
	base_drop_storable(ctx, &shade->storable);
}

base_rect
base_bound_shade(base_context *ctx, base_shade *shade, base_matrix ctm)
{
	float *v;
	base_rect r, s;
	base_point p;
	int i, ncomp, nvert;

	ctm = base_concat(shade->matrix, ctm);
	ncomp = shade->use_function ? 3 : 2 + shade->colorspace->n;
	nvert = shade->mesh_len / ncomp;
	v = shade->mesh;

	s = base_transform_rect(ctm, shade->bbox);
	if (shade->type == base_LINEAR)
		return base_intersect_rect(s, base_infinite_rect);
	if (shade->type == base_RADIAL)
		return base_intersect_rect(s, base_infinite_rect);

	if (nvert == 0)
		return base_empty_rect;

	p.x = v[0];
	p.y = v[1];
	v += ncomp;
	p = base_transform_point(ctm, p);
	r.x0 = r.x1 = p.x;
	r.y0 = r.y1 = p.y;

	for (i = 1; i < nvert; i++)
	{
		p.x = v[0];
		p.y = v[1];
		p = base_transform_point(ctm, p);
		v += ncomp;
		if (p.x < r.x0) r.x0 = p.x;
		if (p.y < r.y0) r.y0 = p.y;
		if (p.x > r.x1) r.x1 = p.x;
		if (p.y > r.y1) r.y1 = p.y;
	}

	return base_intersect_rect(s, r);
}

void
base_print_shade(base_context *ctx, FILE *out, base_shade *shade)
{
	int i, j, n;
	float *vertex;
	int triangle;

	fprintf(out, "shading {\n");

	switch (shade->type)
	{
	case base_LINEAR: fprintf(out, "\ttype linear\n"); break;
	case base_RADIAL: fprintf(out, "\ttype radial\n"); break;
	case base_MESH: fprintf(out, "\ttype mesh\n"); break;
	}

	fprintf(out, "\tbbox [%g %g %g %g]\n",
		shade->bbox.x0, shade->bbox.y0,
		shade->bbox.x1, shade->bbox.y1);

	fprintf(out, "\tcolorspace %s\n", shade->colorspace->name);

	fprintf(out, "\tmatrix [%g %g %g %g %g %g]\n",
			shade->matrix.a, shade->matrix.b, shade->matrix.c,
			shade->matrix.d, shade->matrix.e, shade->matrix.f);

	if (shade->use_background)
	{
		fprintf(out, "\tbackground [");
		for (i = 0; i < shade->colorspace->n; i++)
			fprintf(out, "%s%g", i == 0 ? "" : " ", shade->background[i]);
		fprintf(out, "]\n");
	}

	if (shade->use_function)
	{
		fprintf(out, "\tfunction\n");
		n = 3;
	}
	else
		n = 2 + shade->colorspace->n;

	fprintf(out, "\tvertices: %d\n", shade->mesh_len);

	vertex = shade->mesh;
	triangle = 0;
	i = 0;
	while (i < shade->mesh_len)
	{
		fprintf(out, "\t%d:(%g, %g): ", triangle, vertex[0], vertex[1]);

		for (j = 2; j < n; j++)
			fprintf(out, "%s%g", j == 2 ? "" : " ", vertex[j]);
		fprintf(out, "\n");

		vertex += n;
		i++;
		if (i % 3 == 0)
			triangle++;
	}

	fprintf(out, "}\n");
}
