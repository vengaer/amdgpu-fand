#ifndef MACRO_H
#define MACRO_H

#define array_size(x) \
    (sizeof(x) / sizeof(x[0]))

#define CAT(a,b) a ## b
#define CAT_EXPAND(a,b) CAT(a,b)

#define STR(x) #x
#define STR_EXPAND(x) STR(x)

#endif /* MACRO_H */
