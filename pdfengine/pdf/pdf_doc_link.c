#include "pdf-internal.h"

void
base_free_link_dest(base_context *ctx, base_link_dest *dest)
{
	switch(dest->kind)
	{
	case base_LINK_NONE:
	case base_LINK_GOTO:
		break;
	case base_LINK_URI:
		base_free(ctx, dest->ld.uri.uri);
		break;
	case base_LINK_LAUNCH:
		base_free(ctx, dest->ld.launch.file_spec);
		break;
	case base_LINK_NAMED:
		base_free(ctx, dest->ld.named.named);
		break;
	case base_LINK_GOTOR:
		base_free(ctx, dest->ld.gotor.file_spec);
		break;
	}
}

base_link *
base_new_link(base_context *ctx, base_rect bbox, base_link_dest dest)
{
	base_link *link;

	base_try(ctx)
	{
		link = base_malloc_struct(ctx, base_link);
		link->refs = 1;
	}
	base_catch(ctx)
	{
		base_free_link_dest(ctx, &dest);
		base_rethrow(ctx);
	}
	link->dest = dest;
	link->rect = bbox;
	link->next = NULL;
	return link;
}

base_link *
base_keep_link(base_context *ctx, base_link *link)
{
	if (link)
		link->refs++;
	return link;
}

void
base_drop_link(base_context *ctx, base_link *link)
{
	while (link && --link->refs == 0)
	{
		base_link *next = link->next;
		base_free_link_dest(ctx, &link->dest);
		base_free(ctx, link);
		link = next;
	}
}
