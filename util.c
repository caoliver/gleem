#include <stdlib.h>
#include <unistd.h>
#include <string.h>

void *xrealloc(void *oldptr, size_t size)
{
  static char errmsg[] = "Memory exhausted!\n";
	
  void *newptr = realloc(oldptr, size);
  if (newptr)
    return newptr;
  
  write(2, errmsg, sizeof(errmsg)-1);
  sleep(5);
  abort();
}

void *xcalloc(size_t elts, size_t size)
{
  size_t bytes = elts * size;
  void *newptr = xrealloc(NULL, bytes);
  memset(newptr, 0, bytes);
  return newptr;
}

char *xstrdup(const char *str)
{
  int len = strlen(str);
  char *new = xrealloc(NULL, len + 1);
  return strcpy(new, str);
}
