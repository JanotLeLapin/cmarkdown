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
  CMARK_CODE_START,
  CMARK_CODE_END,

  CMARK_HEADER,
  CMARK_LIST_ITEM,
};

struct CMarkText {
  char *ptr;
  typeof(sizeof(0)) length;
};

union CMarkElemData {
  struct CMarkText plain;
  struct CMarkText anchor_end_href;
  char emphasis_flags;
  struct {
    char lang[16];
    char is_multi_line;
  } code;
  unsigned char header_level;
  char list_item_symbol;
};

struct CMarkElem {
  enum CMarkElemType type;
  union CMarkElemData data;
};

struct CMarkParser cmark_new_parser(void *file);

struct CMarkElem cmark_next(struct CMarkParser *parser);
