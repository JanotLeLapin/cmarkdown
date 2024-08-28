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

int
main()
{
  union CMarkNodeData ad, bd, cd;
  struct CMarkNode a, b, c;
  char *at, *bt;
  size_t i;

  at = malloc(32);
  bt = malloc(32);
  strcpy(at, "hello, world!");
  strcpy(bt, "foo bar");

  ad.plain = at;
  bd.plain = bt;
  cd.header.level = 2;

  a = create_node(CMARK_PLAIN, ad, 0);
  b = create_node(CMARK_PLAIN, bd, 5);
  c = create_node(CMARK_HEADER, cd, 2);

  add_child(&c, a);
  add_child(&c, b);

  for (i = 0; i < c.children_count; i++) {
    switch (c.children[i].type) {
      case CMARK_PLAIN:
        printf("found plain text: %s\n", c.children[i].data.plain);
        break;
      default:
        printf("found other\n");
        break;
    }
  }

  free_node(c);

  return 0;
}
