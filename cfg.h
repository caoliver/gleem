#ifndef _CFG_H_
#define _CFG_H_

//// Resource names

// Main resources

#define MAIN_RESOURCE_PREFIX "gleem."

#define RNAME_IGNORE_CAPSLOCK ignore-capslock
#define RNAME_NUMLOCK numlock
#define RNAME_HIDE_MOUSE hide-mouse
#define RNAME_AUTO_LOGIN auto-login
#define RNAME_FOCUS_PASSWORD focus-password
#define RNAME_DEFAULT_USER default-user
#define RNAME_WELCOME_MESSAGE welcome-message
#define RNAME_MESSAGE_DURATION message-duration
#define RNAME_XINERAMA_SCREEN xinerama-screen
#define RNAME_THEME_DIRECTORY theme.directory
#define RNAME_THEME_SELECTION theme.selection
#define RNAME_PASS_PROMPT password.prompt.string
#define RNAME_USER_PROMPT username.prompt.string


// Theme resources

#define THEME_RESOURCE_PREFIX ""

// -- Screen placement

#define RNAME_PANEL_POSITION panel.position
#define RNAME_MESSAGE_POSITION message.position
#define RNAME_WELCOME_POSITION welcome.position
#define RNAME_PASSWORD_PROMPT_POSITION password.prompt.position
#define RNAME_PASSWORD_INPUT_POSITION password.input.position
#define RNAME_USERNAME_PROMPT_POSITION username.prompt.position
#define RNAME_USERNAME_INPUT_POSITION username.input.position

// -- Shadows offsets

#define RNAME_MESSAGE_SHADOW_OFFSET message.shadow.offset
#define RNAME_INPUT_SHADOW_OFFSET input.shadow.offset
#define RNAME_WELCOME_SHADOW_OFFSET welcome.shadow.offset
#define RNAME_PROMPT_SHADOW_OFFSET prompt.shadow.offset

// -- Input field dimensions

#define RNAME_PASSWORD_INPUT_WIDTH password.input.width
#define RNAME_USERNAME_INPUT_WIDTH username.input.width
#define RNAME_INPUT_HEIGHT input.height

// -- Colors

#define RNAME_CURSOR_COLOR cursor.color
#define RNAME_BACKGROUND_COLOR background.color
#define RNAME_PANEL_COLOR panel.color
#define RNAME_MESSAGE_COLOR message.color
#define RNAME_MESSAGE_SHADOW_COLOR message.shadow.color
#define RNAME_WELCOME_COLOR welcome.color
#define RNAME_WELCOME_SHADOW_COLOR welcome.shadow.color
#define RNAME_PROMPT_COLOR prompt.color
#define RNAME_PROMPT_SHADOW_COLOR prompt.shadow.color
#define RNAME_INPUT_COLOR input.color
#define RNAME_INPUT_SHADOW_COLOR input.shadow.color
#define RNAME_INPUT_ALTERNATE_COLOR input.alternate.color
#define RNAME_INPUT_HIGHLIGHT_COLOR input.highlight.color

// -- Fonts

#define RNAME_MESSAGE_FONT message.font
#define RNAME_WELCOME_FONT welcome.font
#define RNAME_INPUT_FONT input.font
#define RNAME_PROMPT_FONT prompt.font

// -- Image files

#define RNAME_BACKGROUND_FILE background.file
#define RNAME_PANEL_FILE panel.file

// -- Flags

#define RNAME_CURSOR_BLINK cursor.blink
#define RNAME_INPUT_HIGHLIGHT input.highlight

// -- Miscelaneous

#define RNAME_BACKGROUND_STYLE background-style
#define RNAME_PASS_DISPLAY password.input-display
#define RNAME_CURSOR_SIZE cursor.size
#define RNAME_CURSOR_OFFSET cursor.offset
#define RNAME_BUTTON_BOX button-box


//// Resource default values

// Cursor geometry isn't run time configurable.  Should it be?
#define DEFAULT_CURSOR_SIZE "3 22"
#define DEFAULT_CURSOR_OFFSET "1"

#define DEFAULT_THEME_DIRECTORY "./themes"
#define DEFAULT_THEME_SELECTION "default"

#define DEFAULT_MESSAGE_DURATION "3"
#define DEFAULT_XINERAMA_SCREEN "0"
#define DEFAULT_EXEC_DELAY "0"

#define DEFAULT_NUMLOCK "false"
#define DEFAULT_HIDE_MOUSE "false"
#define DEFAULT_IGNORE_CAPSLOCK "false"
#define DEFAULT_AUTO_LOGIN "false"
#define DEFAULT_FOCUS_PASSWORD "false"
#define DEFAULT_CURSOR_BLINK "false"
#define DEFAULT_INPUT_HIGHLIGHT "false"

#define DEFAULT_WELCOME_MESSAGE "Welcome to ~h"
#define DEFAULT_USER_PROMPT "Username:"
#define DEFAULT_PASS_PROMPT "Password:"
#define DEFAULT_PASS_MASK " "

#define DEFAULT_CURSOR_COLOR "gray50"
#define DEFAULT_BKGND_STYLE "color"
#define DEFAULT_BKGND_COLOR "black"
#define DEFAULT_PANEL_COLOR "gray30"
#define DEFAULT_MESSAGE_COLOR "white"
#define DEFAULT_MESSAGE_SHADOW_COLOR "gray"
#define DEFAULT_WELCOME_COLOR "white"
#define DEFAULT_WELCOME_SHADOW_COLOR "gray"
#define DEFAULT_PROMPT_COLOR "white"
#define DEFAULT_PROMPT_SHADOW_COLOR "gray"
#define DEFAULT_INPUT_COLOR "deepskyblue4"
#define DEFAULT_INPUT_ALTERNATE_COLOR "royalblue4"
#define DEFAULT_INPUT_SHADOW_COLOR "black"
#define DEFAULT_INPUT_HIGHLIGHT_COLOR "gray75"

#define DEFAULT_MESSAGE_FONT "Verdana:size=20:dpi=75"
#define DEFAULT_WELCOME_FONT "Verdana:size=20:dpi=75"
#define DEFAULT_INPUT_FONT "Verdana:size=20:dpi=75"
#define DEFAULT_PROMPT_FONT "Verdana:size=24:dpi=75"

#define DEFAULT_PANEL_POSN "50% 50%"
#define DEFAULT_PANEL_WIDTH 600
#define DEFAULT_PANEL_HEIGHT 300
#define DEFAULT_MESSAGE_POSN "50% 30% center"
#define DEFAULT_WELCOME_POSN "50% 40% center"
#define DEFAULT_PASS_PROMPT_POSN "160 180 left"
#define DEFAULT_PASS_INPUT_POSN "180 180 right"
#define DEFAULT_USER_PROMPT_POSN "160 180 left"
#define DEFAULT_USER_INPUT_POSN "180 180 right"
#define DEFAULT_MESSAGE_SHADOW_OFFSET "0 0"
#define DEFAULT_WELCOME_SHADOW_OFFSET "0 0"
#define DEFAULT_PROMPT_SHADOW_OFFSET "0 0"
#define DEFAULT_INPUT_HEIGHT "24"
#define DEFAULT_PASS_INPUT_WIDTH "250"
#define DEFAULT_USER_INPUT_WIDTH "250"
#define DEFAULT_INPUT_SHADOW_OFFSET "0 0"

#define DEFAULT_BUTTON_BOX NULL

// NO USER SERVICABLE PARTS BELOW


#define CURSOR_BLINK_SPEED 500

#define THEME_FILE_NAME "theme.defn"

#define MAX_THEMES 64
struct _ScreenSpecs {
  unsigned int xoffset;
  unsigned int yoffset;
  unsigned int width;
  unsigned int height;
  unsigned int total_width;
  unsigned int total_height;
};

typedef struct _ScreenSpecs ScreenSpecs;

struct _XYPosition {
  int x, y, flags;
};

typedef struct _XYPosition XYPosition;

#define ADD_ALLOC_FLAG(TYPE, NAME) TYPE NAME; int NAME##_ALLOC

struct _Cfg {
  struct image background_image, panel_image;
  int numlock, ignore_capslock, hide_mouse, auto_login, focus_password;
  int cursor_blink, input_highlight;
  int message_duration;
  ScreenSpecs screen_specs;
  int background_style;
  char password_mask;
  XYPosition panel_position;
  XYPosition message_position, welcome_position;
  XYPosition message_shadow_offset, welcome_shadow_offset;
  XYPosition password_prompt_position, username_prompt_position;
  XYPosition prompt_shadow_offset;
  XYPosition input_shadow_offset;
  XYPosition password_input_position, username_input_position;
  int password_input_width, username_input_width, input_height;
  XYPosition cursor_size;
  int cursor_offset;
  ADD_ALLOC_FLAG(XftColor, background_color);
  ADD_ALLOC_FLAG(XftColor, cursor_color);
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
  ADD_ALLOC_FLAG(char *, button_box);
  ADD_ALLOC_FLAG(char *, default_user);
  ADD_ALLOC_FLAG(char *, welcome_message);
  ADD_ALLOC_FLAG(char *, sessions);
  ADD_ALLOC_FLAG(char *, username_prompt);
  ADD_ALLOC_FLAG(char *, password_prompt);
  ADD_ALLOC_FLAG(char *, background_filename);
  ADD_ALLOC_FLAG(char *, panel_filename);
};

typedef struct _Cfg Cfg;

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


Cfg *get_cfg(Display *dpy);
void release_cfg(Display *dpy, Cfg *cfg);
void position_to_coord(XYPosition *posn, int width, int height, Cfg* cfg,
		       int is_text);
int translate_position(XYPosition *posn, int width, int height, Cfg* cfg,
		       int is_text);

#endif /* _CFG_H_ */
