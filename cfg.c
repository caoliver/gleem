#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/Xft/Xft.h>
#include <X11/Xresource.h>
#include <X11/XKBlib.h>
#include <ctype.h>
// #include "dm.h"
#include "util.h"
#include "cfg.h"
#include "keywords.h"

#if __STDC_VERSION__ >= 199901L
#define INLINE_DECL inline
#elif defined(__GNUC__)
#define INLINE_DECL __inline
#else
#define INLINE_DECL
#endif

#define UNMANAGE_DISPLAY 0
#define LogError printf

static void get_cfg_boolean(XrmDatabase db,
			    const char *name, const char *class,
			    int *valptr, int default_value)
{
  char *type;
  XrmValue value;

  *valptr =
    XrmGetResource(db, name, class, &type, &value)
    ? (!strcmp(value.addr, "true") ||
       !strcmp(value.addr, "on") ||
       !strcmp(value.addr, "yes"))
    : default_value;
}

static void get_cfg_count(XrmDatabase db,
			    const char *name, const char *class,
			  int *valptr, int default_value)
{
  char *type, *end;
  XrmValue value;

  if (!XrmGetResource(db, name, class, &type, &value))
    {
      *valptr = default_value;
      return;
    }

  *valptr = strtol(value.addr, &end, 10);
  if (end != value.addr &&
      (end - value.addr) == strlen(value.addr) &&
      *valptr >= 0)
    return;

  LogError("Invalid count resource %s for %s %s\n",
	   value.addr, name, class);
  exit(UNMANAGE_DISPLAY);
}

static void get_cfg_string(XrmDatabase db,
			   const char *name, const char *class,
			   char **valptr, char *default_value)
{
  char *type;
  XrmValue value;

  *valptr =
    XrmGetResource(db, name, class, &type, &value)
    ? xstrdup(value.addr)
    : default_value;
}

#define RESOURCE_PREFIX "gleem.command."

#define GETCMD(TYPE, NAME, PLACE, DEFAULT)				\
  sprintf(&name[sizeof(RESOURCE_PREFIX)-1], "%d."#NAME, cmd_id);	\
  get_cfg_##TYPE(db, name, "Gleem", PLACE, DEFAULT)

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

static void
parse_action(struct command *cmd, const char *action, const char *name)
{
  const char *str = action;
  const char *end = scan_to_space(str);
  switch (cmd->action = lookup_keyword(str, end - str))
    {
    case KW_EXEC:
      if (*(str = skip_space(end)))
	{
	  cmd->action_params = xstrdup(str);
	  break;
	}
      LogError("Missing command to exec in %s\n", name);
      exit(UNMANAGE_DISPLAY);

    case KW_SWITCH_SESSION:
      cmd->action_params = *(str = skip_space(end)) ? xstrdup(str) : NULL;
      break;

    default:
      LogError("Invalid action: %s: %s\n", name, str);
      exit(UNMANAGE_DISPLAY);
    }
}

static void
parse_shortcut(Display *dpy, struct command *cmd, const char *shortcut,
	       const char *name)
{
  const char *str = shortcut, *end;
  while (*str)
    {
      if (!*(end = scan_to_space(str)))
	break;

      switch (lookup_keyword(str, end - str))
	{
	case KW_CTRL:
	  cmd->mod_state |= ControlMask;
	  break;
	case KW_SHIFT:
	  cmd->mod_state |= ShiftMask;
	  break;
	case KW_MOD1:
	  cmd->mod_state |= Mod1Mask;
	  break;
	case KW_MOD2:
	  cmd->mod_state |= Mod2Mask;
	  break;
	case KW_MOD3:
	  cmd->mod_state |= Mod3Mask;
	  break;
	case KW_MOD4:
	  cmd->mod_state |= Mod4Mask;
	  break;
	case KW_MOD5:
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
	  return;
	}
    }

  LogError("Bad key symbol in %s: context = %s\n", name, str);
  exit(UNMANAGE_DISPLAY);
}

static void
get_command(Display *dpy, XrmDatabase db, struct command *cmd, int cmd_id)
{
  static char name[48] = RESOURCE_PREFIX;
  char *type;
  XrmValue value;

  sprintf(&name[sizeof(RESOURCE_PREFIX)-1], "%d.action", cmd_id);
  if (!XrmGetResource(db, name, "Gleem", &type, &value))
    return;
  parse_action(cmd, value.addr, name);

  GETCMD(count, delay, &cmd->delay, 0);
  GETCMD(string, name, &cmd->name, NULL);
  GETCMD(string, message, &cmd->message, NULL);
  GETCMD(string, users, &cmd->allowed_users, NULL);

  sprintf(&name[sizeof(RESOURCE_PREFIX)-1], "%d.shortcut", cmd_id);
  if (!XrmGetResource(db, name, "Gleem", &type, &value))
    return;
  parse_shortcut(dpy, cmd, value.addr, name);
}

void free_command(struct command *cmd)
{
  free(cmd->action_params);
  free(cmd->name);
  free(cmd->message);
}

#define GETCFG(DB, TYPE, NAME, PLACE, DEFAULT)			\
  get_cfg_##TYPE(DB, "gleem."#NAME, "Gleem", PLACE, DEFAULT)	\

#define MAX_CMDS 256

struct cfg *read_cfg(Display *dpy)
{
  struct cfg *cfg = xmalloc(sizeof(*cfg));
  XrmDatabase db;
  char *theme_directory, *themes;

  /* db = XrmGetStringDatabase(XResourceManagerString(dpy)); */
  if (!(db = XrmGetFileDatabase("gleem.conf")))
    abort();

  GETCFG(db, boolean, numlock, &cfg->numlock, 0);
  GETCFG(db, boolean, ignore-capslock, &cfg->ignore_capslock, 1);
  GETCFG(db, boolean, hide-mouse, &cfg->hide_mouse, 0);
  GETCFG(db, boolean, auto-login, &cfg->auto_login, 0);
  GETCFG(db, boolean, focus-password, &cfg->focus_password, 0);
  GETCFG(db, string, default-user, &cfg->default_user, NULL);
  GETCFG(db, string, welcome-message, &cfg->welcome_message, NULL);
  GETCFG(db, string, theme-directory, &theme_directory, xstrdup("."));
  GETCFG(db, string, current-theme, &themes, NULL);
  GETCFG(db, string, sessions, &cfg->sessions, NULL);
  cfg->current_session = cfg->sessions;
  GETCFG(db, count, command.scan-to, &cfg->command_count, 16);
  cfg->commands = xcalloc(sizeof(struct command), cfg->command_count);
  if (cfg->command_count > MAX_CMDS)
    cfg->command_count = MAX_CMDS;
  
  for (int i = 0; i < cfg->command_count; i++)
    get_command(dpy, db, &(cfg->commands)[i], i + 1);
  __asm__("int3");
  free(themes);
  free(theme_directory);
  return cfg;
}

void free_cfg(struct cfg *cfg)
{
  for (int i = cfg->command_count; i-- > 0;)
    free_command(&(cfg->commands[i]));
  free(cfg->commands);
  free(cfg->welcome_message);
  free(cfg->default_user);
  free(cfg);
}

#if 1
int main(int argc, char *argv[])
{
  struct cfg *cfg = read_cfg(XOpenDisplay(NULL));
  free_cfg(cfg);
  return 0;
}
#endif
