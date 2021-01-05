


inline int double2int(double d)
{
    d += 6755399441055744.0;
    return reinterpret_cast<int&>(d);
}

#ifdef __APPLE__
#define COMPILER_BARRIER() asm volatile("" ::: "memory")
#define POPCOUNT(x) (__builtin_popcount(x))
#endif

#if defined _WIN32 || defined _WIN64
#include <intrin.h>
#define COMPILER_BARRIER() _ReadWriteBarrier()
#define POPCOUNT(x) __popcnt(x)
#endif
