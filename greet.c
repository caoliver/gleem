#include <X11/Xlib.h>
#include <X11/Xft/Xft.h>
#include <time.h>

#include "dm.h"
#include "dm_error.h"
#include "greet.h"

#include <ctype.h>
#include <stdlib.h>
#include <err.h>

#include <poll.h>

#include "image.h"
#include "cfg.h"
#include "gfx.h"

/*
 * Function pointers filled in by the initial call ito the library
 */

int     (*__xdm_PingServer)(struct display *d, Display *alternateDpy) = NULL;
void    (*__xdm_SessionPingFailed)(struct display *d) = NULL;
void    (*__xdm_Debug)(const char * fmt, ...) = NULL;
void    (*__xdm_RegisterCloseOnFork)(int fd) = NULL;
void    (*__xdm_SecureDisplay)(struct display *d, Display *dpy) = NULL;
void    (*__xdm_UnsecureDisplay)(struct display *d, Display *dpy) = NULL;
void    (*__xdm_ClearCloseOnFork)(int fd) = NULL;
void    (*__xdm_SetupDisplay)(struct display *d) = NULL;
void    (*__xdm_LogError)(const char * fmt, ...) = NULL;
void    (*__xdm_SessionExit)(struct display *d, int status, int removeAuth) = NULL;
void    (*__xdm_DeleteXloginResources)(struct display *d, Display *dpy) = NULL;
int     (*__xdm_source)(char **environ, char *file) = NULL;
char    **(*__xdm_defaultEnv)(void) = NULL;
char    **(*__xdm_setEnv)(char **e, char *name, char *value) = NULL;
char    **(*__xdm_putEnv)(const char *string, char **env) = NULL;
char    **(*__xdm_parseArgs)(char **argv, char *string) = NULL;
void    (*__xdm_printEnv)(char **e) = NULL;
char    **(*__xdm_systemEnv)(struct display *d, char *user, char *home) = NULL;
void    (*__xdm_LogOutOfMem)(const char * fmt, ...) = NULL;
void    (*__xdm_setgrent)(void) = NULL;
struct group    *(*__xdm_getgrent)(void) = NULL;
void    (*__xdm_endgrent)(void) = NULL;
struct spwd   *(*__xdm_getspnam)(GETSPNAM_ARGS) = NULL;
void   (*__xdm_endspent)(void) = NULL;
struct passwd   *(*__xdm_getpwnam)(GETPWNAM_ARGS) = NULL;
void   (*__xdm_endpwent)(void) = NULL;
char     *(*__xdm_crypt)(CRYPT_ARGS) = NULL;


static int InitGreet(struct display *d, Gfx *gfx)
{
  memset(gfx, 0, sizeof(*gfx));
  Display *dpy = gfx->dpy = XOpenDisplay(d->name);
  if (!dpy)
    return 0;
  RegisterCloseOnFork(ConnectionNumber(dpy));
  SecureDisplay(d, dpy);
  int scr = gfx->screen = DefaultScreen(dpy);
  gfx->root_win = RootWindow(dpy, scr);
  XSetWindowBackground(dpy, gfx->root_win, BlackPixel(dpy, scr));
  XClearWindow(dpy, gfx->root_win);
  gfx->colormap = DefaultColormap(dpy, gfx->screen);
  gfx->visual = DefaultVisual(dpy, gfx->screen);
  return 1;
}

static void CloseGreet(struct display *d, Gfx *gfx)
{
  Display *dpy = gfx->dpy;
  XDestroyWindow(dpy, gfx->background_win);
  XClearWindow(dpy, RootWindow(dpy, gfx->screen));
  UnsecureDisplay(d, dpy);
  ClearCloseOnFork(XConnectionNumber(dpy));
  XCloseDisplay(dpy);
}

__inline__ static void raise_button_bar()
{
  dprintf(1, "\nRaise button bar\n");
}

#define BUFFER_LEN 128
#define USERNAME 0
#define PASSWORD 1

char input_buffer[2][BUFFER_LEN];
int input_buffer_ix[2];

__inline__ static void wipe_char(int bufferix)
{
  if (input_buffer_ix[bufferix] > 0)
    input_buffer[bufferix][--input_buffer_ix[bufferix]] = 0;
}

__inline__ static void wipe_field(int bufferix)
{
  input_buffer_ix[bufferix] = 0;
  input_buffer[bufferix][0] = 0;
}

int reuse_input_area;
TextAttrs *PromptAttrsPtr, *ClockAttrsPtr;

void show_input_prompts(Cfg *cfg, Gfx *gfx, int active_field)
{
  static int old_field = -1;

  if (!reuse_input_area)
    {
      SHOW_TEXT_AT(gfx, cfg, PromptAttrsPtr,
		   &cfg->username_prompt_position,
		   cfg->input_height, cfg->username_prompt);
      SHOW_TEXT_AT(gfx, cfg, PromptAttrsPtr,
		   &cfg->password_prompt_position,
		   cfg->input_height, cfg->password_prompt);
    }
  else if (active_field == 1)
    {
      CLEAR_TEXT_AT(gfx, cfg, PromptAttrsPtr,
		    &cfg->username_prompt_position,
		    cfg->input_height, cfg->username_prompt);
      SHOW_TEXT_AT(gfx, cfg, PromptAttrsPtr,
		   &cfg->password_prompt_position,
		   cfg->input_height, cfg->password_prompt);
    }
  else
    {
      CLEAR_TEXT_AT(gfx, cfg, PromptAttrsPtr,
		    &cfg->password_prompt_position,
		    cfg->input_height, cfg->password_prompt);
      SHOW_TEXT_AT(gfx, cfg, PromptAttrsPtr,
		   &cfg->username_prompt_position,
		   cfg->input_height, cfg->username_prompt);
    }
}

void show_input_fields(Cfg *cfg, Gfx *gfx, int active_field)
{
  if (!reuse_input_area)
    if (active_field)
      show_input_at(gfx, cfg, input_buffer[0], &cfg->username_input_position, 
		    cfg->username_input_width, 0, 0);
    else
      show_input_at(gfx, cfg, input_buffer[1], &cfg->password_input_position, 
		    cfg->password_input_width, 0, 1);

  if (!active_field)
    show_input_at(gfx, cfg, input_buffer[0], &cfg->username_input_position, 
		  cfg->username_input_width, 1, 0);
  else
    show_input_at(gfx, cfg, input_buffer[1], &cfg->password_input_position, 
		  cfg->password_input_width, 1, 1);

  activate_cursor(gfx, cfg, 1);
}

__inline__ static void show_clock(Cfg *cfg, Gfx *gfx)
{
  static char out[256];
  static int needs_erasing;

  if (needs_erasing)
    CLEAR_TEXT_AT(gfx, cfg, ClockAttrsPtr, &cfg->clock_position, 0,
		  out);
  needs_erasing = 0;
  time_t t = time(NULL);
  struct tm *tm = localtime(&t);
  if (tm && strftime(out, sizeof(out), cfg->clock_format, tm))
    {
      needs_erasing = 1;
      SHOW_TEXT_AT(gfx, cfg, ClockAttrsPtr, &cfg->clock_position, 0,
		   out);
    }
}


_X_EXPORT
greet_user_rtn GreetUser(
    struct display          *d,
    Display          **display,
    struct verify_info      *verify,
    struct greet_info       *greet,
    struct dlfuncs        *dlfuncs)

{
  Display *dpy;
  Pixmap pixmap;
  Cfg *cfg;
  Gfx gfx;


  // Stop program for debugging.
  //    raise(SIGSTOP);
  //   __asm__("int3");

  /*
   * These must be set before they are used.
   */
  __xdm_PingServer = dlfuncs->_PingServer;
  __xdm_SessionPingFailed = dlfuncs->_SessionPingFailed;
  __xdm_Debug = dlfuncs->_Debug;
  __xdm_RegisterCloseOnFork = dlfuncs->_RegisterCloseOnFork;
  __xdm_SecureDisplay = dlfuncs->_SecureDisplay;
  __xdm_UnsecureDisplay = dlfuncs->_UnsecureDisplay;
  __xdm_ClearCloseOnFork = dlfuncs->_ClearCloseOnFork;
  __xdm_SetupDisplay = dlfuncs->_SetupDisplay;
  __xdm_LogError = dlfuncs->_LogError;
  __xdm_SessionExit = dlfuncs->_SessionExit;
  __xdm_DeleteXloginResources = dlfuncs->_DeleteXloginResources;
  __xdm_source = dlfuncs->_source;
  __xdm_defaultEnv = dlfuncs->_defaultEnv;
  __xdm_setEnv = dlfuncs->_setEnv;
  __xdm_putEnv = dlfuncs->_putEnv;
  __xdm_parseArgs = dlfuncs->_parseArgs;
  __xdm_printEnv = dlfuncs->_printEnv;
  __xdm_systemEnv = dlfuncs->_systemEnv;
  __xdm_LogOutOfMem = dlfuncs->_LogOutOfMem;
  __xdm_setgrent = dlfuncs->_setgrent;
  __xdm_getgrent = dlfuncs->_getgrent;
  __xdm_endgrent = dlfuncs->_endgrent;
  __xdm_getspnam = dlfuncs->_getspnam;
  __xdm_endspent = dlfuncs->_endspent;
  __xdm_getpwnam = dlfuncs->_getpwnam;
  __xdm_endpwent = dlfuncs->_endpwent;
  __xdm_crypt = dlfuncs->_crypt;

  if (!(InitGreet(d, &gfx)))
    {
      LogError ("Cannot reopen display %s for greet window\n", d->name);
      exit (RESERVER_DISPLAY);
    }
  dpy = gfx.dpy;
  cfg = get_cfg(dpy);

  reuse_input_area = 
    (cfg->username_input_position.x == cfg->password_input_position.x &&
     cfg->username_input_position.y == cfg->password_input_position.y &&
     cfg->username_input_position.flags == cfg->password_input_position.flags);
    
  gfx.background_win = XCreateSimpleWindow(dpy, RootWindow(dpy, gfx.screen),
					   cfg->screen_specs.xoffset,
					   cfg->screen_specs.yoffset,
					   cfg->screen_specs.width,
					   cfg->screen_specs.height,
					   0, 0, 255);
  gfx.background_draw = XftDrawCreate(dpy, gfx.background_win,
				      gfx.visual, gfx.colormap);
  if (cfg->background_image.width > 0)
    resize_background(&cfg->background_image, cfg->screen_specs.width,
		      cfg->screen_specs.height);
  else
    frame_background(&cfg->background_image,
		     cfg->screen_specs.width, cfg->screen_specs.height,
		     cfg->screen_specs.width + 1, 0, &cfg->background_color);
  pixmap = imageToPixmap(dpy, &cfg->background_image,
			 gfx.screen, gfx.background_win);
  XSetWindowBackgroundPixmap(dpy, gfx.background_win, pixmap);
  XClearWindow(dpy, gfx.background_win);
  XFreePixmap(dpy, pixmap);
  if (cfg->panel_image.width == 0)
    {
      cfg->input_highlight = 1;
      frame_background(&cfg->panel_image,
		       DEFAULT_PANEL_WIDTH, DEFAULT_PANEL_HEIGHT,
		       cfg->screen_specs.width + 1, 0, &cfg->panel_color);
    }
  TRANSLATE_POSITION(&cfg->panel_position, cfg->panel_image.width,
		     cfg->panel_image.height, cfg, 0);
  merge_with_background(&cfg->panel_image,
			&cfg->background_image,
			TO_XY(cfg->panel_position));
  free_image_buffers(&cfg->background_image);
  gfx.panel_win = XCreateSimpleWindow(dpy, gfx.background_win,
				      TO_XY(cfg->panel_position),
				      cfg->panel_image.width,
				      cfg->panel_image.height,
				      0, 0, 255);
  gfx.panel_draw = XftDrawCreate(dpy, gfx.panel_win,
				 gfx.visual, gfx.colormap);
  pixmap = imageToPixmap(dpy, &cfg->panel_image, gfx.screen, gfx.panel_win);
  free_image_buffers(&cfg->panel_image);
  XSetWindowBackgroundPixmap(dpy, gfx.panel_win, pixmap);
  XClearWindow(dpy, gfx.panel_win);
  XFreePixmap(dpy, pixmap);
  XMapWindow(dpy, gfx.background_win);
  XMapWindow(dpy, gfx.panel_win);

  set_cursor_dimensions(&gfx, cfg,
                        cfg->cursor_size.x, cfg->cursor_size.y,
                        cfg->cursor_offset);

  XSelectInput(dpy, gfx.root_win, KeyPressMask);
  XSelectInput(dpy, gfx.panel_win, ExposureMask);

  struct pollfd pfd = {0};
  pfd.fd = ConnectionNumber(dpy);
  pfd.events = POLLIN;

  int again = 1;
  int which_field = 0;
  TextAttrs WelcomeAttrs = {
    cfg->welcome_font, &cfg->welcome_color,
    &cfg->welcome_shadow_color, &cfg->welcome_shadow_offset
  };
  TextAttrs PromptAttrs = {
    cfg->prompt_font, &cfg->prompt_color,
    &cfg->prompt_shadow_color, &cfg->prompt_shadow_offset
  };
  PromptAttrsPtr = &PromptAttrs;
  TextAttrs ClockAttrs = {
    cfg->clock_font, &cfg->clock_color,
    &cfg->clock_shadow_color, &cfg->clock_shadow_offset
  };
  ClockAttrsPtr = &ClockAttrs;

  SHOW_TEXT_AT(&gfx, cfg, &WelcomeAttrs, &cfg->welcome_position,
	       0, cfg->welcome_message);
  show_input_prompts(cfg, &gfx, 0);
  show_input_fields(cfg, &gfx, 0);
  while (again)
    {
      if (cfg->clock_format)
	show_clock(cfg, &gfx);

      if (!XPending(dpy))
	{
	  switch (poll(&pfd, 1, CURSOR_BLINK_SPEED))
	    {
	    case -1:
	      continue;
	    case 0:
	      activate_cursor(&gfx, cfg,
			      !cfg->cursor_blink || !get_cursor_state(&gfx));
	      break;
	    }
	}

      while (XPending(dpy))
	{
	  XEvent event;
	  KeySym keysym;
	  XComposeStatus compstatus;
	  char ascii;
	  
	  XNextEvent(dpy, &event);
	  switch (event.type)
	    {
	    case Expose:
	      show_input_prompts(cfg, &gfx, which_field);
	      SHOW_TEXT_AT(&gfx, cfg, &WelcomeAttrs, &cfg->welcome_position,
			   0, cfg->welcome_message);
	      break;
	    case KeyPress:
	      XLookupString(&event.xkey, &ascii, 1, &keysym, &compstatus);
	      switch (keysym)
		{
		case XK_Delete:
		case XK_BackSpace:
		  if (((XKeyEvent *) & event)->state & Mod1Mask)
		    {
		      wipe_field(which_field);
		      break;
		    }
		  if (((XKeyEvent *) & event)->state & ControlMask)
		    {
		      which_field = 0;
		      show_input_prompts(cfg, &gfx, which_field);
		      wipe_field(0);
		      wipe_field(1);
		      break;
		    }
		  wipe_char(which_field);
		  break;
		case XK_Tab:
		  which_field = !which_field;
		  wipe_field(1);
		  show_input_prompts(cfg, &gfx, which_field);
		  break;
		case XK_Return:
		case XK_KP_Enter:
		  if (!which_field)
		    {
		      which_field = 1;
		      wipe_field(1);
		      show_input_prompts(cfg, &gfx, which_field);
		      break;
		    }
		  goto done;
		  break;
		case XK_space:
		  if (((XKeyEvent *) & event)->state & ControlMask)
		    {
		      raise_button_bar();
		      break;
		    }
		default:
		  if (((XKeyEvent *) & event)->state & Mod1Mask)
		    break;
		  if (((XKeyEvent *) & event)->state & ControlMask)
		    {
		      switch(keysym)
			{
			case XK_c:
			  which_field = 0;
			  wipe_field(0);
			  wipe_field(1);
			  break;
			case XK_u:
			case XK_w:
			  wipe_field(which_field);
			  break;
			}
		      break;
		    }
		  if (isprint(ascii) &&
		      (keysym < XK_Shift_L || keysym > XK_Hyper_R) &&
		      input_buffer_ix[which_field] < BUFFER_LEN - 1)
		    {
		      input_buffer[which_field][input_buffer_ix[which_field]++]=
			ascii;
		      input_buffer[which_field][input_buffer_ix[which_field]]=
			0;
		    }
		}
	    }
	  show_input_fields(cfg, &gfx, which_field);
	}
    }

 done:

  dprintf(1, "username is: %s\n", input_buffer[0]);
  dprintf(1, "password is: %s\n", input_buffer[1]);

  CloseGreet(d, &gfx);
  return Greet_Failure;
}
