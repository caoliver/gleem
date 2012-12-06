#ifndef _CFG_H_
#define _CFG_H_

#define ADD_ALLOC_FLAG(TYPE, NAME) TYPE NAME; int NAME##_ALLOC

struct screen_specs {
  unsigned int xoffset;
  unsigned int yoffset;
  unsigned int width;
  unsigned int height;
  unsigned int total_width;
  unsigned int total_height;
};

struct position {
  int x, y, flags;
};

#define PANEL_POSITION_NAME panel_coords

struct command {
  int action;  // Keyword whitespace [optional params]
  char* action_params;
  int delay;
  KeySym keysym;
  unsigned mod_state;
  ADD_ALLOC_FLAG(char *, name);
  ADD_ALLOC_FLAG(char *, message);
  ADD_ALLOC_FLAG(char *, allowed_users);
};

struct cfg {
  int numlock, ignore_capslock, hide_mouse, auto_login, focus_password;
  int message_duration, command_count;
  struct screen_specs screen_specs;
  int background_style;
  char password_mask;
  struct command *commands;
  char *current_session;  // Pointer into session string.
  struct position PANEL_POSITION_NAME;
  ADD_ALLOC_FLAG(XftColor, background_color);
  ADD_ALLOC_FLAG(XftColor, message_color);
  ADD_ALLOC_FLAG(XftColor, message_shadow_color);
  ADD_ALLOC_FLAG(XftColor, welcome_color);
  ADD_ALLOC_FLAG(XftColor, welcome_shadow_color);
  ADD_ALLOC_FLAG(XftColor, user_color);
  ADD_ALLOC_FLAG(XftColor, user_shadow_color);
  ADD_ALLOC_FLAG(XftColor, pass_color);
  ADD_ALLOC_FLAG(XftColor, pass_shadow_color);
  ADD_ALLOC_FLAG(XftColor, input_color);
  ADD_ALLOC_FLAG(XftColor, input_alternate_color);
  ADD_ALLOC_FLAG(XftColor, input_shadow_color);
  ADD_ALLOC_FLAG(XftFont *, message_font);
  ADD_ALLOC_FLAG(XftFont *, welcome_font);
  ADD_ALLOC_FLAG(XftFont *, input_font);
  ADD_ALLOC_FLAG(XftFont *, pass_font);
  ADD_ALLOC_FLAG(XftFont *, user_font);
  ADD_ALLOC_FLAG(char *, default_user);
  ADD_ALLOC_FLAG(char *, welcome_message);
  ADD_ALLOC_FLAG(char *, sessions);
  ADD_ALLOC_FLAG(char *, username_prompt);
  ADD_ALLOC_FLAG(char *, password_prompt);
};


struct cfg *get_cfg(Display *dpy);
void release_cfg(Display *dpy, struct cfg *cfg);
void position_to_coord(struct position *posn, int *x, int *y,
		       int width, int height, struct cfg* cfg, int is_text);

#endif /* _CFG_H_ */
