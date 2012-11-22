#include <X11/Xlib.h>
#include <X11/Xft/Xft.h>

#include <ctype.h>
#include <stdlib.h>
#include <err.h>

#include <poll.h>

#include "image.h"

void show_input_at(Display *dpy, int scrid, Window wid, char *str,
		   int x, int y, int w, int h, int cursor)
{
  Visual* visual = DefaultVisual(dpy, scrid);
  Colormap colormap = DefaultColormap(dpy, scrid);

  XftFont *font;
  XftColor color;

#define TSIZE 16
#define UNQUOTE(PARAM) #PARAM
#define TO_STR(PARAM) UNQUOTE(PARAM)

  XClearArea(dpy, wid, x, y - h, w, h, False);
  XftColorAllocName(dpy, visual, colormap, "NavyBlue", &color);
  font = XftFontOpenName(dpy, scrid, "URW Gothic:size=" TO_STR(TSIZE));
  XftDraw *draw = XftDrawCreate(dpy, wid, visual, colormap);
  XGlyphInfo extents;
  int len = strlen(str);
  XftTextExtents8(dpy, font, (unsigned char *)"|", 1, &extents);
  int bump = extents.height - extents.y;
  int cursorx = x - extents.width + 1;
  int too_long;
  do
    {
      XftTextExtents8(dpy, font, (unsigned char *)str, len, &extents);
      if ((too_long = extents.width >= w))
	{
	  str++;
	  len--;
	}
    }
  while (len > 0 && too_long);
  if (len > 0)
    {
      XftDrawString8(draw, &color, font, x, y - bump,
		     (unsigned char *)str, len - 1);
      if (cursor)
	XftDrawRect(draw, &color, cursorx + extents.width, y - h + 1, 1, 23);
    }
  XftColorFree(dpy, visual, colormap, &color);
  XftDrawDestroy(draw);
}

void do_stuff()
{
  Display *dpy;
  int scrid;
  Window wid, pwid, rootid;
  struct image bg, pan;
  Pixmap pixmap;

#define XOFF 50
#define YOFF 500

  dpy = XOpenDisplay(NULL);
  scrid = DefaultScreen(dpy);
  rootid = RootWindow(dpy, scrid);
#ifdef ON_ROOT
  wid = rootid;
#else
  wid = XCreateSimpleWindow(dpy, rootid, 0, 0, 1024, 768, 0, 0, 0);
#endif
  pwid = XCreateSimpleWindow(dpy, wid, XOFF, YOFF, 587, 235, 0, 0, 255);
  read_image("background.jpg", &bg);
  resize_background(&bg, 1024, 768);
  pixmap = imageToPixmap(dpy, &bg, scrid, wid);
  XSetWindowBackgroundPixmap(dpy, wid, pixmap);
  XClearWindow(dpy,wid);

  read_image("panel.png", &pan);
  merge_with_background(&pan, &bg, XOFF, YOFF);
  free_image_buffers(&bg);
  pixmap = imageToPixmap(dpy, &pan, scrid, pwid);
  free_image_buffers(&pan);
  XSetWindowBackgroundPixmap(dpy, pwid, pixmap);

  XMapWindow(dpy, pwid);
  XMapWindow(dpy, wid);
  //  XFlush(dpy);

//  XUnmapWindow(dpy, pwid);
//  XSync(dpy, False);
//  sleep(1);
//  XMapWindow(dpy, pwid);

  XSelectInput(dpy, wid, KeyPressMask);
  XSelectInput(dpy, pwid, ExposureMask);
  struct pollfd pfd = {0};
  pfd.fd = ConnectionNumber(dpy);
  pfd.events = POLLIN;
#ifdef ON_ROOT
  XGrabKeyboard(dpy, pwid, False, GrabModeAsync, GrabModeAsync, CurrentTime);
#endif
  int again = 1;
  char out[128]="j";
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
	      show_input_at(dpy, scrid, pwid, out, 389, 188, 193, 24, 0);
	      break;
	      
	    case KeyPress:
	      XLookupString(&event.xkey, &ascii, 1, &keysym, &compstatus);
	      switch(keysym)
		{
		case XK_Delete:
		case XK_BackSpace:
		  if (outix > 0)
		    {
		      outix--;
		      strcpy(&out[outix], "|");
		    }
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
		      outix = 0;
		      strcpy(out, "|");
		      break;
		    }
		default:
		  if (isprint(ascii) &&
		      (keysym < XK_Shift_L || keysym > XK_Hyper_R) &&
		      outix < 64)
		    sprintf(&out[outix++], "%c|", ascii);
		}
	      show_input_at(dpy, scrid, pwid, out, 389, 188, 193, 24, 0);
	      break;
	    }
	}
}

int main(int argc, char *argv[])
{
  do_stuff();
  return 0;
}
