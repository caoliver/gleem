#include <X11/Xlib.h>
#include <X11/Xft/Xft.h>
#include "keywords.h"
#include "cfg.h"

char *skip_space(char *), *scan_to_space(char *);

#define SCAN_COORD(COORD, DIMEN, FLAG)		\
  scan = strtol(str, &end, 10);			\
  if (str == end)				\
    goto bad_coord;				\
  if (*end == '%')				\
    {						\
      *flags |= FLAG;				\
      *COORD = (DIMEN * scan) / 100;		\
      end++;					\
    }						\
  else						\
    *COORD += scan

int parse_placement(char *coords, int width, int height, int *x, int *y,
		    int *flags)
{
  long scan;
  char *end, *str = coords;

  *flags &= ~(X_IS_PIXEL_COORD | Y_IS_PIXEL_COORD);
  SCAN_COORD(x, width, X_IS_PIXEL_COORD);
  if (!isspace(*end))
    goto bad_coord;
  str = end;
  SCAN_COORD(y, height, Y_IS_PIXEL_COORD);
  if (!*end)
    return 1;
  if (!isspace(*end))
    goto bad_coord;

  str = skip_space(end);
  while (*str)
    {
      static int flags_true[5] =
	{ PUT_LEFT, PUT_RIGHT, PUT_ABOVE, PUT_BELOW, PUT_CENTER };
      static int flags_masked[5] =
	{ ~HORIZ_MASK, ~HORIZ_MASK, ~VERT_MASK, ~VERT_MASK, 0 };
      int key_num;

      end = scan_to_space(str);
      switch (key_num = lookup_keyword(str, end-str))
	{
	case KEYWORD_C:
	  key_num = KEYWORD_CENTER;
	case KEYWORD_LEFT:
	case KEYWORD_RIGHT:
	case KEYWORD_ABOVE:
	case KEYWORD_BELOW:
	  *flags =
	    (*flags & flags_masked[key_num - 1]) | flags_true[key_num - 1];
	  break;
	default:
	  goto bad_coord;
	}
      str = skip_space(end);
    }

  return 1;
 bad_coord:
  return 0;
}
