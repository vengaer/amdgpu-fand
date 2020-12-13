#ifndef MACRO_H
#define MACRO_H

#define array_size(x) \
    (sizeof(x) / sizeof(x[0]))

#define CAT(a,b) a ## b
#define CAT_EXPAND(a,b) CAT(a,b)

#endif /* MACRO_H */
