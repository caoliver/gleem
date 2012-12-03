unsigned int
lookup_keyword(register const char *str, register unsigned int len);

// modifiers besides lock.
#define KEYWORD_MOD1 1
#define KEYWORD_MOD2 2
#define KEYWORD_MOD3 3
#define KEYWORD_MOD4 4
#define KEYWORD_MOD5 5
#define KEYWORD_CTRL 6
#define KEYWORD_SHIFT 7

#define KEYWORD_CENTER 8
#define KEYWORD_LEFT 9
#define KEYWORD_RIGHT 10
#define KEYWORD_BELOW 11
#define KEYWORD_ABOVE 12

#define KEYWORD_C 13

#define KEYWORD_TILE 14
#define KEYWORD_STRETCH 15
#define KEYWORD_COLOR 16

// Action keywords
#define KEYWORD_EXEC 17
#define KEYWORD_SWITCH_SESSION 18
