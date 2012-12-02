unsigned int
lookup_keyword(register const char *str, register unsigned int len);

// modifiers besides lock.
#define KW_MOD1 1
#define KW_MOD2 2
#define KW_MOD3 3
#define KW_MOD4 4
#define KW_MOD5 5
#define KW_CTRL 6
#define KW_SHIFT 7

// Action keywords
#define KW_EXEC 8
#define KW_SWITCH_SESSION 9
