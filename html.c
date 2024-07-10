#include "markdown.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
    string->capacity *= 2;
    string->value = realloc(string->value, string->capacity);
  }

  memcpy(string->value + string->length, value, len + 1);
  string->length = new_len;
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
  case ROOT:
    push_string(string, "<!DOCTYPE HTML><body>");
    for (i = 0; i < node->children_count; i++) {
      push_string(string, compile_node(node->children[i]));
    }
    push_string(string, "</body>");
    break;
  case HEADING: {
    char heading_tag[4];
    snprintf(heading_tag, sizeof(heading_tag), "h%d>", *((uint8_t*) node->value));
    push_string(string, "<");
    push_string(string, heading_tag);
    for (i = 0; i < node->children_count; i++) {
      push_string(string, compile_node(node->children[i]));
    }
    push_string(string, "</");
    push_string(string, heading_tag);
    break;
  }
  case LINK:
    push_string(string, "<a target=\"_blank\" href=\"");
    push_string(string, from_text_data(node->value));
    push_string(string, "\">");
    for (i = 0; i < node->children_count; i++) {
      push_string(string, compile_node(node->children[i]));
    }
    push_string(string, "</a>");
    break;
  case NEWLINE:
    push_string(string, "</br>");
    break;
  case TEXT:
    push_string(string, from_text_data(node->value));
    break;
  }

  char* value = string->value;
  free(string);
  return value;
}
