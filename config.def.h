/* table of contents */
#define TOC 1
#define TOC_TITLE "Table of contents"

/* code */
static const char *js_kw[] = { "abstract", "await", "boolean", "break", "case", "catch", "char", "class", "const", "continue", "default", "delete", "do", "else", "enum", "eval", "export", "extends", "false", "final", "finally", "for", "function", "goto", "if", "implements", "import", "in", "instanceof", "interface", "let", "new", "null", "private", "protected", "public", "return", "static", "super", "switch", "this", "throw", "throws", "true", "try", "typeof", "var", "while", "yield", NULL };
static const char *c_kw[]  = { "auto", "break", "case", "char", "const", "continue", "default", "do", "double", "else", "enum", "extern", "float", "for", "goto", "if", "int", "long", "register", "return", "short", "signed", "sizeof", "static", "struct", "switch", "typedef", "union", "unsigned", "void", "volatile", "while", NULL };

static const Lang langs[] = {
  /* lang      keywords */
  { "js",      js_kw },
  { "c",       c_kw },
};
