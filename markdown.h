#include <stddef.h>
#include <stdint.h>

typedef enum {
  ROOT,

  PARAGRAPH,
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

TextData*
new_text_data(char* source, size_t start, size_t end);

Node*
new_node(NodeType type, void* value, size_t children_count, Node** children);

void
free_node(Node* node);

Node*
parse_source(char* source);

char*
compile_node(Node* node);
