#ifndef _CMARKDOWN_H
#define _CMARKDOWN_H

#include <stddef.h>

typedef struct {
  const char *src;
  size_t len;
  size_t i;
  unsigned int flags;
} cmark_ctx_t;

typedef struct {
  const char *p;
  size_t len;
} cmark_str_t;

typedef int cmark_elem_heading_data_t;
typedef char cmark_elem_list_start_data_t;
typedef cmark_str_t cmark_elem_plain_data_t;
typedef cmark_str_t cmark_elem_anchor_link_data_t;

typedef struct {
  enum {
    CMARK_ELEM_HEADING,
    CMARK_ELEM_LIST_START,
    CMARK_ELEM_LIST_ITEM,
    CMARK_ELEM_LIST_END,

    CMARK_ELEM_PLAIN,
    CMARK_ELEM_ANCHOR_TEXT,
    CMARK_ELEM_ANCHOR_LINK,
    CMARK_ELEM_BREAK,
    CMARK_ELEM_EOF,
  } type;
  union {
    cmark_elem_heading_data_t heading;
    cmark_elem_list_start_data_t list_start;
    cmark_elem_plain_data_t plain;
    cmark_elem_anchor_link_data_t anchor_link;
  };
} cmark_elem_t;

cmark_elem_t cmark_next(cmark_ctx_t *ctx);

#endif
