#include "cmarkdown.h"

#define HAS_FLAG(ctx, flag) ((ctx->flags & flag) == flag)

#define MASK_FLAG_LIST (FLAG_LIST_ASTERISK | FLAG_LIST_DASH)

typedef enum {
  FLAG_BEGIN_LINE = 1 << 0,
  FLAG_ANCHOR_TEXT = 1 << 1,
  FLAG_ANCHOR_LINK = 1 << 2,
  FLAG_LIST_ASTERISK = 1 << 3,
  FLAG_LIST_DASH = 1 << 4,
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

static inline flags_t
list_flag_from_char(char c)
{
  switch (c) {
  case '*':
    return FLAG_LIST_ASTERISK;
  case '-':
    return FLAG_LIST_DASH;
  default:
    return 0;
  }
}

static inline char
list_char_from_flag(flags_t flag)
{
  switch (flag) {
  case FLAG_LIST_ASTERISK:
    return '*';
  case FLAG_LIST_DASH:
    return '-';
  default:
    return '\0';
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

static inline char
parse_break(cmark_ctx_t *ctx)
{
  size_t start = ctx->i;

  while ('\n' == ctx->src[ctx->i] && ctx->i < ctx->len) {
    ctx->i++;
  }

  return ctx->i - start > 1;
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
    ctx->flags |= FLAG_BEGIN_LINE;
    if (parse_break(ctx)) {
      e.type = CMARK_ELEM_BREAK;
      return e;
    }
    break;
  default:
    break;
  }

  if (HAS_FLAG(ctx, FLAG_BEGIN_LINE)) {
    if (0 != (ctx->flags & MASK_FLAG_LIST) && ctx->src[ctx->i] != list_char_from_flag(ctx->flags & MASK_FLAG_LIST)) {
      ctx->flags &= ~MASK_FLAG_LIST;
      e.type = CMARK_ELEM_LIST_END;
      return e;
    }

    ctx->flags &= ~FLAG_BEGIN_LINE;

    switch (ctx->src[ctx->i]) {
    case '#':
      e.type = CMARK_ELEM_HEADING;
      e.heading = parse_heading(ctx);
      return e;
    case '*':
    case '-':
      if (0 != (ctx->flags & MASK_FLAG_LIST)) {
        ctx->i++;
        e.type = CMARK_ELEM_LIST_ITEM;
      } else {
        ctx->flags = ctx->flags | FLAG_BEGIN_LINE | list_flag_from_char(ctx->src[ctx->i]);
        e.type = CMARK_ELEM_LIST_START;
        e.list_start =  ctx->src[ctx->i];
      }
      return e;
    default:
      break;
    }
  }

  switch (ctx->src[ctx->i]) {
  case '[':
    if (HAS_FLAG(ctx, FLAG_ANCHOR_TEXT) || is_anchor(ctx)) {
      ctx->i++;
      e.type = CMARK_ELEM_ANCHOR_TEXT;
    } else {
      e.type = CMARK_ELEM_PLAIN;
      e.plain = parse_plain(ctx);
    }
    break;
  case '(':
    if (HAS_FLAG(ctx, FLAG_ANCHOR_LINK)) {
      ctx->flags &= ~FLAG_ANCHOR_LINK;
      e.type = CMARK_ELEM_ANCHOR_LINK;
      e.anchor_link = parse_anchor_link(ctx);
    } else {
      e.type = CMARK_ELEM_PLAIN;
      e.plain = parse_plain(ctx);
    }
    break;
  default:
    e.type = CMARK_ELEM_PLAIN;
    e.plain = parse_plain(ctx);
    break;
  }

  ctx->flags &= ~FLAG_BEGIN_LINE;

  return e;
}
