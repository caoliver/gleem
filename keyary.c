#include <stdio.h>
#include "util.h"
#include "atomizer.h"

#include "tf.h"

#define NUMPTRS(BYTES) (sizeof(BYTES)/sizeof((BYTES)[0]))

int main(int argc, char *argv[])
{
  char buf[80];

  for (int i = 0; i < haystack_size; i++)
    haystack[i] = intern_string(haystack[i]);
  
  while (printf("> "), gets(buf))
    {
      const char *needle = intern_string(buf);
      int i;

      for (i = haystack_size; --i >=0;)
	if (needle == haystack[i])
	  break;
      
      printf("%s -> %d\n", buf, i);
    }

  puts("");
  return 0;
}
