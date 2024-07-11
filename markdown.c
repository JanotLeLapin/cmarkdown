#include "markdown.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define new_node(t, v, cc, c) malloc(sizeof(Node)); \
  node->type = t; \
  node->value = v; \
  node->children_count = cc; \
  node->children = c;
#define new_node_nc(t, v) new_node(t, v, 0, NULL)

Node*
new_root_node(size_t children_count, Node** children)
{
  Node* node = new_node(ROOT, NULL, children_count, children);
  return node;
}

Node*
new_text_node(TextData* text)
{
  Node* node = new_node_nc(TEXT, text);
  return node;
}

TextData*
new_text_data(char* text, size_t start, size_t end)
{
  TextData* data = malloc(sizeof(TextData));
  data->text = text + start;
  data->length = end - start;
  return data;
}

Node*
new_newline_node()
{
  Node* node = new_node_nc(NEWLINE, NULL);
  return node;
}

Node*
new_heading_node(uint8_t* level, size_t children_count, Node** children)
{
  Node* node = new_node(HEADING, level, children_count, children);
  return node;
}

Node*
new_link_node(TextData* href, size_t children_count, Node** children)
{
  Node* node = new_node(LINK, href, children_count, children);
  return node;
}

Node*
new_unordered_list_node(size_t children_count, Node** children)
{
  Node* node = new_node(UNORDERED_LIST, NULL, children_count, children);
  return node;
}

Node*
new_aside_node(TextData* type, TextData* title, size_t children_count, Node** children)
{
  AsideData* data = malloc(sizeof(AsideData));
  data->type = type;
  data->title = title;

  Node* node = new_node(ASIDE, data, children_count, children);
  return node;
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

  Node* root = parse_source(source);
  char* html = compile_node(root);

  printf("%s", html);

  free(source);
  free(html);
  free_node(root);
}
