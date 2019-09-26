#include "pdf-internal.h"
#include "pdfengine-internal.h"

static inline int iswhite(int ch)
{
	return
		ch == '\000' || ch == '\011' || ch == '\012' ||
		ch == '\014' || ch == '\015' || ch == '\040';
}

static void
pdf_load_version(pdf_document *xref)
{
	char buf[20];

	base_seek(xref->file, 0, 0);
	base_read_line(xref->file, buf, sizeof buf);
	if (memcmp(buf, "%PDF-", 5) != 0)
		base_throw(xref->ctx, "cannot recognize version marker");

	xref->version = atoi(buf + 5) * 10 + atoi(buf + 7);
}

static void
pdf_read_start_xref(pdf_document *xref)
{
	unsigned char buf[1024];
	int t, n;
	int i;

	base_seek(xref->file, 0, 2);

	xref->file_size = base_tell(xref->file);

	t = MAX(0, xref->file_size - (int)sizeof buf);
	base_seek(xref->file, t, 0);

	n = base_read(xref->file, buf, sizeof buf);
	if (n < 0)
		base_throw(xref->ctx, "cannot read from file");

	for (i = n - 9; i >= 0; i--)
	{
		if (memcmp(buf + i, "startxref", 9) == 0)
		{
			i += 9;
			while (iswhite(buf[i]) && i < n)
				i ++;
			xref->startxref = atoi((char*)(buf + i));

			return;
		}
	}

	base_throw(xref->ctx, "cannot find startxref");
}

static void
pdf_read_old_trailer(pdf_document *xref, pdf_lexbuf *buf)
{
	int len;
	char *s;
	int t;
	int tok;
	int c;

	base_read_line(xref->file, buf->scratch, buf->size);
	if (strncmp(buf->scratch, "xref", 4) != 0)
		base_throw(xref->ctx, "cannot find xref marker");

	while (1)
	{
		c = base_peek_byte(xref->file);
		if (!(c >= '0' && c <= '9'))
			break;

		base_read_line(xref->file, buf->scratch, buf->size);
		s = buf->scratch;
		base_strsep(&s, " "); 
		if (!s)
			base_throw(xref->ctx, "invalid range marker in xref");
		len = atoi(base_strsep(&s, " "));

		
		if (s && *s != '\0')
			base_seek(xref->file, -(2 + (int)strlen(s)), 1);

		t = base_tell(xref->file);
		if (t < 0)
			base_throw(xref->ctx, "cannot tell in file");

		base_seek(xref->file, t + 20 * len, 0);
	}

	base_try(xref->ctx)
	{
		tok = pdf_lex(xref->file, buf);
		if (tok != PDF_TOK_TRAILER)
			base_throw(xref->ctx, "expected trailer marker");

		tok = pdf_lex(xref->file, buf);
		if (tok != PDF_TOK_OPEN_DICT)
			base_throw(xref->ctx, "expected trailer dictionary");

		xref->trailer = pdf_parse_dict(xref, xref->file, buf);
	}
	base_catch(xref->ctx)
	{
		base_throw(xref->ctx, "cannot parse trailer");
	}
}

static void
pdf_read_new_trailer(pdf_document *xref, pdf_lexbuf *buf)
{
	base_try(xref->ctx)
	{
		xref->trailer = pdf_parse_ind_obj(xref, xref->file, buf, NULL, NULL, NULL);
	}
	base_catch(xref->ctx)
	{
		base_throw(xref->ctx, "cannot parse trailer (compressed)");
	}
}

static void
pdf_read_trailer(pdf_document *xref, pdf_lexbuf *buf)
{
	int c;

	base_seek(xref->file, xref->startxref, 0);

	while (iswhite(base_peek_byte(xref->file)))
		base_read_byte(xref->file);

	base_try(xref->ctx)
	{
		c = base_peek_byte(xref->file);
		if (c == 'x')
			pdf_read_old_trailer(xref, buf);
		else if (c >= '0' && c <= '9')
			pdf_read_new_trailer(xref, buf);
		else
			base_throw(xref->ctx, "cannot recognize xref format: '%c'", c);
	}
	base_catch(xref->ctx)
	{
		base_throw(xref->ctx, "cannot read trailer");
	}
}

void
pdf_resize_xref(pdf_document *xref, int newlen)
{
	int i;

	xref->table = base_resize_array(xref->ctx, xref->table, newlen, sizeof(pdf_xref_entry));
	for (i = xref->len; i < newlen; i++)
	{
		xref->table[i].type = 0;
		xref->table[i].ofs = 0;
		xref->table[i].gen = 0;
		xref->table[i].stm_ofs = 0;
		xref->table[i].obj = NULL;
	}
	xref->len = newlen;
}

static pdf_obj *
pdf_read_old_xref(pdf_document *xref, pdf_lexbuf *buf)
{
	int ofs, len;
	char *s;
	int n;
	int tok;
	int i;
	int c;
	pdf_obj *trailer;

	base_read_line(xref->file, buf->scratch, buf->size);
	if (strncmp(buf->scratch, "xref", 4) != 0)
		base_throw(xref->ctx, "cannot find xref marker");

	while (1)
	{
		c = base_peek_byte(xref->file);
		if (!(c >= '0' && c <= '9'))
			break;

		base_read_line(xref->file, buf->scratch, buf->size);
		s = buf->scratch;
		ofs = atoi(base_strsep(&s, " "));
		len = atoi(base_strsep(&s, " "));

		
		if (s && *s != '\0')
		{
			base_warn(xref->ctx, "broken xref section. proceeding anyway.");
			base_seek(xref->file, -(2 + (int)strlen(s)), 1);
		}

		
		if (ofs + len > xref->len)
		{
			base_warn(xref->ctx, "broken xref section, proceeding anyway.");
			pdf_resize_xref(xref, ofs + len);
		}

		for (i = ofs; i < ofs + len; i++)
		{
			n = base_read(xref->file, (unsigned char *) buf->scratch, 20);
			if (n < 0)
				base_throw(xref->ctx, "cannot read xref table");
			if (!xref->table[i].type)
			{
				s = buf->scratch;

				
				while (*s != '\0' && iswhite(*s))
					s++;

				xref->table[i].ofs = atoi(s);
				xref->table[i].gen = atoi(s + 11);
				xref->table[i].type = s[17];
				if (s[17] != 'f' && s[17] != 'n' && s[17] != 'o')
					base_throw(xref->ctx, "unexpected xref type: %#x (%d %d R)", s[17], i, xref->table[i].gen);
			}
		}
	}

	base_try(xref->ctx)
	{
		tok = pdf_lex(xref->file, buf);
		if (tok != PDF_TOK_TRAILER)
			base_throw(xref->ctx, "expected trailer marker");

		tok = pdf_lex(xref->file, buf);
		if (tok != PDF_TOK_OPEN_DICT)
			base_throw(xref->ctx, "expected trailer dictionary");

		trailer = pdf_parse_dict(xref, xref->file, buf);
	}
	base_catch(xref->ctx)
	{
		base_throw(xref->ctx, "cannot parse trailer");
	}
	return trailer;
}

static void
pdf_read_new_xref_section(pdf_document *xref, base_stream *stm, int i0, int i1, int w0, int w1, int w2)
{
	int i, n;

	if (i0 < 0 || i0 + i1 > xref->len)
		base_throw(xref->ctx, "xref stream has too many entries");

	for (i = i0; i < i0 + i1; i++)
	{
		int a = 0;
		int b = 0;
		int c = 0;

		if (base_is_eof(stm))
			base_throw(xref->ctx, "truncated xref stream");

		for (n = 0; n < w0; n++)
			a = (a << 8) + base_read_byte(stm);
		for (n = 0; n < w1; n++)
			b = (b << 8) + base_read_byte(stm);
		for (n = 0; n < w2; n++)
			c = (c << 8) + base_read_byte(stm);

		if (!xref->table[i].type)
		{
			int t = w0 ? a : 1;
			xref->table[i].type = t == 0 ? 'f' : t == 1 ? 'n' : t == 2 ? 'o' : 0;
			xref->table[i].ofs = w1 ? b : 0;
			xref->table[i].gen = w2 ? c : 0;
		}
	}
}

static pdf_obj *
pdf_read_new_xref(pdf_document *xref, pdf_lexbuf *buf)
{
	base_stream *stm = NULL;
	pdf_obj *trailer = NULL;
	pdf_obj *index = NULL;
	pdf_obj *obj = NULL;
	int num, gen, stm_ofs;
	int size, w0, w1, w2;
	int t;
	base_context *ctx = xref->ctx;

	base_var(trailer);
	base_var(stm);

	base_try(ctx)
	{
		trailer = pdf_parse_ind_obj(xref, xref->file, buf, &num, &gen, &stm_ofs);
	}
	base_catch(ctx)
	{
		base_throw(ctx, "cannot parse compressed xref stream object");
	}

	base_try(ctx)
	{
		base_unlock(ctx, base_LOCK_FILE);
		obj = pdf_dict_gets(trailer, "Size");
		if (!obj)
			base_throw(ctx, "xref stream missing Size entry (%d %d R)", num, gen);

		size = pdf_to_int(obj);
		if (size > xref->len)
			pdf_resize_xref(xref, size);

		if (num < 0 || num >= xref->len)
			base_throw(ctx, "object id (%d %d R) out of range (0..%d)", num, gen, xref->len - 1);

		obj = pdf_dict_gets(trailer, "W");
		if (!obj)
			base_throw(ctx, "xref stream missing W entry (%d %d R)", num, gen);
		w0 = pdf_to_int(pdf_array_get(obj, 0));
		w1 = pdf_to_int(pdf_array_get(obj, 1));
		w2 = pdf_to_int(pdf_array_get(obj, 2));

		index = pdf_dict_gets(trailer, "Index");

		stm = pdf_open_stream_with_offset(xref, num, gen, trailer, stm_ofs);
		

		if (!index)
		{
			pdf_read_new_xref_section(xref, stm, 0, size, w0, w1, w2);
			
		}
		else
		{
			int n = pdf_array_len(index);
			for (t = 0; t < n; t += 2)
			{
				int i0 = pdf_to_int(pdf_array_get(index, t + 0));
				int i1 = pdf_to_int(pdf_array_get(index, t + 1));
				pdf_read_new_xref_section(xref, stm, i0, i1, w0, w1, w2);
			}
		}
	}
	base_always(ctx)
	{
		base_close(stm);
	}
	base_catch(ctx)
	{
		pdf_drop_obj(trailer);
		pdf_drop_obj(index);
		base_rethrow(ctx);
	}
	base_lock(ctx, base_LOCK_FILE);

	return trailer;
}

static pdf_obj *
pdf_read_xref(pdf_document *xref, int ofs, pdf_lexbuf *buf)
{
	int c;
	base_context *ctx = xref->ctx;
	pdf_obj *trailer;

	base_seek(xref->file, ofs, 0);

	while (iswhite(base_peek_byte(xref->file)))
		base_read_byte(xref->file);

	base_try(ctx)
	{
		c = base_peek_byte(xref->file);
		if (c == 'x')
			trailer = pdf_read_old_xref(xref, buf);
		else if (c >= '0' && c <= '9')
			trailer = pdf_read_new_xref(xref, buf);
		else
			base_throw(ctx, "cannot recognize xref format");
	}
	base_catch(ctx)
	{
		base_throw(ctx, "cannot read xref (ofs=%d)", ofs);
	}
	return trailer;
}

static void
pdf_read_xref_sections(pdf_document *xref, int ofs, pdf_lexbuf *buf)
{
	pdf_obj *trailer = NULL;
	pdf_obj *xrefstm = NULL;
	pdf_obj *prev = NULL;
	base_context *ctx = xref->ctx;

	base_var(trailer);
	base_var(xrefstm);
	base_var(prev);

	base_try(ctx)
	{
		do
		{
			trailer = pdf_read_xref(xref, ofs, buf);

			
			xrefstm = pdf_dict_gets(trailer, "XRefStm");
			prev = pdf_dict_gets(trailer, "Prev");
			
			if (xrefstm && prev)
				pdf_read_xref_sections(xref, pdf_to_int(xrefstm), buf);
			if (prev)
				ofs = pdf_to_int(prev);
			else if (xrefstm)
				ofs = pdf_to_int(xrefstm);
			pdf_drop_obj(trailer);
			trailer = NULL;
		}
		while (prev || xrefstm);
	}
	base_catch(ctx)
	{
		pdf_drop_obj(trailer);
		base_throw(ctx, "cannot read xref at offset %d", ofs);
	}
}

static void
pdf_load_xref(pdf_document *xref, pdf_lexbuf *buf)
{
	pdf_obj *size;
	int i;
	base_context *ctx = xref->ctx;

	pdf_load_version(xref);

	pdf_read_start_xref(xref);

	pdf_read_trailer(xref, buf);

	size = pdf_dict_gets(xref->trailer, "Size");
	if (!size)
		base_throw(ctx, "trailer missing Size entry");

	pdf_resize_xref(xref, pdf_to_int(size));

	pdf_read_xref_sections(xref, xref->startxref, buf);

	
	if (xref->table[0].type != 'f')
		base_throw(ctx, "first object in xref is not free");

	
	for (i = 0; i < xref->len; i++)
	{
		if (xref->table[i].type == 'n')
		{
			
			if (xref->table[i].ofs == 0)
				xref->table[i].type = 'f';
			else if (xref->table[i].ofs <= 0 || xref->table[i].ofs >= xref->file_size)
				base_throw(ctx, "object offset out of range: %d (%d 0 R)", xref->table[i].ofs, i);
		}
		if (xref->table[i].type == 'o')
			if (xref->table[i].ofs <= 0 || xref->table[i].ofs >= xref->len || xref->table[xref->table[i].ofs].type != 'n')
				base_throw(ctx, "invalid reference to an objstm that does not exist: %d (%d 0 R)", xref->table[i].ofs, i);
	}
}

void
pdf_ocg_set_config(pdf_document *xref, int config)
{
	int i, j, len, len2;
	pdf_ocg_descriptor *desc = xref->ocg;
	pdf_obj *obj, *cobj;
	char *name;

	obj = pdf_dict_gets(pdf_dict_gets(xref->trailer, "Root"), "OCProperties");
	if (!obj)
	{
		if (config == 0)
			return;
		else
			base_throw(xref->ctx, "Unknown OCG config (None known!)");
	}
	if (config == 0)
	{
		cobj = pdf_dict_gets(obj, "D");
		if (!cobj)
			base_throw(xref->ctx, "No default OCG config");
	}
	else
	{
		cobj = pdf_array_get(pdf_dict_gets(obj, "Configs"), config);
		if (!cobj)
			base_throw(xref->ctx, "Illegal OCG config");
	}

	if (desc->intent)
		pdf_drop_obj(desc->intent);
	desc->intent = pdf_dict_gets(cobj, "Intent");
	if (desc->intent)
		pdf_keep_obj(desc->intent);

	len = desc->len;
	name = pdf_to_name(pdf_dict_gets(cobj, "BaseState"));
	if (strcmp(name, "Unchanged") == 0)
	{
		
	}
	else if (strcmp(name, "OFF") == 0)
	{
		for (i = 0; i < len; i++)
		{
			desc->ocgs[i].state = 0;
		}
	}
	else 
	{
		for (i = 0; i < len; i++)
		{
			desc->ocgs[i].state = 1;
		}
	}

	obj = pdf_dict_gets(cobj, "ON");
	len2 = pdf_array_len(obj);
	for (i = 0; i < len2; i++)
	{
		pdf_obj *o = pdf_array_get(obj, i);
		int n = pdf_to_num(o);
		int g = pdf_to_gen(o);
		for (j=0; j < len; j++)
		{
			if (desc->ocgs[j].num == n && desc->ocgs[j].gen == g)
			{
				desc->ocgs[j].state = 1;
				break;
			}
		}
	}

	obj = pdf_dict_gets(cobj, "OFF");
	len2 = pdf_array_len(obj);
	for (i = 0; i < len2; i++)
	{
		pdf_obj *o = pdf_array_get(obj, i);
		int n = pdf_to_num(o);
		int g = pdf_to_gen(o);
		for (j=0; j < len; j++)
		{
			if (desc->ocgs[j].num == n && desc->ocgs[j].gen == g)
			{
				desc->ocgs[j].state = 0;
				break;
			}
		}
	}

	
	
	
	
	
	
	
}

static void
pdf_read_ocg(pdf_document *xref)
{
	pdf_obj *obj, *ocg;
	int len, i;
	pdf_ocg_descriptor *desc;
	base_context *ctx = xref->ctx;

	base_var(desc);

	obj = pdf_dict_gets(pdf_dict_gets(xref->trailer, "Root"), "OCProperties");
	if (!obj)
		return;
	ocg = pdf_dict_gets(obj, "OCGs");
	if (!ocg || !pdf_is_array(ocg))
		
		return;
	len = pdf_array_len(ocg);
	base_try(ctx)
	{
		desc = base_calloc(ctx, 1, sizeof(*desc));
		desc->len = len;
		desc->ocgs = base_calloc(ctx, len, sizeof(*desc->ocgs));
		desc->intent = NULL;
		for (i=0; i < len; i++)
		{
			pdf_obj *o = pdf_array_get(ocg, i);
			desc->ocgs[i].num = pdf_to_num(o);
			desc->ocgs[i].gen = pdf_to_gen(o);
			desc->ocgs[i].state = 0;
		}
		xref->ocg = desc;
	}
	base_catch(ctx)
	{
		if (desc)
			base_free(ctx, desc->ocgs);
		base_free(ctx, desc);
		base_rethrow(ctx);
	}

	pdf_ocg_set_config(xref, 0);
}

static void
pdf_free_ocg(base_context *ctx, pdf_ocg_descriptor *desc)
{
	if (!desc)
		return;

	if (desc->intent)
		pdf_drop_obj(desc->intent);
	base_free(ctx, desc->ocgs);
	base_free(ctx, desc);
}

static void pdf_init_document(pdf_document *xref);

pdf_document *
pdf_open_document_with_stream(base_stream *file)
{
	pdf_document *xref;
	pdf_obj *encrypt, *id;
	pdf_obj *dict = NULL;
	pdf_obj *obj;
	pdf_obj *nobj = NULL;
	int i, repaired = 0;
	int locked;
	base_context *ctx = file->ctx;

	base_var(dict);
	base_var(nobj);
	base_var(locked);

	xref = base_malloc_struct(ctx, pdf_document);
	pdf_init_document(xref);
	xref->lexbuf.base.size = PDF_LEXBUF_LARGE;

	xref->file = base_keep_stream(file);
	xref->ctx = ctx;

	base_lock(ctx, base_LOCK_FILE);
	locked = 1;

	base_try(ctx)
	{
		pdf_load_xref(xref, &xref->lexbuf.base);
	}
	base_catch(ctx)
	{
		if (xref->table)
		{
			base_free(xref->ctx, xref->table);
			xref->table = NULL;
			xref->len = 0;
		}
		if (xref->trailer)
		{
			pdf_drop_obj(xref->trailer);
			xref->trailer = NULL;
		}
		base_warn(xref->ctx, "trying to repair broken xref");
		repaired = 1;
	}

	base_try(ctx)
	{
		int hasroot, hasinfo;

		if (repaired)
			pdf_repair_xref(xref, &xref->lexbuf.base);

		base_unlock(ctx, base_LOCK_FILE);
		locked = 0;

		encrypt = pdf_dict_gets(xref->trailer, "Encrypt");
		id = pdf_dict_gets(xref->trailer, "ID");
		if (pdf_is_dict(encrypt))
			xref->crypt = pdf_new_crypt(ctx, encrypt, id);

		
		pdf_authenticate_password(xref, "");

		if (repaired)
		{
			pdf_repair_obj_stms(xref);

			hasroot = (pdf_dict_gets(xref->trailer, "Root") != NULL);
			hasinfo = (pdf_dict_gets(xref->trailer, "Info") != NULL);

			for (i = 1; i < xref->len; i++)
			{
				if (xref->table[i].type == 0 || xref->table[i].type == 'f')
					continue;

				base_try(ctx)
				{
					dict = pdf_load_object(xref, i, 0);
				}
				base_catch(ctx)
				{
					base_warn(ctx, "ignoring broken object (%d 0 R)", i);
					continue;
				}

				if (!hasroot)
				{
					obj = pdf_dict_gets(dict, "Type");
					if (pdf_is_name(obj) && !strcmp(pdf_to_name(obj), "Catalog"))
					{
						nobj = pdf_new_indirect(ctx, i, 0, xref);
						pdf_dict_puts(xref->trailer, "Root", nobj);
						pdf_drop_obj(nobj);
						nobj = NULL;
					}
				}

				if (!hasinfo)
				{
					if (pdf_dict_gets(dict, "Creator") || pdf_dict_gets(dict, "Producer"))
					{
						nobj = pdf_new_indirect(ctx, i, 0, xref);
						pdf_dict_puts(xref->trailer, "Info", nobj);
						pdf_drop_obj(nobj);
						nobj = NULL;
					}
				}

				pdf_drop_obj(dict);
				dict = NULL;
			}
		}
	}
	base_always(ctx)
	{
		if (locked)
			base_unlock(ctx, base_LOCK_FILE);
	}
	base_catch(ctx)
	{
		pdf_drop_obj(dict);
		pdf_drop_obj(nobj);
		pdf_close_document(xref);
		base_throw(ctx, "cannot open document");
	}

	base_try(ctx)
	{
		pdf_read_ocg(xref);
	}
	base_catch(ctx)
	{
		base_warn(ctx, "Ignoring Broken Optional Content");
	}

	return xref;
}

void
pdf_close_document(pdf_document *xref)
{
	int i;
	base_context *ctx;

	if (!xref)
		return;
	ctx = xref->ctx;

	if (xref->table)
	{
		for (i = 0; i < xref->len; i++)
		{
			if (xref->table[i].obj)
			{
				pdf_drop_obj(xref->table[i].obj);
				xref->table[i].obj = NULL;
			}
		}
		base_free(xref->ctx, xref->table);
	}

	if (xref->page_objs)
	{
		for (i = 0; i < xref->page_len; i++)
			pdf_drop_obj(xref->page_objs[i]);
		base_free(ctx, xref->page_objs);
	}

	if (xref->page_refs)
	{
		for (i = 0; i < xref->page_len; i++)
			pdf_drop_obj(xref->page_refs[i]);
		base_free(ctx, xref->page_refs);
	}

	if (xref->file)
		base_close(xref->file);
	if (xref->trailer)
		pdf_drop_obj(xref->trailer);
	if (xref->crypt)
		pdf_free_crypt(ctx, xref->crypt);

	pdf_free_ocg(ctx, xref->ocg);

	base_empty_store(ctx);

	base_free(ctx, xref);
}

void
pdf_print_xref(pdf_document *xref)
{
	int i;
	printf("xref\n0 %d\n", xref->len);
	for (i = 0; i < xref->len; i++)
	{
		printf("%05d: %010d %05d %c (stm_ofs=%d)\n", i,
			xref->table[i].ofs,
			xref->table[i].gen,
			xref->table[i].type ? xref->table[i].type : '-',
			xref->table[i].stm_ofs);
	}
}

static void
pdf_load_obj_stm(pdf_document *xref, int num, int gen, pdf_lexbuf *buf)
{
	base_stream *stm = NULL;
	pdf_obj *objstm = NULL;
	int *numbuf = NULL;
	int *ofsbuf = NULL;

	pdf_obj *obj;
	int first;
	int count;
	int i;
	int tok;
	base_context *ctx = xref->ctx;

	base_var(numbuf);
	base_var(ofsbuf);
	base_var(objstm);
	base_var(stm);

	base_try(ctx)
	{
		objstm = pdf_load_object(xref, num, gen);

		count = pdf_to_int(pdf_dict_gets(objstm, "N"));
		first = pdf_to_int(pdf_dict_gets(objstm, "First"));

		numbuf = base_calloc(ctx, count, sizeof(int));
		ofsbuf = base_calloc(ctx, count, sizeof(int));

		stm = pdf_open_stream(xref, num, gen);
		for (i = 0; i < count; i++)
		{
			tok = pdf_lex(stm, buf);
			if (tok != PDF_TOK_INT)
				base_throw(ctx, "corrupt object stream (%d %d R)", num, gen);
			numbuf[i] = buf->i;

			tok = pdf_lex(stm, buf);
			if (tok != PDF_TOK_INT)
				base_throw(ctx, "corrupt object stream (%d %d R)", num, gen);
			ofsbuf[i] = buf->i;
		}

		base_seek(stm, first, 0);

		for (i = 0; i < count; i++)
		{
			base_seek(stm, first + ofsbuf[i], 0);

			obj = pdf_parse_stm_obj(xref, stm, buf);
			

			if (numbuf[i] < 1 || numbuf[i] >= xref->len)
			{
				pdf_drop_obj(obj);
				base_throw(ctx, "object id (%d 0 R) out of range (0..%d)", numbuf[i], xref->len - 1);
			}

			if (xref->table[numbuf[i]].type == 'o' && xref->table[numbuf[i]].ofs == num)
			{
				if (xref->table[numbuf[i]].obj)
					pdf_drop_obj(xref->table[numbuf[i]].obj);
				xref->table[numbuf[i]].obj = obj;
			}
			else
			{
				pdf_drop_obj(obj);
			}
		}
	}
	base_always(ctx)
	{
		base_close(stm);
		base_free(xref->ctx, ofsbuf);
		base_free(xref->ctx, numbuf);
		pdf_drop_obj(objstm);
	}
	base_catch(ctx)
	{
		base_throw(ctx, "cannot open object stream (%d %d R)", num, gen);
	}
}

void
pdf_cache_object(pdf_document *xref, int num, int gen)
{
	pdf_xref_entry *x;
	int rnum, rgen;
	base_context *ctx = xref->ctx;

	if (num < 0 || num >= xref->len)
		base_throw(ctx, "object out of range (%d %d R); xref size %d", num, gen, xref->len);

	x = &xref->table[num];

	if (x->obj)
		return;

	if (x->type == 'f')
	{
		x->obj = pdf_new_null(ctx);
		return;
	}
	else if (x->type == 'n')
	{
		base_lock(ctx, base_LOCK_FILE);
		base_seek(xref->file, x->ofs, 0);

		base_try(ctx)
		{
			x->obj = pdf_parse_ind_obj(xref, xref->file, &xref->lexbuf.base,
					&rnum, &rgen, &x->stm_ofs);
		}
		base_catch(ctx)
		{
			base_unlock(ctx, base_LOCK_FILE);
			base_throw(ctx, "cannot parse object (%d %d R)", num, gen);
		}

		if (rnum != num)
		{
			pdf_drop_obj(x->obj);
			x->obj = NULL;
			base_unlock(ctx, base_LOCK_FILE);
			base_throw(ctx, "found object (%d %d R) instead of (%d %d R)", rnum, rgen, num, gen);
		}

		if (xref->crypt)
			pdf_crypt_obj(ctx, xref->crypt, x->obj, num, gen);
		base_unlock(ctx, base_LOCK_FILE);
	}
	else if (x->type == 'o')
	{
		if (!x->obj)
		{
			base_try(ctx)
			{
				pdf_load_obj_stm(xref, x->ofs, 0, &xref->lexbuf.base);
			}
			base_catch(ctx)
			{
				base_throw(ctx, "cannot load object stream containing object (%d %d R)", num, gen);
			}
			if (!x->obj)
				base_throw(ctx, "object (%d %d R) was not found in its object stream", num, gen);
		}
	}
	else
	{
		base_throw(ctx, "assert: corrupt xref struct");
	}
}

pdf_obj *
pdf_load_object(pdf_document *xref, int num, int gen)
{
	base_context *ctx = xref->ctx;

	base_try(ctx)
	{
		pdf_cache_object(xref, num, gen);
	}
	base_catch(ctx)
	{
		base_throw(ctx, "cannot load object (%d %d R) into cache", num, gen);
	}

	assert(xref->table[num].obj);

	return pdf_keep_obj(xref->table[num].obj);
}

pdf_obj *
pdf_resolve_indirect(pdf_obj *ref)
{
	int sanity = 10;
	int num;
	int gen;
	base_context *ctx = NULL; 
	pdf_document *xref;

	while (pdf_is_indirect(ref))
	{
		if (--sanity == 0)
		{
			base_warn(ctx, "Too many indirections (possible indirection cycle involving %d %d R)", num, gen);
			return NULL;
		}
		xref = pdf_get_indirect_document(ref);
		if (!xref)
			return NULL;
		ctx = xref->ctx;
		num = pdf_to_num(ref);
		gen = pdf_to_gen(ref);
		base_try(ctx)
		{
			pdf_cache_object(xref, num, gen);
		}
		base_catch(ctx)
		{
			base_warn(ctx, "cannot load object (%d %d R) into cache", num, gen);
			return NULL;
		}
		if (!xref->table[num].obj)
			return NULL;
		ref = xref->table[num].obj;
	}

	return ref;
}

int pdf_count_objects(pdf_document *doc)
{
	return doc->len;
}

void
pdf_update_object(pdf_document *xref, int num, int gen, pdf_obj *newobj)
{
	pdf_xref_entry *x;

	if (num < 0 || num >= xref->len)
	{
		base_warn(xref->ctx, "object out of range (%d %d R); xref size %d", num, gen, xref->len);
		return;
	}

	x = &xref->table[num];

	if (x->obj)
		pdf_drop_obj(x->obj);

	x->obj = pdf_keep_obj(newobj);
	x->type = 'n';
	x->ofs = 0;
}

pdf_document *
pdf_open_document(base_context *ctx, const wchar_t *filename)
{
	base_stream *file = NULL;
	pdf_document *xref;

	base_var(file);
	base_try(ctx)
	{
		file = base_open_file(ctx, filename);
		xref = pdf_open_document_with_stream(file);
	}
	base_catch(ctx)
	{
		base_close(file);
		base_throw(ctx, "cannot load document '%s'", filename);
	}

	base_close(file);
	return xref;
}

static void pdf_close_document_shim(base_document *doc)
{
	pdf_close_document((pdf_document*)doc);
}

static int pdf_needs_password_shim(base_document *doc)
{
	return pdf_needs_password((pdf_document*)doc);
}

static int pdf_authenticate_password_shim(base_document *doc, char *password)
{
	return pdf_authenticate_password((pdf_document*)doc, password);
}

static base_outline *pdf_load_outline_shim(base_document *doc)
{
	return pdf_load_outline((pdf_document*)doc);
}

static int pdf_count_pages_shim(base_document *doc)
{
	return pdf_count_pages((pdf_document*)doc);
}

static base_page *pdf_load_page_shim(base_document *doc, int number)
{
	return (base_page*) pdf_load_page((pdf_document*)doc, number);
}

static base_link *pdf_load_links_shim(base_document *doc, base_page *page)
{
	return pdf_load_links((pdf_document*)doc, (pdf_page*)page);
}

static base_rect pdf_bound_page_shim(base_document *doc, base_page *page)
{
	return pdf_bound_page((pdf_document*)doc, (pdf_page*)page);
}

static void pdf_run_page_shim(base_document *doc, base_page *page, base_device *dev, base_matrix transform, base_cookie *cookie)
{
	pdf_run_page((pdf_document*)doc, (pdf_page*)page, dev, transform, cookie);
}

static void pdf_free_page_shim(base_document *doc, base_page *page)
{
	pdf_free_page((pdf_document*)doc, (pdf_page*)page);
}

static int pdf_meta(base_document *doc_, int key, void *ptr, int size)
{
	pdf_document *doc = (pdf_document *)doc_;

	switch(key)
	{
	
	case base_META_FORMAT_INFO:
		sprintf((char *)ptr, "PDF %d.%d", doc->version/10, doc->version % 10);
		return base_META_OK;
	case base_META_CRYPT_INFO:
		if (doc->crypt)
			sprintf((char *)ptr, "Standard V%d %d-bit %s",
				pdf_crypt_revision(doc),
				pdf_crypt_length(doc),
				pdf_crypt_method(doc));
		else
			sprintf((char *)ptr, "None");
		return base_META_OK;
	case base_META_HAS_PERMISSION:
	{
		int i;
		switch (size)
		{
		case base_PERMISSION_PRINT:
			i = PDF_PERM_PRINT;
			break;
		case base_PERMISSION_CHANGE:
			i = PDF_PERM_CHANGE;
			break;
		case base_PERMISSION_COPY:
			i = PDF_PERM_COPY;
			break;
		case base_PERMISSION_NOTES:
			i = PDF_PERM_NOTES;
			break;
		default:
			return 0;
		}
		return pdf_has_permission(doc, i);
	}
	case base_META_INFO:
	{
		pdf_obj *info = pdf_dict_gets(doc->trailer, "Info");
		if (!info)
		{
			if (ptr)
				*(char *)ptr = 0;
			return 0;
		}
		info = pdf_dict_gets(info, *(char **)ptr);
		if (!info)
		{
			if (ptr)
				*(char *)ptr = 0;
			return 0;
		}
		if (info && ptr && size)
		{
			char *utf8 = pdf_to_utf8(doc->ctx, info);
			strncpy(ptr, utf8, size);
			((char *)ptr)[size-1] = 0;
			base_free(doc->ctx, utf8);
		}
		return 1;
	}
	default:
		return base_META_UNKNOWN_KEY;
	}
}

static void
pdf_init_document(pdf_document *doc)
{
	doc->super.close = pdf_close_document_shim;
	doc->super.needs_password = pdf_needs_password_shim;
	doc->super.authenticate_password = pdf_authenticate_password_shim;
	doc->super.load_outline = pdf_load_outline_shim;
	doc->super.count_pages = pdf_count_pages_shim;
	doc->super.load_page = pdf_load_page_shim;
	doc->super.load_links = pdf_load_links_shim;
	doc->super.bound_page = pdf_bound_page_shim;
	doc->super.run_page = pdf_run_page_shim;
	doc->super.free_page = pdf_free_page_shim;
	doc->super.meta = pdf_meta;
}
