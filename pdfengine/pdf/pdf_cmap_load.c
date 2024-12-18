#include "pdf-internal.h"
#include "pdfengine-internal.h"

unsigned int
pdf_cmap_size(base_context *ctx, pdf_cmap *cmap)
{
	if (cmap == NULL)
		return 0;
	if (cmap->storable.refs < 0)
		return 0;

	return cmap->rcap * sizeof(pdf_range) + cmap->tcap * sizeof(short) + pdf_cmap_size(ctx, cmap->usecmap);
}

pdf_cmap *
pdf_load_embedded_cmap(pdf_document *xref, pdf_obj *stmobj)
{
	base_stream *file = NULL;
	pdf_cmap *cmap = NULL;
	pdf_cmap *usecmap;
	pdf_obj *wmode;
	pdf_obj *obj = NULL;
	base_context *ctx = xref->ctx;
	int phase = 0;

	base_var(phase);
	base_var(obj);
	base_var(file);
	base_var(cmap);

	if ((cmap = pdf_find_item(ctx, pdf_free_cmap_imp, stmobj)))
	{
		return cmap;
	}

	base_try(ctx)
	{
		file = pdf_open_stream(xref, pdf_to_num(stmobj), pdf_to_gen(stmobj));
		phase = 1;
		cmap = pdf_load_cmap(ctx, file);
		phase = 2;
		base_close(file);
		file = NULL;

		wmode = pdf_dict_gets(stmobj, "WMode");
		if (pdf_is_int(wmode))
			pdf_set_cmap_wmode(ctx, cmap, pdf_to_int(wmode));
		obj = pdf_dict_gets(stmobj, "UseCMap");
		if (pdf_is_name(obj))
		{
			usecmap = pdf_load_system_cmap(ctx, pdf_to_name(obj));
			pdf_set_usecmap(ctx, cmap, usecmap);
			pdf_drop_cmap(ctx, usecmap);
		}
		else if (pdf_is_indirect(obj))
		{
			phase = 3;
			usecmap = pdf_load_embedded_cmap(xref, obj);
			pdf_set_usecmap(ctx, cmap, usecmap);
			pdf_drop_cmap(ctx, usecmap);
		}

		pdf_store_item(ctx, stmobj, cmap, pdf_cmap_size(ctx, cmap));
	}
	base_catch(ctx)
	{
		if (file)
			base_close(file);
		if (cmap)
			pdf_drop_cmap(ctx, cmap);
		if (phase < 1)
			base_throw(ctx, "cannot open cmap stream (%d %d R)", pdf_to_num(stmobj), pdf_to_gen(stmobj));
		else if (phase < 2)
			base_throw(ctx, "cannot parse cmap stream (%d %d R)", pdf_to_num(stmobj), pdf_to_gen(stmobj));
		else if (phase < 3)
			base_throw(ctx, "cannot load system usecmap '%s'", pdf_to_name(obj));
		else
			base_throw(ctx, "cannot load embedded usecmap (%d %d R)", pdf_to_num(obj), pdf_to_gen(obj));
	}

	return cmap;
}

pdf_cmap *
pdf_new_identity_cmap(base_context *ctx, int wmode, int bytes)
{
	pdf_cmap *cmap = pdf_new_cmap(ctx);
	base_try(ctx)
	{
		sprintf(cmap->cmap_name, "Identity-%c", wmode ? 'V' : 'H');
		pdf_add_codespace(ctx, cmap, 0x0000, 0xffff, bytes);
		pdf_map_range_to_range(ctx, cmap, 0x0000, 0xffff, 0);
		pdf_sort_cmap(ctx, cmap);
		pdf_set_cmap_wmode(ctx, cmap, wmode);
	}
	base_catch(ctx)
	{
		pdf_drop_cmap(ctx, cmap);
		base_rethrow(ctx);
	}
	return cmap;
}

pdf_cmap *
pdf_load_system_cmap(base_context *ctx, char *cmap_name)
{
	pdf_cmap *usecmap;
	pdf_cmap *cmap;

	cmap = pdf_load_builtin_cmap(ctx, cmap_name);
	if (!cmap)
		base_throw(ctx, "no builtin cmap file: %s", cmap_name);

	if (cmap->usecmap_name[0] && !cmap->usecmap)
	{
		usecmap = pdf_load_builtin_cmap(ctx, cmap->usecmap_name);
		if (!usecmap)
			base_throw(ctx, "nu builtin cmap file: %s", cmap->usecmap_name);
		pdf_set_usecmap(ctx, cmap, usecmap);
	}

	return cmap;
}
