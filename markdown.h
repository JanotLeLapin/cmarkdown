#include <stddef.h>

typedef enum {
  ROOT,

  HEADING,

  TEXT,
  NEWLINE
} NodeType;

typedef struct {
  NodeType type;
  void* value;
  size_t children_count;
  void* children;
} Node;

typedef struct {
  char* text;
  size_t length;
} TextData;
