typedef typeof(sizeof(0)) size_t;

enum CMarkNodeType {
  CMARK_ROOT,
  CMARK_NULL,
  
  CMARK_HEADER,
  CMARK_PARAGRAPH,

  CMARK_ANCHOR,
  CMARK_CODE,
  CMARK_PLAIN,

  CMARK_WHITESPACE,
  CMARK_BREAK,
};

struct CMarkNodeHeaderData {
  unsigned char level;
};

struct CMarkNodeAnchorData {
  char *href;
};

struct CMarkNodeCodeData {
  char *lang;
  char *content;
};

union CMarkNodeData {
  struct CMarkNodeHeaderData header;
  struct CMarkNodeAnchorData anchor;
  struct CMarkNodeCodeData code;
  char *plain;
  char null;
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

struct CMarkContext *cmark_create_context(void *file);
struct CMarkNode cmark_parse(struct CMarkContext *ctx);
void cmark_free_node(struct CMarkNode node);
