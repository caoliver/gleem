#include <X11/Xlib.h>
#include <X11/Xft/Xft.h>

#include <ctype.h>
#include <stdlib.h>
#include <err.h>

#include <poll.h>

#include "image.h"
#include "cfg.h"


void text_op(Display *dpy, Window background, Window panel, int scr,
	     struct cfg *cfg,
	     char *str,
	     XftFont *font,
	     XftColor *color, XftColor *shadow_color,
	     struct position *position, struct position *shadow_offset,
	     int input_height)
{
  int len = strlen(str);
  XGlyphInfo extents;

  int bump = 0;
  if (input_height)
    {
      XftTextExtents8(dpy, cfg->input_font, (unsigned char *)"|", 1, &extents);
      bump = extents.height - extents.y;
    }

  XftTextExtentsUtf8(dpy, font, (unsigned char *)str, len, &extents);
  int height = input_height ? input_height : extents.height;
  if (TRANSLATE_POSITION(position, extents.width, height, cfg, 1))
    position->y -= bump;

  int erasex = (shadow_offset->x < 0 ? shadow_offset->x : 0) - extents.x,
    erasey = (shadow_offset->y < 0 ? shadow_offset->y : 0) - extents.y,
    erasew = extents.width +
    (shadow_offset->x < 0
     ? -shadow_offset->x
     : shadow_offset->x),
    eraseh = extents.height +
    (shadow_offset->y < 0
     ? -shadow_offset->y
     : shadow_offset->y);
	
  XClearArea(dpy, background, ADD_POS(*position, XY(erasex, erasey)),
	     erasew, eraseh, False);
  XClearArea(dpy, panel,
	     ADD_XY(TO_XY(*position),
		    ADD_XY(NEGATE_POS(cfg->panel_position),
			   XY(erasex, erasey))),
	     erasew, eraseh, False);

  if (color)
    {
      XftDraw *draw;
      draw = XftDrawCreate(dpy, background, DefaultVisual(dpy, scr),
      			   DefaultColormap(dpy, scr));

      if (shadow_offset->x || shadow_offset->y)
      	XftDrawStringUtf8(draw, shadow_color, font,
      			  ADD_POS(*position, TO_XY(*shadow_offset)),
      			  (unsigned char *)str, len);
      XftDrawStringUtf8(draw, color, font, TO_XY(*position),
      			(unsigned char *)str, len);
      XftDrawDestroy(draw);

      draw = XftDrawCreate(dpy, panel, DefaultVisual(dpy, scr),
			   DefaultColormap(dpy, scr));
  
      if (shadow_offset->x || shadow_offset->y)
	XftDrawStringUtf8(draw, shadow_color, font,
			  ADD_POS(*position,
				  ADD_POS(*shadow_offset,
					  NEGATE_POS(cfg->panel_position))),
			  (unsigned char *)str, len);
      XftDrawStringUtf8(draw, color, font,
			ADD_POS(*position,
				NEGATE_POS(cfg->panel_position)),
			(unsigned char *)str, len);
      XftDrawDestroy(draw);
      return;
    }
}


int cursor_x, cursor_y, cursor_h, cursor_w, cursor_state, cursor_offset;

void draw_cursor(Display *dpy, int scr, Window win, struct cfg *cfg)
{
  if (cursor_state || cfg->input_highlight)
    {
      Visual* visual = DefaultVisual(dpy, scr);
      Colormap colormap = DefaultColormap(dpy, scr);
      XftDraw *draw = XftDrawCreate(dpy, win, visual, colormap);
      XftDrawRect(draw, 
		  cursor_state
		  ? &cfg->cursor_color
		  : &cfg->input_highlight_color,
		  cursor_x, cursor_y,
		  cursor_w, cursor_h);
    }
  else
    XClearArea(dpy, win, cursor_x, cursor_y,
	       cursor_w, cursor_h, False);
}

void show_input_at(Display *dpy, struct cfg *cfg,
		   int scr, Window win, char *str,
		   struct position *position,
		   int w,
		   int cursor, int is_secret)
{
  Visual* visual = DefaultVisual(dpy, scr);
  Colormap colormap = DefaultColormap(dpy, scr);
  XftFont *font;
  
  int h = cfg->input_height;
  if (TRANSLATE_POSITION(position, w, h, cfg, 1))
    {
      position->x -= cfg->panel_position.x;
      position->y -= cfg->panel_position.y;
    }
  int ASSIGN_XY(x, y, TO_XY(*position));

  XftColor *color = &cfg->input_color,
    *color2 = &cfg->input_alternate_color,
    *scolor = &cfg->input_shadow_color;
  XftDraw *draw = XftDrawCreate(dpy, win, visual, colormap);

  int star_width;
  int ASSIGN_XY(sxoff, syoff, TO_XY(cfg->input_shadow_offset));

  if (cfg->input_highlight)
    XftDrawRect(draw, &cfg->input_highlight_color,
		x, y - h, w, h + (syoff < 0 ? -syoff : syoff));
  else
    XClearArea(dpy, win, x, y - h, w, h + (syoff < 0 ? -syoff : syoff), False);
  w -= sxoff < 0 ? -sxoff : sxoff;
  y += syoff < 0 ? -syoff : 0;
  x += sxoff < 0 ? -sxoff : 0;

  font = cfg->input_font;
  XGlyphInfo extents;
  int len = strlen(str);
  XftTextExtents8(dpy, font, (unsigned char *)"|", 1, &extents);
  int bump = extents.height - extents.y;
  int right_margin = cursor ? 1 + extents.width : 1;
  if (is_secret)
    {
      unsigned char secret_mask = cfg->password_mask;
      XftTextExtents8(dpy, font, &secret_mask, 1, &extents);
      star_width = extents.width + (extents.width >> 3);
      if (star_width < 8)
	star_width += 2;
      int extras =
	((len + 1) * star_width - 1 + right_margin + cursor_w - w)
	/ star_width;
      if (extras < 0)
	extras = 0;
      if (extents.width > 0)
	for (int i = extras; i < len; i++, x += star_width)
	  {
	    if (sxoff != 0 || syoff != 0)
	      XftDrawString8(draw, scolor, font,
			     x + sxoff, y + syoff - bump, &secret_mask, 1);
	      
	    XftDrawString8(draw, (i % 8 < 4) ? color : color2,
			   font, x, y - bump, &secret_mask, 1);
	  }
      else
	x += star_width * (len - extras);
      extents.width = 0;
    }
  else
    {
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
	  if ((too_long = extents.width + right_margin + cursor_w > w))
	    {
	      str++;
	      len--;
	    }
	}
      while (len > 0 && too_long);
      if (len > 0)
	{
	  if (sxoff != 0 || syoff != 0)
	    XftDrawString8(draw, scolor, font, x + sxoff, y + syoff - bump,
			   (unsigned char *)str, len);
	  XftDrawString8(draw,
			 color, font, x, y - bump, (unsigned char *)str, len);
	}
    }
  if (cursor)
    {
      cursor_state = 1;
      cursor_x = x + right_margin + extents.width;
      cursor_y = y - cursor_offset;
    }
  XftDrawDestroy(draw);
}

void do_stuff()
{
  Display *dpy;
  int scr;
  Window wid, pwid, rootid;
  Pixmap pixmap;
  struct cfg *cfg;

  dpy = XOpenDisplay(NULL);
  cfg = get_cfg(dpy);
  scr = DefaultScreen(dpy);
  rootid = RootWindow(dpy, scr);
#ifndef NOZEP  
  XSetWindowBackground(dpy, rootid, BlackPixel(dpy, DefaultScreen(dpy)));
  XClearWindow(dpy, rootid);
#endif
  wid = XCreateSimpleWindow(dpy, rootid,
			    cfg->screen_specs.xoffset,
			    cfg->screen_specs.yoffset,
			    cfg->screen_specs.width,
			    cfg->screen_specs.height,
			    0, 0, 255);
  if (cfg->background_image.width > 0)
    resize_background(&cfg->background_image, cfg->screen_specs.width,
		      cfg->screen_specs.height);
  else
    frame_background(&cfg->background_image,
		     cfg->screen_specs.width, cfg->screen_specs.height,
		     cfg->screen_specs.width + 1, 0, &cfg->background_color);
  pixmap = imageToPixmap(dpy, &cfg->background_image, scr, wid);
  XSetWindowBackgroundPixmap(dpy, wid, pixmap);
  XClearWindow(dpy, wid);
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
  pwid = XCreateSimpleWindow(dpy, wid,
			     TO_XY(cfg->panel_position),
			     cfg->panel_image.width,
			     cfg->panel_image.height,
			     0, 0, 255);
  pixmap = imageToPixmap(dpy, &cfg->panel_image, scr, pwid);
  free_image_buffers(&cfg->panel_image);
  XSetWindowBackgroundPixmap(dpy, pwid, pixmap);
  XClearWindow(dpy, pwid);
  XFreePixmap(dpy, pixmap);

  XMapWindow(dpy, pwid);
  XMapWindow(dpy, wid);

  cursor_w = cfg->cursor_size.x;
  cursor_h = cfg->cursor_size.y;
  cursor_offset = cfg->cursor_offset + cursor_h;
  if (cursor_offset > cfg->input_height)
    cursor_offset = cfg->input_height;
  if (cursor_h > cfg->input_height)
    cursor_h = cfg->input_height;

  XSelectInput(dpy, wid, KeyPressMask);
  XSelectInput(dpy, pwid, ExposureMask);

  struct pollfd pfd = {0};
  pfd.fd = ConnectionNumber(dpy);
  pfd.events = POLLIN;
#ifndef NOZEP
  XGrabKeyboard(dpy, pwid, False, GrabModeAsync, GrabModeAsync, CurrentTime);
#endif
  int is_secret = 0;

  int again = 1;
  char out[128]="";
  int outix = 0;
#define XROOT -50
#define YROOT 50
  
  cfg->cursor_blink = 1;
  text_op(dpy, wid, pwid, DefaultScreen(dpy), cfg,
	  cfg->username_prompt, cfg->prompt_font,
	  &cfg->prompt_color, &cfg->prompt_shadow_color,
	  &cfg->username_prompt_position,
	  &cfg->prompt_shadow_offset, cfg->input_height);
  text_op(dpy, wid, pwid, DefaultScreen(dpy), cfg,
	  cfg->welcome_message, cfg->welcome_font, &cfg->welcome_color,
	  &cfg->welcome_shadow_color,
	  &cfg->welcome_position, &cfg->welcome_shadow_offset, 0);
  show_input_at(dpy, cfg, scr, pwid, out,
		&cfg->username_input_position,
		cfg->username_input_width,
		1, is_secret);
  draw_cursor(dpy, scr, pwid, cfg);
  while  (again)
    if(!XPending(dpy))
      {
	switch (poll(&pfd, 1, CURSOR_BLINK_SPEED))
	  {
	  case -1:
	    continue;
	  case 0:
	    cursor_state = !cfg->cursor_blink || !cursor_state;
	    draw_cursor(dpy, scr, pwid, cfg);
	    XSync(dpy, False);
	  }
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
		text_op(dpy, wid, pwid, DefaultScreen(dpy), cfg,
			cfg->username_prompt, cfg->prompt_font,
			&cfg->prompt_color, &cfg->prompt_shadow_color,
			&cfg->username_prompt_position,
			&cfg->prompt_shadow_offset, cfg->input_height);
		text_op(dpy, wid, pwid, DefaultScreen(dpy), cfg,
			cfg->welcome_message, cfg->welcome_font,
			&cfg->welcome_color,
			&cfg->welcome_shadow_color,
			&cfg->welcome_position, &cfg->welcome_shadow_offset, 0);		show_input_at(dpy, cfg, scr, pwid, out,
			      &cfg->username_input_position,
			      cfg->username_input_width,
			      1, is_secret);
		draw_cursor(dpy, scr, pwid, cfg);
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
		    text_op(dpy, wid, pwid, DefaultScreen(dpy), cfg,
			    cfg->welcome_message, cfg->welcome_font, NULL, NULL,
			    &cfg->welcome_position, &cfg->welcome_shadow_offset,
			    0);
		    XSync(dpy, False);
		    //		  sleep(2);
		    XDestroyWindow(dpy, wid);
		    XClearWindow(dpy, rootid);
		    XSync(dpy, False);
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
			outix < 128)
		      {
			out[outix++] = ascii;
			out[outix] = 0;
		      }
		  }
		show_input_at(dpy, cfg, scr, pwid, out,
			      &cfg->username_input_position,
			      cfg->username_input_width,
			      1, is_secret);
		draw_cursor(dpy, scr, pwid, cfg);
		break;
	      }
	  }
      }
}

int main(int argc, char *argv[])
{
#ifndef NOZEP
  if (!strcmp(getenv("DISPLAY"), ":0"))
    errx(1, "Please use Xephyr");
#endif
  do_stuff();
  return 0;
}
