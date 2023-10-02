#ifndef FILE_PROCESS_HXX
#define FILE_PROCESS_HXX

#include <cstddef>

void Close(int fd);

void Write(int fd, const char *buf, std::size_t size);

#endif
