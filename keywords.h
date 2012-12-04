unsigned int
lookup_keyword(register const char *str, register unsigned int len);

// modifiers besides lock.

// Don't change the first five.  They are used as array indices.
#define KEYWORD_LEFT 1
#define KEYWORD_RIGHT 2
#define KEYWORD_ABOVE 3
#define KEYWORD_BELOW 4
#define KEYWORD_CENTER 5

#define KEYWORD_TRUE 6
#define KEYWORD_FALSE 7


#define KEYWORD_TILE 8
#define KEYWORD_STRETCH 9
#define KEYWORD_COLOR 10

#define KEYWORD_MOD1 11
#define KEYWORD_MOD2 12
#define KEYWORD_MOD3 13
#define KEYWORD_MOD4 14
#define KEYWORD_MOD5 15
#define KEYWORD_CTRL 16
#define KEYWORD_SHIFT 17

#define KEYWORD_C 18

// Action keywords
#define KEYWORD_EXEC 19
#define KEYWORD_SWITCH_SESSION 20
