#include "markdown.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"

typedef struct {
  size_t length;
  size_t capacity;
  char* value;
} String;

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
push_string_free(String* string, char* value)
{
  push_string(string, value);
  free(value);
}

char*
from_text_data(TextData* data)
{
  char* str = malloc(data->length);
  int i;
  for (i = 0; i < data->length; i++) {
    str[i] = data->text[i];
  }

  return str;
}

char* 
compile_node(Node* node)
{
  String* string = new_string(32);
  int i;
  switch (node->type) {
  case ROOT: {
    String* contents = new_string(32);
    push_string(contents, "<nav class=\"contents\"><h3>");
    push_string(contents, TOC_TITLE);
    push_string(contents, "</h3><ul>");
    push_string(string, "<!DOCTYPE HTML><body>");
    for (i = 0; i < node->children_count; i++) {
      Node* child = node->children[i];
      if (HEADING == child->type) {
        push_string(contents, "<li class=\"contents-item-");
        char heading_level[2];
        snprintf(heading_level, sizeof(heading_level), "%d", *((uint8_t*) child->value));
        push_string(contents, heading_level);
        push_string(contents, "\">");
        int j;
        for (j = 0; j < child->children_count; j++) {
          push_string_free(contents, compile_node(child->children[j]));
        }
        push_string(contents, "</li>");
      }
      push_string_free(string, compile_node(child));
    }
    push_string(contents, "</ul></nav>");

    char* contents_str = contents->value;
    free(contents);

    push_string(string, contents_str);
    free(contents_str);

    push_string(string, "</body>");
    break;
  }
  case HEADING: {
    char heading_tag[4];
    snprintf(heading_tag, sizeof(heading_tag), "h%d>", *((uint8_t*) node->value));
    push_string(string, "<");
    push_string(string, heading_tag);
    for (i = 0; i < node->children_count; i++) {
      push_string_free(string, compile_node(node->children[i]));
    }
    push_string(string, "</");
    push_string(string, heading_tag);
    break;
  }
  case LINK:
    push_string(string, "<a target=\"_blank\" href=\"");
    push_string_free(string, from_text_data(node->value));
    push_string(string, "\">");
    for (i = 0; i < node->children_count; i++) {
      push_string_free(string, compile_node(node->children[i]));
    }
    push_string(string, "</a>");
    break;
  case UNORDERED_LIST:
    push_string(string, "<ul>");
    for (i = 0; i < node->children_count; i++) {
      push_string(string, "<li>");
      push_string_free(string, compile_node(node->children[i]));
      push_string(string, "</li>");
    }
    push_string(string, "</ul>");
    break;
  case NEWLINE:
    push_string(string, "</br>");
    break;
  case TEXT:
    push_string_free(string, from_text_data(node->value));
    break;
  }

  char* value = string->value;
  free(string);
  return value;
}
