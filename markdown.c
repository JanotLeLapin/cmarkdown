#include "markdown.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

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
create_text_node(Parser* parser, size_t start, size_t end)
{
  TextData* data = malloc(sizeof(TextData));
  data->text = parser->source + start;
  data->length = end - start;

  Node* node = malloc(sizeof(Node));
  node->type = TEXT;
  node->value = data;

  return node;
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
parse_link(Parser* parser)
{
  size_t start = parser->idx;
  if ('[' != parser->source[parser->idx])
    return create_text_node(parser, start, parser->idx);

  char c;

  parser->idx += 1;
  Node* text;
  size_t text_start = parser->idx;
  size_t text_end;
  for (;;) {
    c = parser->source[parser->idx];

    switch (c) {
    case ']': goto end_text_loop;
    case '\n' | '\0': return create_text_node(parser, start, parser->idx);
    }
    parser->idx += 1;
  }

end_text_loop:
  text_end = parser->idx;

  parser->idx += 1;
  if ('(' != parser->source[parser->idx])
    return create_text_node(parser, start, parser->idx);

  parser->idx += 1;
  TextData* href;
  size_t href_start = parser->idx;
  for (;;) {
    c = parser->source[parser->idx];

    switch (c) {
    case ')': goto end_href_loop;
    case '\n' | '\0': return create_text_node(parser, start, parser->idx);
    }
    parser->idx += 1;
  }

end_href_loop:
  text = create_text_node(parser, text_start, text_end);
  href = malloc(sizeof(TextData));
  href->text = parser->source + href_start;
  href->length = parser->idx - href_start;

  Node* node = malloc(sizeof(Node));
  node->type = LINK;
  node->value = href;
  node->children_count = 1;
  node->children = malloc(sizeof(void*));
  node->children[0] = text;

  parser->idx += 1;

  return node;
}

Node*
parse_text(Parser* parser)
{
  size_t start = parser->idx;
  char c;

  for (;;) {
    c = parser->source[parser->idx];
    switch (c) {
    case '\n': goto end_loop;
    case '[': return parse_link(parser);
    }

    parser->idx += 1;
  }

end_loop:
  return create_text_node(parser, start, parser->idx);
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

char*
read_file(char* filename)
{
  size_t capacity = 256;
  char* res = malloc(capacity);
  
  FILE* file;
  char buffer[256];

  file = fopen(filename, "r");
  if (NULL == file)
    return NULL;

  size_t offset = 0;
  for (;;) {
    if (NULL == fgets(buffer, sizeof(buffer), file))
      break;

    size_t line_len = strlen(buffer);
    if (offset + line_len >= capacity) {
      capacity *= 2;
      res = realloc(res, capacity);
    }
    memcpy(res + offset, buffer, line_len);
    offset += line_len;
  }

  res[offset] = '\0';
  return res;
}

int
main(int argc, char** argv)
{
  if (argc <= 1) {
    printf("Please specify file path\n");
    return 1;
  }

  char* source = read_file(argv[1]);
  if (NULL == source) {
    printf("Could not open file %s\n", argv[1]);
    return 1;
  }

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

    if (LINK == node->type) {
      Node* text_node = node->children[0];
      TextData* text = text_node->value;
      TextData* href = node->value;
      printf("link '");
      int j;
      for (j = 0; j < text->length; j++) {
        putchar(*(text->text + j));
      }
      printf("' goes to: '");
      for (j = 0; j < href->length; j++) {
        putchar(*(href->text + j));
      }
      printf("'\n");
    }

    if (TEXT == node->type) {
      TextData* text = node->value;
      printf("text: '");
      int j;
      for (j = 0; j < text->length; j++) {
        putchar(*(text->text + j));
      }
      printf("'\n");
    }
  }
  
  free_node(root);
}
