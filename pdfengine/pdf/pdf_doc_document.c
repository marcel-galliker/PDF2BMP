#include "pdf-internal.h"

extern struct pdf_document *pdf_open_document(base_context *ctx, wchar_t *filename);

static inline int base_tolower(int c)
{
	if (c >= 'A' && c <= 'Z')
		return c + 32;
	return c;
}

static inline int base_strcasecmp(char *a, char *b)
{
	while (base_tolower(*a) == base_tolower(*b))
	{
		if (*a++ == 0)
			return 0;
		b++;
	}
	return base_tolower(*a) - base_tolower(*b);
}

/***************************************************************************************/
/* function name:	base_open_document
/* description:		open the pdf document
/* param 1:			pointer to the context
/* param 2:			filename to open
/* return:			pointer to opened document
/* creator & modifier : 
/* create:			2012.09.10 ~
/* working:			2012.09.10 ~
/***************************************************************************************/

base_document *
base_open_document(base_context *ctx, wchar_t *filename)
{
	return (base_document*) pdf_open_document(ctx, filename);
}

/***************************************************************************************/
/* function name:	base_close_document
/* description:		close the opened document
/* param 1:			pdf document to close
/* return:			none
/* creator & modifier : 
/* create:			2012.09.10 ~
/* working:			2012.09.10 ~
/***************************************************************************************/

void
base_close_document(base_document *doc)
{
	if (doc && doc->close)
		doc->close(doc);
}

/***************************************************************************************/
/* function name:	base_needs_password
/* description:		check whether pdf document is password-protected
/* param 1:			pointer to pdf document
/* return:			0, if it is not password-protected.
					positive integer, otherwise.
/* creator & modifier : 
/* create:			2012.09.10 ~
/* working:			2012.09.10 ~
/***************************************************************************************/

int
base_needs_password(base_document *doc)
{
	if (doc && doc->needs_password)
		return doc->needs_password(doc);
	return 0;
}

/***************************************************************************************/
/* function name:	base_authenticate_password
/* description:		authenticate pdf document that is password-protected
/* param 1:			pdf document to authenticate
/* param 2:			password
/* return:			0, if authenticate fails.
					positive integer, otherwise.
/* creator & modifier : 
/* create:			2012.09.10 ~
/* working:			2012.09.10 ~
/***************************************************************************************/

int
base_authenticate_password(base_document *doc)
{
	if (doc && doc->authenticate_password)
		return doc->authenticate_password(doc, "");
	return 1;
}

base_outline *
base_load_outline(base_document *doc)
{
	if (doc && doc->load_outline)
		return doc->load_outline(doc);
	return NULL;
}

/***************************************************************************************/
/* function name:	base_count_pages
/* description:		get the page count of the pdf document
/* param 1:			pointer to input document
/* return:			none
/* creator & modifier : 
/* create:			2012.09.10 ~
/* working:			2012.09.10 ~
/***************************************************************************************/

int
base_count_pages(base_document *doc)
{
	if (doc && doc->count_pages)
		return doc->count_pages(doc);
	return 0;
}

/***************************************************************************************/
/* function name:	base_load_page
/* description:		load one page of the opened pdf document
/* param 1:			pdf document to load
/* param 2:			page number to load
/* return:			pointer to the loaded page
/* creator & modifier : 
/* create:			2012.09.10 ~
/* working:			2012.09.10 ~
/***************************************************************************************/

base_page *
base_load_page(base_document *doc, int number)
{
	if (doc && doc->load_page)
		return doc->load_page(doc, number);
	return NULL;
}

base_link *
base_load_links(base_document *doc, base_page *page)
{
	if (doc && doc->load_links && page)
		return doc->load_links(doc, page);
	return NULL;
}

/***************************************************************************************/
/* function name:	base_bound_page
/* description:		get the bound area of opened pdf document
/* param 1:			pointer to the pdf document
/* param 2:			pointer to the page
/* return:			bound rect of the page
/* creator & modifier : 
/* create:			2012.09.10 ~
/* working:			2012.09.10 ~
/***************************************************************************************/

base_rect
base_bound_page(base_document *doc, base_page *page)
{
	if (doc && doc->bound_page && page)
		return doc->bound_page(doc, page);
	return base_empty_rect;
}

/***************************************************************************************/
/* function name:	base_run_page
/* description:		get the contents of the page into the device
/* param 1:			pointer to the pdf document
/* param 2:			pointer to the page
/* param 3:			pointer to the device
/* param 4:			transform matrix
/* param 5:			pointer to the cookie
/* return:			none
/* creator & modifier : 
/* create:			2012.09.10 ~
/* working:			2012.09.10 ~
/***************************************************************************************/

void
base_run_page(base_document *doc, base_page *page, base_device *dev, base_matrix transform, base_cookie *cookie)
{
	if (doc && doc->run_page && page)
		doc->run_page(doc, page, dev, transform, cookie);
}

/***************************************************************************************/
/* function name:	base_free_page
/* description:		free the page
/* param 1:			pointer to the pdf document
/* param 2:			pointer to the page
/* return:			none
/* creator & modifier : 
/* create:			2012.09.10 ~
/* working:			2012.09.10 ~
/***************************************************************************************/

void
base_free_page(base_document *doc, base_page *page)
{
	if (doc && doc->free_page && page)
		doc->free_page(doc, page);
}

int
base_meta(base_document *doc, int key, void *ptr, int size)
{
	if (doc && doc->meta)
		return doc->meta(doc, key, ptr, size);
	return base_META_UNKNOWN_KEY;
}
