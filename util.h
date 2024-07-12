#include <stdlib.h>

typedef struct {
  size_t capacity;
  size_t length;
  void **elems;
} Vec;

Vec *
new_vec(size_t capacity);

void
push_vec(Vec *vec, void *elem);

void
free_vec(Vec *vec);
