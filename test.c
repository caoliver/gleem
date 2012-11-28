#include <X11/Xlib.h>
#include <X11/Xft/Xft.h>

#include <ctype.h>
#include <stdlib.h>
#include <err.h>

#include <poll.h>

#include "image.h"

enum placement {placement_left, placement_center, placement_right};

struct text_properties {
  XftFont *font;
  XftColor color, shadow_color;
  int shadow_xoff, shadow_yoff;
};

void text_op(Display *dpy, Window win, int scr,
	     char *str,
	     int x, int y, enum placement placement,
	     struct text_properties *properties, int draw_it)
{
  int len = strlen(str);
  XGlyphInfo extents;
  XftTextExtents8(dpy, properties->font, str, len, &extents);
  switch (placement)
    {
    case placement_left:
      x -= extents.width;
      break;
    case placement_center:
      x -= extents.width / 2;      
    }

  if (draw_it)
    {
      Visual* visual = DefaultVisual(dpy, scr);
      Colormap colormap = DefaultColormap(dpy, scr);
      XftDraw *draw = XftDrawCreate(dpy, win, visual, colormap);
  
      if (properties->shadow_xoff || properties->shadow_yoff)
	XftDrawString8(draw, &properties->shadow_color, properties->font,
		       x + properties->shadow_xoff,
		       y + properties->shadow_yoff,
		       (unsigned char *)str, len - 1);
      XftDrawString8(draw, &properties->color,
		     properties->font, x, y, (unsigned char *)str, len - 1);
      XftDrawDestroy(draw);
      return;
    }

  int erasex = x - extents.x +
    (properties->shadow_xoff < 0 ? properties->shadow_xoff : 0),
    erasey = y - extents.y +
    (properties->shadow_yoff < 0 ? properties->shadow_yoff : 0),
    erasew = extents.width +
    (properties->shadow_xoff < 0
     ? -properties->shadow_xoff
     : properties->shadow_xoff),
    eraseh = extents.height +
    (properties->shadow_yoff < 0
     ? -properties->shadow_yoff
     : properties->shadow_yoff);
	
  XClearArea(dpy, win,  erasex, erasey, erasew, eraseh, True);
}

#define PUT_TEXT_AT(DPY, WIN, SCR, STR, X, Y, PLACEMENT, PROPS)	\
  text_op(DPY, WIN, SCR, STR, X, Y, PLACEMENT, PROPS, 1)

#define CLEAR_TEXT_AT(DPY, WIN, SCR, STR, X, Y, PLACEMENT, PROPS)	\
  text_op(DPY, WIN, SCR, STR, X, Y, PLACEMENT, PROPS, 0)

void test_tp(Display *dpy, Window win, char *str, int x, int y)
{
  int scr = DefaultScreen(dpy);
  Visual* visual = DefaultVisual(dpy, scr);
  Colormap colormap = DefaultColormap(dpy, scr);
  struct text_properties tp;

  tp.font = XftFontOpenName(dpy, scr, "URW Gothic L:size=24:dpi=75");
  XftColorAllocName(dpy, visual, colormap, "#f9f9f9", &tp.color);
  XftColorAllocName(dpy, visual, colormap, "#702342", &tp.shadow_color);
  tp.shadow_yoff = tp.shadow_xoff = 1;
  PUT_TEXT_AT(dpy, win, scr, str, x, y, placement_left, &tp);
  PUT_TEXT_AT(dpy, win, scr, str, x, y - 36, placement_center, &tp);
  PUT_TEXT_AT(dpy, win, scr, str, x, y - 72, placement_right, &tp);
}

void show_input_at(Display *dpy, int scr, Window wid, char *str,
		   int x, int y, int w, int h, int cursor)
{
  Visual* visual = DefaultVisual(dpy, scr);
  Colormap colormap = DefaultColormap(dpy, scr);

  XftFont *font;
  XftColor color;

#define TSIZE "16"
#define CURSOR_PIXELS 3

  XClearArea(dpy, wid, x, y - h, w, h, False);
  XftColorAllocName(dpy, visual, colormap, "#702342", &color);
  font = XftFontOpenName(dpy, scr, "URW Gothic:size=" TSIZE);
  XftDraw *draw = XftDrawCreate(dpy, wid, visual, colormap);
  XGlyphInfo extents;
  int len = strlen(str);
  XftTextExtents8(dpy, font, (unsigned char *)"|", 1, &extents);
  int bump = extents.height - extents.y;
  int right_margin = cursor ? 5 - extents.width : 1;
  // Make sure to show some cursor movement for trailing space
  // since Xft doesn't seem to.
  if (cursor && len > 0 && str[len - 1] == ' ')
    {
      XftTextExtents8(dpy, font, (unsigned char *)"  ", 2, &extents);
      right_margin += extents.width;
    }
  int too_long;
  do
    {
      XftTextExtents8(dpy, font, (unsigned char *)str, len, &extents);
      if ((too_long = extents.width + right_margin + CURSOR_PIXELS > w))
	{
	  str++;
	  len--;
	}
    }
  while (len > 0 && too_long);
  if (len > 0)
    XftDrawString8(draw, &color, font, x, y - bump, (unsigned char *)str, len);
  if (cursor)
    XftDrawRect(draw, &color, x + right_margin + extents.width, y - h + 1,
		CURSOR_PIXELS, h - 1);
  XftColorFree(dpy, visual, colormap, &color);
  XftDrawDestroy(draw);
}

void do_stuff()
{
  Display *dpy;
  int scr;
  Window wid, pwid, rootid;
  struct image bg, pan;
  Pixmap pixmap;

#define XOFF 50
#define YOFF 500

  dpy = XOpenDisplay(NULL);
  scr = DefaultScreen(dpy);
  rootid = RootWindow(dpy, scr);
#ifdef ON_ROOT
  wid = rootid;
#else
  wid = XCreateSimpleWindow(dpy, rootid, 0, 0, 1024, 768, 0, 0, 0);
#endif
  pwid = XCreateSimpleWindow(dpy, wid, XOFF, YOFF, 587, 235, 0, 0, 255);
  read_image("background.jpg", &bg);
  resize_background(&bg, 1024, 768);
  pixmap = imageToPixmap(dpy, &bg, scr, wid);
  XSetWindowBackgroundPixmap(dpy, wid, pixmap);
  XFreePixmap(dpy, pixmap);
  XClearWindow(dpy,wid);

  read_image("panel.png", &pan);
  merge_with_background(&pan, &bg, XOFF, YOFF);
  free_image_buffers(&bg);
  pixmap = imageToPixmap(dpy, &pan, scr, pwid);
  free_image_buffers(&pan);
  XSetWindowBackgroundPixmap(dpy, pwid, pixmap);
  XFreePixmap(dpy, pixmap);

  XMapWindow(dpy, pwid);
  XMapWindow(dpy, wid);

  XSelectInput(dpy, wid, KeyPressMask|ExposureMask);
  XSelectInput(dpy, pwid, ExposureMask);
  struct pollfd pfd = {0};
  pfd.fd = ConnectionNumber(dpy);
  pfd.events = POLLIN;
#ifdef ON_ROOT
  XGrabKeyboard(dpy, pwid, False, GrabModeAsync, GrabModeAsync, CurrentTime);
#endif
  test_tp(dpy, wid, "Hello, world!", 300, 400);

  int again = 1;
  char out[128]="";
  int outix = 0;
  while  (again)
    if(XPending(dpy) || poll(&pfd, 1, -1) > 0)
      while(XPending(dpy))
	{
	  XEvent event;
	  KeySym keysym;
	  XComposeStatus compstatus;
	  char ascii;

	  XNextEvent(dpy, &event);
	  switch(event.type)
	    {
	    case Expose:
	      test_tp(dpy, wid, "Hello, world!", 300, 400);
	      show_input_at(dpy, scr, pwid, out, 389, 188, 193, 24, 1);
	      break;
	      
	    case KeyPress:
	      XLookupString(&event.xkey, &ascii, 1, &keysym, &compstatus);
	      switch(keysym)
		{
		case XK_Delete:
		case XK_BackSpace:
		  if (outix > 0)
		    out[--outix] = 0;
		  break;
		case XK_Return:
		case XK_Tab:
		case XK_KP_Enter:
		  write(1, "Result: ", 9);
		  if (outix)
		    write(1, out, outix);
		  write(1, "\n", 1);
		  return;
		case XK_w:
		case XK_u:
		case XK_c:
		  if (((XKeyEvent *)&event)->state & ControlMask)
		    {
		      if (keysym == XK_c)
			puts("Restart");
		      out[outix = 0] = 0;
		      break;
		    }
		default:
		  if (isprint(ascii) &&
		      (keysym < XK_Shift_L || keysym > XK_Hyper_R) &&
		      outix < 64)
		    {
		      out[outix++] = ascii;
		      out[outix] = 0;
		    }
		}
	      show_input_at(dpy, scr, pwid, out, 389, 188, 193, 24, 1);
	      break;
	    }
	}
}

int main(int argc, char *argv[])
{
  do_stuff();
  return 0;
}
