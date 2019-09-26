#include "pdf-internal.h"
#include "pdfengine-internal.h"

int
pdf_is_stream(pdf_document *xref, int num, int gen)
{
	if (num < 0 || num >= xref->len)
		return 0;

	pdf_cache_object(xref, num, gen);
	

	return xref->table[num].stm_ofs > 0;
}

static int
pdf_stream_has_crypt(base_context *ctx, pdf_obj *stm)
{
	pdf_obj *filters;
	pdf_obj *obj;
	int i;

	filters = pdf_dict_getsa(stm, "Filter", "F");
	if (filters)
	{
		if (!strcmp(pdf_to_name(filters), "Crypt"))
			return 1;
		if (pdf_is_array(filters))
		{
			int n = pdf_array_len(filters);
			for (i = 0; i < n; i++)
			{
				obj = pdf_array_get(filters, i);
				if (!strcmp(pdf_to_name(obj), "Crypt"))
					return 1;
			}
		}
	}
	return 0;
}

static base_stream *
build_filter(base_stream *chain, pdf_document * xref, pdf_obj * f, pdf_obj * p, int num, int gen, pdf_image_params *params)
{
	base_context *ctx = chain->ctx;
	char *s = pdf_to_name(f);

	int predictor = pdf_to_int(pdf_dict_gets(p, "Predictor"));
	int columns = pdf_to_int(pdf_dict_gets(p, "Columns"));
	int colors = pdf_to_int(pdf_dict_gets(p, "Colors"));
	int bpc = pdf_to_int(pdf_dict_gets(p, "BitsPerComponent"));

	if (predictor == 0) predictor = 1;
	if (columns == 0) columns = 1;
	if (colors == 0) colors = 1;
	if (bpc == 0) bpc = 8;

	if (!strcmp(s, "ASCIIHexDecode") || !strcmp(s, "AHx"))
		return base_open_ahxd(chain);

	else if (!strcmp(s, "ASCII85Decode") || !strcmp(s, "A85"))
		return base_open_a85d(chain);

	else if (!strcmp(s, "CCITTFaxDecode") || !strcmp(s, "CCF"))
	{
		pdf_obj *k = pdf_dict_gets(p, "K");
		pdf_obj *eol = pdf_dict_gets(p, "EndOfLine");
		pdf_obj *eba = pdf_dict_gets(p, "EncodedByteAlign");
		pdf_obj *columns = pdf_dict_gets(p, "Columns");
		pdf_obj *rows = pdf_dict_gets(p, "Rows");
		pdf_obj *eob = pdf_dict_gets(p, "EndOfBlock");
		pdf_obj *bi1 = pdf_dict_gets(p, "BlackIs1");
		if (params)
		{
			
			params->type = PDF_IMAGE_FAX;
			params->u.fax.k = (k ? pdf_to_int(k) : 0);
			params->u.fax.eol = (eol ? pdf_to_bool(eol) : 0);
			params->u.fax.eba = (eba ? pdf_to_bool(eba) : 0);
			params->u.fax.columns = (columns ? pdf_to_int(columns) : 1728);
			params->u.fax.rows = (rows ? pdf_to_int(rows) : 0);
			params->u.fax.eob = (eob ? pdf_to_bool(eob) : 1);
			params->u.fax.bi1 = (bi1 ? pdf_to_bool(bi1) : 0);
			return chain;
		}
		return base_open_faxd(chain,
				k ? pdf_to_int(k) : 0,
				eol ? pdf_to_bool(eol) : 0,
				eba ? pdf_to_bool(eba) : 0,
				columns ? pdf_to_int(columns) : 1728,
				rows ? pdf_to_int(rows) : 0,
				eob ? pdf_to_bool(eob) : 1,
				bi1 ? pdf_to_bool(bi1) : 0);
	}

	else if (!strcmp(s, "DCTDecode") || !strcmp(s, "DCT"))
	{
		pdf_obj *ct = pdf_dict_gets(p, "ColorTransform");
		if (params)
		{
			
			params->type = PDF_IMAGE_JPEG;
			params->u.jpeg.ct = (ct ? pdf_to_int(ct) : -1);
			return chain;
		}
		return base_open_dctd(chain, ct ? pdf_to_int(ct) : -1);
	}

	else if (!strcmp(s, "RunLengthDecode") || !strcmp(s, "RL"))
	{
		if (params)
		{
			
			params->type = PDF_IMAGE_RLD;
			return chain;
		}
		return base_open_rld(chain);
	}
	else if (!strcmp(s, "FlateDecode") || !strcmp(s, "Fl"))
	{
		if (params)
		{
			
			params->type = PDF_IMAGE_FLATE;
			params->u.flate.predictor = predictor;
			params->u.flate.columns = columns;
			params->u.flate.colors = colors;
			params->u.flate.bpc = bpc;
			return chain;
		}
		chain = base_open_flated(chain);
		if (predictor > 1)
			chain = base_open_predict(chain, predictor, columns, colors, bpc);
		return chain;
	}

	else if (!strcmp(s, "LZWDecode") || !strcmp(s, "LZW"))
	{
		pdf_obj *ec = pdf_dict_gets(p, "EarlyChange");
		if (params)
		{
			
			params->type = PDF_IMAGE_LZW;
			params->u.lzw.predictor = predictor;
			params->u.lzw.columns = columns;
			params->u.lzw.colors = colors;
			params->u.lzw.bpc = bpc;
			params->u.lzw.ec = (ec ? pdf_to_int(ec) : 1);
			return chain;
		}
		chain = base_open_lzwd(chain, ec ? pdf_to_int(ec) : 1);
		if (predictor > 1)
			chain = base_open_predict(chain, predictor, columns, colors, bpc);
		return chain;
	}

	else if (!strcmp(s, "JBIG2Decode"))
	{
		base_buffer *globals = NULL;
		pdf_obj *obj = pdf_dict_gets(p, "JBIG2Globals");
		if (obj)
			globals = pdf_load_stream(xref, pdf_to_num(obj), pdf_to_gen(obj));
		
		return base_open_jbig2d(chain, globals);
	}

	else if (!strcmp(s, "JPXDecode"))
		return chain; 

	else if (!strcmp(s, "Crypt"))
	{
		pdf_obj *name;

		if (!xref->crypt)
		{
			base_warn(ctx, "crypt filter in unencrypted document");
			return chain;
		}

		name = pdf_dict_gets(p, "Name");
		if (pdf_is_name(name))
			return pdf_open_crypt_with_filter(chain, xref->crypt, pdf_to_name(name), num, gen);

		return chain;
	}

	base_warn(ctx, "unknown filter name (%s)", s);
	return chain;
}

static base_stream *
build_filter_chain(base_stream *chain, pdf_document *xref, pdf_obj *fs, pdf_obj *ps, int num, int gen, pdf_image_params *params)
{
	pdf_obj *f;
	pdf_obj *p;
	int i, n;

	n = pdf_array_len(fs);
	for (i = 0; i < n; i++)
	{
		f = pdf_array_get(fs, i);
		p = pdf_array_get(ps, i);
		chain = build_filter(chain, xref, f, p, num, gen, (i == n-1 ? params : NULL));
	}

	return chain;
}

static base_stream *
pdf_open_raw_filter(base_stream *chain, pdf_document *xref, pdf_obj *stmobj, int num, int gen)
{
	int hascrypt;
	int len;
	base_context *ctx = chain->ctx;

	
	base_keep_stream(chain);

	len = pdf_to_int(pdf_dict_gets(stmobj, "Length"));
	chain = base_open_null(chain, len);

	base_try(ctx)
	{
		hascrypt = pdf_stream_has_crypt(ctx, stmobj);
		if (xref->crypt && !hascrypt)
			chain = pdf_open_crypt(chain, xref->crypt, num, gen);
	}
	base_catch(ctx)
	{
		base_close(chain);
		base_rethrow(ctx);
	}

	return chain;
}

static base_stream *
pdf_open_filter(base_stream *chain, pdf_document *xref, pdf_obj *stmobj, int num, int gen, pdf_image_params *imparams)
{
	pdf_obj *filters;
	pdf_obj *params;

	filters = pdf_dict_getsa(stmobj, "Filter", "F");
	params = pdf_dict_getsa(stmobj, "DecodeParms", "DP");

	chain = pdf_open_raw_filter(chain, xref, stmobj, num, gen);

	if (pdf_is_name(filters))
		chain = build_filter(chain, xref, filters, params, num, gen, imparams);
	else if (pdf_array_len(filters) > 0)
		chain = build_filter_chain(chain, xref, filters, params, num, gen, imparams);

	base_lock_stream(chain);
	return chain;
}

base_stream *
pdf_open_inline_stream(pdf_document *xref, pdf_obj *stmobj, int length, base_stream *chain, pdf_image_params *imparams)
{
	pdf_obj *filters;
	pdf_obj *params;

	filters = pdf_dict_getsa(stmobj, "Filter", "F");
	params = pdf_dict_getsa(stmobj, "DecodeParms", "DP");

	
	base_keep_stream(chain);

	if (pdf_is_name(filters))
		return build_filter(chain, xref, filters, params, 0, 0, imparams);
	if (pdf_array_len(filters) > 0)
		return build_filter_chain(chain, xref, filters, params, 0, 0, imparams);

	return base_open_null(chain, length);
}

base_stream *
pdf_open_raw_stream(pdf_document *xref, int num, int gen)
{
	pdf_xref_entry *x;
	base_stream *stm;

	base_var(x);

	if (num < 0 || num >= xref->len)
		base_throw(xref->ctx, "object id out of range (%d %d R)", num, gen);

	x = xref->table + num;

	pdf_cache_object(xref, num, gen);
	

	if (x->stm_ofs == 0)
		base_throw(xref->ctx, "object is not a stream");

	stm = pdf_open_raw_filter(xref->file, xref, x->obj, num, gen);
	base_lock_stream(stm);
	base_seek(xref->file, x->stm_ofs, 0);
	return stm;
}

base_stream *
pdf_open_stream(pdf_document *xref, int num, int gen)
{
	return pdf_open_image_stream(xref, num, gen, NULL);
}

base_stream *
pdf_open_image_stream(pdf_document *xref, int num, int gen, pdf_image_params *params)
{
	pdf_xref_entry *x;
	base_stream *stm;

	if (num < 0 || num >= xref->len)
		base_throw(xref->ctx, "object id out of range (%d %d R)", num, gen);

	x = xref->table + num;

	pdf_cache_object(xref, num, gen);
	

	if (x->stm_ofs == 0)
		base_throw(xref->ctx, "object is not a stream");

	stm = pdf_open_filter(xref->file, xref, x->obj, num, gen, params);
	base_seek(xref->file, x->stm_ofs, 0);
	return stm;
}

base_stream *
pdf_open_image_decomp_stream(base_context *ctx, base_buffer *buffer, pdf_image_params *params, int *factor)
{
	base_stream *chain = base_open_buffer(ctx, buffer);

	switch (params->type)
	{
	case PDF_IMAGE_FAX:
		*factor = 1;
		return base_open_faxd(chain,
				params->u.fax.k,
				params->u.fax.eol,
				params->u.fax.eba,
				params->u.fax.columns,
				params->u.fax.rows,
				params->u.fax.eob,
				params->u.fax.bi1);
	case PDF_IMAGE_JPEG:
		if (*factor > 8)
			*factor = 8;
		return base_open_resized_dctd(chain, params->u.jpeg.ct, *factor);
	case PDF_IMAGE_RLD:
		*factor = 1;
		return base_open_rld(chain);
	case PDF_IMAGE_FLATE:
		*factor = 1;
		chain = base_open_flated(chain);
		if (params->u.flate.predictor > 1)
			chain = base_open_predict(chain, params->u.flate.predictor, params->u.flate.columns, params->u.flate.colors, params->u.flate.bpc);
		return chain;
	case PDF_IMAGE_LZW:
		*factor = 1;
		chain = base_open_lzwd(chain, params->u.lzw.ec);
		if (params->u.lzw.predictor > 1)
			chain = base_open_predict(chain, params->u.lzw.predictor, params->u.lzw.columns, params->u.lzw.colors, params->u.lzw.bpc);
		return chain;
	default:
		*factor = 1;
		break;
	}

	return chain;
}

base_stream *
pdf_open_stream_with_offset(pdf_document *xref, int num, int gen, pdf_obj *dict, int stm_ofs)
{
	base_stream *stm;

	if (stm_ofs == 0)
		base_throw(xref->ctx, "object is not a stream");

	stm = pdf_open_filter(xref->file, xref, dict, num, gen, NULL);
	base_seek(xref->file, stm_ofs, 0);
	return stm;
}

base_buffer *
pdf_load_raw_stream(pdf_document *xref, int num, int gen)
{
	base_stream *stm;
	pdf_obj *dict;
	int len;
	base_buffer *buf;

	dict = pdf_load_object(xref, num, gen);
	

	len = pdf_to_int(pdf_dict_gets(dict, "Length"));

	pdf_drop_obj(dict);

	stm = pdf_open_raw_stream(xref, num, gen);
	

	buf = base_read_all(stm, len);
	

	base_close(stm);
	return buf;
}

static int
pdf_guess_filter_length(int len, char *filter)
{
	if (!strcmp(filter, "ASCIIHexDecode"))
		return len / 2;
	if (!strcmp(filter, "ASCII85Decode"))
		return len * 4 / 5;
	if (!strcmp(filter, "FlateDecode"))
		return len * 3;
	if (!strcmp(filter, "RunLengthDecode"))
		return len * 3;
	if (!strcmp(filter, "LZWDecode"))
		return len * 2;
	return len;
}

base_buffer *
pdf_load_stream(pdf_document *xref, int num, int gen)
{
	return pdf_load_image_stream(xref, num, gen, NULL);
}

base_buffer *
pdf_load_image_stream(pdf_document *xref, int num, int gen, pdf_image_params *params)
{
	base_context *ctx = xref->ctx;
	base_stream *stm = NULL;
	pdf_obj *dict, *obj;
	int i, len, n;
	base_buffer *buf;

	base_var(buf);

	dict = pdf_load_object(xref, num, gen);
	

	len = pdf_to_int(pdf_dict_gets(dict, "Length"));
	obj = pdf_dict_gets(dict, "Filter");
	len = pdf_guess_filter_length(len, pdf_to_name(obj));
	n = pdf_array_len(obj);
	for (i = 0; i < n; i++)
		len = pdf_guess_filter_length(len, pdf_to_name(pdf_array_get(obj, i)));

	pdf_drop_obj(dict);

	stm = pdf_open_image_stream(xref, num, gen, params);
	

	base_try(ctx)
	{
		buf = base_read_all(stm, len);
	}
	base_always(ctx)
	{
		base_close(stm);
	}
	base_catch(ctx)
	{
		base_throw(ctx, "cannot read raw stream (%d %d R)", num, gen);
	}

	return buf;
}
