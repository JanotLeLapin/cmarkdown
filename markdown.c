#include "markdown.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

typedef struct {
  char* source;
  size_t idx;
} Parser;

Node*
parse_newline(Parser* parser) {
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
  node->children = text;
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
  default: return NULL;
  }
  return node;
}

int
main()
{
  char* source = "# Hello, world!\n\nThis is text\n\n## Something";
  Parser parser = { .source = source, .idx = 0 };
  Node* node = parse_line(&parser);

  if (HEADING == node->type) {
    Node* text_node = node->children;
    TextData* text = text_node->value;
    printf("heading lvl %d: '", *((uint8_t*) node->value));

    int i;
    for (i = 0; i < text->length; i++) {
      putchar(*(text->text + i));
    }
    printf("'\n");
  }
}
