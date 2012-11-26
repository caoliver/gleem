#include "util.h"
#include "atomizer.h"

#define ACTION_exit 0
#define ACTION_exec 1
#define KEYDEF(NAME) [ACTION_##NAME]=#NAME
static char *keys[] = { KEYDEF(exit), KEYDEF(exec) };
#define NUMPTRS(BYTES) ((BYTES)/sizeof(void *))

int find_intern(char *needle, char *haystack[], int haybytes)
{
  int i;
  for (i = NUMPTRS(haybytes); --i >=0;)
    if (needle == haystack[i])
      break;
  return i;
}

int main(int argc, char *argv[])
{
  char buf[80];

  for (int i = 0; i < NUMPTRS(sizeof(keys)); i++)
    keys[i] = intern_string(keys[i]);
  
  while (printf("> "), gets(buf))
    printf("%s -> %d\n", buf,
	   find_intern(intern_string(buf), keys, sizeof(keys)));

  puts("");
  return 0;
}
