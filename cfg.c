#include <unistd.h>
#include <limits.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/Xft/Xft.h>
#include <X11/Xresource.h>
#include <X11/XKBlib.h>
#include <X11/extensions/Xinerama.h>
#include <stddef.h>
#include <ctype.h>
#include <errno.h>
#include <time.h>

#ifdef TESTGUI
#define LogError printf
#define UNMANAGE_DISPLAY 0
#define RESERVER_DISPLAY 1
#else
#include "dm.h"
#include "greet.h"
#endif

#include "util.h"
#include "keywords.h"
#include "image.h"
#include "cfg.h"

#if __STDC_VERSION__ >= 199901L
#define INLINE_DECL inline
#elif defined(__GNUC__)
#define INLINE_DECL __inline
#else
#define INLINE_DECL
#endif

#define DUMMY_RESOURCE_CLASS "_dummy_"

#define ALLOC_STATIC 0
#define ALLOC_DYNAMIC 1
#define GET_CFG_FAIL -1

#define X_IS_PANEL_COORD 2
#define Y_IS_PANEL_COORD 4
#define PUT_CENTER 0
#define PUT_LEFT 8
#define PUT_RIGHT 16
#define PUT_ABOVE 32
#define PUT_BELOW 64
#define HORIZ_MASK (PUT_RIGHT | PUT_LEFT)
#define VERT_MASK (PUT_ABOVE | PUT_BELOW)
#define CENTER_MASK (HORIZ_MASK | VERT_MASK)

struct _ResourceSpec {
  char *name;
  int (*allocate)(Display *dpy, void *where, char *deflt);
  void (*free)(Display *dpy, void *where);
  void *default_value;
  size_t offset;
  size_t allocated;
};

typedef struct _ResourceSpec ResourceSpec;

static Cfg cfg;

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

static int get_cfg_boolean(Display *dpy, void *valptr, char *bool_name)
{
  switch (lookup_keyword(bool_name, strlen(bool_name)))
    {
    case KEYWORD_TRUE:
      *(int *)valptr = 1;
      return ALLOC_STATIC;
    case KEYWORD_FALSE:
      *(int *)valptr = 0;
      return ALLOC_STATIC;
    }
  
  LogError("Invalid boolean resource %s", bool_name);
  return GET_CFG_FAIL;
}

static int get_cfg_count(Display *dpy, void *valptr, char *num_str)
{
  char *end;

  *(int *)valptr = strtol(num_str, &end, 10);
  if (end != num_str && !*end && *(int *)valptr >= 0)
    return ALLOC_STATIC;

  LogError("Invalid count resource %s\n", num_str);
  return GET_CFG_FAIL;
}

static int get_cfg_xinerama(Display *dpy, void *valptr, char *screen_name)
{
  XineramaScreenInfo *screen_info;
  ScreenSpecs *specs = (ScreenSpecs *)valptr;
  int num_screens, desired_screen = 0;

  int scr = DefaultScreen(dpy);
  specs->total_width = XWidthOfScreen(ScreenOfDisplay(dpy, scr));
  specs->total_height = XHeightOfScreen(ScreenOfDisplay(dpy, scr));

  if (!XineramaIsActive(dpy))
    {
      specs->xoffset = 0;
      specs->yoffset = 0;
      specs->width = specs->total_width;
      specs->height = specs->total_height;
      return ALLOC_STATIC;
    }

  if (!(screen_info = XineramaQueryScreens(dpy, &num_screens)) ||
      num_screens == 0)
    {
      LogError("Xinerama not reporting screens.\n");
      exit(UNMANAGE_DISPLAY);
    }

  char *end;
  int new_value;

  new_value = strtol(screen_name, &end, 10);
  if (screen_name == end || *end || new_value < 0)
    LogError("Invalid Xinerama screen.  Trying screen zero.\n");
  else
    desired_screen = new_value;

  int i;
  for (i = 0; i < num_screens; i++)
    if (screen_info[i].screen_number == desired_screen)
      break;
  if (i == num_screens)
    {
      LogError("Can't find Xinerama screen %d.  Using the first.\n",
	       desired_screen);
      i = 0;
    }

  specs->xoffset = screen_info[i].x_org;
  specs->yoffset = screen_info[i].y_org;
  specs->width = screen_info[i].width;
  specs->height = screen_info[i].height;

  return ALLOC_STATIC;
}

static int get_cfg_string(Display *dpy, void *valptr, char *value)
{
  if (value)
    {
      *(char **)valptr = xstrdup(value);
      return ALLOC_DYNAMIC;
    }
  *(char **)valptr = NULL;
  return ALLOC_STATIC;
}

static int get_cfg_welcome(Display *dpy, void *valptr, char *src_string)
{
  int result_len = 1;
  char *host = NULL, *domain = NULL;
  int host_len, domain_len;

  char *ptr = src_string;

  while (*ptr)
    {
      if (*ptr == '~')
	switch (*++ptr)
	  {
	  case 'h':
	    if (!host)
	      {
		int size = sysconf(_SC_HOST_NAME_MAX) + 1;
		host = xmalloc(size);
		if (gethostname(host, size) == -1)
		  {
		    LogError("gethostname failed: %s\n", strerror(errno));
		    exit(UNMANAGE_DISPLAY);
		  }
		host_len = strlen(host);
		result_len += host_len;
	      }
	    break;
	  case 'd':
	    if (!domain)
	      {
		int size = sysconf(_SC_HOST_NAME_MAX) + 2;
		domain = xmalloc(size);
		domain[0] = '.';
		if (getdomainname(&domain[1], size) == -1)
		  {
		    LogError("getdomainname failed: %s\n", strerror(errno));
		    exit(UNMANAGE_DISPLAY);
		  }
		domain_len =
		  domain[1] && strcmp(&domain[1], "(none)")
		  ? strlen(domain)
		  : 0;
		result_len += domain_len;
	      }
	    break;
	  case '~':
	    result_len++;
	    break;
	  default:
	    LogError(isprint(*ptr)
		     ? "Bad string escape in welcome message: ~%c\n"
		     : "Bad string escape in welcome message: ~0x%02x\n",
		     0x0FF & *ptr);
	    return GET_CFG_FAIL;
	  }
      else
	result_len++;

      ptr++;
    }

  char *result_str = xmalloc(result_len);
  char *dst = result_str;

  ptr = src_string;

  while (*ptr)
    {
      if (*ptr == '~')
	switch (*++ptr)
	  {
	  case 'h':
	    strcpy(dst, host);
	    dst += host_len;
	    break;
	  case 'd':
	    if (!domain_len)
	      break;
	    strcpy(dst, domain);
	    dst += domain_len;
	    break;
	  case '~':
	    *dst++ = *ptr;
	    result_len++;
	    break;
	  }
      else
	{
	  result_len++;
	  *dst++ = *ptr;
	}

      ptr++;
    }

  *dst = '\0';
  *(char **)valptr = result_str;

  return ALLOC_DYNAMIC;
}

static int get_cfg_char(Display *dpy, void *valptr, char *value)
{
  if (strlen(value) != 1)
    {
      LogError("Invalid char value: %s", value);
      return GET_CFG_FAIL;
    }

  *(char *)valptr = value[0];
  return ALLOC_STATIC;
}

static void free_cfg_string(Display *dpy, void *ptr)
{
  free(*(void **)ptr);
  *(void **)ptr = NULL;
}

static int get_cfg_bkgnd_style(Display *dpy, void *valptr, char *style_name)
{
  switch ((*(int *)valptr = lookup_keyword(style_name, strlen(style_name))))
    {
    case KEYWORD_TILE:
    case KEYWORD_CENTER:
    case KEYWORD_STRETCH:
    case KEYWORD_COLOR:
      break;
    default:
      LogError("Invalid background style %s\n", style_name);
      return GET_CFG_FAIL;
    }

  return ALLOC_STATIC;
}

#define SCAN_COORD(COORD, DIMEN, FLAG)					\
  scan = strtol(str, &end, 10);						\
  if (str == end)							\
    goto bad_position;							\
  if (*end == '%')							\
    {									\
      if (pixel_pair)							\
	goto bad_position;						\
      result_position->flags &= ~FLAG;					\
      result_position->COORD =						\
	(cfg.screen_specs.DIMEN * scan) / 100;				\
      end++;								\
    }									\
  else									\
    result_position->COORD = scan; // Needs panel offset added at use.  

static int
get_cfg_position_internal(Display *dpy, void *valptr, char *position_string,
			  int pixel_pair)
{
  long scan;
  XYPosition *result_position = valptr;

  const char *str = position_string;
  char *end;
  result_position->flags |= X_IS_PANEL_COORD | Y_IS_PANEL_COORD;
  SCAN_COORD(x, width, X_IS_PANEL_COORD);
  if (!isspace(*end))
    goto bad_position;
  str = end;
  SCAN_COORD(y, height, Y_IS_PANEL_COORD);
  if (!*end)
    return ALLOC_STATIC;
  if (pixel_pair || !isspace(*end))
    goto bad_position;

  str = skip_space(end);
  while (*str)
    {
      static int flags_true[] =
	{ PUT_LEFT, PUT_RIGHT, PUT_ABOVE, PUT_BELOW, PUT_CENTER };
      static int flags_masked[] =
	{ ~HORIZ_MASK, ~HORIZ_MASK, ~VERT_MASK, ~VERT_MASK, ~CENTER_MASK };
      int key_num;

      end = (char *)scan_to_space(str);
      switch (key_num = lookup_keyword(str, end-str))
	{
	case KEYWORD_LEFT:
	case KEYWORD_RIGHT:
	case KEYWORD_ABOVE:
	case KEYWORD_BELOW:
	case KEYWORD_CENTER:
	  key_num -= KEYWORD_LEFT;
	  result_position->flags =
	    (result_position->flags & flags_masked[key_num]) |
	    flags_true[key_num];
	  break;
	default:
	  goto bad_position;
	}
      str = skip_space(end);
    }

  return ALLOC_STATIC;

 bad_position:
  LogError("Bad position: %s\n", position_string);
  return GET_CFG_FAIL;
}

static int get_cfg_position(Display *dpy, void *valptr, char *posn_string)
{
  return get_cfg_position_internal(dpy, valptr, posn_string, 0);
}

static int get_cfg_pixel_pair(Display *dpy, void *valptr, char *posn_string)
{
  return get_cfg_position_internal(dpy, valptr, posn_string, 1);
}

static int get_cfg_font(Display *dpy, void *valptr, char *font_name)
{
  int scr = DefaultScreen(dpy);

  if ((*(XftFont **)valptr = XftFontOpenName(dpy, scr, font_name)))
    return ALLOC_DYNAMIC;

  LogError("Can't open font %s\n", font_name);
  return GET_CFG_FAIL;
}

static void free_cfg_font(Display *dpy, void *where)
{
  XftFontClose(dpy, *(XftFont **)where);
  *(XftFont **)where = NULL;
}

static int get_cfg_color(Display *dpy, void *valptr, char *color_name)
{
  int scr = DefaultScreen(dpy);
  Colormap colormap = DefaultColormap(dpy, scr);
  Visual *visual = DefaultVisual(dpy, scr);

  if (XftColorAllocName(dpy, visual, colormap, color_name, valptr))
    return ALLOC_DYNAMIC;

  LogError("Ran out of colors\n");
  return GET_CFG_FAIL;
}

static void free_cfg_color(Display *dpy, void *where)
{
  int scr = DefaultScreen(dpy);
  Colormap colormap = DefaultColormap(dpy, scr);
  Visual *visual = DefaultVisual(dpy, scr);

  XftColorFree(dpy, visual, colormap, where);
}

#define STRINGIFY_HELPER(SYMBOL) #SYMBOL
#define STRINGIFY(SYMBOL) STRINGIFY_HELPER(SYMBOL)

#define DECLSTATIC(NAME, ALLOC, DEFAULT, FIELD)		\
  {   STRINGIFY(RNAME_ ## NAME),				\
      ALLOC, NULL,					\
      DEFAULT, offsetof(DECL_TYPE, FIELD), 0 }

#define DECLDYNAMIC(NAME, ALLOC, FREE, DEFAULT, FIELD)	\
  {   STRINGIFY(RNAME_ ## NAME),				\
      ALLOC, FREE,					\
      DEFAULT,						\
      offsetof(DECL_TYPE, FIELD),		\
      offsetof(DECL_TYPE, FIELD##_ALLOC) }

#define DECLSTRING(NAME, DEFAULT, PLACE) \
  DECLDYNAMIC(NAME, get_cfg_string,  free_cfg_string, DEFAULT, PLACE)

#define DECLCOLOR(NAME, DEFAULT, PLACE)					\
  DECLDYNAMIC(NAME, get_cfg_color, free_cfg_color, DEFAULT_##DEFAULT, PLACE)

#define DECLFONT(NAME, DEFAULT, PLACE)					\
  DECLDYNAMIC(NAME, get_cfg_font, free_cfg_font, DEFAULT_##DEFAULT, PLACE)

#define DECLPOSITION(NAME, DEFAULT, PLACE)			\
  DECLSTATIC(NAME, get_cfg_position, DEFAULT_##DEFAULT, PLACE)

#define DECLPIXELPAIR(NAME, DEFAULT, PLACE)				\
  DECLSTATIC(NAME, get_cfg_pixel_pair, DEFAULT_##DEFAULT, PLACE)

#define DECLBOOLEAN(NAME, DEFAULT,PLACE)			\
  DECLSTATIC(NAME, get_cfg_boolean, DEFAULT_##DEFAULT, PLACE)

#define DECLCOUNT(NAME, DEFAULT,PLACE)				\
  DECLSTATIC(NAME, get_cfg_count, DEFAULT_##DEFAULT, PLACE)

#define DECL_TYPE Cfg

static ResourceSpec cfg_resources[] = {
  DECLBOOLEAN(AUTO_LOGIN, AUTO_LOGIN, auto_login),
  DECLBOOLEAN(FOCUS_PASSWORD, FOCUS_PASSWORD, focus_password),
  DECLBOOLEAN(ALLOW_ROOT, ALLOW_ROOT, allow_root),
  DECLBOOLEAN(ALLOW_NULL_PASS, ALLOW_NULL_PASS, allow_null_pass),
  DECLSTRING(DEFAULT_USER, NULL, default_user),
  DECLDYNAMIC(WELCOME_MESSAGE, get_cfg_welcome,  free_cfg_string,
	      DEFAULT_WELCOME_MESSAGE, welcome_message),
  DECLCOUNT(MESSAGE_DURATION, MESSAGE_DURATION, message_duration),
  DECLCOUNT(BAD_PASS_DELAY, BAD_PASS_DELAY, bad_pass_delay),
  DECLSTATIC(XINERAMA_SCREEN, get_cfg_xinerama,
	     DEFAULT_XINERAMA_SCREEN, screen_specs),
  DECLSTRING(EXTENSION_PROGRAM, DEFAULT_EXTENSION_PROGRAM, extension_program),
  DECLSTRING(PASS_PROMPT, DEFAULT_PASS_PROMPT, password_prompt),
  DECLSTRING(USER_PROMPT, DEFAULT_USER_PROMPT, username_prompt),
  DECLSTRING(MSG_BAD_PASS, DEFAULT_MSG_BAD_PASS, msg_bad_pass),
  DECLSTRING(MSG_BAD_SHELL, DEFAULT_MSG_BAD_SHELL, msg_bad_shell),
  DECLSTRING(MSG_PASS_REQD, DEFAULT_MSG_PASS_REQD, msg_pass_reqd),
  DECLSTRING(MSG_NO_LOGIN, DEFAULT_MSG_NO_LOGIN, msg_no_login),
  DECLSTRING(MSG_NO_ROOT, DEFAULT_MSG_NO_ROOT, msg_no_root),
  DECLSTATIC(PASS_DISPLAY, get_cfg_char, DEFAULT_PASS_MASK, password_mask),
};

#define NUM_CFG (sizeof(cfg_resources) / sizeof(ResourceSpec))

static ResourceSpec theme_resources[] = {
  DECLPOSITION(PANEL_POSITION, PANEL_POSN, panel_position),
  DECLPOSITION(MESSAGE_POSITION, MESSAGE_POSN, message_position),
  DECLPOSITION(WELCOME_POSITION, WELCOME_POSN, welcome_position),
  DECLPOSITION(CLOCK_POSITION, CLOCK_POSN, clock_position),
  DECLPOSITION(PASSWORD_PROMPT_POSITION, PASS_PROMPT_POSN,
	       password_prompt_position),
  DECLPOSITION(PASSWORD_INPUT_POSITION, PASS_INPUT_POSN,
	       password_input_position),
  DECLPOSITION(USERNAME_PROMPT_POSITION, USER_PROMPT_POSN,
	       username_prompt_position),
  DECLPOSITION(USERNAME_INPUT_POSITION, USER_INPUT_POSN,
	       username_input_position),
  DECLPIXELPAIR(MESSAGE_SHADOW_OFFSET, MESSAGE_SHADOW_OFFSET,
		message_shadow_offset),
  DECLPIXELPAIR(INPUT_SHADOW_OFFSET, INPUT_SHADOW_OFFSET,
		input_shadow_offset),
  DECLPIXELPAIR(WELCOME_SHADOW_OFFSET, WELCOME_SHADOW_OFFSET,
		welcome_shadow_offset),
  DECLPIXELPAIR(CLOCK_SHADOW_OFFSET, CLOCK_SHADOW_OFFSET,
		clock_shadow_offset),
  DECLPIXELPAIR(PROMPT_SHADOW_OFFSET, PROMPT_SHADOW_OFFSET,
		prompt_shadow_offset),
  DECLPIXELPAIR(CURSOR_SIZE, CURSOR_SIZE, cursor_size),
  DECLCOLOR(CURSOR_COLOR, CURSOR_COLOR, cursor_color),
  DECLCOLOR(BACKGROUND_COLOR, BKGND_COLOR, background_color),
  DECLCOLOR(PANEL_COLOR, PANEL_COLOR, panel_color),
  DECLCOLOR(MESSAGE_COLOR, MESSAGE_COLOR, message_color),
  DECLCOLOR(MESSAGE_SHADOW_COLOR, MESSAGE_SHADOW_COLOR, message_shadow_color),
  DECLCOLOR(WELCOME_COLOR, WELCOME_COLOR, welcome_color),
  DECLCOLOR(WELCOME_SHADOW_COLOR, WELCOME_SHADOW_COLOR, welcome_shadow_color),
  DECLCOLOR(CLOCK_COLOR, CLOCK_COLOR, clock_color),
  DECLCOLOR(CLOCK_SHADOW_COLOR, CLOCK_SHADOW_COLOR, clock_shadow_color),
  DECLCOLOR(PROMPT_COLOR, PROMPT_COLOR, prompt_color),
  DECLCOLOR(PROMPT_SHADOW_COLOR, PROMPT_SHADOW_COLOR, prompt_shadow_color),
  DECLCOLOR(INPUT_COLOR, INPUT_COLOR, input_color),
  DECLCOLOR(INPUT_SHADOW_COLOR, INPUT_SHADOW_COLOR, input_shadow_color),
  DECLCOLOR(INPUT_ALTERNATE_COLOR, INPUT_ALTERNATE_COLOR,
	    input_alternate_color),
  DECLCOLOR(INPUT_HIGHLIGHT_COLOR, INPUT_HIGHLIGHT_COLOR,
	    input_highlight_color),
  DECLFONT(MESSAGE_FONT, MESSAGE_FONT, message_font),
  DECLFONT(WELCOME_FONT, WELCOME_FONT, welcome_font),
  DECLFONT(CLOCK_FONT, CLOCK_FONT, clock_font),
  DECLFONT(INPUT_FONT, INPUT_FONT, input_font),
  DECLFONT(PROMPT_FONT, PROMPT_FONT, prompt_font),
  DECLSTRING(BACKGROUND_FILE, NULL, background_filename),
  DECLSTRING(PANEL_FILE , NULL, panel_filename),
  DECLSTRING(CLOCK_FORMAT , NULL, clock_format),
  DECLBOOLEAN(CURSOR_BLINK, CURSOR_BLINK, cursor_blink),
  DECLBOOLEAN(INPUT_HIGHLIGHT, INPUT_HIGHLIGHT, input_highlight),
  DECLCOUNT(CURSOR_OFFSET, CURSOR_OFFSET, cursor_offset),
  DECLCOUNT(PASSWORD_INPUT_WIDTH, PASS_INPUT_WIDTH, password_input_width),
  DECLCOUNT(USERNAME_INPUT_WIDTH, USER_INPUT_WIDTH, username_input_width),
  DECLCOUNT(INPUT_HEIGHT, INPUT_HEIGHT, input_height),
  DECLSTATIC(BACKGROUND_STYLE, get_cfg_bkgnd_style, DEFAULT_BKGND_STYLE,
	     background_style),
};

#define NUM_THEME (sizeof(theme_resources) / sizeof(ResourceSpec))

int translate_position(XYPosition *posn, int width, int height,
		        Cfg* cfg, int is_text)
{
  if (posn->flags & TRANSLATION_IS_CACHED)
    return 0;
  posn->flags |= TRANSLATION_IS_CACHED;

  switch (posn->flags & HORIZ_MASK)
    {
    case PUT_LEFT:
      posn->x -= width;
      break;
    case PUT_CENTER:
      posn->x -= width / 2;
    }
  if (posn->flags & X_IS_PANEL_COORD)
    posn->x += cfg->panel_position.x;
  switch (posn->flags & VERT_MASK)
    {
    case PUT_ABOVE:
      posn->y -= height;
      break;
    case PUT_CENTER:
      posn->y -= height / 2;
    }
  if (posn->flags & Y_IS_PANEL_COORD)
    posn->y += cfg->panel_position.y;
  if (is_text)
    posn->y += height;
  return 1;
}


static void free_theme_resources(Display *dpy, Cfg *cfg)
{
  ResourceSpec *spec = theme_resources;

  for (int i = 0; i < NUM_THEME ; i++, spec++)
    if (spec->free && *(int *)((char *)cfg + spec->allocated))
      (spec->free)(dpy, (void *)((char *)cfg + spec->offset));

  free_image_buffers(&cfg->panel_image);
  free_image_buffers(&cfg->background_image);
}



static int get_theme(Display *dpy, Cfg *cfg, char *theme_path)
{
  XrmDatabase db;
  int rc = 0;
  char name[48];
  ResourceSpec *spec;
  char *filepath = NULL;

  if (theme_path)
    {
      char *dbname = mkfilepath(2, theme_path, THEME_FILE_NAME);
      db = XrmGetFileDatabase(dbname);
      free(dbname);
      if (!db)
	{
	  LogError("Can't read theme file!\n");
	  goto bugout;
	}
    }
  else
    // Use compiled-in defaults.
    db = XrmGetStringDatabase("");

  strcpy(name, THEME_RESOURCE_PREFIX);
  spec = theme_resources;
  for (int i = 0; i < NUM_THEME ; i++, spec++)
    {
      char *type, *param;
      XrmValue value;

      strcpy(&name[sizeof(THEME_RESOURCE_PREFIX)-1], spec->name);
      param = !XrmGetResource(db, name, DUMMY_RESOURCE_CLASS, &type, &value)
	? spec->default_value
	: value.addr;
      switch ((spec->allocate)(dpy, (char *)cfg + spec->offset, param))
	{
	case GET_CFG_FAIL:
	  LogError("Invalid parameter: %s\n", name);
	  goto bugout;
	case ALLOC_DYNAMIC:
	  *(int *)((char *)cfg + spec->allocated) = 1;
	}
    }

  if (cfg->panel_filename)
    {
      filepath = mkfilepath(2, theme_path, cfg->panel_filename);
      if (!read_image(filepath, &cfg->panel_image))
	{
	  LogError("Missing panel image: %s\n", filepath);
	  goto bugout;
	}
    }

  if (cfg->background_filename)
    {
      free(filepath);
      filepath = mkfilepath(2, theme_path, cfg->background_filename);

      if (!read_image(filepath, &cfg->background_image))
	{
	  LogError("Missing background image: %s\n", filepath);
	  goto bugout;
	}
    }

  XrmDestroyDatabase(db);

  rc = 1;
  goto done;

 bugout:
  free_theme_resources(dpy, cfg);
  LogError("Bad theme: %s\n", theme_path);

 done:
  if (filepath)
    free(filepath);
  return rc;
}


Cfg *get_cfg(Display *dpy)
{
  char *type, *param;
  XrmValue value;
  XrmDatabase db;
  char *theme_dir, *themes;
  static char name[48];

  char *RMString = XResourceManagerString(dpy);
  db = XrmGetStringDatabase(RMString ? RMString : "");

  memset(&cfg, 0, sizeof(cfg));
  strcpy(name, MAIN_RESOURCE_PREFIX);
  ResourceSpec *spec = cfg_resources;
  for (int i = 0; i < NUM_CFG ; i++, spec++)
    {
      strcpy(&name[sizeof(MAIN_RESOURCE_PREFIX)-1], spec->name);
      param = !XrmGetResource(db, name, DUMMY_RESOURCE_CLASS, &type, &value)
	? spec->default_value
	: value.addr;
      switch ((spec->allocate)(dpy, (char *)&cfg + spec->offset, param))
	{
	case GET_CFG_FAIL:
	  LogError("Bad parameter: %s\n", name);
	  exit(UNMANAGE_DISPLAY);
	case ALLOC_DYNAMIC:
	  *(int *)((char *)&cfg + spec->allocated) = 1;
	}
    }
  theme_dir =
    xstrdup(XrmGetResource(db,
			   MAIN_RESOURCE_PREFIX
			   STRINGIFY(RNAME_THEME_DIRECTORY),
			   DUMMY_RESOURCE_CLASS, &type, &value)
	    ? value.addr
	    : DEFAULT_THEME_DIRECTORY);
  themes =
    xstrdup(XrmGetResource(db,
			   MAIN_RESOURCE_PREFIX
			   STRINGIFY(RNAME_THEME_SELECTION),
			   DUMMY_RESOURCE_CLASS, &type, &value)
	    ? value.addr
	    : DEFAULT_THEME_SELECTION);

  XrmDestroyDatabase(db);

  char *theme_list[MAX_THEMES];
  int theme_count = break_tokens(themes, theme_list, MAX_THEMES);  

  srand((int)time(NULL));

  while (theme_count)
    {
      int theme_number = rand() % theme_count;
      char *theme_path = mkfilepath(2, theme_dir, theme_list[theme_number]);
      int success = get_theme(dpy, &cfg, theme_path);
      free(theme_path);
      if (success)
	break;
      theme_list[theme_number] = theme_list[--theme_count];
    }

  if (!theme_count)
    {
      char *theme_path = mkfilepath(2, theme_dir, DEFAULT_THEME_SELECTION);
      int success = get_theme(dpy, &cfg, theme_path);
      free(theme_path);
      if (!success && !get_theme(dpy, &cfg, NULL))
	{
	  LogError("Wow!  Internal theme is bad.\n");
	  exit(UNMANAGE_DISPLAY);
	}
    }

  free(theme_dir);
  free(themes);
  return &cfg;
}
  
void release_cfg(Display *dpy, Cfg *cfg)
{
  ResourceSpec *spec = cfg_resources;
  for (int i = 0; i < NUM_CFG ; i++, spec++)
    {
      if (spec->free && *(int *)((char *)cfg + spec->allocated))
	(spec->free)(dpy, (void *)((char *)cfg + spec->offset));
    }

  free_theme_resources(dpy, cfg);
}
