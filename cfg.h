#ifndef _CFG_H_
#define _CFG_H_

#define DEFAULT_BKGND_STYLE KEYWORD_COLOR
#define DEFAULT_BKGND_COLOR "black"
#define DEFAULT_MESSAGE_COLOR "white"
#define DEFAULT_MESSAGE_SHADOW_COLOR "gray"
#define DEFAULT_WELCOME_COLOR "white"
#define DEFAULT_WELCOME_SHADOW_COLOR "gray"
#define DEFAULT_USER_COLOR "white"
#define DEFAULT_USER_SHADOW_COLOR "gray"
#define DEFAULT_PASS_COLOR "white"
#define DEFAULT_PASS_SHADOW_COLOR "gray"
#define DEFAULT_INPUT_COLOR "DeepSkyBlue4"
#define DEFAULT_INPUT_ALTERNATE_COLOR "RoyalBlue4"
#define DEFAULT_INPUT_SHADOW_COLOR "grey25"

#define DEFAULT_MESSAGE_FONT "Verdana:size=14:dpi=75"
#define DEFAULT_WELCOME_FONT "Verdana:size=14:dpi=75"
#define DEFAULT_INPUT_FONT "Verdana:size=14:dpi=75"
#define DEFAULT_USER_FONT "Verdana:size=14:dpi=75"
#define DEFAULT_PASS_FONT "Verdana:size=14:dpi=75"

struct command {
  char *name;
  int action;  // Keyword whitespace [optional params]
  char *action_params;
  int delay;
  char *message;
  KeySym keysym;
  unsigned mod_state;
  char *allowed_users;
};

struct cfg {
  int numlock, ignore_capslock, hide_mouse;
  int auto_login, focus_password;
  char *default_user;
  char *welcome_message;
  char *sessions, *current_session;  // whitespace list and ptr to current
  int command_count;
  struct command *commands;

  XftColor background_color;
  XftColor message_color, message_shadow_color;
  XftColor welcome_color, welcome_shadow_color;
  XftColor user_color, user_shadow_color;
  XftColor pass_color, pass_shadow_color;
  XftColor input_color, input_alternate_color, input_shadow_color;

  XftFont *message_font, *welcome_font, *input_font, *pass_font, *user_font;

  int background_style;

  char *username_prompt, *password_prompt;
  char password_display;
};


struct cfg *read_cfg(Display *dpy);
void free_cfg(Display *dpy, struct cfg *cfg);

#endif /* _CFG_H_ */
