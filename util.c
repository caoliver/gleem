#include <stdlib.h>
#include <unistd.h>
#include <string.h>

void out_of_memory()
{
  static char errmsg[] = "Memory exhausted!\n";
	
  write(2, errmsg, sizeof(errmsg)-1);
  sleep(5);
  abort();
}

void *xrealloc(void *oldptr, size_t size)
{
  void *newptr = realloc(oldptr, size);
  if (newptr)
    return newptr;

  out_of_memory();
  // Needed to make compiler happy.  :-(
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
