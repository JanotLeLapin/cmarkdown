#include "cmarkdown.h"

#include <stdlib.h>
#include <stdio.h>

enum Flag {
  FLAG_NEWLINE = 1 << 0,
  FLAG_ANCHOR = 1 << 1,
  FLAG_CODE_INLINE = 1 << 2,
  FLAG_CODE_MULTILINE = 1 << 3,
  FLAG_LIST = 1 << 4,
};

void
read_line(struct CMarkParser *p)
{
  fgets(p->buf, sizeof(p->buf), p->file);
  p->i = 0;
  p->flags |= FLAG_NEWLINE;
}

char is_anchor(struct CMarkParser *p)
{
  size_t i = p->i;

  if ('[' != p->buf[i]) {
    return 0;
  }

  while (']' != p->buf[i]) {
    i++;
    if (p->buf[i] == '\n') {
      return 0;
    }
  }

  i++;
  if ('(' != p->buf[i]) {
    return 0;
  }

  while (')' != p->buf[i]) {
    i++;
    if (p->buf[i] == '\n') {
      return 0;
    }
  }

  return 1;
}

char is_code(struct CMarkParser *p)
{
  size_t i = p->i;

  if ('`' != p->buf[i]) {
    return 0;
  }
  i++;

  if ('`' == p->buf[i] && '`' == p->buf[i + 1]) {
    return 2;
  }

  while ('`' != p->buf[i]) {
    i++;
    if (p->buf[i] == '\n') {
      return 0;
    }
  }

  return 1;
}

struct CMarkParser
cmark_new_parser(void *file)
{
  struct CMarkParser p = {
    .file = file,
    .flags = 0,
    .blockquote_depth = 0,
  };

  read_line(&p);
  return p;
}

struct CMarkElem
cmark_next(struct CMarkParser *p)
{
  unsigned short start = p->i, end;

  while (1) {
    if (feof(p->file)) {
      return (struct CMarkElem) { .type = CMARK_EOF };
    }

    if (p->flags & FLAG_CODE_MULTILINE) {
      if ('\n' == p->buf[p->i]) {
        read_line(p);
        return (struct CMarkElem) { .type = CMARK_BREAK };
      }

      if ('`' == p->buf[p->i] && '`' == p->buf[p->i + 1] && '`' == p->buf[p->i + 2]) {
        p->i += 3;
        p->flags &= ~FLAG_CODE_MULTILINE;
        return (struct CMarkElem) { .type = CMARK_CODE_END };
      }

      while ('\n' != p->buf[p->i]) {
        p->i++;
      }

      return (struct CMarkElem) {
        .type = CMARK_PLAIN,
        .data = (struct CMarkText) {
          .ptr = p->buf,
          .length = p->i,
        },
      };
    }

    if (p->flags & FLAG_NEWLINE) {
      p->flags &= ~FLAG_NEWLINE;

      if (p->blockquote_depth && '>' != p->buf[p->i]) {
        p->blockquote_depth = 0;
        return (struct CMarkElem) { .type = CMARK_BLOCKQUOTE_END };
      }

      if ('*' == p->buf[p->i] || '-' == p->buf[p->i]) {
        if (p->flags & FLAG_LIST) {
          p->i += 2;
          return (struct CMarkElem) { .type = CMARK_LIST_ITEM };
        } else {
          p->flags |= FLAG_LIST;
          p->flags |= FLAG_NEWLINE;
          return (struct CMarkElem) { .type = CMARK_LIST_START };
        }
      } else if (p->flags & FLAG_LIST) {
        p->flags &= ~FLAG_LIST;
        return (struct CMarkElem) { .type = CMARK_LIST_END };
      }

      switch (p->buf[p->i]) {
        case '#':
          while ('#' == p->buf[p->i]) {
            p->i++;
          }
          end = p->i;

          if (' ' == p->buf[p->i]) {
            p->i++;
          }

          return (struct CMarkElem) {
            .type = CMARK_HEADER,
            .data.header_level = end - start,
          };
        case '>':
          while ('>' == p->buf[p->i]) {
            p->i++;
          }
          end = p->i;

          char diff = p->blockquote_depth - (end - start);

          if (1 == abs(diff)) {
            p->blockquote_depth = end - start;
            p->i++;
            return (struct CMarkElem) { .type = diff > 0 ? CMARK_BLOCKQUOTE_END : CMARK_BLOCKQUOTE_START };
          }

          start = p->i + 1;
          break;
        default:
          break;
      }
    }

    switch (p->buf[p->i]) {
      case '\n':
        read_line(p);
        return (struct CMarkElem) { .type = CMARK_BREAK };
      case '[':
        if (!is_anchor(p)) {
          p->i++;
          continue;
        }
        p->i++;
        p->flags |= FLAG_ANCHOR;
        return (struct CMarkElem) { .type = CMARK_ANCHOR_START };
      case ']':
        if (!(p->flags & FLAG_ANCHOR)) {
          p->i++;
          continue;
        }

        p->flags &= ~FLAG_ANCHOR;

        while (')' != p->buf[p->i]) {
          p->i++;
        }
        p->i++;

        return (struct CMarkElem) {
          .type = CMARK_ANCHOR_END,
          .data.anchor_end_href.ptr = p->buf + start + 2,
          .data.anchor_end_href.length = p->i - start - 3,
        };
      case '`':
        if (p->flags & FLAG_CODE_INLINE) {
          p->i++;
          p->flags &= ~FLAG_CODE_INLINE;
          return (struct CMarkElem) {
            .type = CMARK_CODE_END,
          };
        }

        switch (is_code(p)) {
          case 0:
            p->i++;
            continue;
          case 1:
            p->i++;
            p->flags |= FLAG_CODE_INLINE;
            return (struct CMarkElem) {
              .type = CMARK_CODE_START,
            };
          case 2:
            p->i += 3;
            p->flags |= FLAG_CODE_MULTILINE;

            size_t start = p->i;
            union CMarkElemData data;

            while (1) {
              if ('\n' == p->buf[p->i]) {
                break;
              }

              data.code.lang[p->i - start] = p->buf[p->i];
              p->i++;
            }
            data.code.lang[p->i - start] = '\0';
            data.code.is_multi_line = 1;

            return (struct CMarkElem) {
              .type = CMARK_CODE_START,
              .data = data,
            };
        }
      default:
        while (1) {
          switch (p->buf[p->i]) {
            case '\n':
              break;
            case '[':
              if (!(p->flags & FLAG_CODE_INLINE) && is_anchor(p)) {
                break;
              }
              p->i++;
              continue;
            case ']':
              if (p->flags & FLAG_ANCHOR) {
                break;
              }
              p->i++;
              continue;
            case '`':
              if ((p->flags & (FLAG_CODE_INLINE | FLAG_CODE_MULTILINE)) || is_code(p)) {
                break;
              }
              p->i++;
              break;
            default:
              p->i++;
              continue;
          }
          break;
        }

        return (struct CMarkElem) {
          .type = CMARK_PLAIN,
          .data.plain.ptr = p->buf + start,
          .data.plain.length = p->i - start,
        };
    }
  }
}
