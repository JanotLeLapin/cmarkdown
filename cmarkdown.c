#include "cmarkdown.h"

#include <stdio.h>

void
read_line(struct CMarkParser *p)
{
  fgets(p->buf, sizeof(p->buf), p->file);
  p->i = 0;
  p->flags |= 1;
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

    if (0x08 == (p->flags & 0x08)) {
      if ('\n' == p->buf[p->i]) {
        read_line(p);
        return (struct CMarkElem) { .type = CMARK_BREAK };
      }

      if ('`' == p->buf[p->i] && '`' == p->buf[p->i + 1] && '`' == p->buf[p->i + 2]) {
        p->i += 3;
        p->flags &= ~0x08;
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
        case '-':
        case '*':
          p->i++;
          if (' ' != p->buf[p->i]) {
            break;
          }
          p->i++;

          return (struct CMarkElem) {
            .type = CMARK_LIST_ITEM,
            .data.list_item_symbol = p->buf[p->i - 2],
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
        if (!is_anchor(p)) {
          p->i++;
          continue;
        }
        p->i++;
        p->flags |= 0x02;
        return (struct CMarkElem) { .type = CMARK_ANCHOR_START };
      case ']':
        if ((p->flags & 0x02) != 0x02) {
          p->i++;
          continue;
        }

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
        if ((p->flags & 0x04) == 0x04) {
          p->i++;
          p->flags &= ~0x04;
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
            p->flags |= 0x04;
            return (struct CMarkElem) {
              .type = CMARK_CODE_START,
            };
          case 2:
            p->i += 3;
            p->flags |= 0x08;

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
              if ((p->flags & (0x04 | 0x08)) || is_code(p)) {
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
