// atomizer - A trivial sexpr parser for configuration files.
//
// Christopher Oliver - November 24, 2012

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <setjmp.h>
#include <ctype.h>
#include "util.h"
#include "khash.h"
#include "atomizer.h"

static char err_msg[80];
static int line_number = 1;
static const char *this_file;

static struct cell *alloc_cell(int type)
{
  struct cell *new = xmalloc(sizeof(struct cell));
  CELL_TYPE(new) = type;
  return new;
}

jmp_buf panic;

#define SET_ERR_MSG(MSG, LINE) \
  snprintf(err_msg, 80, "%s : line %d of %s", MSG, LINE, this_file)

#define DIE_IF_FILE_ERROR(ERRFILE)		\
  if (ferror(ERRFILE))				\
    {						\
      snprintf(err_msg, 80, "%s : file %s ",	\
	       strerror(errno), this_file);	\
      longjmp(panic, 1);			\
    }

static int skip_whitespace(FILE *file)
{
  int c;

  while ((c = getc(file)) != EOF)
    {
      if (!isspace(c))
	{
	  ungetc(c, file);
	  return 1;
	}

      if (c == '\n')
	line_number++;
    }
  

  DIE_IF_FILE_ERROR(file);
  return 0;
}

static int skip_comment(FILE *file)
{
  int c;

  while ((c = getc(file)) != EOF)
    if (c == '\n')
      {
	line_number++;
	return 1;
      }
  
  DIE_IF_FILE_ERROR(file);
  return 0;
}

DEFINE_NIL(nil);

static char *readbuf;
static int readbuf_size, readbuf_used;

void add_to_readbuf(int c)
{
  if (readbuf_size <= readbuf_used + 1)
    {
      readbuf_size = readbuf_size == 0 ? 256 : 2 * readbuf_size;
      readbuf = xrealloc(readbuf, readbuf_size);
    }
  readbuf[readbuf_used++] = c;
}

static struct cell *readbuf_to_atom(int line)
{
  readbuf[readbuf_used++] = 0;
  struct cell *new = alloc_cell(ATOM);
  ATOM_NAME(new) = readbuf;
  ATOM_NUMBER(new) = intern_string(&ATOM_NAME(new));
  ATOM_LINE(new) = line;
  DYNAMIC_FLAG(new) = 0;
  ATOM_DATA(new) = 0;
  readbuf_used = 0;
  return new;
}

int hex_table['f'+1] = {
  ['0'] = 1,
  ['1'] = 2,
  ['2'] = 3,
  ['3'] = 4,
  ['4'] = 5,
  ['5'] = 6,
  ['6'] = 7,
  ['7'] = 8,
  ['8'] = 9,
  ['9'] = 10,
  ['a'] = 11,
  ['A'] = 11,
  ['b'] = 12,
  ['B'] = 12,
  ['c'] = 13,
  ['C'] = 13,
  ['d'] = 14,
  ['D'] = 14,
  ['e'] = 15,
  ['E'] = 15,
  ['f'] = 16,
  ['F'] = 16,
};

static struct cell *read_string(FILE *file)
{
  int digit, cvtval, escaped = 0, hex = 0, oct = 0;
  char *specials = "\a\b\f\n\t\r\v", *normals = "abfntrv";
  int starting_line = line_number;

  while (1)
    {
      int c = getc(file);

      if (c != EOF)
	{
	  if (hex)
	    {
	      digit = c <= 'f' ? hex_table[c] : 0;
	      switch (hex)
		{
		case 1:
		  if (digit)
		    {
		      hex++;
		      cvtval = digit - 1;
		      continue;
		    }
		  SET_ERR_MSG("Invalid hex digit", line_number);
		  longjmp(panic, 1);
		case 2:
		  if (digit)
		    {
		      cvtval = (cvtval << 8) | (digit - 1);
		      add_to_readbuf(cvtval);
		      hex = 0;
		      continue;
		    }
		  add_to_readbuf(cvtval);
		  hex = 0;
		}
	    }
	  else if (oct)
	    {
	      digit = (c < '0' || c > '7') ? 0 : c - '0' + 1;
	      if (digit)
		{
		  cvtval = (cvtval << 3) | (digit - 1);
		  switch (oct)
		    {
		    case 1:
		      oct++;
		      continue;
		    case 2:
		      add_to_readbuf(cvtval & 0x0FF);
		      oct = 0;
		      continue;
		    }
		}
	      add_to_readbuf(cvtval & 0x0FF);
	      oct = 0;
	    }
	}

      switch(c)
	{
	case EOF:
	  SET_ERR_MSG("Unterminated string", starting_line);
	  longjmp(panic, 1);
	case '"':
	  if (!escaped)
	    return readbuf_to_atom(starting_line);
	  add_to_readbuf(c);
	  break;
	case '\n':
	  line_number++;
	  add_to_readbuf(c);
	  escaped = 0;
	  break;
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	  if (!escaped)
	    {
	      add_to_readbuf(c);
	      break;
	    }
	  oct = 1;
	  escaped = 0;
	  cvtval = c - '0';
	  break;
	case '\'':
	case '?':
	  add_to_readbuf(c);
	  escaped = 0;
	  break;
	case 'a':
	case 'b':
	case 'f':
	case 'n':
	case 'r':
	case 't':
	case 'v':
	  add_to_readbuf(escaped ? specials[normals - strchr(normals, c)] : c);
	  escaped = 0;
	  break;
	case 'x':
	  if (!escaped)
	    {
	      add_to_readbuf(c);
	      break;
	    }
	  hex = 1;
	  escaped = 0;
	  break;
	case '\\':
	  if (!escaped)
	    {
	      escaped = 1;
	      break;
	    }
	  escaped = 0;
	default:
	  if (escaped)
	    {
	      SET_ERR_MSG("Invalid escape", line_number);
	      longjmp(panic, 1);
	    }
	  add_to_readbuf(c);
	}
    }
}

static struct cell *read_atom(FILE *file)
{
  int done = 0;
  int c;

  do
    {
      c = getc(file);
      if (!(done = (isspace(c) || c == EOF)))
	switch(c)
	  {
	  case ')':
	  case ';':
	    ungetc(c, file);
	    done = 1;
	    break;
	  default:
	    add_to_readbuf(c);
	  }
    }
  while (!done);
  return readbuf_to_atom(c == '\n' ? line_number++ : line_number);
}

static struct cell *read_cell(FILE *file);

static struct cell *read_list(FILE *file, int need_close)
{
  struct cell *head = NULL, **tail = &head;
  struct cell *last_read;
  int starting_line = line_number;

  while ((last_read = read_cell(file)))
    {
      struct cell *new = alloc_cell(CONS);
      *tail = new;
      tail = &CDR(new);
      *tail = NULL;
      CAR(new) = last_read;
    }

  *tail = &nil;

  if (!need_close || getc(file) == ')')
    return head;

  SET_ERR_MSG("Missing closing parenthesis but starts at", starting_line);
  longjmp(panic, 1);
}

static struct cell *read_cell(FILE *file)
{
  int c;

 retry:

  if (!skip_whitespace(file))
    return NULL;

  switch ((c=getc(file)))
    {
    case ')':
      ungetc(c, file);
    case EOF:
      return NULL;
    case ';':
      skip_comment(file);
      goto retry;
    case '(':
      return read_list(file, 1);
    case '"':
      return read_string(file);
    default:
      ungetc(c, file);
      return read_atom(file);
    }
  return NULL;
}

char *read_cfg_error(void)
{
  return err_msg;
}

static int hash_exists;

static void free_cell(struct cell *cell)
{
  if (IS_NIL(cell))
    return;

  // Don't do atoms; the names live in the hash table.
  switch (CELL_TYPE(cell))
    {
    case CONS:
      free_cell(CAR(cell));
      free_cell(CDR(cell));
      break;
    case ATOM:
      if (DYNAMIC_FLAG(cell))
	free(ATOM_DATA(cell));
    }

  free(cell);
}

struct cell *read_cfg_file(const char *filename)
{
  FILE *file = stdin;
  struct cell *result;

  if (filename && !(file = fopen(filename, "r")))
    {
      snprintf(err_msg, 80, "%s: file %s", strerror(errno), filename);
      return NULL;
    }

  this_file = filename ? filename : "*STDIN*";

  // We leak allocs on error, but we're going to exit anyhow.
  if (setjmp(panic))
    return NULL;

  result = read_list(file, 0);
  if (filename)
    fclose(file);
  return result;
}


void free_cfg(struct cell *cfg)
{
  free_cell(cfg);
}

KHASH_MAP_INIT_STR(symbol_table, int)
khash_t(symbol_table) *symbols;

unsigned int intern_string(const char **str)
{
  khint_t hash_index;
  int rc;
  static unsigned int count;

  if (!hash_exists)
    {
      hash_exists = 1;
      symbols = kh_init(symbol_table);
    }

  hash_index = kh_get(symbol_table, symbols, *str);
  if (hash_index != kh_end(symbols))
    {
      *str = kh_key(symbols, hash_index);
      return kh_value(symbols, hash_index);
    }
  char *hashstr = xmalloc(strlen(*str));
  *str = strcpy(hashstr, *str);
  hash_index = kh_put(symbol_table, symbols, hashstr, &rc);
  kh_value(symbols, hash_index) = ++count;
  return count;
}

void free_symbols()
{
  if (readbuf_size)
    {
      readbuf_size = 0;
      readbuf_used = 0;
      free(readbuf);
    }

  if (hash_exists)
    {
      for (khint_t k = kh_begin(symbols); k != kh_end(symbols); k++)
  	if (kh_exist(symbols, k))
  	  free((void *)kh_key(symbols, k));
      kh_destroy(symbol_table, symbols);
      hash_exists = 0;
    }
}


static __inline__ void set_atom_data(struct cell *atom, void *data, int dyn)
{
  if (IS_ATOM(atom))
    {
      if (DYNAMIC_FLAG(atom))
	free(ATOM_DATA(atom));
      DYNAMIC_FLAG(atom) = dyn;
      ATOM_DATA(atom) = data;
    }
}

void set_static_atom_data(struct cell *atom, void *data)
{
  set_atom_data(atom, data, 0);
}

void alloc_dynamic_atom_data(struct cell *atom, size_t size)
{
  set_atom_data(atom, xmalloc(size), 1);
}

struct cell *assoc_key(struct cell *list, unsigned int key)
{
  for (; IS_CONS(list); list = CDR(list))
    {
      struct cell *car = CAR(list);

      if (IS_CONS(car) && IS_ATOM(CAR(car)) && key == ATOM_NUMBER(CAR(car)))
	{
	  struct cell *new = alloc_cell(CONS);
	  CAR(new) = CDR(car);
	  CDR(new) = CDR(list);
	  return new;
	}
    }

  return NULL;
}
