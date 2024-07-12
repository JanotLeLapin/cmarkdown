#include "util.h"

Vec *
new_vec(size_t capacity)
{
  Vec *vec;

  vec = malloc(sizeof(Vec));
  vec->capacity = capacity;
  vec->length = 0;
  vec->elems = malloc(sizeof(void *) * vec->capacity);
  return vec;
}

void
push_vec(Vec *vec, void *elem)
{
  size_t new_length;

  new_length = vec->length + 1;
  if (new_length >= vec->capacity) {
    vec->capacity *= 2;
    vec->elems = realloc(vec->elems, sizeof(void *) * vec->capacity);
  }

  vec->elems[vec->length] = elem;
  vec->length += 1;
}
