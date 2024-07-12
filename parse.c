#include "markdown.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
  char* source;
  size_t idx;
} Parser;

typedef struct {
  size_t capacity;
  size_t length;
  Node **nodes;
} NodeVec;

Node *
parse_text(Parser *parser);

Node *
parse_line(Parser *parser);

Node *
parse_inline(Parser *parser);

Node *
parse_paragraph(Parser *parser);

void
free_nodevec(NodeVec *vec)
{
  size_t i;

  for (i = 0; i < vec->length; i++) {
    free_node(vec->nodes[i]);
  }
  free(vec->nodes);
  free(vec);
}

NodeVec *
new_nodevec(size_t capacity)
{
  NodeVec *nodes = malloc(sizeof(NodeVec));

  nodes->capacity = capacity;
  nodes->length = 0;
  nodes->nodes = malloc(sizeof(void*) * nodes->capacity);
  return nodes;
}

void
push(NodeVec *vec, Node *node)
{
  size_t new_length = vec->length + 1;

  if (new_length >= vec->capacity) {
    vec->capacity *= 2;
    vec->nodes = realloc(vec->nodes, sizeof(void *) * vec->capacity);
  }

  vec->nodes[vec->length] = node;
  vec->length += 1;
}

void
skip_whitespace(Parser *parser)
{
  char c;

  for (;;) {
    c = parser->source[parser->idx];
    if (c != ' ' && c != '\t')
      break;

    parser->idx += 1;
  }
}

Node *
parse_newline(Parser *parser)
{
  Node *node = new_node(NODE_NEWLINE, NULL, 0, NULL);
  char c;

  for (;;) {
    c = parser->source[parser->idx];
    if ('\n' != c)
      break;

    parser->idx += 1;
  }

  return node;
}

Node *
parse_link(Parser *parser)
{
  size_t start, href_start;
  char c;
  NodeVec *children;
  TextData *href;
  Node *node;

  start = parser->idx;
  if ('[' != parser->source[parser->idx])
    return NULL;
  parser->idx += 1;

  children = new_nodevec(8);
  for (;;) {
    c = parser->source[parser->idx];
    if ('\n' == c || '\0' == c) {
      free_nodevec(children);
      return NULL;
    }
    if (']' == c) {
      break;
    }

    push(children, parse_inline(parser));
  }

  parser->idx += 1;
  if ('(' != parser->source[parser->idx]) {
    free_nodevec(children);
    return NULL;
  }

  parser->idx += 1;
  href_start = parser->idx;
  for (;;) {
    c = parser->source[parser->idx];

    if (')' == c)
      break;

    if ('\n' == c || '\0' == c) {
      free_nodevec(children);
      return NULL;
    }

    parser->idx += 1;
  }

  href = new_text_data(parser->source, href_start, parser->idx);
  parser->idx += 1;

  node = new_node(NODE_LINK, href, children->length, children->nodes);
  free(children);
  return node;
}

Node *
parse_unordered_list(Parser *parser)
{
  NodeVec *children;
  Node *node;
  char c;
  size_t line_start;

  children = new_nodevec(4);
  for(;;) {
    c = parser->source[parser->idx];
    if ('-' != c)
      break;
    parser->idx += 1;
    skip_whitespace(parser);

    push(children, parse_paragraph(parser));
  }

  node = new_node(NODE_UNORDERED_LIST, NULL, children->length, children->nodes);
  free(children);
  return node;
}

Node *
parse_aside(Parser* parser)
{
  char c;
  size_t start, type_start, type_end, title_start, title_end, text_end;
  uint8_t linebreaks, colons;
  TextData *title;
  NodeVec *children;
  TextData *type;
  AsideData *data;
  Node *node;

  start = parser->idx;
  while (':' == parser->source[parser->idx])
    parser->idx += 1;

  if (3 != parser->idx - start)
    return NULL;

  type_start = parser->idx;
  for(;;) {
    c = parser->source[parser->idx];
    if ('\n' == c || '[' == c)
      break;
    parser->idx += 1;
  }
  type_end = parser->idx;

  title = NULL;
  if ('[' == c) {
    parser->idx += 1;
    title_start = parser->idx;
    for (;;) {
      c = parser->source[parser->idx];
      if ('\n' == c)
        return NULL;
      if (']' == c)
        break;
      parser->idx += 1;
    }
    title_end = parser->idx;

    title = new_text_data(parser->source, title_start, title_end);
    parser->idx += 1;
  }

  parser->idx += 1;
  children = new_nodevec(8);
  for (;;) {
    linebreaks = 0;

    push(children, parse_paragraph(parser));
    if ('\0' == parser->source[parser->idx])
      return NULL;

    while ('\n' == parser->source[parser->idx]) {
      linebreaks += 1;
      parser->idx += 1;
    }
    if (linebreaks > 1)
      push(children, new_node(NODE_NEWLINE, NULL, 0, NULL));

    colons = 0;
    while (':' == parser->source[parser->idx]) {
      colons += 1;
      parser->idx += 1;
    }

    if (colons == 3)
      break;
  }
  text_end = parser->idx - 3;

  type = new_text_data(parser->source, type_start, type_end);
  data = malloc(sizeof(AsideData));
  data->type = type;
  data->title = title;
  node = new_node(NODE_ASIDE, data, children->length, children->nodes);
  free(children);
  return node;
}

Node *
parse_code(Parser *parser)
{
  char c;
  size_t start, lang_start, lang_end, node_start;
  NodeVec* children;
  TextData* lang;

  start = parser->idx;
  while ('`' == parser->source[parser->idx])
    parser->idx += 1;

  if (parser->idx - start != 3)
    return NULL;

  lang_start = parser->idx;
  while ('\n' != parser->source[parser->idx])
    parser->idx += 1;
  lang_end = parser->idx;

  parser->idx += 1;
  children = new_nodevec(16);

  for (;;) {
    c = parser->source[parser->idx];

    node_start = parser->idx;

    if ('\0' == c) {
      free_nodevec(children);
      return NULL;
    }

    if ('\n' == c) {
      parser->idx += 1;
      start = parser->idx;
      while ('`' == parser->source[parser->idx])
        parser->idx += 1;

      if (parser->idx - start == 3)
        break;

      parser->idx = start;
      push(children, new_node(NODE_NEWLINE, NULL, 0, NULL));
      continue;
    }

    parser->idx += 1;
    if (isalnum(c)) {
      while (isalnum(parser->source[parser->idx]))
        parser->idx += 1;
    }
    push(children, new_node(NODE_TEXT, new_text_data(parser->source, node_start, parser->idx), 0, NULL));
  }

  lang = NULL;
  if (lang_end > lang_start)
    lang = new_text_data(parser->source, lang_start, lang_end);
  return new_node(NODE_CODE, lang, children->length, children->nodes);
}

Node *
parse_text(Parser *parser)
{
  size_t start;

  start = parser->idx;
  for (;;) {
    switch (parser->source[parser->idx]) {
    case '\n':
    case '\0':
    case '[':
    case ']':
      break;
    default:
      parser->idx += 1;
      continue;
    }

    break;
  }

  if (start == parser->idx)
    return NULL;

  return new_node(NODE_TEXT, new_text_data(parser->source, start, parser->idx), 0, NULL);
}

Node *
parse_paragraph(Parser *parser)
{
  size_t start;
  char c;
  NodeVec *nodes;
  Node *node;

  nodes = new_nodevec(8);
  for (;;) {
    c = parser->source[parser->idx];
    if ('\n' == c)
      parser->idx += 1;

    if ('\n' == c || '\0' == c)
      break;

    start = parser->idx;
    node = parse_inline(parser);
    if (NULL == node) {
      parser->idx += 1;
      push(nodes, new_node(NODE_TEXT, new_text_data(parser->source, start, parser->idx), 0, NULL));
      continue;
    }

    push(nodes, node);
  }

  node = new_node(NODE_PARAGRAPH, NULL, nodes->length, nodes->nodes);
  free(nodes);
  return node;
}

Node *
parse_heading(Parser *parser)
{
  uint8_t *level;
  char c;
  NodeVec *children;
  Node *node;

  level = malloc(1);
  *level = 0;
  for (;;) {
    c = parser->source[parser->idx];
    if ('#' != c)
      break;

    parser->idx += 1;
    *level += 1;
  }
  skip_whitespace(parser);

  children = new_nodevec(2);
  for (;;) {
    node = parse_inline(parser);
    if (NULL == node)
      break;
    push(children, node);
  }

  node = new_node(NODE_HEADING, level, children->length, children->nodes);
  free(children);
  return node;
}

Node *
parse_inline(Parser *parser)
{
  switch (parser->source[parser->idx]) {
  case '\0':
  case '\n':
    return NULL;
  case '[':
    return parse_link(parser);
  default:
    return parse_text(parser);
  }
}

Node *
parse_line(Parser *parser)
{
  char c;
  size_t start;
  Node *node;

  c = parser->source[parser->idx];
  if ('\0' == c)
    return NULL;

  start = parser->idx;
  node = NULL;
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
  case '`':
    node = parse_code(parser);
    break;
  default:
    break;
  }

  if (NULL == node) {
    parser->idx = start;
    return parse_paragraph(parser);
  } else {
    return node;
  }
}

Node *
parse(Parser *parser)
{
  NodeVec *nodes;
  Node *node;

  nodes = new_nodevec(8);
  for (;;) {
    node = parse_line(parser);
    if (NULL == node) {
      break;
    }
    push(nodes, node);
  }

  node = new_node(NODE_ROOT, NULL, nodes->length, nodes->nodes);
  free(nodes);
  return node;
}

Node *
parse_source(char *source)
{
  Parser parser;

  parser.source = source;
  parser.idx = 0;
  return parse(&parser);
}
