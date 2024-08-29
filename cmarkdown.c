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
  char *ptr = ctx->buffer + ctx->i;
  size_t len;

  len = strlen(ptr);
  data.plain = malloc(len);
  strncpy(data.plain, ptr, len - 1);

  return create_node(CMARK_PLAIN, data, 0);
}

struct CMarkNode
parse_header(struct CMarkContext *ctx)
{
  union CMarkNodeData data;
  struct CMarkNode node, child;
  size_t start = ctx->i;
 
  while ('#' == ctx->buffer[ctx->i]) {
    ctx->i++;
  }
  data.header.level = (unsigned char) (ctx->i - start);

  child = parse_inline(ctx);

  node = create_node(CMARK_HEADER, data, 1);
  add_child(&node, child);
  return node;
}

struct CMarkNode
parse(struct CMarkContext *ctx)
{
  struct CMarkNode root = create_node(CMARK_ROOT, (union CMarkNodeData) { .root = 0 }, 8);

  read_line(ctx);
  add_child(&root, parse_header(ctx));

  return root;
}

int
main()
{
  FILE *file;
  struct CMarkContext *ctx;
  struct CMarkNode root, node;
  size_t i;

  file = fopen("file.md", "r");
  ctx = create_context(file);
  root = parse(ctx);

  for (i = 0; i < root.children_count; i++) {
    node = root.children[i];
    switch (node.type) {
      case CMARK_HEADER:
        printf("Found header of level %d with: '%s'\n", node.data.header.level, node.children[0].data.plain);
        break;
      default:
        break;
    }
  }

  free(ctx);
  fclose(file);
  free_node(root);
  
  return 0;
}
