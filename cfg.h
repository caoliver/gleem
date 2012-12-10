#ifndef _CFG_H_
#define _CFG_H_

#define THEME_FILE_NAME "theme.defn"

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

#define TRANSLATION_IS_CACHED 1

#define TRANSLATE_POSITION(POSITION, WIDTH, HEIGHT, CFG, IS_TEXT)	\
  (((POSITION)->flags & TRANSLATION_IS_CACHED)				\
   ? 0									\
   :  translate_position(POSITION, WIDTH, HEIGHT, CFG, IS_TEXT))


#define XY(X,Y) X,Y
#define TO_XY(POSITION) (POSITION).x, (POSITION).y
#define ASSIGN_XY_HELPER(VAR1, VAR2, VAL1, VAL2) VAR1 = VAL1, VAR2 = VAL2
#define ASSIGN_XY(VAR1, VAR2, ARG) ASSIGN_XY_HELPER(VAR1, VAR2, ARG)
#define ADD_XY_HELPER(X,Y,XDELTA,YDELTA) X+XDELTA,Y+YDELTA
#define ADD_XY(A,B) ADD_XY_HELPER(A,B)
#define ADD_POS(A,B) ADD_XY(TO_XY(A),XY(B))
#define NEGATE_XY_HELPER(X,Y) -X,-Y
#define NEGATE_XY(ARG) NEGATE_XY_HELPER(ARG)
#define NEGATE_POS(A) NEGATE_XY(TO_XY(A))

#define PANEL_POSITION_NAME panel_position

#define ADD_ALLOC_FLAG(TYPE, NAME) TYPE NAME; int NAME##_ALLOC

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
  struct image background_image, panel_image;
  int numlock, ignore_capslock, hide_mouse, auto_login, focus_password;
  int message_duration, command_count;
  struct screen_specs screen_specs;
  int background_style;
  char password_mask;
  struct command *commands;
  char *current_session;  // Pointer into session string.
  struct position PANEL_POSITION_NAME;
  struct position message_position, welcome_position;
  struct position message_shadow_offset, welcome_shadow_offset;
  struct position password_prompt_position, username_prompt_position;
  struct position prompt_shadow_offset;
  struct position input_shadow_offset;
  struct position password_input_position, username_input_position;
  struct position password_input_size, username_input_size;
  ADD_ALLOC_FLAG(XftColor, background_color);
  ADD_ALLOC_FLAG(XftColor, panel_color);
  ADD_ALLOC_FLAG(XftColor, message_color);
  ADD_ALLOC_FLAG(XftColor, message_shadow_color);
  ADD_ALLOC_FLAG(XftColor, welcome_color);
  ADD_ALLOC_FLAG(XftColor, welcome_shadow_color);
  ADD_ALLOC_FLAG(XftColor, prompt_color);
  ADD_ALLOC_FLAG(XftColor, prompt_shadow_color);
  ADD_ALLOC_FLAG(XftColor, input_color);
  ADD_ALLOC_FLAG(XftColor, input_alternate_color);
  ADD_ALLOC_FLAG(XftColor, input_highlight_color);
  ADD_ALLOC_FLAG(XftColor, input_shadow_color);
  ADD_ALLOC_FLAG(XftFont *, message_font);
  ADD_ALLOC_FLAG(XftFont *, welcome_font);
  ADD_ALLOC_FLAG(XftFont *, input_font);
  ADD_ALLOC_FLAG(XftFont *, prompt_font);
  ADD_ALLOC_FLAG(char *, default_user);
  ADD_ALLOC_FLAG(char *, welcome_message);
  ADD_ALLOC_FLAG(char *, sessions);
  ADD_ALLOC_FLAG(char *, username_prompt);
  ADD_ALLOC_FLAG(char *, password_prompt);
  ADD_ALLOC_FLAG(char *, background_filename);
  ADD_ALLOC_FLAG(char *, panel_filename);
};


struct cfg *get_cfg(Display *dpy);
void release_cfg(Display *dpy, struct cfg *cfg);
void position_to_coord(struct position *posn, int width, int height,
		       struct cfg* cfg, int is_text);
int translate_position(struct position *posn, int width, int height,
                        struct cfg* cfg, int is_text);

#endif /* _CFG_H_ */
