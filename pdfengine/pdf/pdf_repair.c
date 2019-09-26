#include "pdf-internal.h"
#include "pdfengine-internal.h"

struct entry
{
	int num;
	int gen;
	int ofs;
	int stm_ofs;
	int stm_len;
};

static void
pdf_repair_obj(base_stream *file, pdf_lexbuf *buf, int *stmofsp, int *stmlenp, pdf_obj **encrypt, pdf_obj **id)
{
	int tok;
	int stm_len;
	int n;
	base_context *ctx = file->ctx;

	*stmofsp = 0;
	*stmlenp = -1;

	stm_len = 0;

	tok = pdf_lex(file, buf);
	
	if (tok == PDF_TOK_OPEN_DICT)
	{
		pdf_obj *dict, *obj;

		
		base_try(ctx)
		{
			dict = pdf_parse_dict(NULL, file, buf);
		}
		base_catch(ctx)
		{
			
			if (file->eof)
				base_throw(ctx, "broken object at EOF ignored");
			
			dict = pdf_new_dict(ctx, 2);
		}

		obj = pdf_dict_gets(dict, "Type");
		if (pdf_is_name(obj) && !strcmp(pdf_to_name(obj), "XRef"))
		{
			obj = pdf_dict_gets(dict, "Encrypt");
			if (obj)
			{
				if (*encrypt)
					pdf_drop_obj(*encrypt);
				*encrypt = pdf_keep_obj(obj);
			}

			obj = pdf_dict_gets(dict, "ID");
			if (obj)
			{
				if (*id)
					pdf_drop_obj(*id);
				*id = pdf_keep_obj(obj);
			}
		}

		obj = pdf_dict_gets(dict, "Length");
		if (!pdf_is_indirect(obj) && pdf_is_int(obj))
			stm_len = pdf_to_int(obj);

		pdf_drop_obj(dict);
	}

	while ( tok != PDF_TOK_STREAM &&
		tok != PDF_TOK_ENDOBJ &&
		tok != PDF_TOK_ERROR &&
		tok != PDF_TOK_EOF &&
		tok != PDF_TOK_INT )
	{
		tok = pdf_lex(file, buf);
		
	}

	if (tok == PDF_TOK_INT)
	{
		while (buf->len-- > 0)
			base_unread_byte(file);
	}
	else if (tok == PDF_TOK_STREAM)
	{
		int c = base_read_byte(file);
		if (c == '\r') {
			c = base_peek_byte(file);
			if (c == '\n')
				base_read_byte(file);
		}

		*stmofsp = base_tell(file);
		if (*stmofsp < 0)
			base_throw(ctx, "cannot seek in file");

		if (stm_len > 0)
		{
			base_seek(file, *stmofsp + stm_len, 0);
			base_try(ctx)
			{
				tok = pdf_lex(file, buf);
			}
			base_catch(ctx)
			{
				base_warn(ctx, "cannot find endstream token, falling back to scanning");
			}
			if (tok == PDF_TOK_ENDSTREAM)
				goto atobjend;
			base_seek(file, *stmofsp, 0);
		}

		n = base_read(file, (unsigned char *) buf->scratch, 9);
		if (n < 0)
			base_throw(ctx, "cannot read from file");

		while (memcmp(buf->scratch, "endstream", 9) != 0)
		{
			c = base_read_byte(file);
			if (c == EOF)
				break;
			memmove(&buf->scratch[0], &buf->scratch[1], 8);
			buf->scratch[8] = c;
		}

		*stmlenp = base_tell(file) - *stmofsp - 9;

atobjend:
		tok = pdf_lex(file, buf);
		
		if (tok != PDF_TOK_ENDOBJ)
			base_warn(ctx, "object missing 'endobj' token");
	}
}

static void
pdf_repair_obj_stm(pdf_document *xref, int num, int gen)
{
	pdf_obj *obj;
	base_stream *stm = NULL;
	int tok;
	int i, n, count;
	base_context *ctx = xref->ctx;
	pdf_lexbuf buf;

	base_var(stm);

	buf.size = PDF_LEXBUF_SMALL;

	base_try(ctx)
	{
		obj = pdf_load_object(xref, num, gen);

		count = pdf_to_int(pdf_dict_gets(obj, "N"));

		pdf_drop_obj(obj);

		stm = pdf_open_stream(xref, num, gen);

		for (i = 0; i < count; i++)
		{
			tok = pdf_lex(stm, &buf);
			if (tok != PDF_TOK_INT)
				base_throw(ctx, "corrupt object stream (%d %d R)", num, gen);

			n = buf.i;
			if (n >= xref->len)
				pdf_resize_xref(xref, n + 1);

			xref->table[n].ofs = num;
			xref->table[n].gen = i;
			xref->table[n].stm_ofs = 0;
			pdf_drop_obj(xref->table[n].obj);
			xref->table[n].obj = NULL;
			xref->table[n].type = 'o';

			tok = pdf_lex(stm, &buf);
			if (tok != PDF_TOK_INT)
				base_throw(ctx, "corrupt object stream (%d %d R)", num, gen);
		}
	}
	base_always(ctx)
	{
		base_close(stm);
	}
	base_catch(ctx)
	{
		base_throw(ctx, "cannot load object stream object (%d %d R)", num, gen);
	}
}

void
pdf_repair_xref(pdf_document *xref, pdf_lexbuf *buf)
{
	pdf_obj *dict, *obj;
	pdf_obj *length;

	pdf_obj *encrypt = NULL;
	pdf_obj *id = NULL;
	pdf_obj *root = NULL;
	pdf_obj *info = NULL;

	struct entry *list = NULL;
	int listlen;
	int listcap;
	int maxnum = 0;

	int num = 0;
	int gen = 0;
	int tmpofs, numofs = 0, genofs = 0;
	int stm_len, stm_ofs = 0;
	int tok;
	int next;
	int i, n, c;
	base_context *ctx = xref->ctx;

	base_var(encrypt);
	base_var(id);
	base_var(root);
	base_var(info);
	base_var(list);

	base_seek(xref->file, 0, 0);

	base_try(ctx)
	{
		listlen = 0;
		listcap = 1024;
		list = base_malloc_array(ctx, listcap, sizeof(struct entry));

		
		n = base_read(xref->file, (unsigned char *)buf->scratch, MIN(buf->size, 1024));
		if (n < 0)
			base_throw(ctx, "cannot read from file");

		base_seek(xref->file, 0, 0);
		for (i = 0; i < n - 4; i++)
		{
			if (memcmp(&buf->scratch[i], "%PDF", 4) == 0)
			{
				base_seek(xref->file, i + 8, 0); 
				break;
			}
		}

		
		c = base_read_byte(xref->file);
		while (c >= 0 && (c == ' ' || c == '%'))
			c = base_read_byte(xref->file);
		base_unread_byte(xref->file);

		while (1)
		{
			tmpofs = base_tell(xref->file);
			if (tmpofs < 0)
				base_throw(ctx, "cannot tell in file");

			base_try(ctx)
			{
				tok = pdf_lex(xref->file, buf);
			}
			base_catch(ctx)
			{
				base_warn(ctx, "ignoring the rest of the file");
				break;
			}

			if (tok == PDF_TOK_INT)
			{
				numofs = genofs;
				num = gen;
				genofs = tmpofs;
				gen = buf->i;
			}

			else if (tok == PDF_TOK_OBJ)
			{
				base_try(ctx)
				{
					pdf_repair_obj(xref->file, buf, &stm_ofs, &stm_len, &encrypt, &id);
				}
				base_catch(ctx)
				{
					
					if (!root)
						base_rethrow(ctx);
					base_warn(ctx, "cannot parse object (%d %d R) - ignoring rest of file", num, gen);
					break;
				}

				if (listlen + 1 == listcap)
				{
					listcap = (listcap * 3) / 2;
					list = base_resize_array(ctx, list, listcap, sizeof(struct entry));
				}

				list[listlen].num = num;
				list[listlen].gen = gen;
				list[listlen].ofs = numofs;
				list[listlen].stm_ofs = stm_ofs;
				list[listlen].stm_len = stm_len;
				listlen ++;

				if (num > maxnum)
					maxnum = num;
			}

			
			else if (tok == PDF_TOK_OPEN_DICT)
			{
				base_try(ctx)
				{
					dict = pdf_parse_dict(xref, xref->file, buf);
				}
				base_catch(ctx)
				{
					
					if (!root)
						base_rethrow(ctx);
					base_warn(ctx, "cannot parse trailer dictionary - ignoring rest of file");
					break;
				}

				obj = pdf_dict_gets(dict, "Encrypt");
				if (obj)
				{
					if (encrypt)
						pdf_drop_obj(encrypt);
					encrypt = pdf_keep_obj(obj);
				}

				obj = pdf_dict_gets(dict, "ID");
				if (obj)
				{
					if (id)
						pdf_drop_obj(id);
					id = pdf_keep_obj(obj);
				}

				obj = pdf_dict_gets(dict, "Root");
				if (obj)
				{
					if (root)
						pdf_drop_obj(root);
					root = pdf_keep_obj(obj);
				}

				obj = pdf_dict_gets(dict, "Info");
				if (obj)
				{
					if (info)
						pdf_drop_obj(info);
					info = pdf_keep_obj(obj);
				}

				pdf_drop_obj(dict);
			}

			else if (tok == PDF_TOK_ERROR)
				base_read_byte(xref->file);

			else if (tok == PDF_TOK_EOF)
				break;
		}

		

		pdf_resize_xref(xref, maxnum + 1);

		for (i = 0; i < listlen; i++)
		{
			xref->table[list[i].num].type = 'n';
			xref->table[list[i].num].ofs = list[i].ofs;
			xref->table[list[i].num].gen = list[i].gen;

			xref->table[list[i].num].stm_ofs = list[i].stm_ofs;

			
			if (list[i].stm_len >= 0)
			{
				base_unlock(ctx, base_LOCK_FILE);
				base_try(ctx)
				{
					dict = pdf_load_object(xref, list[i].num, list[i].gen);
				}
				base_always(ctx)
				{
					base_lock(ctx, base_LOCK_FILE);
				}
				base_catch(ctx)
				{
					base_rethrow(ctx);
				}
				

				length = pdf_new_int(ctx, list[i].stm_len);
				pdf_dict_puts(dict, "Length", length);
				pdf_drop_obj(length);

				pdf_drop_obj(dict);
			}

		}

		xref->table[0].type = 'f';
		xref->table[0].ofs = 0;
		xref->table[0].gen = 65535;
		xref->table[0].stm_ofs = 0;
		xref->table[0].obj = NULL;

		next = 0;
		for (i = xref->len - 1; i >= 0; i--)
		{
			if (xref->table[i].type == 'f')
			{
				xref->table[i].ofs = next;
				if (xref->table[i].gen < 65535)
					xref->table[i].gen ++;
				next = i;
			}
		}

		

		xref->trailer = pdf_new_dict(ctx, 5);

		obj = pdf_new_int(ctx, maxnum + 1);
		pdf_dict_puts(xref->trailer, "Size", obj);
		pdf_drop_obj(obj);

		if (root)
		{
			pdf_dict_puts(xref->trailer, "Root", root);
			pdf_drop_obj(root);
		}
		if (info)
		{
			pdf_dict_puts(xref->trailer, "Info", info);
			pdf_drop_obj(info);
		}

		if (encrypt)
		{
			if (pdf_is_indirect(encrypt))
			{
				
				obj = pdf_new_indirect(ctx, pdf_to_num(encrypt), pdf_to_gen(encrypt), xref);
				pdf_drop_obj(encrypt);
				encrypt = obj;
			}
			pdf_dict_puts(xref->trailer, "Encrypt", encrypt);
			pdf_drop_obj(encrypt);
		}

		if (id)
		{
			if (pdf_is_indirect(id))
			{
				
				obj = pdf_new_indirect(ctx, pdf_to_num(id), pdf_to_gen(id), xref);
				pdf_drop_obj(id);
				id = obj;
			}
			pdf_dict_puts(xref->trailer, "ID", id);
			pdf_drop_obj(id);
		}

		base_free(ctx, list);
	}
	base_catch(ctx)
	{
		if (encrypt) pdf_drop_obj(encrypt);
		if (id) pdf_drop_obj(id);
		if (root) pdf_drop_obj(root);
		if (info) pdf_drop_obj(info);
		base_free(ctx, list);
		base_rethrow(ctx);
	}
}

void
pdf_repair_obj_stms(pdf_document *xref)
{
	pdf_obj *dict;
	int i;

	for (i = 0; i < xref->len; i++)
	{
		if (xref->table[i].stm_ofs)
		{
			dict = pdf_load_object(xref, i, 0);
			if (!strcmp(pdf_to_name(pdf_dict_gets(dict, "Type")), "ObjStm"))
				pdf_repair_obj_stm(xref, i, 0);
			pdf_drop_obj(dict);
		}
	}

	
	for (i = 0; i < xref->len; i++)
		if (xref->table[i].type == 'o' && xref->table[xref->table[i].ofs].type != 'n')
			base_throw(xref->ctx, "invalid reference to non-object-stream: %d (%d 0 R)", xref->table[i].ofs, i);
}
