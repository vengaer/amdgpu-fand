#include "ipc.h"
#include "request.h"
#include "request_test.h"
#include "test.h"

void test_request_convert(void) {
    ipc_request req;

    fand_assert(request_convert("matrix", &req) == 0);
    fand_assert(req == ipc_req_matrix);

    fand_assert(request_convert("speed", &req) == 0);
    fand_assert(req == ipc_req_speed);

    fand_assert(request_convert("temp", &req) == 0);
    fand_assert(req == ipc_req_temp);

    fand_assert(request_convert("temperature", &req) == 0);
    fand_assert(req == ipc_req_temp);

    fand_assert(request_convert("asdf", &req) == -1);
    fand_assert(req == ipc_req_inval);
}
