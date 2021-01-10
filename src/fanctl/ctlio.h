#ifndef CTLIO_H
#define CTLIO_H

#include <stdio.h>

#ifdef FAND_FUZZ_CONFIG
#define ctl_perror(...) do { } while(0)
#define ctl_fprintf(...) do { } while(0)
#define ctl_fputs(...) do { }  while(0)
#else
#define ctl_perror(...) perror(__VA_ARGS__)
#define ctl_fprintf(...) fprintf(__VA_ARGS__)
#define ctl_fputs(...) fputs(__VA_ARGS__)
#endif

#endif /* CTLIO_H */
