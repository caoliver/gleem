#ifndef _CFG_H_
#define _CFG_H_

#define DEFAULT_WELCOME_MESSAGE "Welcome to %host"
#define DEFAULT_MESSAGE_DURATION 3
#define DEFAULT_COMMAND_COUNT 16

#define DEFAULT_BKGND_STYLE "color"
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

#define DEFAULT_USER_PROMPT "Username: "
#define DEFAULT_PASS_PROMPT "Password: "

#define DEFAULT_PASS_MASK "*"

#define WITH_FREE_FLAG(TYPE, NAME) TYPE NAME; int NAME##_allocated

struct command {
  WITH_FREE_FLAG(char *, name);
  int action;  // Keyword whitespace [optional params]
  char* action_params;
  int delay;
  WITH_FREE_FLAG(char *, message);
  KeySym keysym;
  unsigned mod_state;
  WITH_FREE_FLAG(char *, allowed_users);
};

struct cfg {
  int numlock, ignore_capslock, hide_mouse;
  int auto_login, focus_password;
  int message_duration;
  WITH_FREE_FLAG(char *, default_user);
  WITH_FREE_FLAG(char *, welcome_message);
  WITH_FREE_FLAG(char *, sessions);
  char *current_session;  // whitespace list and ptr to current
  int command_count;
  struct command *commands;

  WITH_FREE_FLAG(XftColor, background_color);
  WITH_FREE_FLAG(XftColor, message_color);
  WITH_FREE_FLAG(XftColor, message_shadow_color);
  WITH_FREE_FLAG(XftColor, welcome_color);
  WITH_FREE_FLAG(XftColor, welcome_shadow_color);
  WITH_FREE_FLAG(XftColor, user_color);
  WITH_FREE_FLAG(XftColor, user_shadow_color);
  WITH_FREE_FLAG(XftColor, pass_color);
  WITH_FREE_FLAG(XftColor, pass_shadow_color);
  WITH_FREE_FLAG(XftColor, input_color);
  WITH_FREE_FLAG(XftColor, input_alternate_color);
  WITH_FREE_FLAG(XftColor, input_shadow_color);

  WITH_FREE_FLAG(XftFont *, message_font);
  WITH_FREE_FLAG(XftFont *, welcome_font);
  WITH_FREE_FLAG(XftFont *, input_font);
  WITH_FREE_FLAG(XftFont *, pass_font);
  WITH_FREE_FLAG(XftFont *, user_font);

  int background_style;

  WITH_FREE_FLAG(char *, username_prompt);
  WITH_FREE_FLAG(char *, password_prompt);
  char password_mask;
};


struct cfg *read_cfg(Display *dpy);
void free_cfg(Display *dpy, struct cfg *cfg);

#endif /* _CFG_H_ */
