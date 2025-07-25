#include "cmarkdown.h"

cmark_elem_t
cmark_next(cmark_ctx_t *ctx)
{
  return (cmark_elem_t) { .type = CMARK_ELEM_EOF };
}
