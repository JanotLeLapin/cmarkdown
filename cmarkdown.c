#include "cmarkdown.h"

#include <stdio.h>

void
read_line(struct CMarkParser *p)
{
  fgets(p->buf, sizeof(p->buf), p->file);
  p->i = 0;
  p->flags = 1;
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
      default:
        while ('\n' != p->buf[p->i]) {
          p->i++;
        }

        return (struct CMarkElem) {
          .type = CMARK_PLAIN,
          .data.plain.ptr = p->buf + start,
          .data.plain.length = p->i - start,
        };
    }
  }
}
