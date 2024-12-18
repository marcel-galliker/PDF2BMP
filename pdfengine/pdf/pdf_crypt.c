#include "pdf-internal.h"
#include "pdfengine-internal.h"

enum
{
	PDF_CRYPT_NONE,
	PDF_CRYPT_RC4,
	PDF_CRYPT_AESV2,
	PDF_CRYPT_AESV3,
	PDF_CRYPT_UNKNOWN,
};

typedef struct pdf_crypt_filter_s pdf_crypt_filter;

struct pdf_crypt_filter_s
{
	int method;
	int length;
};

struct pdf_crypt_s
{
	pdf_obj *id;

	int v;
	int length;
	pdf_obj *cf;
	pdf_crypt_filter stmf;
	pdf_crypt_filter strf;

	int r;
	unsigned char o[48];
	unsigned char u[48];
	unsigned char oe[32];
	unsigned char ue[32];
	int p;
	int encrypt_metadata;

	unsigned char key[32]; 
	base_context *ctx;
};

static void pdf_parse_crypt_filter(base_context *ctx, pdf_crypt_filter *cf, pdf_obj *dict, char *name, int defaultlength);

pdf_crypt *
pdf_new_crypt(base_context *ctx, pdf_obj *dict, pdf_obj *id)
{
	pdf_crypt *crypt;
	pdf_obj *obj;

	crypt = base_malloc_struct(ctx, pdf_crypt);

	

	obj = pdf_dict_gets(dict, "Filter");
	if (!pdf_is_name(obj))
	{
		pdf_free_crypt(ctx, crypt);
		base_throw(ctx, "unspecified encryption handler");
	}
	if (strcmp(pdf_to_name(obj), "Standard") != 0)
	{
		pdf_free_crypt(ctx, crypt);
		base_throw(ctx, "unknown encryption handler: '%s'", pdf_to_name(obj));
	}

	crypt->v = 0;
	obj = pdf_dict_gets(dict, "V");
	if (pdf_is_int(obj))
		crypt->v = pdf_to_int(obj);
	if (crypt->v != 1 && crypt->v != 2 && crypt->v != 4 && crypt->v != 5)
	{
		pdf_free_crypt(ctx, crypt);
		base_throw(ctx, "unknown encryption version");
	}

	crypt->length = 40;
	if (crypt->v == 2 || crypt->v == 4)
	{
		obj = pdf_dict_gets(dict, "Length");
		if (pdf_is_int(obj))
			crypt->length = pdf_to_int(obj);

		
		if (crypt->length < 40)
			crypt->length = crypt->length * 8;

		if (crypt->length % 8 != 0)
		{
			pdf_free_crypt(ctx, crypt);
			base_throw(ctx, "invalid encryption key length");
		}
		if (crypt->length > 256)
		{
			pdf_free_crypt(ctx, crypt);
			base_throw(ctx, "invalid encryption key length");
		}
	}

	if (crypt->v == 5)
		crypt->length = 256;

	if (crypt->v == 1 || crypt->v == 2)
	{
		crypt->stmf.method = PDF_CRYPT_RC4;
		crypt->stmf.length = crypt->length;

		crypt->strf.method = PDF_CRYPT_RC4;
		crypt->strf.length = crypt->length;
	}

	if (crypt->v == 4 || crypt->v == 5)
	{
		crypt->stmf.method = PDF_CRYPT_NONE;
		crypt->stmf.length = crypt->length;

		crypt->strf.method = PDF_CRYPT_NONE;
		crypt->strf.length = crypt->length;

		obj = pdf_dict_gets(dict, "CF");
		if (pdf_is_dict(obj))
		{
			crypt->cf = pdf_keep_obj(obj);
		}
		else
		{
			crypt->cf = NULL;
		}

		base_try(ctx)
		{
			obj = pdf_dict_gets(dict, "StmF");
			if (pdf_is_name(obj))
				pdf_parse_crypt_filter(ctx, &crypt->stmf, crypt->cf, pdf_to_name(obj), crypt->length);

			obj = pdf_dict_gets(dict, "StrF");
			if (pdf_is_name(obj))
				pdf_parse_crypt_filter(ctx, &crypt->strf, crypt->cf, pdf_to_name(obj), crypt->length);
		}
		base_catch(ctx)
		{
			pdf_free_crypt(ctx, crypt);
			base_throw(ctx, "cannot parse string crypt filter (%d %d R)", pdf_to_num(obj), pdf_to_gen(obj));
		}

		
		if (crypt->strf.method != PDF_CRYPT_NONE)
			crypt->length = crypt->stmf.length;
	}

	

	obj = pdf_dict_gets(dict, "R");
	if (pdf_is_int(obj))
		crypt->r = pdf_to_int(obj);
	else if (crypt->v <= 4)
	{
		base_warn(ctx, "encryption dictionary missing revision value, guessing...");
		if (crypt->v < 2)
			crypt->r = 2;
		else if (crypt->v == 2 || crypt->v == 3)
			crypt->r = 3;
		else if (crypt->v == 4)
			crypt->r = 4;
	}
	else
	{
		pdf_free_crypt(ctx, crypt);
		base_throw(ctx, "encryption dictionary missing version and revision value");
	}

	obj = pdf_dict_gets(dict, "O");
	if (pdf_is_string(obj) && pdf_to_str_len(obj) == 32)
		memcpy(crypt->o, pdf_to_str_buf(obj), 32);
	
	else if (crypt->r == 5 && pdf_is_string(obj) && pdf_to_str_len(obj) >= 48)
		memcpy(crypt->o, pdf_to_str_buf(obj), 48);
	else
	{
		pdf_free_crypt(ctx, crypt);
		base_throw(ctx, "encryption dictionary missing owner password");
	}

	obj = pdf_dict_gets(dict, "U");
	if (pdf_is_string(obj) && pdf_to_str_len(obj) == 32)
		memcpy(crypt->u, pdf_to_str_buf(obj), 32);
	else if (pdf_is_string(obj) && pdf_to_str_len(obj) >= 48 && crypt->r == 5)
		memcpy(crypt->u, pdf_to_str_buf(obj), 48);
	else if (pdf_is_string(obj) && pdf_to_str_len(obj) < 32)
	{
		base_warn(ctx, "encryption password key too short (%d)", pdf_to_str_len(obj));
		memcpy(crypt->u, pdf_to_str_buf(obj), pdf_to_str_len(obj));
	}
	else
	{
		pdf_free_crypt(ctx, crypt);
		base_throw(ctx, "encryption dictionary missing user password");
	}

	obj = pdf_dict_gets(dict, "P");
	if (pdf_is_int(obj))
		crypt->p = pdf_to_int(obj);
	else
	{
		base_warn(ctx, "encryption dictionary missing permissions");
		crypt->p = 0xfffffffc;
	}

	if (crypt->r == 5)
	{
		obj = pdf_dict_gets(dict, "OE");
		if (!pdf_is_string(obj) || pdf_to_str_len(obj) != 32)
		{
			pdf_free_crypt(ctx, crypt);
			base_throw(ctx, "encryption dictionary missing owner encryption key");
		}
		memcpy(crypt->oe, pdf_to_str_buf(obj), 32);

		obj = pdf_dict_gets(dict, "UE");
		if (!pdf_is_string(obj) || pdf_to_str_len(obj) != 32)
		{
			pdf_free_crypt(ctx, crypt);
			base_throw(ctx, "encryption dictionary missing user encryption key");
		}
		memcpy(crypt->ue, pdf_to_str_buf(obj), 32);
	}

	crypt->encrypt_metadata = 1;
	obj = pdf_dict_gets(dict, "EncryptMetadata");
	if (pdf_is_bool(obj))
		crypt->encrypt_metadata = pdf_to_bool(obj);

	

	if (pdf_is_array(id) && pdf_array_len(id) == 2)
	{
		obj = pdf_array_get(id, 0);
		if (pdf_is_string(obj))
			crypt->id = pdf_keep_obj(obj);
	}
	else
		base_warn(ctx, "missing file identifier, may not be able to do decryption");

	return crypt;
}

void
pdf_free_crypt(base_context *ctx, pdf_crypt *crypt)
{
	if (crypt->id) pdf_drop_obj(crypt->id);
	if (crypt->cf) pdf_drop_obj(crypt->cf);
	base_free(ctx, crypt);
}

static void
pdf_parse_crypt_filter(base_context *ctx, pdf_crypt_filter *cf, pdf_obj *cf_obj, char *name, int defaultlength)
{
	pdf_obj *obj;
	pdf_obj *dict;
	int is_identity = (strcmp(name, "Identity") == 0);
	int is_stdcf = (!is_identity && (strcmp(name, "StdCF") == 0));

	if (!is_identity && !is_stdcf)
		base_throw(ctx, "Crypt Filter not Identity or StdCF (%d %d R)", pdf_to_num(cf_obj), pdf_to_gen(cf_obj));

	cf->method = PDF_CRYPT_NONE;
	cf->length = defaultlength;

	if (!cf_obj)
	{
		cf->method = (is_identity ? PDF_CRYPT_NONE : PDF_CRYPT_RC4);
		return;
	}

	dict = pdf_dict_gets(cf_obj, name);
	if (!pdf_is_dict(dict))
		base_throw(ctx, "cannot parse crypt filter (%d %d R)", pdf_to_num(cf_obj), pdf_to_gen(cf_obj));

	obj = pdf_dict_gets(dict, "CFM");
	if (pdf_is_name(obj))
	{
		if (!strcmp(pdf_to_name(obj), "None"))
			cf->method = PDF_CRYPT_NONE;
		else if (!strcmp(pdf_to_name(obj), "V2"))
			cf->method = PDF_CRYPT_RC4;
		else if (!strcmp(pdf_to_name(obj), "AESV2"))
			cf->method = PDF_CRYPT_AESV2;
		else if (!strcmp(pdf_to_name(obj), "AESV3"))
			cf->method = PDF_CRYPT_AESV3;
		else
			base_warn(ctx, "unknown encryption method: %s", pdf_to_name(obj));
	}

	obj = pdf_dict_gets(dict, "Length");
	if (pdf_is_int(obj))
		cf->length = pdf_to_int(obj);

	
	if (cf->length < 40)
		cf->length = cf->length * 8;

	if ((cf->length % 8) != 0)
		base_throw(ctx, "invalid key length: %d", cf->length);
}

static const unsigned char padding[32] =
{
	0x28, 0xbf, 0x4e, 0x5e, 0x4e, 0x75, 0x8a, 0x41,
	0x64, 0x00, 0x4e, 0x56, 0xff, 0xfa, 0x01, 0x08,
	0x2e, 0x2e, 0x00, 0xb6, 0xd0, 0x68, 0x3e, 0x80,
	0x2f, 0x0c, 0xa9, 0xfe, 0x64, 0x53, 0x69, 0x7a
};

static void
pdf_compute_encryption_key(pdf_crypt *crypt, unsigned char *password, int pwlen, unsigned char *key)
{
	unsigned char buf[32];
	unsigned int p;
	int i, n;
	base_md5 md5;

	n = crypt->length / 8;

	
	if (pwlen > 32)
		pwlen = 32;
	memcpy(buf, password, pwlen);
	memcpy(buf + pwlen, padding, 32 - pwlen);

	
	base_md5_init(&md5);
	base_md5_update(&md5, buf, 32);

	
	base_md5_update(&md5, crypt->o, 32);

	
	p = (unsigned int) crypt->p;
	buf[0] = (p) & 0xFF;
	buf[1] = (p >> 8) & 0xFF;
	buf[2] = (p >> 16) & 0xFF;
	buf[3] = (p >> 24) & 0xFF;
	base_md5_update(&md5, buf, 4);

	
	base_md5_update(&md5, (unsigned char *)pdf_to_str_buf(crypt->id), pdf_to_str_len(crypt->id));

	
	if (crypt->r >= 4)
	{
		if (!crypt->encrypt_metadata)
		{
			buf[0] = 0xFF;
			buf[1] = 0xFF;
			buf[2] = 0xFF;
			buf[3] = 0xFF;
			base_md5_update(&md5, buf, 4);
		}
	}

	
	base_md5_final(&md5, buf);

	
	if (crypt->r >= 3)
	{
		for (i = 0; i < 50; i++)
		{
			base_md5_init(&md5);
			base_md5_update(&md5, buf, n);
			base_md5_final(&md5, buf);
		}
	}

	
	memcpy(key, buf, n);
}

static void
pdf_compute_encryption_key_r5(pdf_crypt *crypt, unsigned char *password, int pwlen, int ownerkey, unsigned char *validationkey)
{
	unsigned char buffer[128 + 8 + 48];
	base_sha256 sha256;
	base_aes aes;

	

	if (pwlen > 127)
		pwlen = 127;

	

	memcpy(buffer, password, pwlen);
	if (ownerkey)
	{
		memcpy(buffer + pwlen, crypt->o + 32, 8);
		memcpy(buffer + pwlen + 8, crypt->u, 48);
	}
	else
		memcpy(buffer + pwlen, crypt->u + 32, 8);

	base_sha256_init(&sha256);
	base_sha256_update(&sha256, buffer, pwlen + 8 + (ownerkey ? 48 : 0));
	base_sha256_final(&sha256, validationkey);

	

	memcpy(buffer + pwlen, crypt->u + 40, 8);

	base_sha256_init(&sha256);
	base_sha256_update(&sha256, buffer, pwlen + 8);
	base_sha256_final(&sha256, buffer);

	
	memset(buffer + 32, 0, sizeof(buffer) - 32);
	aes_setkey_dec(&aes, buffer, crypt->length);
	aes_crypt_cbc(&aes, AES_DECRYPT, 32, buffer + 32, ownerkey ? crypt->oe : crypt->ue, crypt->key);
}

static void
pdf_compute_user_password(pdf_crypt *crypt, unsigned char *password, int pwlen, unsigned char *output)
{
	if (crypt->r == 2)
	{
		base_arc4 arc4;

		pdf_compute_encryption_key(crypt, password, pwlen, crypt->key);
		base_arc4_init(&arc4, crypt->key, crypt->length / 8);
		base_arc4_encrypt(&arc4, output, padding, 32);
	}

	if (crypt->r == 3 || crypt->r == 4)
	{
		unsigned char xor[32];
		unsigned char digest[16];
		base_md5 md5;
		base_arc4 arc4;
		int i, x, n;

		n = crypt->length / 8;

		pdf_compute_encryption_key(crypt, password, pwlen, crypt->key);

		base_md5_init(&md5);
		base_md5_update(&md5, padding, 32);
		base_md5_update(&md5, (unsigned char*)pdf_to_str_buf(crypt->id), pdf_to_str_len(crypt->id));
		base_md5_final(&md5, digest);

		base_arc4_init(&arc4, crypt->key, n);
		base_arc4_encrypt(&arc4, output, digest, 16);

		for (x = 1; x <= 19; x++)
		{
			for (i = 0; i < n; i++)
				xor[i] = crypt->key[i] ^ x;
			base_arc4_init(&arc4, xor, n);
			base_arc4_encrypt(&arc4, output, output, 16);
		}

		memcpy(output + 16, padding, 16);
	}

	if (crypt->r == 5)
	{
		pdf_compute_encryption_key_r5(crypt, password, pwlen, 0, output);
	}
}

static int
pdf_authenticate_user_password(pdf_crypt *crypt, unsigned char *password, int pwlen)
{
	unsigned char output[32];
	pdf_compute_user_password(crypt, password, pwlen, output);
	if (crypt->r == 2 || crypt->r == 5)
		return memcmp(output, crypt->u, 32) == 0;
	if (crypt->r == 3 || crypt->r == 4)
		return memcmp(output, crypt->u, 16) == 0;
	return 0;
}

static int
pdf_authenticate_owner_password(pdf_crypt *crypt, unsigned char *ownerpass, int pwlen)
{
	unsigned char pwbuf[32];
	unsigned char key[32];
	unsigned char xor[32];
	unsigned char userpass[32];
	int i, n, x;
	base_md5 md5;
	base_arc4 arc4;

	if (crypt->r == 5)
	{
		

		pdf_compute_encryption_key_r5(crypt, ownerpass, pwlen, 1, key);

		return !memcmp(key, crypt->o, 32);
	}

	n = crypt->length / 8;

	

	
	if (pwlen > 32)
		pwlen = 32;
	memcpy(pwbuf, ownerpass, pwlen);
	memcpy(pwbuf + pwlen, padding, 32 - pwlen);

	
	base_md5_init(&md5);
	base_md5_update(&md5, pwbuf, 32);
	base_md5_final(&md5, key);

	
	if (crypt->r >= 3)
	{
		for (i = 0; i < 50; i++)
		{
			base_md5_init(&md5);
			base_md5_update(&md5, key, 16);
			base_md5_final(&md5, key);
		}
	}

	
	if (crypt->r == 2)
	{
		base_arc4_init(&arc4, key, n);
		base_arc4_encrypt(&arc4, userpass, crypt->o, 32);
	}

	
	if (crypt->r >= 3)
	{
		memcpy(userpass, crypt->o, 32);
		for (x = 0; x < 20; x++)
		{
			for (i = 0; i < n; i++)
				xor[i] = key[i] ^ (19 - x);
			base_arc4_init(&arc4, xor, n);
			base_arc4_encrypt(&arc4, userpass, userpass, 32);
		}
	}

	return pdf_authenticate_user_password(crypt, userpass, 32);
}

int
pdf_authenticate_password(pdf_document *xref, char *password)
{
	if (xref->crypt)
	{
		if (!password)
			password = "";
		if (pdf_authenticate_user_password(xref->crypt, (unsigned char *)password, strlen(password)))
			return 1;
		if (pdf_authenticate_owner_password(xref->crypt, (unsigned char *)password, strlen(password)))
			return 1;
		return 0;
	}
	return 1;
}

int
pdf_needs_password(pdf_document *xref)
{
	if (!xref->crypt)
		return 0;
	if (pdf_authenticate_password(xref, ""))
		return 0;
	return 1;
}

int
pdf_has_permission(pdf_document *xref, int p)
{
	if (!xref->crypt)
		return 1;
	return xref->crypt->p & p;
}

unsigned char *
pdf_crypt_key(pdf_document *xref)
{
	if (xref->crypt)
		return xref->crypt->key;
	return NULL;
}

int
pdf_crypt_revision(pdf_document *xref)
{
	if (xref->crypt)
		return xref->crypt->v;
	return 0;
}

char *
pdf_crypt_method(pdf_document *xref)
{
	if (xref->crypt)
	{
		switch (xref->crypt->strf.method)
		{
		case PDF_CRYPT_NONE: return "None";
		case PDF_CRYPT_RC4: return "RC4";
		case PDF_CRYPT_AESV2: return "AES";
		case PDF_CRYPT_AESV3: return "AES";
		case PDF_CRYPT_UNKNOWN: return "Unknown";
		}
	}
	return "None";
}

int
pdf_crypt_length(pdf_document *xref)
{
	if (xref->crypt)
		return xref->crypt->length;
	return 0;
}

static int
pdf_compute_object_key(pdf_crypt *crypt, pdf_crypt_filter *cf, int num, int gen, unsigned char *key)
{
	base_md5 md5;
	unsigned char message[5];

	if (cf->method == PDF_CRYPT_AESV3)
	{
		memcpy(key, crypt->key, crypt->length / 8);
		return crypt->length / 8;
	}

	base_md5_init(&md5);
	base_md5_update(&md5, crypt->key, crypt->length / 8);
	message[0] = (num) & 0xFF;
	message[1] = (num >> 8) & 0xFF;
	message[2] = (num >> 16) & 0xFF;
	message[3] = (gen) & 0xFF;
	message[4] = (gen >> 8) & 0xFF;
	base_md5_update(&md5, message, 5);

	if (cf->method == PDF_CRYPT_AESV2)
		base_md5_update(&md5, (unsigned char *)"sAlT", 4);

	base_md5_final(&md5, key);

	if (crypt->length / 8 + 5 > 16)
		return 16;
	return crypt->length / 8 + 5;
}

static void
pdf_crypt_obj_imp(base_context *ctx, pdf_crypt *crypt, pdf_obj *obj, unsigned char *key, int keylen)
{
	unsigned char *s;
	int i, n;

	if (pdf_is_indirect(obj))
		return;

	if (pdf_is_string(obj))
	{
		s = (unsigned char *)pdf_to_str_buf(obj);
		n = pdf_to_str_len(obj);

		if (crypt->strf.method == PDF_CRYPT_RC4)
		{
			base_arc4 arc4;
			base_arc4_init(&arc4, key, keylen);
			base_arc4_encrypt(&arc4, s, s, n);
		}

		if (crypt->strf.method == PDF_CRYPT_AESV2 || crypt->strf.method == PDF_CRYPT_AESV3)
		{
			if (n == 0)
			{
				
			}
			else if (n & 15 || n < 32)
				base_warn(ctx, "invalid string length for aes encryption");
			else
			{
				unsigned char iv[16];
				base_aes aes;
				memcpy(iv, s, 16);
				aes_setkey_dec(&aes, key, keylen * 8);
				aes_crypt_cbc(&aes, AES_DECRYPT, n - 16, iv, s + 16, s);
				
				if (s[n - 17] < 1 || s[n - 17] > 16)
					base_warn(ctx, "aes padding out of range");
				else
					pdf_set_str_len(obj, n - 16 - s[n - 17]);
			}
		}
	}

	else if (pdf_is_array(obj))
	{
		n = pdf_array_len(obj);
		for (i = 0; i < n; i++)
		{
			pdf_crypt_obj_imp(ctx, crypt, pdf_array_get(obj, i), key, keylen);
		}
	}

	else if (pdf_is_dict(obj))
	{
		n = pdf_dict_len(obj);
		for (i = 0; i < n; i++)
		{
			pdf_crypt_obj_imp(ctx, crypt, pdf_dict_get_val(obj, i), key, keylen);
		}
	}
}

void
pdf_crypt_obj(base_context *ctx, pdf_crypt *crypt, pdf_obj *obj, int num, int gen)
{
	unsigned char key[32];
	int len;

	len = pdf_compute_object_key(crypt, &crypt->strf, num, gen, key);

	pdf_crypt_obj_imp(ctx, crypt, obj, key, len);
}

static base_stream *
pdf_open_crypt_imp(base_stream *chain, pdf_crypt *crypt, pdf_crypt_filter *stmf, int num, int gen)
{
	unsigned char key[32];
	int len;

	crypt->ctx = chain->ctx;
	len = pdf_compute_object_key(crypt, stmf, num, gen, key);

	if (stmf->method == PDF_CRYPT_RC4)
		return base_open_arc4(chain, key, len);

	if (stmf->method == PDF_CRYPT_AESV2 || stmf->method == PDF_CRYPT_AESV3)
		return base_open_aesd(chain, key, len);

	return base_open_copy(chain);
}

base_stream *
pdf_open_crypt(base_stream *chain, pdf_crypt *crypt, int num, int gen)
{
	return pdf_open_crypt_imp(chain, crypt, &crypt->stmf, num, gen);
}

base_stream *
pdf_open_crypt_with_filter(base_stream *chain, pdf_crypt *crypt, char *name, int num, int gen)
{
	if (strcmp(name, "Identity"))
	{
		pdf_crypt_filter cf;
		pdf_parse_crypt_filter(chain->ctx, &cf, crypt->cf, name, crypt->length);
		return pdf_open_crypt_imp(chain, crypt, &cf, num, gen);
	}
	return chain;
}

void pdf_print_crypt(pdf_crypt *crypt)
{
	int i;

	printf("crypt {\n");

	printf("\tv=%d length=%d\n", crypt->v, crypt->length);
	printf("\tstmf method=%d length=%d\n", crypt->stmf.method, crypt->stmf.length);
	printf("\tstrf method=%d length=%d\n", crypt->strf.method, crypt->strf.length);
	printf("\tr=%d\n", crypt->r);

	printf("\to=<");
	for (i = 0; i < 32; i++)
		printf("%02X", crypt->o[i]);
	printf(">\n");

	printf("\tu=<");
	for (i = 0; i < 32; i++)
		printf("%02X", crypt->u[i]);
	printf(">\n");

	printf("}\n");
}
