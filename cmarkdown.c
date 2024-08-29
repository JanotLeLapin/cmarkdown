#include "cmarkdown.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

  if (CMARK_PLAIN == node.type) {
    free(node.data.plain);
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
parse_inline(struct CMarkContext *ctx)
{
  union CMarkNodeData data;
  size_t start = ctx->i, len;

  while (1) {
    switch (ctx->buffer[ctx->i]) {
      case '\n':
      case '\t':
      case ' ':
        break;
      default:
        ctx->i++;
        continue;
    }
    break;
  }

  len = ctx->i - start;
  if (!len) {
    return create_node(CMARK_NULL, (union CMarkNodeData) { .null = 0 }, 0);
  }
  
  data.plain = malloc(len + 1);
  strncpy(data.plain, ctx->buffer + start, len);
  data.plain[len] = '\0';

  return create_node(CMARK_PLAIN, data, 0);
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
  while ('\n' != ctx->buffer[ctx->i]) {
    ctx->i++;
    add_child(&node, parse_inline(ctx));
  }

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
      return create_node(CMARK_NULL, (union CMarkNodeData) { .null = 0 }, 0);
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

  for (i = 0; i < root.children_count; i++) {
    node = root.children[i];
    switch (node.type) {
      case CMARK_HEADER:
        printf("Found header of level %d\n", node.data.header.level);
        for (j = 0; j < node.children_count; j++) {
          child = node.children[j];
          switch (child.type) {
            case CMARK_PLAIN:
              printf(" '%s'\n", node.children[j].data.plain);
              break;
            case CMARK_NULL:
              printf(" empty node\n");
              break;
            default:
              printf(" other\n");
              break;
          }
        }
        break;
      default:
        printf("not a header\n");
        break;
    }
  }

  free(ctx);
  fclose(file);
  free_node(root);
  
  return 0;
}
