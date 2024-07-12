#include "markdown.h"

#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"

typedef struct {
  size_t length;
  size_t capacity;
  char *value;
} String;

typedef struct {
  uint8_t level;
  char *text;
  char *id;
} Heading;

typedef struct {
  String *string;
  size_t heading_count;
  Heading **headings;
} HtmlCompiler;

char *
to_kebabcase(const char *input)
{
  int len, i, j, prev_is_space;
  char *result;

  len = strlen(input);
  result = malloc(len + 1);
  prev_is_space = 0;

  for (i = 0, j = 0; input[i]; i++) {
    if (isalnum(input[i])) {
      result[j++] = tolower(input[i]);
      prev_is_space = 0;
    } else if (!prev_is_space && j > 0) {
      result[j++] = '-';
      prev_is_space = 1;
    }
  }

  if (j > 0 && result[j - 1] == '-') {
    j--;
  }

  result[j] = '\0';
  return realloc(result, j + 1);
}

String *
new_string(size_t capacity)
{
  String *string;

  string = malloc(sizeof(String));
  string->length = 0;
  string->capacity = capacity;
  string->value = malloc(capacity);
  string->value[0] = '\0';
  return string;
}

void
push_string(String *string, char *value)
{
  size_t len, new_len;

  len = strlen(value);
  new_len = string->length + len;
  if (new_len >= string->capacity) {
    string->capacity = new_len * 2 + 1;
    string->value = realloc(string->value, string->capacity);
  }

  memcpy(string->value + string->length, value, len + 1);
  string->length = new_len;
}

void
push_char(String *string, char value)
{
  size_t new_len;

  new_len = string->length + 1;
  if (new_len >= string->capacity) {
    string->capacity = new_len * 2;
    string->value = realloc(string->value, string->capacity);
  }

  string->value[string->length] = value;
  string->value[new_len] = '\0';
  string->length = new_len;
}

void
free_heading(Heading *heading)
{
  free(heading->id);
  free(heading->text);
  free(heading);
}

void
free_compiler(HtmlCompiler *compiler)
{
  free(compiler->string);
  free(compiler->headings);
  free(compiler);
}

char *
from_text_data(TextData *data)
{
  char* str;

  str = malloc(data->length + 1);
  memcpy(str, data->text, data->length);
  str[data->length] = '\0';

  return str;
}

void
compile_str(String *s, Node *node)
{
  char* text;

  switch (node->type) {
  case NODE_TEXT: {
    text = from_text_data(node->value);
    push_string(s, text);
    free(text);
    break;
  }
  case NODE_NEWLINE:
    push_string(s, "<br>");
    break;
  default:
    break;
  }
}

void
compile(HtmlCompiler *compiler, Node *node)
{
  String *s, *text;
  size_t i;
  uint8_t level;
  Heading *heading;
  char *url;
  AsideData *aside;
  TextData *lang;

  s = compiler->string;
  switch (node->type) {
  case NODE_ROOT:
    for (i = 0; i < node->children_count; i++) {
      compile(compiler, node->children[i]);
    }
    break;
  case NODE_PARAGRAPH:
    push_string(s, "<p>");
    for (i = 0; i < node->children_count; i++) {
      compile(compiler, node->children[i]);
    }
    push_string(s, "</p>");
    break;
  case NODE_HEADING: {
    level = *((uint8_t*) node->value);
    text = new_string(32);
    for (i = 0; i < node->children_count; i++) {
      compile_str(text, node->children[i]);
    }

    heading = malloc(sizeof(Heading));
    heading->id = to_kebabcase(text->value);
    heading->text = text->value;
    heading->level = level;

    compiler->headings[compiler->heading_count] = heading;
    compiler->heading_count += 1;
    push_string(s, "<h");
    push_char(s, '0' + level);
    push_string(s, " id=\"");
    push_string(s, heading->id);
    push_string(s, "\">");
    push_string(s, text->value);
    push_string(s, "</h");
    push_char(s, '0' + level);
    push_char(s, '>');

    free(text);

    break;
  }
  case NODE_LINK: {
    url = from_text_data(node->value);
    push_string(s, "<a href=\"");
    push_string(s, url);
    push_string(s, "\">");
    free(url);
    for (i = 0; i < node->children_count; i++) {
      compile(compiler, node->children[i]);
    }
    push_string(s, "</a>");
    break;
  }
  case NODE_UNORDERED_LIST:
    push_string(s, "<ul>");
    for (i = 0; i < node->children_count; i++) {
      push_string(s, "<li>");
      compile(compiler, node->children[i]);
      push_string(s, "</li>");
    }
    push_string(s, "</ul>");
    break;
  case NODE_ASIDE: {
    aside = node->value;
    char* type = from_text_data(aside->type);
    push_string(s, "<aside class=\"aside-");
    push_string(s, type);
    free(type);
    push_string(s, "\">");
    if (NULL != aside->title) {
      char* title = from_text_data(aside->title);
      push_string(s, "<h3>");
      push_string(s, title);
      free(title);
      push_string(s, "</h3>");
    }
    for (i = 0; i < node->children_count; i++) {
      compile(compiler, node->children[i]);
    }
    push_string(s, "</aside>");
    break;
  }
  case NODE_CODE:
    lang = node->value;
    push_string(s, "<pre class=\"code\">");
    for (i = 0; i < node->children_count; i++) {
      push_string(s, "<span>");
      compile(compiler, node->children[i]);
      push_string(s, "</span>");
    }
    push_string(s, "</pre>");
  default:
    compile_str(compiler->string, node);
    break;
  }
}

char * 
compile_node(Node *node)
{
  String *s;
  HtmlCompiler *compiler;
  Heading *heading;
  char *res;
  size_t i;

  s = new_string(32);
  compiler = malloc(sizeof(HtmlCompiler));
  compiler->string = s;
  compiler->heading_count = 0;
  compiler->headings = malloc(sizeof(Heading) * 32);

  compile(compiler, node);
  push_string(s, "<nav class=\"contents\"><ul>");
  for (i = 0; i < compiler->heading_count; i++) {
    heading = compiler->headings[i];
    push_string(s, "<li><a href=\"#");
    push_string(s, heading->id);
    push_string(s, "\">");
    push_string(s, heading->text);
    push_string(s, "</a></li>");
    free_heading(heading);
  }
  push_string(s, "</nav>");
  
  res = s->value;

  free_compiler(compiler);
  return res;
}
