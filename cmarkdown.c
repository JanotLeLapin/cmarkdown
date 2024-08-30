#include "cmarkdown.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define create_empty_node() create_node(CMARK_NULL, (union CMarkNodeData) { .null = 0 }, 0);

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
    node->children = realloc(node->children, sizeof(struct CMarkNode) * node->children_size * 2);
  }

  node->children[node->children_count] = child;
  node->children_count++;
}

void
free_node(struct CMarkNode node)
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
    free_node(node.children[i]);
  }
  free(node.children);
}

struct CMarkContext *
create_context(void *file)
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
    return create_empty_node();
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
        return create_empty_node();
      default:
        continue;
    }
    break;
  }

  ctx->i++;
  if ('(' != ctx->buffer[ctx->i]) {
    free(node.children);
    return create_empty_node();
  }

  href_start = ctx->i + 1;
  while (1) {
    ctx->i++;
    switch (ctx->buffer[ctx->i]) {
      case ')':
        break;
      case '\n':
        free(node.children);
        return create_empty_node();
      default:
        continue;
    }
    break;
  }

  href_len = ctx->i - href_start;
  if (!href_len) {
    free(node.children);
    return create_empty_node();
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
      return create_node(CMARK_WHITESPACE, (union CMarkNodeData) { .null = 0 }, 0);
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
    default:
      return parse_paragraph(ctx);
  }
}

struct CMarkNode
parse(struct CMarkContext *ctx)
{
  struct CMarkNode root = create_node(CMARK_ROOT, (union CMarkNodeData) { .null = 0 }, 8);

  while (1) {
    read_line(ctx);
    if (feof(ctx->file))
      break;
    add_child(&root, parse_line(ctx));
  }

  return root;
}

void
print_node(struct CMarkNode node, int depth)
{
  char margin[16];
  size_t i;

  for (i = 0; i < depth; i++) {
    margin[i] = ' ';
  }
  margin[depth] = '\0';
  
  switch (node.type) {
    case CMARK_ROOT:
      printf("%sRoot\n", margin);
      break;
    case CMARK_HEADER:
      printf("%sHeader (%d)\n", margin, node.data.header.level);
      break;
    case CMARK_PARAGRAPH:
      printf("%sParagraph\n", margin);
      break;
    case CMARK_ANCHOR:
      printf("%sAnchor (%s)\n", margin, node.data.anchor.href);
      break;
    case CMARK_PLAIN:
      printf("%sText ('%s')\n", margin, node.data.plain);
      break;
    case CMARK_NULL:
      printf("%sNull\n", margin);
      break;
    case CMARK_WHITESPACE:
      printf("%sWhitespace\n", margin);
      break;
  }

  for (i = 0; i < node.children_count; i++) {
    print_node(node.children[i], depth + 1);
  }
}

int
main()
{
  FILE *file;
  struct CMarkContext *ctx;
  struct CMarkNode root, node, child;
  size_t i, j;

  file = fopen("file.md", "r");
  ctx = create_context(file);
  root = parse(ctx);

  print_node(root, 0);

  free(ctx);
  fclose(file);
  free_node(root);
  
  return 0;
}
