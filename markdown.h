#include <stddef.h>
#include <stdint.h>

typedef enum {
  NODE_ROOT,

  NODE_PARAGRAPH,
  NODE_HEADING,
  NODE_LINK,
  NODE_UNORDERED_LIST,
  NODE_ASIDE,
  NODE_CODE,

  NODE_TEXT,
  NODE_NEWLINE
} NodeType;

typedef enum {
  CODE_STRING,
  CODE_SPACING,
  CODE_PLAIN,
} CodeElemType;

typedef struct Node {
  NodeType type;
  void *value;
  size_t children_count;
  struct Node **children;
} Node;

typedef struct {
  const char *text;
  size_t length;
} TextData;

typedef struct {
  TextData *type;
  TextData *title;
} AsideData;

typedef struct {
  CodeElemType type;
  void *value;
} CodeElem;

typedef struct {
  size_t length;
  CodeElem **elements;
  TextData *lang;
} CodeData;

TextData *
new_text_data(const char *source, size_t start, size_t end);

Node *
new_node(NodeType type, void *value, size_t children_count, Node **children);

void
free_node(Node *node);

Node *
parse_source(char *source);

char *
compile_node(Node *node);
