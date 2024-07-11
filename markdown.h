#include <stddef.h>
#include <stdint.h>

typedef enum {
  ROOT,

  HEADING,
  LINK,
  UNORDERED_LIST,
  ASIDE,

  TEXT,
  NEWLINE
} NodeType;

typedef struct Node {
  NodeType type;
  void* value;
  size_t children_count;
  struct Node** children;
} Node;

typedef struct {
  char* text;
  size_t length;
} TextData;

typedef struct {
  TextData* type;
  TextData* title;
} AsideData;

Node*
new_root_node(size_t children_count, Node** children);

Node*
new_text_node(TextData* text);

TextData*
new_text_data(char* source, size_t start, size_t end);

Node*
new_newline_node();

Node*
new_heading_node(uint8_t* level, size_t children_count, Node** children);

Node*
new_link_node(TextData* href, size_t children_count, Node** children);

Node*
new_unordered_list_node(size_t children_count, Node** children);

Node*
new_aside_node(TextData* type, TextData* title, size_t children_count, Node** children);

void
free_node(Node* node);

Node*
parse_source(char* source);

char*
compile_node(Node* node);
