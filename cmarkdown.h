struct CMarkParser {
  void *file;
  char buf[256];
  unsigned short i;
  char flags;
};

enum CMarkElemType {
  CMARK_EOF,
  CMARK_BREAK,

  CMARK_PLAIN,
  CMARK_EMPHASIS_START,
  CMARK_EMPHASIS_END,
  CMARK_ANCHOR_START,
  CMARK_ANCHOR_END,

  CMARK_HEADER,
};

union CMarkElemData {
  char *plain;
  char *anchor_end_href;
  char emphasis_flags;
  unsigned char header_level;
};

struct CMarkElem {
  enum CMarkElemType type;
  union CMarkElemData data;
};

struct CMarkParser cmark_new_parser(void *file);

struct CMarkElem cmark_next(struct CMarkParser *parser);
