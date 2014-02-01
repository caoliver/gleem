void set_cursor_dimensions(Gfx *gfx, Cfg *cfg,
                           int width, int height, int elevation);
void show_input_at(Gfx *gfx, Cfg *cfg, char *str, XYPosition *position,
                   int w, int cursor, int is_secret);
int get_cursor_state(Gfx *gfx);
void activate_cursor(Gfx *gfx, Cfg *cfg, int state);




