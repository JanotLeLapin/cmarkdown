#include "markdown.h"

#include <stdlib.h>

typedef struct {
  char* source;
  size_t idx;
} Parser;

typedef struct {
  size_t capacity;
  size_t length;
  Node** nodes;
} NodeVec;

Node* parse_text(Parser* parser);
Node* parse_line(Parser* parser);

void
free_nodevec(NodeVec* vec)
{
  size_t i;
  for (i = 0; i < vec->length; i++) {
    free_node(vec->nodes[i]);
  }
  free(vec->nodes);
  free(vec);
}

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
parse_newline(Parser* parser)
{
  Node* node = new_node(NEWLINE, NULL, 0, NULL);
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
  char c;

  if ('[' != parser->source[parser->idx])
    return NULL;
  parser->idx += 1;

  NodeVec* children = new_nodevec(8);
  for (;;) {
    push(children, parse_text(parser));

    c = parser->source[parser->idx];
    if ('\n' == c || '\0' == c) {
      free_nodevec(children);
      return NULL;
    }
    if (']' == c) {
      break;
    }
  }

  parser->idx += 1;
  if ('(' != parser->source[parser->idx]) {
    free_nodevec(children);
    return NULL;
  }

  parser->idx += 1;
  TextData* href;
  size_t href_start = parser->idx;
  for (;;) {
    c = parser->source[parser->idx];

    switch (c) {
    case ')': goto end_href_loop;
    case '\n':
      free_nodevec(children);
      return NULL;
    case '\0':
      free_nodevec(children);
      return NULL;
    }
    parser->idx += 1;
  }

end_href_loop:
  href = new_text_data(parser->source, href_start, parser->idx);
  parser->idx += 1;

  Node* node = new_node(LINK, href, children->length, children->nodes);
  free(children);
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

    push(vec, new_node(TEXT, new_text_data(parser->source, line_start, parser->idx - 1), 0, NULL));
  }

  Node* node = new_node(UNORDERED_LIST, NULL, vec->length, vec->nodes);
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
  if (3 != parser->idx - start)
    return NULL;

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
        return NULL;
      if (']' == c)
        break;
      parser->idx += 1;
    }
    size_t title_end = parser->idx;

    title = new_text_data(parser->source, title_start, title_end);
    parser->idx += 1;
  }

  parser->idx += 1;

  NodeVec* children = new_nodevec(8);
  uint8_t count;
  for (;;) {
    count = 0;

    push(children, parse_text(parser));
    if ('\0' == parser->source[parser->idx])
      return NULL;

    while ('\n' == parser->source[parser->idx]) {
      count += 1;
      parser->idx += 1;
    }
    if (count > 1)
      push(children, new_node(NEWLINE, NULL, 0, NULL));

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
  }
  size_t text_end = parser->idx - 3;

  TextData* type = new_text_data(parser->source, type_start, type_end);
  AsideData* data = malloc(sizeof(AsideData));
  data->type = type;
  data->title = title;
  Node* node = new_node(ASIDE, data, children->length, children->nodes);
  free(children);
  return node;
}

Node*
parse_text(Parser* parser)
{
  size_t start = parser->idx;
  char c = parser->source[parser->idx];

  for (;;) {
    Node* node;
    switch (c) {
    case '\n': goto end_loop;
    case '\0': goto end_loop;
    case ']': goto end_loop;
    case ':': goto end_loop;
    case '[':
      node = parse_link(parser);
      break;
    default:
      parser->idx += 1;
      c = parser->source[parser->idx];
      continue;
    }

    if (NULL == node)
      goto end_loop;
    else
      return node;
  }

end_loop:
  return new_node(TEXT, new_text_data(parser->source, start, parser->idx), 0, NULL);
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
  Node** children = malloc(sizeof(void*));
  children[0] = text;
  return new_node(HEADING, level, 1, children);
}

Node*
parse_line(Parser* parser)
{
  char c = parser->source[parser->idx];
  if ('\0' == c)
    return NULL;

  size_t start = parser->idx;
  Node* node = NULL;
  switch (c) {
  case '\n':
    node = parse_newline(parser);
    break;
  case '#':
    node = parse_heading(parser);
    break;
  case '-':
    node = parse_unordered_list(parser);
    break;
  case ':':
    node = parse_aside(parser);
    break;
  default:
    node = parse_text(parser);
    break;
  }

  if (NULL == node)
    return new_node(TEXT, new_text_data(parser->source, start, parser->idx), 0, NULL);
  else
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

  Node* root = new_node(ROOT, NULL, vec->length, vec->nodes);
  free(vec);
  return root;
}

Node*
parse_source(char* source)
{
  Parser parser = { .source = source, .idx = 0 };
  Node* root = parse(&parser);
  return root;
}