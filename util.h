#ifndef _UTIL_H_
#define _UTIL_H_

#include <unistd.h>
#include <string.h>

void *xrealloc(void *, size_t);
void *xcalloc(size_t elts, size_t size);
char *xstrdup(char *str);
#define xmalloc(SIZE) xrealloc(NULL, SIZE)

#endif /* _UTIL_H_ */
