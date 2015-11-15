
#define THREAD_COUNT 16

extern void handelRequest(int sessionSock, void (*f) (int, void*), void* ptr);