#include <stdlib.h>
#include <unistd.h>

void *xmalloc(size_t size)
{
  static char errmsg[] = "Memory exhausted!\n";
  void *newptr = malloc(size);
  if (newptr)
    return newptr;
  
  write(2, errmsg, sizeof(errmsg)-1);
  sleep(5);
  abort();
}
