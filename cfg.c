#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/Xft/Xft.h>
#include <X11/Xresource.h>
#include <X11/XKBlib.h>
#include <stddef.h>
#include <ctype.h>
#include "util.h"
#include "keywords.h"
#include "cfg.h"
#include "defaults.h"

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

#define MAIN_PREFIX "gleem."
#define COMMAND_PREFIX MAIN_PREFIX "command."
#define THEME_PREFIX ""

struct resource_spec {
  char *name;
  int (*allocate)(Display *dpy, XrmDatabase db,
		  const char *name, void *where, void *deflt);
  void (*free)(Display *dpy, void **where);
  void *default_value;
  size_t offset;
  size_t allocated;
};


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
	exit(UNMANAGE_DISPLAY);
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
      (end - value.addr) == strlen(value.addr) &&
      *(int *)valptr >= 0)
    return ALLOC_STATIC;

  LogError("Invalid count resource %s for %s\n", value.addr, name);
  exit(UNMANAGE_DISPLAY);
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
      *(char *)valptr = **(char **)default_value;
      return ALLOC_STATIC;
    }

  *(char *)valptr = value.addr[1];
  return ALLOC_STATIC;
}


static void free_cfg_string(Display *dpy, void **ptr)
{
  free(*ptr);
  *ptr = NULL;
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
      exit(UNMANAGE_DISPLAY);
    }

  return ALLOC_STATIC;
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
	  exit(UNMANAGE_DISPLAY);
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
  exit(UNMANAGE_DISPLAY);
}

static int get_cfg_font(Display *dpy, XrmDatabase db, const char *name,
	      void *valptr, void *default_value)
{
  char *type;
  XrmValue value;
  char *font_name = (char *)default_value;
  int scr = DefaultScreen(dpy);

  if (XrmGetResource(db, name, DUMMY_RESOURCE_CLASS, &type, &value))
    font_name = value.addr;

  if ((*(XftFont **)valptr = XftFontOpenName(dpy, scr, font_name)))
    return ALLOC_DYNAMIC;

  LogError("Can't open font %s\n", font_name);
  exit(UNMANAGE_DISPLAY);
}

static void free_cfg_font(Display *dpy, void **where)
{
  XftFontClose(dpy, *(XftFont **)where);
  *(XftFont **)where = NULL;
}

static int get_cfg_color(Display *dpy, XrmDatabase db, const char *name,
	      void *valptr, void *default_value)
{
  char *type;
  XrmValue value;
  char *color_name = (char *)default_value;
  int scr = DefaultScreen(dpy);
  Colormap colormap = DefaultColormap(dpy, scr);
  Visual *visual = DefaultVisual(dpy, scr);

  if (XrmGetResource(db, name, DUMMY_RESOURCE_CLASS, &type, &value))
    color_name = value.addr;

  if (XftColorAllocName(dpy, visual, colormap, color_name,
			(XftColor *)valptr))
    return ALLOC_DYNAMIC;

  LogError("Ran out of colors\n");
  exit(RESERVER_DISPLAY);
}

static void free_cfg_color(Display *dpy, void **where)
{
  int scr = DefaultScreen(dpy);
  Colormap colormap = DefaultColormap(dpy, scr);
  Visual *visual = DefaultVisual(dpy, scr);

  XftColorFree(dpy, visual, colormap, (XftColor *)where);
}

static void get_cmd_action(XrmDatabase db, const char *name, void *valptr)
{
  char *type;
  XrmValue value;
  struct command *cmd = valptr;

  if (!XrmGetResource(db, name, DUMMY_RESOURCE_CLASS, &type, &value))
    {
      cmd->action = 0;
      return;
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
      exit(UNMANAGE_DISPLAY);

    case KEYWORD_SWITCH_SESSION:
      cmd->action_params = *(str = skip_space(end)) ? xstrdup(str) : NULL;
      break;

    default:
      LogError("Invalid action: %s: %s\n", name, str);
      exit(UNMANAGE_DISPLAY);
    }
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

#define DECL_STRUCT cfg

struct resource_spec cfg_resources[] = {
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
  DECLSTATIC(command.scan-to, get_cfg_count, &default_command_count,
	     command_count),
};

#define NUM_CFG (sizeof(cfg_resources) / sizeof(struct resource_spec))

struct resource_spec theme_resources[] = {
  DECLSTATIC(background-style, get_cfg_bkgnd_style, DEFAULT_BKGND_STYLE,
	     background_style),
  DECLSTATIC(password.input.display, get_cfg_char, DEFAULT_PASS_MASK,
	     password_mask),
  DECLSTRING(password.prompt.string, DEFAULT_PASS_PROMPT, password_prompt),
  DECLSTRING(username.prompt.string, DEFAULT_USER_PROMPT, username_prompt),
  DECLCOLOR(background.color, BKGND_COLOR, background_color),
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

struct resource_spec cmd_resources[] = {
  { "shortcut", get_cmd_shortcut, NULL, NULL, 0, 0 },
  DECLSTATIC(delay, get_cfg_count, &Zero, delay),
  DECLSTRING(name, NULL, name),
  DECLSTRING(message, NULL, message),
  DECLSTRING(users, NULL, allowed_users),
};

#define NUM_CMD (sizeof(cmd_resources) / sizeof(struct resource_spec))

static struct cfg cfg;

struct cfg *get_cfg(Display *dpy)
{
  XrmDatabase db;
  char *theme_directory, *themes;
  static char name[48];

#ifdef TESTING
  if (!(db = XrmGetFileDatabase("gleem.conf")))
    abort();
#endif

  memset(&cfg, 0, sizeof(cfg));
  strcpy(name, MAIN_PREFIX);
  struct resource_spec *spec = cfg_resources;
  for (int i = 0; i < NUM_CFG ; i++, spec++)
    {
      strcpy(&name[sizeof(MAIN_PREFIX)-1], spec->name);
      if ((spec->allocate)(dpy, db, name, (char *)&cfg + spec->offset,
			   spec->default_value))
	*(int *)((char *)&cfg + spec->allocated) = 1;
    }
  cfg.current_session = cfg.sessions;
  get_cfg_string(dpy, db, MAIN_PREFIX "theme-directory",
		 &theme_directory, xstrdup("."));
  get_cfg_string(dpy, db, MAIN_PREFIX "current-theme", &themes, NULL);

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
	  if ((spec->allocate)(dpy, db, name, (char *)cmd + spec->offset,
			       spec->default_value))
	    *(int *)((char *)cmd + spec->allocated) = 1;
	}
    }

  XrmDestroyDatabase(db);

#ifdef TESTING
  if (!(db = XrmGetFileDatabase("theme.conf")))
    abort();
#endif

  strcpy(name, THEME_PREFIX);
  spec = theme_resources;
  for (int i = 0; i < NUM_THEME ; i++, spec++)
    {
      strcpy(&name[sizeof(THEME_PREFIX)-1], spec->name);
      if((spec->allocate)(dpy, db, name, (char *)&cfg + spec->offset,
			  spec->default_value))
	*(int *)((char *)&cfg + spec->allocated) = 1;      
    }

#if defined(TESTING) && defined(BREAK)
  __asm__("int3");
#endif

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
	(spec->free)(dpy, (void **)((char *)cfg + spec->offset));
    }

  spec = theme_resources;
  for (int i = 0; i < NUM_THEME ; i++, spec++)
    {
      if (spec->free && *(int *)((char *)cfg + spec->allocated))
	(spec->free)(dpy, (void **)((char *)cfg + spec->offset));
    }

  for (int i = 0; i < cfg->command_count; i++)
    {
      struct command *cmd = &cfg->commands[i];
      spec = cmd_resources;
      for (int j = 0; j < NUM_CMD ; j++, spec++)
	{
	  if (spec->free && *(int *)((char *)cmd + spec->allocated))
	    (spec->free)(dpy, (void **)((char *)cmd + spec->offset));
	}
      
      free(cmd->action_params);
    }

  free(cfg->commands);
}

int main(int argc, char *argv[])
{
  Display *dpy;

  struct cfg *cfg = get_cfg(dpy = XOpenDisplay(NULL));
  release_cfg(dpy, cfg);
  XCloseDisplay(dpy);

  return 0;
}
