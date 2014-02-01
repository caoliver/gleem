#include <X11/Xlib.h>
#include <X11/Xft/Xft.h>

#include "image.h"
#include "cfg.h"
#include "gfx.h"

void text_op_at(int draw_it, Gfx *gfx, Cfg *cfg, TextAttrs *attrs,
		       XYPosition *position, int input_height, char *str)
{
  int len = strlen(str);
  XGlyphInfo extents;

  int bump = 0;
  if (input_height)
    {
      XftTextExtents8(gfx->dpy, cfg->input_font, (unsigned char *)"|",
		      1, &extents);
      bump = extents.height - extents.y;
    }

  XftTextExtentsUtf8(gfx->dpy, attrs->font, (unsigned char *)str, len,
		     &extents);
  int height = input_height ? input_height : extents.height;
  if (TRANSLATE_POSITION(position, extents.width, height, cfg, 1))
    position->y -= bump;

  XYPosition *shadow_offs = attrs->shadow_offset;
  int
    erasex =
    (shadow_offs->x < 0 ? shadow_offs->x : 0) - extents.x,
    erasey =
    (shadow_offs->y < 0 ? shadow_offs->y : 0) - extents.y,
    erasew = extents.width + 1 +
    (shadow_offs->x < 0
     ? -shadow_offs->x
     : shadow_offs->x),
    eraseh = extents.height +
    (shadow_offs->y < 0
     ? -shadow_offs->y
     : shadow_offs->y);
	
  XClearArea(gfx->dpy, gfx->background_win,
	     ADD_POS(*position, XY(erasex, erasey)),
	     erasew, eraseh, False);
  XClearArea(gfx->dpy, gfx->panel_win,
	     ADD_POS(*position,
		     ADD_XY(NEGATE_POS(cfg->panel_position),
			    XY(erasex, erasey))),
	     erasew, eraseh, False);

  if (draw_it)
    {
      if (shadow_offs->x || shadow_offs->y)
      	XftDrawStringUtf8(gfx->background_draw,
			  attrs->shadow_color, attrs->font,
      			  ADD_POS(*position, TO_XY(*shadow_offs)),
      			  (unsigned char *)str, len);
      XftDrawStringUtf8(gfx->background_draw, attrs->color, attrs->font,
			TO_XY(*position),
      			(unsigned char *)str, len);

      if (shadow_offs->x || shadow_offs->y)
	XftDrawStringUtf8(gfx->panel_draw, attrs->shadow_color, attrs->font,
			  ADD_POS(*position,
				  ADD_POS(*shadow_offs,
					  NEGATE_POS(cfg->panel_position))),
			  (unsigned char *)str, len);
      XftDrawStringUtf8(gfx->panel_draw, attrs->color, attrs->font,
			ADD_POS(*position,
				NEGATE_POS(cfg->panel_position)),
			(unsigned char *)str, len);
      return;
    }
}


void set_cursor_dimensions(Gfx *gfx, Cfg *cfg,
			   int width, int height, int elevation)
{
  gfx->cursor_width = width;
  gfx->cursor_height = height;
  gfx->cursor_elevation = elevation + height;
  if (height > cfg->input_height)
    gfx->cursor_height = cfg->input_height;
  if (gfx->cursor_elevation > cfg->input_height)
    gfx->cursor_elevation = cfg->input_height;
}

static void draw_cursor(Gfx *gfx, Cfg *cfg)
{
  if (gfx->cursor_state || cfg->input_highlight)
    XftDrawRect(gfx->panel_draw,
		gfx->cursor_state
		? &cfg->cursor_color
		: &cfg->input_highlight_color,
		gfx->cursor_x, gfx->cursor_y,
		gfx->cursor_width, gfx->cursor_height);
  else
    XClearArea(gfx->dpy, gfx->panel_win, gfx->cursor_x, gfx->cursor_y,
	       gfx->cursor_width, gfx->cursor_height, False);
}

void activate_cursor(Gfx *gfx, Cfg *cfg, int state)
{
  if (gfx->cursor_state != state)
    {
      gfx->cursor_state = state;
      draw_cursor(gfx, cfg);
      XSync(gfx->dpy, False);
    }
}

int get_cursor_state(Gfx *gfx)
{
  return gfx->cursor_state;
}

void show_input_at(Gfx *gfx, Cfg *cfg, char *str, XYPosition *position,
		   int w, int cursor, int is_secret)
{
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

  int ASSIGN_XY(sxoff, syoff, TO_XY(cfg->input_shadow_offset));

  if (cfg->input_highlight)
    XftDrawRect(gfx->panel_draw, &cfg->input_highlight_color,
		x, y - h, w, h + (syoff < 0 ? -syoff : syoff));
  else
    XClearArea(gfx->dpy, gfx->panel_win,
	       x, y - h, w, h + (syoff < 0 ? -syoff : syoff), False);
  w -= sxoff < 0 ? -sxoff : sxoff;
  y += syoff < 0 ? -syoff : 0;
  x += sxoff < 0 ? -sxoff : 0;

  XftFont *font = cfg->input_font;
  XGlyphInfo extents;
  int len = strlen(str);
  XftTextExtents8(gfx->dpy, font, (unsigned char *)"|", 1, &extents);
  int bump = extents.height - extents.y;
  int right_margin = cursor ? 1 + extents.width : 1;
  if (is_secret)
    {
      unsigned char secret_mask = cfg->password_mask;
      XftTextExtents8(gfx->dpy, font, &secret_mask, 1, &extents);
      int star_width = extents.width + (extents.width >> 3);
      if (secret_mask < 33)
	{
	  extents.width = 0;
	  star_width = secret_mask == 32 ? 1 : secret_mask;
	}
      else if (star_width < 8)
	star_width += 2;
      int extras;
      if (star_width == 0)
	extras = 0;
      else
	{
	  extras =
	    ((len + 1) * star_width - 1 + right_margin + gfx->cursor_width - w)
	    / star_width;
	  if (extras < 0)
	    extras = 0;
	}
      if (extents.width > 0)
	for (int i = extras; i < len; i++, x += star_width)
	  {
	    if (sxoff != 0 || syoff != 0)
	      XftDrawString8(gfx->panel_draw, scolor, font,
			     x + sxoff, y + syoff - bump, &secret_mask, 1);
	      
	    XftDrawString8(gfx->panel_draw, (i % 8 < 4) ? color : color2,
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
	  XftTextExtents8(gfx->dpy, font, (unsigned char *)"  ", 2, &extents);
	  right_margin += extents.width;
	}
      int too_long;
      do
	{
	  XftTextExtents8(gfx->dpy, font, (unsigned char *)str, len, &extents);
	  if ((too_long =
	       extents.width + right_margin + gfx->cursor_width > w))
	    {
	      str++;
	      len--;
	    }
	}
      while (len > 0 && too_long);
      if (len > 0)
	{
	  if (sxoff != 0 || syoff != 0)
	    XftDrawString8(gfx->panel_draw, scolor, font
			   , x + sxoff, y + syoff - bump,
			   (unsigned char *)str, len);
	  XftDrawString8(gfx->panel_draw,
			 color, font, x, y - bump, (unsigned char *)str, len);
	}
    }
  if (cursor)
    {
      gfx->cursor_x = x + right_margin + extents.width;
      gfx->cursor_y = y - gfx->cursor_elevation;
      draw_cursor(gfx, cfg);
    }
}
