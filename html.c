#include "markdown.h"

#include <ctype.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"

typedef struct {
  size_t length;
  size_t capacity;
  char* value;
} String;

typedef struct {
  uint8_t level;
  char* text;
  char* id;
} Heading;

typedef struct {
  String* string;
  size_t heading_count;
  Heading** headings;
} HtmlCompiler;

String*
new_string(size_t capacity)
{
  String* string = malloc(sizeof(String));
  string->length = 0;
  string->capacity = capacity;
  string->value = malloc(capacity);
  string->value[0] = '\0';
  return string;
}

void
push_string(String* string, char* value)
{
  size_t len = strlen(value);
  size_t new_len = string->length + len;
  if (new_len >= string->capacity) {
    string->capacity = new_len * 2;
    string->value = realloc(string->value, string->capacity);
  }

  strcat(string->value, value);
  string->length = new_len;
}

void
push_char(String* string, char value)
{
  size_t new_len = string->length + 1;
  if (new_len >= string->capacity) {
    string->capacity = new_len * 2;
    string->value = realloc(string->value, string->capacity);
  }

  string->value[string->length] = value;
  string->length = new_len;
  string->value[string->length] = '\0';
}

void
free_heading(Heading* heading)
{
  free(heading->id);
  free(heading->text);
  free(heading);
}

void
free_compiler(HtmlCompiler* compiler)
{
  free(compiler->string);
  free(compiler->headings);
  free(compiler);
}

char*
from_text_data(TextData* data)
{
  char* str = malloc(data->length);
  int i;
  for (i = 0; i < data->length; i++) {
    str[i] = data->text[i];
  }
  str[data->length] = '\0';

  return str;
}

void
compile_str(String* s, Node* node)
{
  size_t i = 0;
  switch (node->type) {
  case TEXT: {
    char* text = from_text_data(node->value);
    push_string(s, text);
    free(text);
    break;
  }
  case NEWLINE:
    push_string(s, "<br>");
    break;
  default:
    break;
  }
}

void
compile(HtmlCompiler* compiler, Node* node)
{
  String* s = compiler->string;

  size_t i = 0;
  switch (node->type) {
  case ROOT:
    while (i < node->children_count) {
      compile(compiler, node->children[i]);
      i++;
    }
    break;
  case HEADING: {
    int level = *(uint8_t*) node->value;
    String* text = new_string(32);
    String* id = new_string(32);
    while (i < node->children_count) {
      compile_str(text, node->children[i]);
      i += 1;
    }

    size_t idx = 0;
    size_t last_char = 0;
    char c;
    for(;;) {
      c = text->value[idx];

      if ('\0' == c) {
        id->value[last_char + 1] = '\0';
        break;
      }

      if (isalnum(c)) {
        push_char(id, tolower(c));
        last_char = idx;
        idx += 1;
      } else {
        push_char(id, '-');
        while (!isalnum(c)) {
          idx += 1;
          c = text->value[idx];
        }
      }
    }

    Heading* heading = malloc(sizeof(Heading));
    heading->id = id->value;
    heading->text = text->value;
    heading->level = level;

    free(text);
    free(id);

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
    break;
  }
  case LINK: {
    char* url = from_text_data(node->value);
    push_string(s, "<a href=\"");
    push_string(s, url);
    push_string(s, "\">");
    free(url);
    while (i < node->children_count) {
      compile(compiler, node->children[i]);
      i += 1;
    }
    push_string(s, "</a>");
    break;
  }
  case UNORDERED_LIST:
    push_string(s, "<ul>");
    while (i < node->children_count) {
      push_string(s, "<li>");
      compile(compiler, node->children[i]);
      push_string(s, "</li>");
      i += 1;
    }
    push_string(s, "</ul>");
    break;
  default:
    compile_str(compiler->string, node);
    break;
  }
}

char* 
compile_node(Node* node)
{
  String* s = new_string(32);
  HtmlCompiler* compiler = malloc(sizeof(HtmlCompiler));
  compiler->string = s;
  compiler->heading_count = 0;
  compiler->headings = malloc(sizeof(Heading) * 32);

  compile(compiler, node);
  push_string(s, "<nav class=\"contents\"><ul>");
  size_t i = 0;
  while (i < compiler->heading_count) {
    Heading* heading = compiler->headings[i];
    push_string(s, "<li><a href=\"#");
    push_string(s, heading->id);
    push_string(s, "\">");
    push_string(s, heading->text);
    push_string(s, "</a></li>");
    free_heading(heading);
    i += 1;
  }
  push_string(s, "</nav>");
  
  char* res = s->value;

  free_compiler(compiler);
  return res;
}
