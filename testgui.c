#include <X11/Xlib.h>
#include <X11/Xft/Xft.h>

#include <ctype.h>
#include <stdlib.h>
#include <err.h>
#include <stdio.h>
#include <stdarg.h>

#include <poll.h>

#include "image.h"
#include "cfg.h"
#include "gfx.h"


int __xdm_LogError(char *fmt, ...)
{
  int res;
  va_list ap;
  va_start(ap, fmt);
  res = vfprintf(stderr, fmt, ap);
  va_end(ap);
  return res;
}


void do_stuff()
{
  Display *dpy;
  int scr;
  Pixmap pixmap;
  Cfg *cfg;
  Gfx gfx_instance, *gfx = &gfx_instance;
  int is_secret = 1;
  
  gfx->dpy = dpy = XOpenDisplay(NULL);
  cfg = get_cfg(dpy);
  scr = gfx->screen = DefaultScreen(dpy);
  gfx->root_win = RootWindow(dpy, scr);
  gfx->colormap = DefaultColormap(dpy, gfx->screen);
  gfx->visual = DefaultVisual(dpy, gfx->screen);

  XSetWindowBackground(dpy, gfx->root_win, BlackPixel(dpy, scr));
  XClearWindow(dpy, gfx->root_win);
  gfx->background_win = XCreateSimpleWindow(dpy, gfx->root_win,
					    cfg->screen_specs.xoffset,
					    cfg->screen_specs.yoffset,
					    cfg->screen_specs.width,
					    cfg->screen_specs.height,
					    0, 0, 255);
  gfx->background_draw = XftDrawCreate(dpy, gfx->background_win,
				       gfx->visual, gfx->colormap);
  if (cfg->background_image.width > 0)
    resize_background(&cfg->background_image, cfg->screen_specs.width,
		      cfg->screen_specs.height);
  else
    frame_background(&cfg->background_image,
		     cfg->screen_specs.width, cfg->screen_specs.height,
		     cfg->screen_specs.width + 1, 0, &cfg->background_color);
  pixmap = imageToPixmap(dpy, &cfg->background_image, scr,
			 gfx->background_win);
  XSetWindowBackgroundPixmap(dpy, gfx->background_win, pixmap);
  XClearWindow(dpy, gfx->background_win);
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
			&cfg->background_image, TO_XY(cfg->panel_position));
  free_image_buffers(&cfg->background_image);
  gfx->panel_win = XCreateSimpleWindow(dpy, gfx->background_win,
				       TO_XY(cfg->panel_position),
				       cfg->panel_image.width,
				       cfg->panel_image.height, 0, 0, 255);
  gfx->panel_draw = XftDrawCreate(dpy, gfx->panel_win,
				  gfx->visual, gfx->colormap);
  pixmap = imageToPixmap(dpy, &cfg->panel_image, scr, gfx->panel_win);
  free_image_buffers(&cfg->panel_image);
  XSetWindowBackgroundPixmap(dpy, gfx->panel_win, pixmap);
  XClearWindow(dpy, gfx->panel_win);
  XFreePixmap(dpy, pixmap);
  
  XMapWindow(dpy, gfx->panel_win);
  XMapWindow(dpy, gfx->background_win);

  set_cursor_dimensions(gfx, cfg, 
			cfg->cursor_size.x, cfg->cursor_size.y,
			cfg->cursor_offset);

  XSelectInput(dpy, gfx->background_win, KeyPressMask);
  XSelectInput(dpy, gfx->panel_win, ExposureMask);

  struct pollfd pfd = { 0 };
  pfd.fd = ConnectionNumber(dpy);
  pfd.events = POLLIN;
  XGrabKeyboard(dpy, gfx->panel_win, False, GrabModeAsync, GrabModeAsync,
		CurrentTime);

  int again = 1;
  char out[128] = "";
  int outix = 0;

  cfg->cursor_blink = 1;

  TextAttrs msg_attrs = {
    cfg->welcome_font, &cfg->welcome_color,
    &cfg->welcome_shadow_color, &cfg->welcome_shadow_offset
  };
  TextAttrs prompt_attrs = {
    cfg->prompt_font, &cfg->prompt_color,
    &cfg->prompt_shadow_color, &cfg->prompt_shadow_offset
  };
    
  SHOW_TEXT_AT(gfx, cfg, &msg_attrs, &cfg->welcome_position,
	       0, cfg->welcome_message);
  SHOW_TEXT_AT(gfx, cfg, &prompt_attrs, &cfg->username_prompt_position,
	       cfg->input_height, cfg->username_prompt);
  show_input_at(gfx, cfg, out, &cfg->username_input_position,
		cfg->username_input_width, 1, is_secret);
  activate_cursor(gfx, cfg, 0);
  activate_cursor(gfx, cfg, 1);

  while (again)
    if (!XPending(dpy))
      {
	switch (poll(&pfd, 1, CURSOR_BLINK_SPEED))
	  {
	  case -1:
	    continue;
	  case 0:
	    activate_cursor(gfx, cfg,
			    !cfg->cursor_blink || !get_cursor_state(gfx));
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
		SHOW_TEXT_AT(gfx, cfg, &msg_attrs, &cfg->welcome_position,
			     0, cfg->welcome_message);
		SHOW_TEXT_AT(gfx, cfg, &prompt_attrs,
			     &cfg->username_prompt_position,
			     cfg->input_height, cfg->username_prompt);
		show_input_at(gfx, cfg, out, &cfg->username_input_position,
			      cfg->username_input_width, 1, is_secret);
		break;

	      case KeyPress:
		XLookupString(&event.xkey, &ascii, 1, &keysym, &compstatus);
		switch (keysym)
		  {
		  case XK_Delete:
		  case XK_BackSpace:
		    if (outix > 0)
		      out[--outix] = 0;
		    break;
		  case XK_Return:
		  case XK_Tab:
		  case XK_KP_Enter:
		    CLEAR_TEXT_AT(gfx, cfg, &msg_attrs, &cfg->welcome_position,
				  0, cfg->welcome_message);
		    XSync(dpy, False);
		    sleep(1);
		    XDestroyWindow(dpy, gfx->background_win);
		    XClearWindow(dpy, gfx->root_win);
		    XSync(dpy, False);
		    write(1, "Result: ", 9);
		    if (outix)
		      write(1, out, outix);
		    write(1, "\n", 1);
		    return;
		  case XK_w:
		  case XK_u:
		  case XK_c:
		    if (((XKeyEvent *) & event)->state & ControlMask)
		      {
			if (keysym == XK_c)
			  puts("Restart");
			out[outix = 0] = 0;
			break;
		      }
		  default:
		    if (isprint(ascii) &&
			(keysym < XK_Shift_L || keysym > XK_Hyper_R) &&
			outix < 128)
		      {
			out[outix++] = ascii;
			out[outix] = 0;
		      }
		  }
		show_input_at(gfx, cfg, out, &cfg->username_input_position,
			      cfg->username_input_width, 1, is_secret);
		activate_cursor(gfx, cfg, 1);
		break;
	      }
	  }
      }
}

int main(int argc, char *argv[])
{
  if (!strcmp(getenv("DISPLAY"), ":0"))
    errx(1, "Please use Xephyr");
  system("xrdb -load $HOME/.Xresources;xrdb -merge gleem.conf");
  do_stuff();
  return 0;
}
