#include "cmarkdown.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DATA_NULL ((union CMarkNodeData) { .null = 0 })

struct CMarkNode parse_inline(struct CMarkContext *ctx);

struct CMarkNode
create_node(enum CMarkNodeType type, union CMarkNodeData data, size_t initial_size)
{
  struct CMarkNode node, *children;
  children = initial_size ? malloc(sizeof(struct CMarkNode) * initial_size) : NULL;
  node.type = type;
  node.data = data;
  node.children_count = 0;
  node.children_size = initial_size;
  node.children = children;

  return node;
}

void
add_child(struct CMarkNode *node, struct CMarkNode child)
{
  if (!node->children) {
    node->children = malloc(sizeof(struct CMarkNode));
    node->children_size = 1;
  }

  if (node->children_count >= node->children_size) {
    node->children_size *= 2;
    node->children = realloc(node->children, sizeof(struct CMarkNode) * node->children_size);
  }

  node->children[node->children_count] = child;
  node->children_count++;
}

void
cmark_free_node(struct CMarkNode node)
{
  size_t i;

  switch (node.type) {
    case CMARK_ANCHOR:
      free(node.data.anchor.href);
      break;
    case CMARK_PLAIN:
      free(node.data.plain);
      break;
    default:
      break;
  }

  for (i = 0; i < node.children_count; i++) {
    cmark_free_node(node.children[i]);
  }
  free(node.children);
}

struct CMarkContext *
cmark_create_context(void *file)
{
  struct CMarkContext *ctx = malloc(sizeof(struct CMarkContext));
  ctx->file = file;
  return ctx;
}

void
read_line(struct CMarkContext *ctx)
{
  fgets(ctx->buffer, sizeof(ctx->buffer), ctx->file);
  ctx->i = 0;
}

struct CMarkNode
parse_plain(struct CMarkContext *ctx)
{
  union CMarkNodeData data;
  size_t start = ctx->i, len;

  while (1) {
    switch (ctx->buffer[ctx->i]) {
      case '\n':
      case '\t':
      case ' ':
      case '[':
      case ']':
        break;
      default:
        ctx->i++;
        continue;
    }
    break;
  }

  len = ctx->i - start;
  if (!len) {
    return create_node(CMARK_NULL, DATA_NULL, 0);
  }
  
  data.plain = malloc(len + 1);
  strncpy(data.plain, ctx->buffer + start, len);
  data.plain[len] = '\0';

  return create_node(CMARK_PLAIN, data, 0);
}

struct CMarkNode
parse_anchor(struct CMarkContext *ctx)
{
  struct CMarkNode node;
  union CMarkNodeData data;
  size_t start = ctx->i, href_len, href_start;
  char *href;

  node.children_size = 4;
  node.children_count = 0;
  node.children = malloc(sizeof(struct CMarkNode) * 4);
  node.type = CMARK_ANCHOR;

  ctx->i++;
  while (1) {
    add_child(&node, parse_inline(ctx));
    switch (ctx->buffer[ctx->i]) {
      case ']':
        break;
      case '\n':
        free(node.children);
        return create_node(CMARK_NULL, DATA_NULL, 0);
      default:
        continue;
    }
    break;
  }

  ctx->i++;
  if ('(' != ctx->buffer[ctx->i]) {
    free(node.children);
    return create_node(CMARK_NULL, DATA_NULL, 0);
  }

  href_start = ctx->i + 1;
  while (1) {
    ctx->i++;
    switch (ctx->buffer[ctx->i]) {
      case ')':
        break;
      case '\n':
        free(node.children);
        return create_node(CMARK_NULL, DATA_NULL, 0);
      default:
        continue;
    }
    break;
  }

  href_len = ctx->i - href_start;
  if (!href_len) {
    free(node.children);
    return create_node(CMARK_NULL, DATA_NULL, 0);
  }

  data.anchor.href = malloc(href_len + 1);
  strncpy(data.anchor.href, ctx->buffer + href_start, href_len);
  data.anchor.href[href_len] = '\0';
  node.data = data;

  ctx->i++;
  return node;
}

struct CMarkNode
parse_inline(struct CMarkContext *ctx)
{
  switch (ctx->buffer[ctx->i]) {
    case '[':
      return parse_anchor(ctx);
    case ' ':
    case '\t':
      while (' ' == ctx->buffer[ctx->i] || '\t' == ctx->buffer[ctx->i]) {
        ctx->i++;
      }
      return create_node(CMARK_WHITESPACE, DATA_NULL, 0);
    default:
      return parse_plain(ctx);
  }
}

struct CMarkNode
parse_header(struct CMarkContext *ctx)
{
  union CMarkNodeData data;
  struct CMarkNode node;
  size_t start = ctx->i;
 
  while ('#' == ctx->buffer[ctx->i]) {
    ctx->i++;
  }
  data.header.level = (unsigned char) (ctx->i - start);

  node = create_node(CMARK_HEADER, data, 4);
  ctx->i++;
  while ('\n' != ctx->buffer[ctx->i]) {
    add_child(&node, parse_inline(ctx));
  }

  return node;
}

struct CMarkNode
parse_paragraph(struct CMarkContext *ctx)
{
  struct CMarkNode node = create_node(CMARK_PARAGRAPH, (union CMarkNodeData) { .null = 0 }, 8);

  while (1) {
    switch (ctx->buffer[ctx->i]) {
      case '\n':
        break;
      default:
        add_child(&node, parse_inline(ctx));
        continue;
    }
    break;
  }

  ctx->i++;
  return node;
}

struct CMarkNode
parse_line(struct CMarkContext *ctx)
{
  union CMarkNodeData data;
  struct CMarkNode node;
  size_t start = ctx->i;

  switch (ctx->buffer[ctx->i]) {
    case '#':
      return parse_header(ctx);
    case '\n':
      return create_node(CMARK_BREAK, (union CMarkNodeData) { .null = 0 }, 0);
    default:
      return parse_paragraph(ctx);
  }
}

struct CMarkNode
cmark_parse(struct CMarkContext *ctx)
{
  struct CMarkNode root = create_node(CMARK_ROOT, DATA_NULL, 8);

  while (1) {
    read_line(ctx);
    if (feof(ctx->file))
      break;
    add_child(&root, parse_line(ctx));
  }

  return root;
}
