#include "pdf-internal.h"
#include "pdfengine-internal.h"

pdf_xobject *
pdf_keep_xobject(base_context *ctx, pdf_xobject *xobj)
{
	return (pdf_xobject *)base_keep_storable(ctx, &xobj->storable);
}

void
pdf_drop_xobject(base_context *ctx, pdf_xobject *xobj)
{
	base_drop_storable(ctx, &xobj->storable);
}

static void
pdf_free_xobject_imp(base_context *ctx, base_storable *xobj_)
{
	pdf_xobject *xobj = (pdf_xobject *)xobj_;

	if (xobj->colorspace)
		base_drop_colorspace(ctx, xobj->colorspace);
	if (xobj->resources)
		pdf_drop_obj(xobj->resources);
	if (xobj->contents)
		base_drop_buffer(ctx, xobj->contents);
	pdf_drop_obj(xobj->me);
	base_free(ctx, xobj);
}

static unsigned int
pdf_xobject_size(pdf_xobject *xobj)
{
	if (xobj == NULL)
		return 0;
	return sizeof(*xobj) + (xobj->colorspace ? xobj->colorspace->size : 0) + (xobj->contents ? xobj->contents->len : 0);
}

pdf_xobject *
pdf_load_xobject(pdf_document *xref, pdf_obj *dict)
{
	pdf_xobject *form;
	pdf_obj *obj;
	base_context *ctx = xref->ctx;

	if ((form = pdf_find_item(ctx, pdf_free_xobject_imp, dict)))
	{
		return form;
	}

	form = base_malloc_struct(ctx, pdf_xobject);
	base_INIT_STORABLE(form, 1, pdf_free_xobject_imp);
	form->resources = NULL;
	form->contents = NULL;
	form->colorspace = NULL;
	form->me = NULL;

	
	pdf_store_item(ctx, dict, form, pdf_xobject_size(form));

	obj = pdf_dict_gets(dict, "BBox");
	form->bbox = pdf_to_rect(ctx, obj);

	obj = pdf_dict_gets(dict, "Matrix");
	if (obj)
		form->matrix = pdf_to_matrix(ctx, obj);
	else
		form->matrix = base_identity;

	form->isolated = 0;
	form->knockout = 0;
	form->transparency = 0;

	obj = pdf_dict_gets(dict, "Group");
	if (obj)
	{
		pdf_obj *attrs = obj;

		form->isolated = pdf_to_bool(pdf_dict_gets(attrs, "I"));
		form->knockout = pdf_to_bool(pdf_dict_gets(attrs, "K"));

		obj = pdf_dict_gets(attrs, "S");
		if (pdf_is_name(obj) && !strcmp(pdf_to_name(obj), "Transparency"))
			form->transparency = 1;

		obj = pdf_dict_gets(attrs, "CS");
		if (obj)
		{
			form->colorspace = pdf_load_colorspace(xref, obj);
			if (!form->colorspace)
				base_throw(ctx, "cannot load xobject colorspace");
		}
	}

	form->resources = pdf_dict_gets(dict, "Resources");
	if (form->resources)
		pdf_keep_obj(form->resources);

	base_try(ctx)
	{
		form->contents = pdf_load_stream(xref, pdf_to_num(dict), pdf_to_gen(dict));
	}
	base_catch(ctx)
	{
		pdf_remove_item(ctx, pdf_free_xobject_imp, dict);
		pdf_drop_xobject(ctx, form);
		base_throw(ctx, "cannot load xobject content stream (%d %d R)", pdf_to_num(dict), pdf_to_gen(dict));
	}
	form->me = pdf_keep_obj(dict);

	return form;
}
