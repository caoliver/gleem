# Generate C definitions for KEYWORD_<foo>

generate_headers()
{
    cat >keywords.h <<EOF
// Automatically generated by keywords.sh
// Don't edit sil vous plait

unsigned int
lookup_keyword(register const char *str, register unsigned int len);
EOF
    keynum=1
    while read keyword dummy; do
	[ -z "$keyword" -o "$keyword" == '#' ] && continue
	echo "#define KEYWORD_$keyword $keynum" >>keywords.h
	keynum=$(($keynum+1))
    done
}

# Create a gperf spec where a given keyword number might have several names.
make_gperf()
{
    cat <<EOF
%{
#include <string.h>
#include "keywords.h"
#if __STDC_VERSION__ >= 199901L
#define INLINE_DECL inline
#elif defined(__GNUC__)
#define INLINE_DECL __inline
#else
#define INLINE_DECL
#endif
%}
%readonly-tables
%language=ANSI-C
%define hash-function-name keyword_hash
%define lookup-function-name lookup_keyword
%compare-strncmp
struct keyword { char *name; int code; };
%%
EOF
    while read keyword tokens; do
	[ -z "$keyword" -o "$keyword" == '#' ] && continue
	for t in $tokens; do
	    echo "$t, KEYWORD_$keyword"
	done
    done
}

# Sed fixes for gperf output.

# Try to use ANSI inline if available.
sedprg="s/\(static unsigned int\)/INLINE_DECL \1/"

# keyword lookup should return int in [1, keyword_count]
# or 0 for no match.
sedprg="$sedprg;s/^const struct keyword \*/unsigned int/"
sedprg="$sedprg;s/return &wordlist\[key\];/return wordlist[key].code;/"

generate_lookup_function()
{
    make_gperf | gperf -t | sed "$sedprg" >keywords.c
}

####
#### Keywords definitions are here.
####

for pass in generate_headers generate_lookup_function;do
    $pass <<TOKEN_DESCRIPTIONS

# Placement token numbers are used as index, so this arrangement
# must be preserved.
LEFT left l
RIGHT right r
ABOVE above a
BELOW below b
CENTER center

# Modifiers
MOD1 mod1 m1
MOD2 mod2 m2
MOD3 mod3 m3
MOD4 mod4 m4
MOD5 mod5 m5
SHIFT shift s
CTRL ctrl

# Booleans
TRUE yes on true
FALSE no off false

# Background styles
COLOR color
TILE tile
STRETCH stretch

# Ambiguous.  Could mean center, could mean ctrl.
C c

# Actions
EXEC exec
SWITCH_SESSION switch-session

TOKEN_DESCRIPTIONS
done
