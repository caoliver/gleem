#ifndef _CFG_H_
#define _CFG_H_

struct command {
  char *name;
  char *action;  // Keyword whitespace [optional params]
  int delay;
  char *message;
  KeySym keysym;
  unsigned mod_mask, mod_state;
};

struct cfg {
  int numlock, hide_mouse;
  int auto_login, focus_password;
  char *default_user;
  char *welcome_message;
  char *sessions, *current_session;  // whitespace list and ptr to current
  int command_count;
  struct command *commands;
};


struct cfg *read_cfg(Display *dpy);
void free_cfg(struct cfg *cfg);

#endif /* _CFG_H_ */

