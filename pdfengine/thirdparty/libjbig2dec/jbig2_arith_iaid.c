

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "os_types.h"

#include <stddef.h>
#include <string.h> 

#ifdef VERBOSE
#include <stdio.h> 
#endif

#include "jbig2.h"
#include "jbig2_priv.h"
#include "jbig2_arith.h"
#include "jbig2_arith_iaid.h"

struct _Jbig2ArithIaidCtx {
  int SBSYMCODELEN;
  Jbig2ArithCx *IAIDx;
};

Jbig2ArithIaidCtx *
jbig2_arith_iaid_ctx_new(Jbig2Ctx *ctx, int SBSYMCODELEN)
{
  Jbig2ArithIaidCtx *result = jbig2_new(ctx, Jbig2ArithIaidCtx, 1);
  int ctx_size = 1 << SBSYMCODELEN;

  if (result == NULL)
  {
      jbig2_error(ctx, JBIG2_SEVERITY_FATAL, -1,
          "failed to allocate storage in jbig2_arith_iaid_ctx_new");
      return result;
  }

  result->SBSYMCODELEN = SBSYMCODELEN;
  result->IAIDx = jbig2_new(ctx, Jbig2ArithCx, ctx_size);
  if (result->IAIDx != NULL)
  {
      memset(result->IAIDx, 0, ctx_size);
  }
  else
  {
      jbig2_error(ctx, JBIG2_SEVERITY_FATAL, -1,
          "failed to allocate symbol ID storage in jbig2_arith_iaid_ctx_new");
  }

  return result;
}

int
jbig2_arith_iaid_decode(Jbig2ArithIaidCtx *ctx, Jbig2ArithState *as,
		       int32_t *p_result)
{
  Jbig2ArithCx *IAIDx = ctx->IAIDx;
  int SBSYMCODELEN = ctx->SBSYMCODELEN;
  int PREV = 1;
  int D;
  int i;

  
  for (i = 0; i < SBSYMCODELEN; i++)
    {
      D = jbig2_arith_decode(as, &IAIDx[PREV]);
#ifdef VERBOSE
      fprintf(stderr, "IAID%x: D = %d\n", PREV, D);
#endif
      PREV = (PREV << 1) | D;
    }
  
  PREV -= 1 << SBSYMCODELEN;
#ifdef VERBOSE
  fprintf(stderr, "IAID result: %d\n", PREV);
#endif
  *p_result = PREV;
  return 0;
}

void
jbig2_arith_iaid_ctx_free(Jbig2Ctx *ctx, Jbig2ArithIaidCtx *iax)
{
    if (iax != NULL)
    {
        jbig2_free(ctx->allocator, iax->IAIDx);
        jbig2_free(ctx->allocator, iax);
    }
}
