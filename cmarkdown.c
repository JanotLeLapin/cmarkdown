#include "cmarkdown.h"

#include <string.h>

#define HAS_FLAG(ctx, flag) ((ctx->flags & flag) == flag)

#define MASK_FLAG_LIST (FLAG_LIST_ASTERISK | FLAG_LIST_DASH)

typedef enum {
  FLAG_BEGIN_LINE = 1 << 0,
  FLAG_ANCHOR = 1 << 1,
  FLAG_LIST_ASTERISK = 1 << 2,
  FLAG_LIST_DASH = 1 << 3,
  FLAG_CODE_INLINE = 1 << 4,
  FLAG_CODE_MULTILINE = 1 << 5,
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

static inline char
is_code_inline(const cmark_ctx_t *ctx)
{
  size_t i = ctx->i;

  if ('`' != ctx->src[i++]) {
    return 0;
  }

  while ('`' != ctx->src[i]) {
    if (i >= ctx->len) {
      return 0;
    }
    i++;
  }

  return 1;
}

static inline char
is_code_multiline(const cmark_ctx_t *ctx)
{
  size_t i = ctx->i;

  if (strncmp("```", ctx->src + i, 3)) {
    return 0;
  }

  i += 3;
  while (i <= ctx->len - 3) {
    if ('`' == ctx->src[i] && !strncmp("```", ctx->src + i, 3)) {
      return 1;
    }
    i++;
  }

  return 0;
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
      if (!HAS_FLAG(ctx, FLAG_ANCHOR) && is_anchor(ctx)) {
        ctx->flags |= FLAG_ANCHOR;
        str.len = ctx->src + ctx->i - str.p;
        return str;
      } else {
        ctx->i++;
        break;
      }
    case ']':
      if (HAS_FLAG(ctx, FLAG_ANCHOR)) {
        str.len = ctx->src + ctx->i - str.p;
        return str;
      } else {
        ctx->i++;
        break;
      }
    case '`':
      if (is_code_multiline(ctx)) {
        ctx->flags |= FLAG_CODE_MULTILINE;
        str.len = ctx->src + ctx->i - str.p;
        return str;
      }
      if (is_code_inline(ctx)) {
        ctx->flags |= FLAG_CODE_INLINE;
        str.len = ctx->src + ctx->i - str.p;
        return str;
      }
      ctx->i++;
      break;
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

static inline cmark_elem_code_inline_data_t
parse_code_inline(cmark_ctx_t *ctx)
{
  cmark_str_t str = { .p = ctx->src + ++ctx->i };

  while ('`' != ctx->src[ctx->i]) {
    ctx->i++;
  }

  str.len = ctx->src + ctx->i++ - str.p;
  return str;
}

static inline cmark_elem_code_multiline_data_t
parse_code_multiline(cmark_ctx_t *ctx)
{
  cmark_elem_code_multiline_data_t data;

  ctx->i += 3;
  if ('\n' != ctx->src[ctx->i]) {
    data.lang.p = ctx->src + ctx->i;
    while ('\n' != ctx->src[ctx->i]) {
      ctx->i++;
    }
    data.lang.len = ctx->src + ctx->i - data.lang.p;
  } else {
    data.lang.p = 0;
    data.lang.len = 0;
  }

  data.content.p = ctx->src + ++ctx->i;
  while ('`' != ctx->src[ctx->i] || strncmp("```", ctx->src + ctx->i, 3)) {
    ctx->i++;
  }

  data.content.len = ctx->src + ctx->i - data.content.p;
  ctx->i += 3;
  return data;
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
      if (0 != (ctx->flags & MASK_FLAG_LIST)) {
        ctx->flags &= ~MASK_FLAG_LIST;
        e.type = CMARK_ELEM_LIST_END;
      } else {
        e.type = CMARK_ELEM_BREAK;
      }
      return e;
    }
    break;
  default:
    break;
  }

  if (ctx->i >= ctx->len) {
    e.type = CMARK_ELEM_EOF;
    return e;
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
    if (HAS_FLAG(ctx, FLAG_ANCHOR) || is_anchor(ctx)) {
      ctx->i++;
      e.type = CMARK_ELEM_ANCHOR_TEXT;
    } else {
      e.type = CMARK_ELEM_PLAIN;
      e.plain = parse_plain(ctx);
    }
    break;
  case ']':
    if (HAS_FLAG(ctx, FLAG_ANCHOR)) {
      ctx->i++;
      ctx->flags &= ~FLAG_ANCHOR;
      e.type = CMARK_ELEM_ANCHOR_LINK;
      e.anchor_link = parse_anchor_link(ctx);
    } else {
      e.type = CMARK_ELEM_PLAIN;
      e.plain = parse_plain(ctx);
    }
    break;
  case '`':
    if (HAS_FLAG(ctx, FLAG_CODE_MULTILINE) || is_code_multiline(ctx)) {
      ctx->flags &= ~FLAG_CODE_MULTILINE;
      e.type = CMARK_ELEM_CODE_MULTILINE;
      e.code_multiline = parse_code_multiline(ctx);
    } else if (HAS_FLAG(ctx, FLAG_CODE_INLINE) || is_code_inline(ctx)) {
      ctx->flags &= ~FLAG_CODE_INLINE;
      e.type = CMARK_ELEM_CODE_INLINE;
      e.code_inline = parse_code_inline(ctx);
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
