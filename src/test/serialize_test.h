#ifndef SERIALIZE_TEST_H
#define SERIALIZE_TEST_H

void test_packf(void);
void test_packf_insufficient_bufsize(void);
void test_packf_invalid_fmtstring(void);
void test_packf_repeat(void);

void test_unpackf(void);
void test_unpackf_insufficient_bufsize(void);
void test_unpackf_invalid_fmtstring(void);
void test_unpackf_repeat(void);

#endif /* SERIALIZE_TEST_H */
