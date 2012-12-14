#include <X11/Xlib.h>
#include <X11/Xft/Xft.h>

#include "dm.h"
#include "dm_error.h"
#include "greet.h"

#include <ctype.h>
#include <stdlib.h>
#include <err.h>

#include <poll.h>

#include "image.h"
#include "cfg.h"

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
# ifdef HAVE_GETSPNAM
struct spwd   *(*__xdm_getspnam)(GETSPNAM_ARGS) = NULL;
#  ifndef QNX4
void   (*__xdm_endspent)(void) = NULL;
#  endif /* QNX4 doesn't use endspent */
# endif
struct passwd   *(*__xdm_getpwnam)(GETPWNAM_ARGS) = NULL;
# if defined(linux) || defined(__GLIBC__)
void   (*__xdm_endpwent)(void) = NULL;
# endif
char     *(*__xdm_crypt)(CRYPT_ARGS) = NULL;
# ifdef USE_PAM
pam_handle_t **(*__xdm_thepamhp)(void) = NULL;
# endif


static Display *InitGreet(struct display *d, struct image *background)
{
  Display *dpy;

  if (!(dpy = XOpenDisplay(d->name)))
    return 0;
  RegisterCloseOnFork(ConnectionNumber(dpy));
  SecureDisplay(d, dpy);
  return dpy;
}

static void CloseGreet(struct display *d, Display *dpy)
{
  UnsecureDisplay(d, dpy);
  ClearCloseOnFork(XConnectionNumber(dpy));
  XCloseDisplay(dpy);
}

_X_EXPORT
greet_user_rtn GreetUser(
    struct display          *d,
    Display          **display,
    struct verify_info      *verify,
    struct greet_info       *greet,
    struct dlfuncs        *dlfuncs)

{
    struct image background, panel;
    int scr;
    Window root_win, panel_win;
    Pixmap pixmap;
    static Display *dpy;
    struct cfg *cfg;


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
# ifdef HAVE_GETSPNAM
    __xdm_getspnam = dlfuncs->_getspnam;
#  ifndef QNX4
    __xdm_endspent = dlfuncs->_endspent;
#  endif /* QNX4 doesn't use endspent */
# endif
    __xdm_getpwnam = dlfuncs->_getpwnam;
# if defined(linux) || defined(__GLIBC__)
    __xdm_endpwent = dlfuncs->_endpwent;
# endif
    __xdm_crypt = dlfuncs->_crypt;
# ifdef USE_PAM
    __xdm_thepamhp = dlfuncs->_thepamhp;
# endif

    if (!(dpy = InitGreet(d, &background)))
      {
	LogError ("Cannot reopen display %s for greet window\n", d->name);
        exit (RESERVER_DISPLAY);
      }

    scr = DefaultScreen(dpy);
    
    cfg = read_cfg(dpy);

    // Trial junk below here.

    read_image("/root/gleem/background.jpg", &background);
    resize_background(&background,
		      WidthOfScreen(ScreenOfDisplay(dpy, scr)),
		      HeightOfScreen(ScreenOfDisplay(dpy, scr)));
    root_win = RootWindow(dpy, scr);
    pixmap = imageToPixmap(dpy, &background, scr, root_win);
    XSetWindowBackgroundPixmap(dpy, root_win, pixmap);
    XFreePixmap(dpy, pixmap);
    XClearWindow(dpy,root_win);
    panel_win = XCreateSimpleWindow(dpy, root_win,
    				   50, 500, 587, 235, 0, 0, 255);
    read_image("/root/gleem/panel.png", &panel);
    merge_with_background(&panel, &background, 50, 050);
    free_image_buffers(&background);
    pixmap = imageToPixmap(dpy, &panel, scr, panel_win);
    free_image_buffers(&panel);
    XSetWindowBackgroundPixmap(dpy, panel_win, pixmap);
    XFreePixmap(dpy, pixmap);

    for (int i=0; i < 5; i++)
      {
	if (~i & 1)
	  XMapWindow(dpy, panel_win);
	else
	  XUnmapWindow(dpy, panel_win);
	XFlush(dpy);
	sleep(1);
      }

    XSelectInput(dpy, RootWindow(dpy, scr), KeyPressMask);
    XSelectInput(dpy, panel_win, ExposureMask);

    struct pollfd pfd = {0};
    pfd.fd = ConnectionNumber(dpy);
    pfd.events = POLLIN;
    sleep(5);

    CloseGreet(d, dpy);
    return Greet_Failure;
}
