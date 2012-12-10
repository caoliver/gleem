#include <limits.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/Xft/Xft.h>
#include <X11/Xresource.h>
#include <X11/XKBlib.h>
#include <X11/extensions/Xinerama.h>
#include <stddef.h>
#include <ctype.h>
#include "util.h"
#include "keywords.h"
#include "image.h"
#include "defaults.h"
#include "cfg.h"

#if __STDC_VERSION__ >= 199901L
#define INLINE_DECL inline
#elif defined(__GNUC__)
#define INLINE_DECL __inline
#else
#define INLINE_DECL
#endif

#define LogError printf
#define UNMANAGE_DISPLAY 0
#define RESERVER_DISPLAY 1

#define DUMMY_RESOURCE_CLASS "_dummy_"

#define ALLOC_STATIC 0
#define ALLOC_DYNAMIC 1
#define GET_CFG_FAIL -1

#define MAIN_PREFIX "gleem."
#define COMMAND_PREFIX MAIN_PREFIX "command."
#define THEME_PREFIX ""

#define X_IS_PANEL_COORD 2
#define Y_IS_PANEL_COORD 4
#define PUT_CENTER 0
#define PUT_LEFT 8
#define PUT_RIGHT 16
#define PUT_ABOVE 32
#define PUT_BELOW 64
#define HORIZ_MASK (PUT_RIGHT | PUT_LEFT)
#define VERT_MASK (PUT_ABOVE | PUT_BELOW)

struct resource_spec {
  char *name;
  int (*allocate)(Display *dpy, XrmDatabase db,
		  const char *name, void *where, void *deflt);
  void (*free)(Display *dpy, void *where);
  void *default_value;
  size_t offset;
  size_t allocated;
};

static struct cfg cfg;

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

static int get_cfg_boolean(Display *dpy, XrmDatabase db, const char *name,
			   void *valptr, void *default_value)
{
  char *type;
  XrmValue value;

  if (XrmGetResource(db, name, DUMMY_RESOURCE_CLASS, &type, &value))
    switch (lookup_keyword(value.addr, value.size - 1))
      {
      case KEYWORD_TRUE:
	*(int *)valptr = 1;
	break;
      case KEYWORD_FALSE:
	*(int *)valptr = 0;
	break;
      default:
	LogError("Invalid boolean resource %s for %s\n", value.addr, name);
	return GET_CFG_FAIL;
      }
  else
    *(int *)valptr = *(int *)default_value;

  return ALLOC_STATIC;
}

static int get_cfg_count(Display *dpy, XrmDatabase db, const char *name,
			 void *valptr, void *default_value)
{
  char *type, *end;
  XrmValue value;

  if (!XrmGetResource(db, name, DUMMY_RESOURCE_CLASS, &type, &value))
    {
      *(int *)valptr = *(int *)default_value;
      return ALLOC_STATIC;
    }

  *(int *)valptr = strtol(value.addr, &end, 10);
  if (end != value.addr &&
      end - value.addr == value.size - 1 &&
      *(int *)valptr >= 0)
    return ALLOC_STATIC;

  LogError("Invalid count resource %s for %s\n", value.addr, name);
  return GET_CFG_FAIL;
}

static int get_cfg_xinerama(Display *dpy, XrmDatabase db, const char *name,
			    void *valptr, void *default_value)
{
  char *type;
  XrmValue value;
  XineramaScreenInfo *screen_info;
  struct screen_specs *specs = (struct screen_specs *)valptr;
  int num_screens, desired_screen = *(int *)default_value;

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

  if (XrmGetResource(db, name, DUMMY_RESOURCE_CLASS, &type, &value))
    {
      char *end;
      int new_value = strtol(value.addr, &end, 10);
      if (value.addr == end || end - value.addr != value.size - 1)
	LogError("Invalid Xinerama screen.  Using default.\n");
      else
	desired_screen = new_value;
    }

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

static int get_cfg_string(Display *dpy, XrmDatabase db, const char *name,
			  void *valptr, void *default_value)
{
  char *type;
  XrmValue value;

  if (!XrmGetResource(db, name, DUMMY_RESOURCE_CLASS, &type, &value))
    {
      *(char **)valptr = (char *)default_value;
      return ALLOC_STATIC;
    }

  *(char **)valptr = xstrdup(value.addr);
  return ALLOC_DYNAMIC;
}

static int get_cfg_char(Display *dpy, XrmDatabase db, const char *name,
			  void *valptr, void *default_value)
{
  char *type;
  XrmValue value;

  if (!XrmGetResource(db, name, DUMMY_RESOURCE_CLASS, &type, &value))
    {
      *(char *)valptr = *(char *)default_value;
      return ALLOC_STATIC;
    }

  if (value.size > 2)
    return GET_CFG_FAIL;

  *(char *)valptr = value.addr[0];
  return ALLOC_STATIC;
}


static void free_cfg_string(Display *dpy, void *ptr)
{
  free(*(void **)ptr);
  *(void **)ptr = NULL;
}


static int get_cfg_bkgnd_style(Display *dpy, XrmDatabase db, const char *name,
			       void *valptr, void *default_value)
{
  char *type;
  XrmValue value;
  char *style_name = (char *)default_value;

  if (XrmGetResource(db, name, DUMMY_RESOURCE_CLASS, &type, &value))
    style_name = value.addr;

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
get_cfg_position_internal(Display *dpy, XrmDatabase db, const char *name,
			  void *valptr, void *default_value, int pixel_pair)
{
  char *type;
  long scan;
  XrmValue value;
  char *position_string = (char *)default_value;
  struct position *result_position = valptr;

  if (XrmGetResource(db, name, DUMMY_RESOURCE_CLASS, &type, &value))
    position_string = value.addr;

  if (!position_string)
    goto bad_position;

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
      static int flags_true[5] =
	{ PUT_LEFT, PUT_RIGHT, PUT_ABOVE, PUT_BELOW, PUT_CENTER };
      static int flags_masked[5] =
	{ ~HORIZ_MASK, ~HORIZ_MASK, ~VERT_MASK, ~VERT_MASK, 0 };
      int key_num;

      end = (char *)scan_to_space(str);
      switch (key_num = lookup_keyword(str, end-str))
	{
	case KEYWORD_C:
	  key_num = KEYWORD_CENTER;
	case KEYWORD_CENTER:
	case KEYWORD_LEFT:
	case KEYWORD_RIGHT:
	case KEYWORD_ABOVE:
	case KEYWORD_BELOW:
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
  LogError(position_string
	   ? "Bad position for %s: %s\n"
	   : "No position for %s\n",
	   name, position_string);
  return GET_CFG_FAIL;
}

static int get_cfg_position(Display *dpy, XrmDatabase db, const char *name,
			    void *valptr, void *default_value)
{
  return get_cfg_position_internal(dpy, db, name, valptr, default_value, 0);
}

static int get_cfg_pixel_pair(Display *dpy, XrmDatabase db, const char *name,
			      void *valptr, void *default_value)
{
  return get_cfg_position_internal(dpy, db, name, valptr, default_value, 1);
}

static int
get_cmd_shortcut(Display *dpy, XrmDatabase db, const char *name,
		 void *valptr, void *default_value)
{
  char *type;
  XrmValue value;
  struct command *cmd = valptr;

  if (!XrmGetResource(db, name, DUMMY_RESOURCE_CLASS, &type, &value))
    return ALLOC_STATIC;

  const char *str = value.addr, *end;
  while (*str)
    {
      if (!*(end = scan_to_space(str)))
	break;

      switch (lookup_keyword(str, end - str))
	{
	case KEYWORD_CTRL:
	case KEYWORD_C:
	  cmd->mod_state |= ControlMask;
	  break;
	case KEYWORD_SHIFT:
	  cmd->mod_state |= ShiftMask;
	  break;
	case KEYWORD_MOD1:
	  cmd->mod_state |= Mod1Mask;
	  break;
	case KEYWORD_MOD2:
	  cmd->mod_state |= Mod2Mask;
	  break;
	case KEYWORD_MOD3:
	  cmd->mod_state |= Mod3Mask;
	  break;
	case KEYWORD_MOD4:
	  cmd->mod_state |= Mod4Mask;
	  break;
	case KEYWORD_MOD5:
	  cmd->mod_state |= Mod5Mask;
	  break;
	default:
	  LogError("Bad modifier in %s: context = %s\n", name, str);
	  return GET_CFG_FAIL;
	}

      str = skip_space(end);
    }

  KeySym keysym = XStringToKeysym(str);
  if (keysym != NoSymbol)
    {
      char ascii;
      int extra;

      XkbTranslateKeySym(dpy, &keysym, cmd->mod_state, &ascii, 1, &extra);
      if (!IsModifierKey(keysym) &&
	  (!isprint(ascii)
	   || (ascii == ' '  && cmd->mod_state)
	   || (cmd->mod_state & ~ShiftMask)))
	{
	  cmd->keysym = keysym;
	  return ALLOC_STATIC;
	}
    }

  LogError("Bad key symbol in %s: context = %s\n", name, str);
  return GET_CFG_FAIL;
}

static int get_cfg_font(Display *dpy, XrmDatabase db, const char *name,
	      void *valptr, void *default_value)
{
  char *type;
  XrmValue value;
  char *font_name = default_value;
  int scr = DefaultScreen(dpy);

  if (XrmGetResource(db, name, DUMMY_RESOURCE_CLASS, &type, &value))
    font_name = value.addr;

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

static int get_cfg_color(Display *dpy, XrmDatabase db, const char *name,
	      void *valptr, void *default_value)
{
  char *type;
  XrmValue value;
  char *color_name = default_value;
  int scr = DefaultScreen(dpy);
  Colormap colormap = DefaultColormap(dpy, scr);
  Visual *visual = DefaultVisual(dpy, scr);

  if (XrmGetResource(db, name, DUMMY_RESOURCE_CLASS, &type, &value))
    color_name = value.addr;

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

static int get_cmd_action(XrmDatabase db, const char *name, void *valptr)
{
  char *type;
  XrmValue value;
  struct command *cmd = valptr;

  if (!XrmGetResource(db, name, DUMMY_RESOURCE_CLASS, &type, &value))
    {
      cmd->action = 0;
      return GET_CFG_FAIL;
    }

  const char *str = value.addr;
  const char *end = scan_to_space(str);
  switch (cmd->action = lookup_keyword(str, end - str))
    {
    case KEYWORD_EXEC:
      if (*(str = skip_space(end)))
	{
	  cmd->action_params = xstrdup(str);
	  break;
	}
      LogError("Missing command to exec in %s\n", name);
      return GET_CFG_FAIL;

    case KEYWORD_SWITCH_SESSION:
      cmd->action_params = *(str = skip_space(end)) ? xstrdup(str) : NULL;
      break;

    default:
      LogError("Invalid action: %s: %s\n", name, str);
      return GET_CFG_FAIL;
    }

  return ALLOC_STATIC;
}

int Zero = 0;
#define Untrue Zero

int default_message_duration = DEFAULT_MESSAGE_DURATION;
int default_command_count = DEFAULT_COMMAND_COUNT;

#define DECLSTATIC(NAME, ALLOC, DEFAULT, FIELD)		\
  {   #NAME,						\
      ALLOC, NULL,					\
      DEFAULT, offsetof(struct DECL_STRUCT, FIELD), 0 }

#define DECLDYNAMIC(NAME, ALLOC, FREE, DEFAULT, FIELD)	\
  {   #NAME,						\
      ALLOC, FREE,					\
      DEFAULT,						\
      offsetof(struct DECL_STRUCT, FIELD),		\
      offsetof(struct DECL_STRUCT, FIELD##_ALLOC) }

#define DECLSTRING(NAME, DEFAULT, PLACE) \
  DECLDYNAMIC(NAME, get_cfg_string,  free_cfg_string, DEFAULT, PLACE)

#define DECLCOLOR(NAME, DEFAULT, PLACE)					\
  DECLDYNAMIC(NAME, get_cfg_color, free_cfg_color, DEFAULT_##DEFAULT, PLACE)

#define DECLFONT(NAME, DEFAULT, PLACE)					\
  DECLDYNAMIC(NAME, get_cfg_font, free_cfg_font, DEFAULT_##DEFAULT, PLACE)

#define DECLPOSITION(NAME, DEFAULT, PLACE)				\
  DECLSTATIC(NAME, get_cfg_position, DEFAULT_##DEFAULT, PLACE)
#define DECLPIXELPAIR(NAME, DEFAULT, PLACE)				\
  DECLSTATIC(NAME, get_cfg_pixel_pair, DEFAULT_##DEFAULT, PLACE)

#define DECL_STRUCT cfg

static struct resource_spec cfg_resources[] = {
  DECLSTATIC(ignore-capslock, get_cfg_boolean, &Untrue, ignore_capslock),
  DECLSTATIC(numlock, get_cfg_boolean, &Untrue, numlock),
  DECLSTATIC(hide-mouse, get_cfg_boolean, &Untrue, hide_mouse),
  DECLSTATIC(auto-login, get_cfg_boolean, &Untrue, auto_login),
  DECLSTATIC(focus-password, get_cfg_boolean, &Untrue, auto_login),
  DECLSTRING(default-user, NULL, default_user),
  DECLSTRING(welcome-message, DEFAULT_WELCOME_MESSAGE, welcome_message),
  DECLSTRING(sessions, NULL, sessions),
  DECLSTATIC(message-duration, get_cfg_count, &default_message_duration,
	     message_duration),
  DECLSTATIC(xinerama-screen, get_cfg_xinerama, &Zero, screen_specs),
  DECLSTATIC(command.scan-to, get_cfg_count, &default_command_count,
	     command_count),
};

#define NUM_CFG (sizeof(cfg_resources) / sizeof(struct resource_spec))

static struct resource_spec theme_resources[] = {
  DECLPOSITION(panel.position, PANEL_POSN, PANEL_POSITION_NAME),
  DECLPOSITION(message.position, MESSAGE_POSN, message_position),
  DECLPOSITION(welcome.position, WELCOME_POSN, welcome_position),
  DECLPOSITION(password.prompt.position, PASS_PROMPT_POSN,
	       password_prompt_position),
  DECLPOSITION(password.input.position, PASS_INPUT_POSN,
	       password_input_position),
  DECLPOSITION(username.prompt.position, USER_PROMPT_POSN,
	       username_prompt_position),
  DECLPOSITION(username.input.position, USER_INPUT_POSN,
	       username_input_position),
  DECLPIXELPAIR(message.shadow.offset, MESSAGE_SHADOW_OFFSET,
		message_shadow_offset),
  DECLPIXELPAIR(input.shadow.offset, INPUT_SHADOW_OFFSET,
		input_shadow_offset),
  DECLPIXELPAIR(welcome.shadow.offset, WELCOME_SHADOW_OFFSET,
		welcome_shadow_offset),
  DECLPIXELPAIR(password.prompt.shadow.offset, PASS_PROMPT_SHADOW_OFFSET,
		pass_prompt_shadow_offset),
  DECLPIXELPAIR(username.prompt.shadow.offset, USER_PROMPT_SHADOW_OFFSET,
		user_prompt_shadow_offset),
  DECLPIXELPAIR(password.input.size, PASS_INPUT_SIZE, password_input_size),
  DECLPIXELPAIR(username.input.size, USER_INPUT_SIZE, username_input_size),
  DECLSTATIC(background-style, get_cfg_bkgnd_style, DEFAULT_BKGND_STYLE,
	     background_style),
  DECLSTATIC(password.input.display, get_cfg_char, DEFAULT_PASS_MASK,
	     password_mask),
  DECLSTRING(background.file, NULL, background_filename),
  DECLSTRING(panel.file, NULL, panel_filename),
  DECLSTRING(password.prompt.string, DEFAULT_PASS_PROMPT, password_prompt),
  DECLSTRING(username.prompt.string, DEFAULT_USER_PROMPT, username_prompt),
  DECLCOLOR(background.color, BKGND_COLOR, background_color),
  DECLCOLOR(panel.color, PANEL_COLOR, panel_color),
  DECLCOLOR(message.color, MESSAGE_COLOR, message_color),
  DECLCOLOR(message.shadow.color, MESSAGE_SHADOW_COLOR, message_shadow_color),
  DECLCOLOR(welcome.color, WELCOME_COLOR, welcome_color),
  DECLCOLOR(welcome.shadow.color, WELCOME_SHADOW_COLOR, welcome_shadow_color),
  DECLCOLOR(pass.prompt.color, PASS_COLOR, pass_color),
  DECLCOLOR(pass.prompt.shadow.color, PASS_SHADOW_COLOR, pass_shadow_color),
  DECLCOLOR(user.prompt.color, USER_COLOR, user_color),
  DECLCOLOR(user.prompt.shadow.color, USER_SHADOW_COLOR, user_shadow_color),
  DECLCOLOR(input.color, INPUT_COLOR, input_color),
  DECLCOLOR(input.shadow.color, INPUT_SHADOW_COLOR, input_shadow_color),
  DECLCOLOR(input.alternate.color, INPUT_ALTERNATE_COLOR,
	    input_alternate_color),
  DECLFONT(message.font, MESSAGE_FONT, message_font),
  DECLFONT(welcome.font, WELCOME_FONT, welcome_font),
  DECLFONT(input.font, INPUT_FONT, input_font),
  DECLFONT(password.prompt.font, PASS_FONT, pass_font),
  DECLFONT(username.prompt.font, USER_FONT, user_font),
};

#define NUM_THEME (sizeof(theme_resources) / sizeof(struct resource_spec))

#undef DECL_STRUCT
#define DECL_STRUCT command

static struct resource_spec cmd_resources[] = {
  { "shortcut", get_cmd_shortcut, NULL, NULL, 0, 0 },
  DECLSTATIC(delay, get_cfg_count, &Zero, delay),
  DECLSTRING(name, NULL, name),
  DECLSTRING(message, NULL, message),
  DECLSTRING(users, NULL, allowed_users),
};

#define NUM_CMD (sizeof(cmd_resources) / sizeof(struct resource_spec))

void translate_position(struct position *posn, int width, int height,
		        struct cfg* cfg, int is_text)
{
  if (posn->flags & TRANSLATION_IS_CACHED)
    return;

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
    posn->x += cfg->PANEL_POSITION_NAME.x;
  switch (posn->flags & VERT_MASK)
    {
    case PUT_ABOVE:
      posn->y -= height;
      break;
    case PUT_CENTER:
      posn->y -= height / 2;
    }
  if (posn->flags & Y_IS_PANEL_COORD)
    posn->y += cfg->PANEL_POSITION_NAME.y;
  if (is_text)
    posn->y += height;
}


static void free_theme_resources(Display *dpy, struct cfg *cfg)
{
  struct resource_spec *spec = theme_resources;

  for (int i = 0; i < NUM_THEME ; i++, spec++)
    if (spec->free && *(int *)((char *)cfg + spec->allocated))
      (spec->free)(dpy, (void *)((char *)cfg + spec->offset));

  free_image_buffers(&cfg->panel_image);
  free_image_buffers(&cfg->background_image);
}


static int get_theme(Display *dpy, struct cfg *cfg, char *theme_path)
{
  XrmDatabase db;
  int rc = 0;
  char *olddir = realpath(".", NULL);
  char name[48];
  struct resource_spec *spec;

  if (!olddir)
    out_of_memory();

  if (theme_path)
    {
      if (chdir(theme_path))
	goto bugout;
      if (!(db = XrmGetFileDatabase(THEME_FILE_NAME)))
	goto bugout;
    }
  else
    // Use compiled-in defaults.
    db = XrmGetStringDatabase("");

  strcpy(name, THEME_PREFIX);
  spec = theme_resources;
  for (int i = 0; i < NUM_THEME ; i++, spec++)
    {
      strcpy(&name[sizeof(THEME_PREFIX)-1], spec->name);
      switch ((spec->allocate)(dpy, db, name, (char *)cfg + spec->offset,
			  spec->default_value))
	{
	case GET_CFG_FAIL:
	  free_theme_resources(dpy, cfg);
	  goto bugout;
	  break;
	case ALLOC_DYNAMIC:
	  *(int *)((char *)cfg + spec->allocated) = 1;
	}
    }

  if (cfg->panel_filename
      && !read_image(cfg->panel_filename, &cfg->panel_image))
    goto bugout;

  if (cfg->background_filename
      && !read_image(cfg->background_filename, &cfg->background_image))
    goto bugout;

  if (chdir(olddir))
    {
      LogError("Bad chdir");
      exit(UNMANAGE_DISPLAY);
    }

  XrmDestroyDatabase(db);

  rc = 1;
  goto done;

 bugout:
  LogError("Bad theme %s\n", theme_path);

 done:
  free(olddir);
  return rc;
}


struct cfg *get_cfg(Display *dpy)
{
  XrmDatabase db;
  char *theme_directory, *themes;
  static char name[48];

#if defined(TESTCFG) || !defined(NOZEP)
  db = XrmGetFileDatabase("gleem.conf");
#else
  char *RMString = XResourceManagerString(dpy);
  db = XrmGetStringDatabase(RMString ? RMString : "");
#endif

  memset(&cfg, 0, sizeof(cfg));
  strcpy(name, MAIN_PREFIX);
  struct resource_spec *spec = cfg_resources;
  for (int i = 0; i < NUM_CFG ; i++, spec++)
    {
      strcpy(&name[sizeof(MAIN_PREFIX)-1], spec->name);
      switch ((spec->allocate)(dpy, db, name, (char *)&cfg + spec->offset,
			       spec->default_value))
	{
	case GET_CFG_FAIL:
	  exit(UNMANAGE_DISPLAY);
	case ALLOC_DYNAMIC:
	  *(int *)((char *)&cfg + spec->allocated) = 1;
	}
    }
  cfg.current_session = cfg.sessions;
  get_cfg_string(dpy, db, MAIN_PREFIX "theme-directory",
		 &theme_directory, xstrdup("./themes"));
  get_cfg_string(dpy, db, MAIN_PREFIX "current-theme", &themes,
		 xstrdup("default"));

  if (cfg.command_count < 16)
    cfg.command_count = 16;
  else if (cfg.command_count > 100)
    cfg.command_count = 100;
  cfg.commands = xcalloc(sizeof(struct command), cfg.command_count);

  strcpy(name, COMMAND_PREFIX);
  for (int cmd_id = 0; cmd_id < cfg.command_count; cmd_id++)
    {
      struct command *cmd = &cfg.commands[cmd_id];
      char *rest_of_name = &name[sizeof(COMMAND_PREFIX)-1];
      rest_of_name += sprintf(rest_of_name, "%d.", cmd_id + 1);
      strcpy(rest_of_name, "action");
      get_cmd_action(db, name, cmd);
      if (!cmd->action)
	continue;
      spec = cmd_resources;
      for (int i = 0; i < NUM_CMD ; i++, spec++)
	{
	  strcpy(rest_of_name, spec->name);
	  switch ((spec->allocate)(dpy, db, name, (char *)cmd + spec->offset,
				   spec->default_value))
	    {
	    case GET_CFG_FAIL:
	      exit(UNMANAGE_DISPLAY);
	    case ALLOC_DYNAMIC:
	      *(int *)((char *)cmd + spec->allocated) = 1;
	    }
	}
    }

  XrmDestroyDatabase(db);

  // Find random theme here.

  get_theme(dpy, &cfg, ("themes",NULL));

  free(theme_directory);
  free(themes);
  return &cfg;
}

void release_cfg(Display *dpy, struct cfg *cfg)
{
  struct resource_spec *spec = cfg_resources;
  for (int i = 0; i < NUM_CFG ; i++, spec++)
    {
      if (spec->free && *(int *)((char *)cfg + spec->allocated))
	(spec->free)(dpy, (void *)((char *)cfg + spec->offset));
    }

  free_theme_resources(dpy, cfg);

  for (int i = 0; i < cfg->command_count; i++)
    {
      struct command *cmd = &cfg->commands[i];
      spec = cmd_resources;
      for (int j = 0; j < NUM_CMD ; j++, spec++)
	{
	  if (spec->free && *(int *)((char *)cmd + spec->allocated))
	    (spec->free)(dpy, (void *)((char *)cmd + spec->offset));
	}
      
      free(cmd->action_params);
    }

  free(cfg->commands);
}

#ifdef TESTCFG
int main(int argc, char *argv[])
{
  Display *dpy = XOpenDisplay(NULL);

  if (dpy)
    get_cfg(dpy);
  
  return 0;
}
#endif
