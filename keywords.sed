# ANSI inline rather than GNU inline

s/\(static unsigned int\)/INLINE_DECL \1/

# Return the keyword number rather than the keyword structure

s/^const struct keyword \*/static INLINE_DECL unsigned int/
s/return &wordlist\[key\];/return wordlist[key].code;/
