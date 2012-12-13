#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include "util.h"

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

int break_tokens(char *str, char **tokens, int max_tokens)
{
    int count = 0;
    for (;;)
      {
	while (isspace(*str))
	  ++str;
	if (!*str)
	  return count;
	if (++count > max_tokens)
	  return count - 1;
        *tokens++ = str++;
	while (*str && !isspace(*str))
	  str++;
        if (*str)
            *str++ = 0;
    }
}

char *mkfilepath(int elements, ...)
{
  va_list aptr, ap_start, ap_prev;
  int to_copy = elements;
  int len = 0;
  char *path, *ptr;

  if (elements < 1)
    return NULL;

  va_start(aptr, elements);
  va_copy(ap_start, aptr);

  while (elements--)
    {
      char *element;
      va_copy(ap_prev, aptr);
      element = va_arg(aptr, char *);
      switch (*element)
	{
	case '\0':
	  continue;
	case '/':
	  len = 0;
	  to_copy = elements + 1;
	  va_copy(ap_start, ap_prev);
	}

      len += strlen(element) + 1;
    }
  va_end(aptr);
  va_end(ap_prev);

  path = ptr = xmalloc(len);

  while (to_copy-- > 0)
    {
      char *element = va_arg(ap_start, char *);
      if (!*element)
	continue;
      if (ptr > path)
        *ptr++ = '/';
      while (*element)
	*ptr++ = *element++;
    }
  va_end(ap_start);

  *ptr = '\0';
  return path;
}
