struct _Gfx {
  Display *dpy;
  int screen;
  Colormap colormap;
  Visual *visual;
  Window root_win, panel_win, background_win;
  XftDraw *panel_draw, *background_draw;
  int cursor_state, cursor_x, cursor_y;
  int cursor_elevation, cursor_height, cursor_width;
};

typedef struct _Gfx Gfx;

// This leaves screen position and input widget height adjustment to the
// caller;
struct _TextAttrs {
  XftFont *font;
  XftColor *color, *shadow_color;
  XYPosition *shadow_offset;
};

typedef struct _TextAttrs TextAttrs;

void text_op_at(int draw_it, Gfx *gfx, Cfg *cfg, TextAttrs *attrs,
		XYPosition *position, int input_height, char *str);

void show_input_at(Gfx *gfx, Cfg *cfg, char *str, XYPosition *position,
		   int w, int cursor, int is_secret);

#define CLEAR_TEXT_AT(GFX, CFG, ATTRS, POSN, IN_HGHT, STR)	\
  text_op_at(0, GFX, CFG, ATTRS, POSN, IN_HGHT, STR)

#define SHOW_TEXT_AT(GFX, CFG, ATTRS, POSN, IN_HGHT, STR)	\
  text_op_at(1, GFX, CFG, ATTRS, POSN, IN_HGHT, STR)


