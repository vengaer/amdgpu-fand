#ifndef REQUEST_H
#define REQUEST_H

int request_convert(char const *target, ipc_request *request);
int request_process_get(ipc_request request);
int request_process_exit(void);

#endif /* REQUEST_H */
