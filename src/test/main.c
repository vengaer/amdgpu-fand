#include "fanctrl_test.h"
#include "interpolation_test.h"
#include "mock_test.h"
#include "request_test.h"
#include "serialize_test.h"
#include "sha1_test.h"
#include "strutils_test.h"
#include "test.h"

int main(void) {
    section(sha1);
    run(test_sha1);

    section(interpolation);
    run(test_lerp);
    run(test_lerp_inverse);

    section(serialize);
    run(test_packf);
    run(test_packf_insufficient_bufsize);
    run(test_packf_invalid_fmtstring);
    run(test_packf_repeat);
    run(test_unpackf);
    run(test_unpackf_insufficient_bufsize);
    run(test_unpackf_invalid_fmtstring);
    run(test_unpackf_repeat);

    section(fanctrl);
    run(test_fanctrl_adjust);

    section(strutils);
    run(test_strscpy_result);
    run(test_strscpy_return_value);
    run(test_strscpy_null_termination);
    run(test_strsncpy_result);
    run(test_strsncpy_return_value);
    run(test_strsncpy_null_termination);

    section(request);
    run(test_request_convert);

    section(mock);
    run(test_validate_mock);

    return test_summary();;
}
