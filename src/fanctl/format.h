#ifndef FORMAT_H
#define FORMAT_H

#include "ipc.h"
#include "serialize.h"

int format(union unpack_result const *result, ipc_request req, ipc_response rsp);

#endif /* FORMAT_H */
