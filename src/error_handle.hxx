#ifndef ERROR_HANDLE_HXX
#define ERROR_HANDLE_HXX

void unix_error(const char *msg);
void posix_error(int code, const char *msg);
void gai_error(int code, const char *msg);

#endif
