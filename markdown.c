#include "markdown.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

typedef struct {
  char* source;
  size_t idx;
} Parser;

void
free_node(Node* node)
{
  free(node->value);

  int i;
  for (i = 0; i < node->children_count; i++) {
    free_node(node->children[i]);
  }

  free(node->children);
  free(node);
}

Node*
parse_newline(Parser* parser)
{
  Node* node = malloc(sizeof(Node));
  node->type = NEWLINE;
  
  char c;
  for (;;) {
    c = parser->source[parser->idx];
    if ('\n' != c)
      break;

    parser->idx += 1;
  }

  return node;
}

Node*
parse_text(Parser* parser)
{
  size_t start = parser->idx;
  char c;

  for (;;) {
    c = parser->source[parser->idx];
    if ('\n' == c)
      break;

    parser->idx += 1;
  }

  TextData* data = malloc(sizeof(TextData));
  data->text = parser->source + start;
  data->length = parser->idx - start;

  Node* node = malloc(sizeof(Node));
  node->type = TEXT;
  node->value = data;

  return node;
}

Node*
parse_heading(Parser* parser)
{
  uint8_t* level = malloc(1);
  char c;

  for (;;) {
    c = parser->source[parser->idx];
    if ('#' != c)
      break;

    parser->idx += 1;
    *level += 1;
  }

  Node* text = parse_text(parser);
  Node* node = malloc(sizeof(Node));
  node->type = HEADING;
  node->value = level;
  node->children_count = 1;
  node->children = malloc(sizeof(void*));
  node->children[0] = text;
  return node;
}

Node*
parse_line(Parser* parser)
{
  char c = parser->source[parser->idx];
  if ('\0' == c)
    return NULL;

  Node* node = malloc(sizeof(Node));
  switch (c) {
  case '\n': return parse_newline(parser);
  case '#': return parse_heading(parser);
  default: return parse_text(parser);
  }
  return node;
}

Node*
parse(Parser* parser)
{
  size_t children_count = 0;
  size_t capacity = 10;
  Node** children = malloc(sizeof(Node) * capacity);

  Node* node;
  for (;;) {
    node = parse_line(parser);
    if (NULL == node) {
      break;
    }
    if (children_count >= capacity) {
      capacity *= 2;
      children = realloc(children, sizeof(Node*) * capacity);
    }
    children[children_count] = node;
    children_count += 1;
  }

  Node* root = malloc(sizeof(Node));
  root->type = ROOT;
  root->children_count = children_count;
  root->children = children;
  return root;
}

int
main()
{
  char* source = "# Hello, world!\n\nThis is text\n\n## Something hello\n";
  Parser parser = { .source = source, .idx = 0 };
  Node* root = parse(&parser);

  size_t i;
  for (i = 0; i < root->children_count; i++) {
    Node* node = root->children[i];
    if (HEADING == node->type) {
      Node* text_node = node->children[0];
      TextData* text = text_node->value;
      printf("heading lvl %d: '", *((uint8_t*) node->value));

      int j;
      for (j = 0; j < text->length; j++) {
        putchar(*(text->text + j));
      }
      printf("'\n");
    }
  }
  
  free_node(root);
}
