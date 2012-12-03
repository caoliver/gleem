#include <stdio.h>
#include <err.h>
#include "keywords.h"

#define INLINE_DECL

#define PIXEL_X 1
#define PIXEL_Y 2
#define H_MASK (3 << 2)
#define LEFT (1 << 2)
#define RIGHT (2 << 2)
#define V_MASK (3 << 4)
#define ABOVE (1 << 4)
#define BELOW (2 << 4)

static INLINE_DECL const char *skip_space(const char *str)
{
  while (isspace(*str))
    str++;
  return str;
}

static INLINE_DECL const char *scan_to_space(const char *str)
{
  while (*str && !isspace(*str))
    str++;
  return str;
}

int parse_placement(char *coords, int width, int height, int *x, int *y,
		    int *flags)
{
  long scan;
  char *end, *str = coords;

  *flags &= ~(PIXEL_X | PIXEL_Y);

  scan = strtol(str, &end, 10);
  if (str == end)
    goto bad_num;
  if (*end == '%')
    {
      *flags |= PIXEL_X;
      *x = (width * scan) / 100;
      end++;
    }
  else
    *x += scan;
  if (!isspace(*end))
    goto bad_num;
  str = end;
  scan = strtol(str, &end, 10);
  if (str == end)
    goto bad_num;
  if (*end == '%')
    {
      *flags |= PIXEL_Y;
      *y = (height * scan) / 100;
      end++;
    }
  else
    *y += scan;
  if (!*end)
    return 1;
  if (!isspace(*end))
    goto bad_num;

  str = skip_space(end);
  while (*str)
    {
      end = scan_to_space(str);
      switch (lookup_keyword(str, end-str))
	{
	case KEYWORD_C:
	case KEYWORD_CENTER:
	  *flags &= ~(H_MASK | V_MASK);
	  break;
	case KEYWORD_LEFT:
	  *flags = (*flags & ~H_MASK) | LEFT;
	  break;
	case KEYWORD_RIGHT:
	  *flags = (*flags & ~H_MASK) | RIGHT;
	  break;
	case KEYWORD_ABOVE:
	  *flags = (*flags & ~V_MASK) | ABOVE;
	  break;
	case KEYWORD_BELOW:
	  *flags = (*flags & ~V_MASK) | BELOW;
	  break;
	default:
	  goto bad_num;
	}
      str = skip_space(end);
    }

  return 1;
 bad_num:
  return 0;
}

int main(int argc, char *argv[])
{
  if (argc < 2)
    return 0;
  int x = 50, y = 400, flags = 0;

  if (!parse_placement(argv[1], 1024, 768, &x, &y, &flags))
    errx(1, "Bad coordinates");
  printf("%d %d %x\n", x, y, flags);

  return 0;
}
