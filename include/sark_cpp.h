#pragma GCC diagnostic ignored "-Wwrite-strings"

extern "C"
{
  #include <sark.h>
}
#include <cstdlib>
#include <new>

inline void* operator new(size_t size) noexcept {
    return sark_alloc(1, size);
}

inline void operator delete(void *p) noexcept {
    sark_free(p);
}

inline void* operator new[](size_t size) noexcept {
    return operator new(size); // Same as regular new
}

inline void operator delete[](void *p) noexcept {
    operator delete(p); // Same as regular delete
}

inline void* operator new(size_t size, std::nothrow_t) noexcept {
    return operator new(size); // Same as regular new
}

inline void operator delete(void *p,  std::nothrow_t) noexcept {
    operator delete(p); // Same as regular delete
}

inline void* operator new[](size_t size, std::nothrow_t) noexcept {
    return operator new(size); // Same as regular new
}

inline void operator delete[](void *p,  std::nothrow_t) noexcept {
    operator delete(p); // Same as regular delete
}

extern "C" inline void __cxa_pure_virtual() {
    io_printf(IO_BUF, "Pure Virtual Function called");
    rt_error(RTE_SWERR);
}
