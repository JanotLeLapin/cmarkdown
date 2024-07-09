#include <stddef.h>

typedef enum {
  ROOT,

  HEADING,

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

void
free_node(Node* node);
