#include "cmarkdown.h"

#include <stdio.h>

void
read_line(struct CMarkParser *p)
{
  fgets(p->buf, sizeof(p->buf), p->file);
  p->i = 0;
  p->flags = 1;
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

    if (0x01 == (p->flags & 0x01)) {
      p->flags &= ~1;
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
        default:
          break;
      }
    }

    switch (p->buf[p->i]) {
      case '\n':
        read_line(p);
        return (struct CMarkElem) { .type = CMARK_BREAK };
      case '[':
        p->flags |= 0x02;
        p->i++;
        return (struct CMarkElem) { .type = CMARK_ANCHOR_START };
      case ']':
        p->flags &= ~0x02;

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
        p->i++;
        if ((p->flags & 0x04) == 0x04) {
          p->flags &= ~0x04;
          return (struct CMarkElem) {
            .type = CMARK_CODE_END,
          };
        } else {
          p->flags |= 0x04;
          return (struct CMarkElem) {
            .type = CMARK_CODE_START,
          };
        }
      default:
        while (1) {
          switch (p->buf[p->i]) {
            case '\n':
              break;
            case '[':
              if ((p->flags & 0x04) != 0x04 && is_anchor(p)) {
                break;
              }
              p->i++;
              continue;
            case ']':
              if ((p->flags & 0x02) == 0x02) {
                break;
              }
              p->i++;
              continue;
            case '`':
              if ((p->flags & 0x04) == 0x04 || is_code(p)) {
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
