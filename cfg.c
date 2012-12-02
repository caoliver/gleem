#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/Xft/Xft.h>
#include <X11/Xresource.h>
// #include "dm.h"
#include "util.h"
#include "cfg.h"

#define UNMANAGE_DISPLAY 0
#define LogError printf

static void get_cfg_boolean(XrmDatabase db, char *name, char *class,
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

static void get_cfg_count(XrmDatabase db, char *name, char *class,
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

static void get_cfg_string(XrmDatabase db, char *name, char *class,
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
;
#define GETCMD(TYPE, NAME, PLACE, DEFAULT)				\
  sprintf(&name[sizeof(RESOURCE_PREFIX)-1], "%d."#NAME, command_id);	\
  get_cfg_##TYPE(db, name, "Gleem", PLACE, DEFAULT)

void get_command(XrmDatabase db, struct command *command, int command_id)
{
  static char name[48] = RESOURCE_PREFIX;
  char *type;
  XrmValue value;

  GETCMD(string, action, &command->action, NULL);
  if (!command->action)
    return;

  GETCMD(count, delay, &command->delay, 0);
  GETCMD(string, name, &command->name, NULL);
  GETCMD(string, message, &command->message, NULL);
}

void free_command(struct command *command)
{
  free(command->action);
  free(command->name);
  free(command->message);
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
    get_command(db, &(cfg->commands)[i], i + 1);
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
