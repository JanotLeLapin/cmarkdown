#include "cmarkdown.h"

static inline void
skip_whitespace(cmark_ctx_t *ctx)
{
  while (1) {
    switch (ctx->src[ctx->i]) {
    case ' ':
    case '\n':
    case '\t':
      ctx->i++;
      break;
    default:
      return;
    }
  }
}

static inline cmark_elem_heading_data_t
parse_heading(cmark_ctx_t *ctx)
{
  size_t start = ctx->i;

  while ('#' == ctx->src[ctx->i]) {
    ctx->i++;
  }

  return (int) (ctx->i - start);
}

static inline cmark_elem_plain_data_t
parse_plain(cmark_ctx_t *ctx)
{
  cmark_str_t str = { .p = ctx->src + ctx->i };

  while (1) {
    switch (ctx->src[ctx->i]) {
    case '\n':
      str.len = ctx->src + ctx->i - str.p;
      return str;
    default:
      ctx->i++;
      break;
    }
  }
}

cmark_elem_t
cmark_next(cmark_ctx_t *ctx)
{
  cmark_elem_t e;

  switch (ctx->src[ctx->i]) {
  case ' ':
  case '\t':
    skip_whitespace(ctx);
    break;
  case '\n':
    ctx->i++;
    e.type = CMARK_ELEM_BREAK;
    return e;
  default:
    break;
  }

  switch (ctx->src[ctx->i]) {
  case '#':
    e.type = CMARK_ELEM_HEADING;
    e.heading = parse_heading(ctx);
    return e;
  default:
    e.type = CMARK_ELEM_PLAIN;
    e.plain = parse_plain(ctx);
    return e;
  }

  return (cmark_elem_t) { .type = CMARK_ELEM_EOF };
}
