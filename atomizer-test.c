#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "atomizer.h"
#include <alloca.h>

char indenter[256];

void show(struct cell *cell, int indent, int suppress)
{
  if (!suppress)
    write(1, indenter, indent);
  if (IS_ATOM(cell))
    {
      char outbuf[1024];
      sprintf(outbuf, "%lx '%s'\n", (long)ATOM_NAME(cell), ATOM_NAME(cell));
      write(1, outbuf, strlen(outbuf));
      return;
    }
  if (IS_NIL(cell))
    {
      write(1, "()\n", 3);
      return;
    }
  write(1, "( ", 2);
  show(CAR(cell), indent+2, 1);
  for (cell = CDR(cell);!IS_NIL(cell); cell = CDR(cell))
    show(CAR(cell), indent + 2, 0);
  write(1, indenter, indent);
  write(1, ")\n", 2);
}

void show_intern(const char *sym)
{
  printf("%s at %lx\n", sym, (long)intern_string(sym));
}

int main(int argc, char *argv[])
{
  struct cell *cfg = read_cfg_file(argv[1]);

  if (!cfg)
    {
      fprintf(stderr, "%s\n" ,read_cfg_error());
      return 1;
    }
  
  memset(indenter, ' ', sizeof(indenter));
  show(cfg, 0, 1);
  free_cfg(cfg);
  free_symbols();
  return 0;
}
