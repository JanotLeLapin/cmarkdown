typedef typeof(sizeof(0)) size_t;

enum CMarkNodeType {
  CMARK_ROOT,
  
  CMARK_HEADER,

  CMARK_PLAIN,
};

struct CMarkNodeHeaderData {
  unsigned char level;
};

union CMarkNodeData {
  struct CMarkNodeHeaderData header;
  char *plain;
  char root;
};

struct CMarkNode {
  enum CMarkNodeType type;
  union CMarkNodeData data;
  size_t children_count;
  size_t children_size;
  struct CMarkNode *children;
};

struct CMarkContext {
  void *file;
  char buffer[512];
  size_t i;
};

struct CMarkNode create_node(enum CMarkNodeType type, union CMarkNodeData data, size_t initial_size);
void add_child(struct CMarkNode *node, struct CMarkNode child);
void free_node(struct CMarkNode node);

struct CMarkContext *create_context(void *file);
struct CMarkNode parse(struct CMarkContext *ctx);
