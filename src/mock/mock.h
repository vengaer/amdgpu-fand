#ifndef MOCK_H
#define MOCK_H

#ifdef FAND_TEST_CONFIG
#define MOCKABLE(...)
#else
#define MOCKABLE(...) __VA_ARGS__
#endif

#endif /* MOCK_H */
