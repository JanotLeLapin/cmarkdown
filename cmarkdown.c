#include "cmarkdown.h"

#define HAS_FLAG(ctx, flag) ((ctx->flags & flag) == flag)

typedef enum {
  FLAG_ANCHOR_TEXT = 1 << 0,
  FLAG_ANCHOR_LINK = 1 << 1,
} flags_t;

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

static inline char
is_anchor(const cmark_ctx_t *ctx)
{
  size_t i = ctx->i;

  if ('[' != ctx->src[i++]) {
    return 0;
  }

  while (']' != ctx->src[i]) {
    if (i >= ctx->len || '\n' == ctx->src[i]) {
      return 0;
    }
    i++;
  }

  i++;
  if ('(' != ctx->src[i++]) {
    return 0;
  }

  while (')' != ctx->src[i]) {
    if (i >= ctx->len || '\n' == ctx->src[i]) {
      return 0;
    }
    i++;
  }

  return 1;
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
    case '[':
      if (!HAS_FLAG(ctx, FLAG_ANCHOR_TEXT) && is_anchor(ctx)) {
        ctx->flags |= FLAG_ANCHOR_TEXT;
        str.len = ctx->src + ctx->i - str.p;
        return str;
      } else {
        ctx->i++;
        break;
      }
    case ']':
      if (HAS_FLAG(ctx, FLAG_ANCHOR_TEXT)) {
        ctx->flags = (ctx->flags & ~FLAG_ANCHOR_TEXT) | FLAG_ANCHOR_LINK;
        str.len = ctx->src + ctx->i++ - str.p;
        return str;
      } else {
        ctx->i++;
        break;
      }
    default:
      ctx->i++;
      break;
    }
  }
}

static inline cmark_elem_anchor_link_data_t
parse_anchor_link(cmark_ctx_t *ctx)
{
  cmark_str_t str = { .p = ctx->src + ++ctx->i };

  while (')' != ctx->src[ctx->i]) {
    ctx->i++;
  }

  str.len = ctx->src + ctx->i++ - str.p;
  return str;
}

cmark_elem_t
cmark_next(cmark_ctx_t *ctx)
{
  cmark_elem_t e;

  if (ctx->i >= ctx->len) {
    e.type = CMARK_ELEM_EOF;
    return e;
  }

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
  case '[':
    if (HAS_FLAG(ctx, FLAG_ANCHOR_TEXT) || is_anchor(ctx)) {
      ctx->i++;
      e.type = CMARK_ELEM_ANCHOR_TEXT;
      return e;
    } else {
      e.type = CMARK_ELEM_PLAIN;
      e.plain = parse_plain(ctx);
      return e;
    }
  case '(':
    if (HAS_FLAG(ctx, FLAG_ANCHOR_LINK)) {
      ctx->flags &= ~FLAG_ANCHOR_LINK;
      e.type = CMARK_ELEM_ANCHOR_LINK;
      e.anchor_link = parse_anchor_link(ctx);
      return e;
    } else {
      e.type = CMARK_ELEM_PLAIN;
      e.plain = parse_plain(ctx);
      return e;
    }
  default:
    e.type = CMARK_ELEM_PLAIN;
    e.plain = parse_plain(ctx);
    return e;
  }

  return (cmark_elem_t) { .type = CMARK_ELEM_EOF };
}
