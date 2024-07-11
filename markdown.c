#include "markdown.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

typedef struct {
  char* source;
  size_t idx;
} Parser;

typedef struct {
  size_t capacity;
  size_t length;
  Node** nodes;
} NodeVec;

NodeVec*
new_nodevec(size_t capacity)
{
  NodeVec* vec = malloc(sizeof(NodeVec));
  vec->capacity = capacity;
  vec->length = 0;
  vec->nodes = malloc(sizeof(void*) * vec->capacity);
  return vec;
}

void
push(NodeVec* vec, Node* node)
{
  size_t new_length = vec->length + 1;
  if (new_length >= vec->capacity) {
    vec->capacity *= 2;
    vec->nodes = realloc(vec->nodes, sizeof(void*) * vec->capacity);
  }

  vec->nodes[vec->length] = node;
  vec->length += 1;
}

void
free_node(Node* node)
{
  switch (node->type) {
  case ROOT:
    free(node->value);
    break;
  case HEADING:
    free(node->value);
    break;
  case LINK:
    free(node->value);
    break;
  case ASIDE: {
    AsideData* data = node->value;
    if (NULL != data->title)
      free(data->title);
    free(data->type);
    free(node->value);
    break;
  }
  case TEXT:
    free(node->value);
    break;
  default:
    break;
  }

  int i;
  for (i = 0; i < node->children_count; i++) {
    free_node(node->children[i]);
  }

  free(node->children);
  free(node);
}

void
skip_whitespace(Parser* parser)
{
  char c;
  for (;;) {
    c = parser->source[parser->idx];
    if (c != ' ' && c != '\t')
      break;

    parser->idx += 1;
  }
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
  node->children_count = 0;
  node->children = NULL;

  return node;
}

Node*
parse_newline(Parser* parser)
{
  Node* node = malloc(sizeof(Node));
  node->type = NEWLINE;
  node->value = NULL;
  node->children_count = 0;
  node->children = NULL;
  
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
parse_unordered_list(Parser* parser)
{
  NodeVec* vec = new_nodevec(4);

  for(;;) {
    char c = parser->source[parser->idx];
    if ('-' != c)
      break;
    parser->idx += 1;
    skip_whitespace(parser);

    size_t line_start = parser->idx;
    for (;;) {
      parser->idx += 1;
      c = parser->source[parser->idx];

      if ('\n' == c || '\0' == c) {
        parser->idx += 1;
        break;
      }
    }

    push(vec, create_text_node(parser, line_start, parser->idx - 1));
  }

  Node* node = malloc(sizeof(Node));
  node->type = UNORDERED_LIST;
  node->value = NULL;
  node->children_count = vec->length;
  node->children = vec->nodes;

  free(vec);
  return node;
}

Node*
parse_aside(Parser* parser)
{
  size_t start = parser->idx;
  char c;

  for (;;) {
    if (':' != parser->source[parser->idx])
      break;
    parser->idx += 1;
  }
  if (3 != parser->idx - start) return create_text_node(parser, start, parser->idx);

  size_t type_start = parser->idx;
  for(;;) {
    c = parser->source[parser->idx];
    if ('\n' == c || '[' == c)
      break;
    parser->idx += 1;
  }
  size_t type_end = parser->idx;

  TextData* title = NULL;
  if ('[' == c) {
    parser->idx += 1;
    size_t title_start = parser->idx;
    for (;;) {
      c = parser->source[parser->idx];
      if ('\n' == c)
        return create_text_node(parser, start, parser->idx);
      if (']' == c)
        break;
      parser->idx += 1;
    }
    size_t title_end = parser->idx;

    title = malloc(sizeof(TextData));
    title->text = parser->source + title_start;
    title->length = title_end - title_start;

    parser->idx += 1;
  }

  parser->idx += 1;

  size_t text_start = parser->idx;
  uint8_t count;
  for (;;) {
    count = 0;
    for (;;) {
      if (':' == parser->source[parser->idx])
        count += 1;
      else
        break;
      parser->idx += 1;
    }
    if (count == 3)
      break;
    parser->idx += 1;
  }
  size_t text_end = parser->idx - 3;

  Node* text = create_text_node(parser, text_start, text_end);
  Node** children = malloc(sizeof(void*));
  children[0] = text;

  TextData* type = malloc(sizeof(TextData));
  type->text = parser->source + type_start;
  type->length = type_end - type_start;

  AsideData* data = malloc(sizeof(AsideData));
  data->type = type;
  data->title = title;

  Node* node = malloc(sizeof(Node));
  node->children_count = 1;
  node->children = children;
  node->type = ASIDE;
  node->value = data;

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
    case '-': return parse_unordered_list(parser);
    case ':': return parse_aside(parser);
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
  *level = 0;

  char c;

  for (;;) {
    c = parser->source[parser->idx];
    if ('#' != c)
      break;

    parser->idx += 1;
    *level += 1;
  }
  skip_whitespace(parser);

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
  NodeVec* vec = new_nodevec(8);

  Node* node;
  for (;;) {
    node = parse_line(parser);
    if (NULL == node) {
      break;
    }
    push(vec, node);
  }

  Node* root = malloc(sizeof(Node));
  root->type = ROOT;
  root->value = NULL;
  root->children_count = vec->length;
  root->children = vec->nodes;

  free(vec);
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

  fclose(file);

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
  char* html = compile_node(root);

  printf("%s", html);

  free(source);
  free(html);
  free_node(root);
}
