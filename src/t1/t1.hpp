// test header, inspired by vpp bugged

#pragma once

#include <time.h>
#include <stdlib.h>
#include <wchar.h>
#include <stdarg.h>

#define printf __builtin_printf
#include <stdio.h>
#undef printf

// ---------- PLATFORM ----------
#if defined(__linux__)
#define t1_Linux 1
#define t1_Windows 0
#define t1_Mac 0
#elif defined(_WIN32) || defined(_WIN64)
#define t1_Linux 0
#define t1_Windows 1
#define t1_Mac 0
#elif defined(__APPLE__)
#define t1_Linux 0
#define t1_Windows 0
#define t1_Mac 1
#endif

// ---------- INCLUDES ---------- 
#if t1_Windows
#include <windows.h>
#include <memoryapi.h>
#else
#include <sys/mman.h>
#include <sys/syscall.h>
#include <string.h>
#include <unistd.h>
#endif

// ---------- MACROS ----------
#ifndef JOIN
#define JOIN(X, Y) X##Y
#endif

#ifndef JOIN3
#define JOIN3(X, Y, Z) X##Y##Z
#endif

#ifndef defer
#if defined(_WIN32) || defined(_WIN64)
#pragma warning(push)
#pragma warning(disable : 4623) // implicitly deleted constructor
#pragma warning(disable : 4626) // implicitly deleted assignment operator
#endif
struct _t1_defer {};
template <class F> struct _t1_deferrer
{
    F f;
    ~_t1_deferrer()
    {
        f();
    }
};
#if defined(_WIN32) || defined(_WIN64)
#pragma warning(pop) 
#endif

template <class F>
_t1_deferrer<F> operator*(_t1_defer, F f)
{
    return {f};
}

#define _t1_DEFER2(X) zz_t1_defer##X
#define _t1_DEFER(X) _t1_DEFER2(X)
#define defer auto _t1_DEFER(__LINE__) = _t1_defer{} * [&]()
#endif

// ---------- NUMBER TYPES ----------
#include <stdint.h>

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t   s8;
typedef int16_t  s16;
typedef int32_t  s32;
typedef int64_t  s64;

// ---------- I/O ----------
#if t1_Windows
typedef void* t1_io_handle;

#define W32(r) __declspec(dllimport) r __stdcall
W32(t1_io_handle) GetStdHandle(int);
W32(int)          WriteFile(t1_io_handle, void *, int, int *, void *);
#else
typedef int t1_io_handle;
#endif

#if Windows
#define _stdin()    GetStdHandle(STD_INPUT_HANDLE)
#define _stdout()   GetStdHandle(STD_OUTPUT_HANDLE)
#define _stderr()   GetStdHandle(STD_ERROR_HANDLE)
#else
#define _stdin()    STDIN_FILENO
#define _stdout()   STDOUT_FILENO
#define _stderr()   STDERR_FILENO
#endif

static s64 t1_io_write(t1_io_handle h, void *buf, u64 size)
{
    s64 ret = 0;

#if Windows
    int tmp;

    if (!::WriteFile(h, buf, (int)size, &tmp, nullptr))
        return -1;

    ret = tmp;
#else
    ret = ::write(h, buf, size);
#endif

    return ret;
}

// ---------- TIME ----------
#define t1_NANOSECONDS_IN_A_SECOND 1000000000l

static inline void t1_get_time(timespec *t)
{
	timespec_get(t, TIME_UTC);
}

static void t1_get_timespec_difference(const timespec *start, const timespec *end, timespec *out)
{
    s64 diff = end->tv_nsec - start->tv_nsec;

	if (diff < 0)
    {
		out->tv_sec = end->tv_sec - start->tv_sec - 1;
		out->tv_nsec = t1_NANOSECONDS_IN_A_SECOND + diff;
	}
    else
    {
		out->tv_sec = end->tv_sec - start->tv_sec;
		out->tv_nsec = diff;
	}
}

static inline double t1_timespec_to_seconds(const timespec *t)
{
    double ret = static_cast<double>(t->tv_sec);
    ret += ((double)t->tv_nsec) / t1_NANOSECONDS_IN_A_SECOND;

    return ret;
}

static double t1_get_seconds_difference(const timespec *start, const timespec *end)
{
    timespec diff;
    t1_get_timespec_difference(start, end, &diff);

    return t1_timespec_to_seconds(&diff);
}

// ---------- MATH ----------
template<typename T>
constexpr inline T t1_ceil_exp2(T x)
{
    x--;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    if constexpr (sizeof(T) >= 2) x |= x >> 8;
    if constexpr (sizeof(T) >= 4) x |= x >> 16;
    if constexpr (sizeof(T) >= 8) x |= x >> 32;
    x++;

    return x;
}

// if multiple is power of 2
template<typename T1, typename T2>
static auto t1_ceil_multiple2(T1 x, T2 multiple)
{
    if (multiple == 0)
        return x;

    return (x + (multiple - 1)) & (-multiple);
}

// ---------- MEMORY ----------
static void *t1_allocate_memory(u64 size)
{
    return ::malloc(size);
}

template<typename T, bool Zero = false>
static T *t1_allocate_memory()
{
    return reinterpret_cast<T*>(t1_allocate_memory(sizeof(T)));
}

static void *t1_reallocate_memory(void *ptr, u64 size)
{
    return ::realloc(ptr, size);
}

template<typename T>
static T *t1_reallocate_memory(T *ptr, u64 n_elements)
{
    return reinterpret_cast<T*>(t1_reallocate_memory(reinterpret_cast<void*>(ptr), sizeof(T) * n_elements));
}

static void t1_free_memory(void *ptr)
{
    ::free(ptr);
}

// ---------- RING BUFFER ----------
#if t1_Linux
static int _memfd_create(const char *name, u32 flags)
{
    return syscall(SYS_memfd_create, name, flags);
}
#endif

#define T1_ALLOC_RETRY_COUNT 10

struct t1_ring_buffer
{
    char *data;
    u64 size;
    u32 mapping_count; // how many times the memory is mapped
};

static bool init(t1_ring_buffer *buf, u64 min_size, u32 mapping_count = 3)
{
    buf->data = nullptr;

#if t1_Linux
    u64 pagesize = (u64)sysconf(_SC_PAGESIZE);
    u64 actual_size = t1_ceil_multiple2(min_size, pagesize);

    int anonfd = _memfd_create("t1_ringbuf", 0);

    if (anonfd == -1)
        return false;

    defer { ::close(anonfd); };

    if (::ftruncate(anonfd, actual_size) == -1)
        return false;

    void *ptr = nullptr;
    int retry_count = 0;

    while (retry_count < T1_ALLOC_RETRY_COUNT)
    {
        bool all_ranges_mapped = true;

        // find an address that works
        ptr = ::mmap(nullptr, actual_size * mapping_count, PROT_NONE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);

        if (ptr == MAP_FAILED)
            return false;

        for (u32 i = 0; i < mapping_count; ++i)
        {
            void *mapped_ptr = ::mmap(((char*)ptr) + (i * actual_size), actual_size,
                                      PROT_READ | PROT_WRITE,
                                      MAP_FIXED | MAP_SHARED,
                                      anonfd, 0);

            if (mapped_ptr == MAP_FAILED)
            {
                // un-roll
                for (u32 j = 0; j < i; ++j)
                if (::munmap(((char*)ptr) + (j * actual_size), actual_size) != 0)
                    return false;

                retry_count += 1;
                all_ranges_mapped = false;
                break;
            }
        }

        if (all_ranges_mapped)
            break;
    }

    buf->data = (char*)ptr;
    buf->size = actual_size;
    buf->mapping_count = mapping_count;

    return true;
#elif t1_Windows
    SYSTEM_INFO info;
    GetSystemInfo(&info);

    u64 actual_size = ceil_multiple2(min_size, info.dwAllocationGranularity);
    u64 total_size = actual_size * mapping_count;

    HANDLE fd = CreateFileMappingA(INVALID_HANDLE_VALUE, 0, PAGE_READWRITE,
                                   (DWORD)(actual_size >> 32), (DWORD)(actual_size & 0xffffffff), 0);

    if (fd == INVALID_HANDLE_VALUE)
        return false;

    char *ptr = (char*)VirtualAlloc2(0, 0, total_size, MEM_RESERVE | MEM_RESERVE_PLACEHOLDER, PAGE_NOACCESS, 0, 0);
    bool mapped = true;

    for (u32 i = 0; i < mapping_count; ++i)
    {
        u64 offset = i * actual_size;
        VirtualFree(ptr + offset, actual_size, MEM_RELEASE | MEM_PRESERVE_PLACEHOLDER);

        void *mapping = MapViewOfFile3(fd, 0, ptr + offset, 0, actual_size, MEM_REPLACE_PLACEHOLDER, PAGE_READWRITE, 0, 0);

        if (mapping == nullptr)
        {
            // un-roll
            for (u32 j = 0; j < i; ++j)
                UnmapViewOfFile(ptr + (j * actual_size));

            mapped = false;

            break;
        }
    }

    CloseHandle(fd);

    buf->data = (char*)ptr;
    buf->size = actual_size;
    buf->mapping_count = mapping_count;

    return true;
#else
    return false;
#endif
}

static bool free(t1_ring_buffer *buf)
{
    if (buf == nullptr)
        return true;

    if (buf->data == nullptr)
        return true;

#if t1_Linux
    for (u32 i = 0; i < buf->mapping_count; ++i)
    if (::munmap(buf->data + (i * buf->size), buf->size) != 0)
        return false;

    buf->data = nullptr;

    return true;
#elif t1_Windows
    for (u32 i = 0; i < buf->mapping_count; ++i)
        UnmapViewOfFile(buf->data + (i * buf->size));

    buf->data = nullptr;

    return true;
#else
    return false;
#endif
}

// ---------- ARRAY ----------
template<typename T>
struct t1_array
{
    typedef T value_type;

    T *data;
    u64 size;
    u64 reserved_size;

          T &operator[](u64 index)       { return data[index]; }
    const T &operator[](u64 index) const { return data[index]; }
};

template<typename T>
static void init(t1_array<T> *arr)
{
    arr->data = nullptr;
    arr->size = 0;
    arr->reserved_size = 0;
}

template<typename T>
static T *t1_add_elements(t1_array<T> *arr, u64 n_elements)
{
    if (n_elements == 0)
        return nullptr;

    u64 nsize = arr->size + n_elements; 

    if (nsize < arr->reserved_size)
    {
        T *ret = arr->data + arr->size;
        arr->size = nsize;
        return ret;
    }

    u64 new_reserved_size = t1_ceil_exp2(arr->reserved_size + n_elements);

    T *n = t1_reallocate_memory<T>(arr->data, new_reserved_size);

    if (n == nullptr)
        return nullptr;

    arr->data = n;

    T *ret = arr->data;

    if (ret != nullptr)
        ret += arr->size;

    arr->size = nsize;
    arr->reserved_size = new_reserved_size;

    return ret;
}

template<typename T>
static inline T *t1_add_at_end(t1_array<T> *arr)
{
    return t1_add_elements(arr, 1);
}

template<typename T>
static inline T *t1_add_at_end(t1_array<T> *arr, T val)
{
    T *ret = t1_add_at_end(arr);
    *ret = val;
    return ret;
}

template<typename T>
static void free(t1_array<T> *arr)
{
    if (arr->data != nullptr)
        t1_free_memory(arr->data);

    arr->data = nullptr;
    arr->size = 0;
    arr->reserved_size = 0;
}

// ---------- STRINGS ----------
struct t1_string
{
    char *data;
    u64 size;
};

// ---------- FORMAT ----------
#define t1_TFORMAT_RING_BUFFER_MIN_SIZE 16384

struct t1_tformat_buffer
{
    t1_ring_buffer buffer;
    u64 offset;
};

static void _t1_format_buffer_cleanup();

static t1_tformat_buffer *_get_static_format_buffer(bool free_buffer = false)
{
    static t1_tformat_buffer _buf{};

    if (free_buffer && _buf.buffer.data != nullptr)
    {
        free(&_buf.buffer);
        _buf.offset = 0;
    }
    else if (!free_buffer && _buf.buffer.data == nullptr)
    {
        if (!init(&_buf.buffer, t1_TFORMAT_RING_BUFFER_MIN_SIZE, 2))
            return nullptr;

        ::atexit(_t1_format_buffer_cleanup);
    }

    return &_buf;
}

static void _t1_format_buffer_cleanup()
{
    _get_static_format_buffer(true);
}

static t1_string t1_tvprintf(const char *fmt, va_list args)
{
    t1_tformat_buffer *buf = _get_static_format_buffer();

    if (buf == nullptr)
        return t1_string{nullptr, 0};

    char *data = buf->buffer.data;
    u64 size = buf->buffer.size - 1;
    u64 offset = buf->offset;

    int bytes_written = vsnprintf(data + offset, size, fmt, args);

    if (bytes_written < 0)
        return t1_string{nullptr, 0};

    data[offset + bytes_written] = '\0';

    t1_string ret{data + offset, (u64)bytes_written};

    buf->offset += bytes_written + 1;

    while (buf->offset > buf->buffer.size)
        buf->offset -= buf->buffer.size;

    return ret;
}

static t1_string t1_tprintf(const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    t1_string ret = t1_tvprintf(fmt, args);
    va_end(args);

    return ret;
}

// ---------- OUTPUT ----------
static void t1_printf(const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    t1_string ret = t1_tvprintf(fmt, args);
    va_end(args);

    if (ret.size > 0)
        t1_io_write(_stdout(), ret.data, ret.size);
}

#define printf t1_printf

static t1_string _t1_to_string(bool x)
{
    if (x)
        return t1_string{(char*)"true", 4};
    else
        return t1_string{(char*)"false", 5};
}

#define define_t1_to_string(TypedVal, Format, ...)\
t1_string _t1_to_string(TypedVal)\
{\
    return t1_tprintf(Format __VA_OPT__(,) __VA_ARGS__);\
}

#define define_direct_t1_to_string(Type, Format)\
    define_t1_to_string(Type x, Format, x)

define_direct_t1_to_string(char,               "%c");
define_direct_t1_to_string(unsigned char,      "%hhu");
define_direct_t1_to_string(signed char,        "%hhd");
define_direct_t1_to_string(short,              "%hd");
define_direct_t1_to_string(unsigned short,     "%hu");
define_direct_t1_to_string(int,                "%d");
define_direct_t1_to_string(unsigned int,       "%u");
define_direct_t1_to_string(long,               "%ld");
define_direct_t1_to_string(unsigned long,      "%lu");
define_direct_t1_to_string(long long,          "%lld");
define_direct_t1_to_string(unsigned long long, "%llu");

define_direct_t1_to_string(float, "%.f");
define_direct_t1_to_string(double, "%.lf");
define_direct_t1_to_string(void*, "%p");
define_direct_t1_to_string(char*, "%s");
define_direct_t1_to_string(const char*, "%s");
#if t1_Windows
define_direct_t1_to_string(wchar_t*, "%ws");
define_direct_t1_to_string(const wchar_t *, "%ws");
#endif

void t1_set_unprintable_was_called();

static t1_string t1_to_string(...)
{
    static char _unprintable[] = "<unprintable>";
    t1_set_unprintable_was_called();
    return t1_string{_unprintable, 13};
}

template<typename T>
static auto t1_to_string(T val)
    -> decltype(_t1_to_string(val))
{
    return _t1_to_string(val);
}

// ---------- MISC ----------
const char *t1_get_filename(const char *path)
{
    if (path == nullptr)
        return nullptr;

    const char *last_slash = nullptr;

    while (*path != '\0')
    {
        bool is_sep = false;

#if Windows
        is_sep = (*path == '/' || *path == '\\');
#else
        is_sep = (*path == '/');
#endif

        if (is_sep)
            last_slash = path;
        
        path++;
    }

    if (last_slash == nullptr)
        return path;

    return last_slash + 1;
}

// ---------- TESTS ----------
#if t1_Windows
#define t1_COLOR_TEST_NAME ""
#define t1_COLOR_CHECK_EXPECTED ""
#define t1_COLOR_CHECK_ACTUAL ""
#define t1_COLOR_ERROR_EXPECTED ""
#define t1_COLOR_EXCEPTION ""
#define t1_COLOR_SOURCE ""
#define t1_COLOR_PASSED ""
#define t1_COLOR_FAILED ""
#define t1_COLOR_WARN ""
#define t1_COLOR_RESET ""
#else
#define t1_COLOR_TEST_NAME "\033[33m"
#define t1_COLOR_CHECK_EXPECTED "\033[32m"
#define t1_COLOR_CHECK_ACTUAL "\033[31m"
#define t1_COLOR_ERROR_EXPECTED "\033[32m"
#define t1_COLOR_EXCEPTION "\033[31m"
#define t1_COLOR_SOURCE "\033[36m"
#define t1_COLOR_PASSED "\033[32m"
#define t1_COLOR_FAILED "\033[31m"
#define t1_COLOR_WARN "\033[33m"
#define t1_COLOR_RESET "\033[0m"
#endif


struct t1_assert_info
{
    const char *file;
    int line;
    const char *str1; // the actual expression string
    const char *str2; // the expected value string, if any
};

struct t1_unit
{
    using FuncPtr = void(*)();
    const char *name;
    FuncPtr func;
    const char *file;
    unsigned int line;
};

struct t1_tests
{
    static t1_array<t1_unit> units;
    static bool stop_on_fail;
    static bool last_passed;
    static bool verbose;
    static bool current_unit_failed;
    static bool unprintable_called;
    static unsigned int total_units_failed;
    static unsigned int total_units;
    static unsigned int total_asserts_failed;
    static unsigned int total_asserts;
    static double total_seconds;
    static t1_unit *current_unit;

    static int add(const t1_unit &u)
    {
        t1_add_at_end(&units, u);
        return 0;
    }

    static void run()
    {
        for (u64 i = 0; i < units.size; ++i)
        {
            t1_unit *unit = units.data + i;

            total_units++;
            current_unit_failed = false;
            current_unit = unit;

            if (t1_tests::verbose)
                printf("%s %s %s...", t1_COLOR_TEST_NAME, unit->name, t1_COLOR_RESET);

            timespec start_time;
            timespec end_time;

            t1_get_time(&start_time);
            unit->func();
            t1_get_time(&end_time);

            last_passed = !current_unit_failed;
            double diff_seconds = t1_get_seconds_difference(&start_time, &end_time);

            total_seconds += diff_seconds;

            if (last_passed)
            {
                if (t1_tests::verbose)
                    printf(" %spasses%s (%.12fs)", t1_COLOR_PASSED, t1_COLOR_RESET, diff_seconds);
            }
            else
                total_units_failed++;
        }
    }
};

t1_array<t1_unit> t1_tests::units{};
bool t1_tests::stop_on_fail = false;
bool t1_tests::last_passed = false;
bool t1_tests::verbose = false;
bool t1_tests::current_unit_failed = false;
bool t1_tests::unprintable_called = false;
unsigned int t1_tests::total_units_failed = 0;
unsigned int t1_tests::total_units = 0;
unsigned int t1_tests::total_asserts_failed = 0;
unsigned int t1_tests::total_asserts = 0;
double t1_tests::total_seconds = 0.0;
t1_unit* t1_tests::current_unit = 0;

void t1_set_unprintable_was_called()
{
    t1_tests::unprintable_called = true;
}

#define ASSERT_FAILED2(INFO, ASRT, VALUE, EXPECTED, DESC)\
{\
    printf("\n[%s%s:%d%s %s%s%s] %sassert failed:%s\n  " ASRT "(%s, %s)\n  %s%s%s " DESC " %s%s%s\n",\
           t1_COLOR_SOURCE, INFO.file, info.line,\
           t1_COLOR_RESET, t1_COLOR_TEST_NAME, t1_tests::current_unit->name,\
           t1_COLOR_RESET,\
           t1_COLOR_EXCEPTION, t1_COLOR_RESET,\
           INFO.str1, INFO.str2,\
           t1_COLOR_CHECK_ACTUAL, t1_to_string(VALUE).data, t1_COLOR_RESET,\
           t1_COLOR_CHECK_EXPECTED, t1_to_string(EXPECTED).data,\
           t1_COLOR_RESET);\
\
    t1_tests::current_unit_failed = true;\
}

#define define_test(NAME) \
    static void JOIN3(test_, NAME, _f)();\
    namespace { static const auto JOIN(test_, NAME) = t1_tests::add(\
            t1_unit{#NAME, JOIN3(test_, NAME, _f), t1_get_filename(__FILE__), __LINE__}); } \
    static void JOIN3(test_, NAME, _f)()

#define DEFINE_ASSERT_OP2(NAME, OP, FAILDESC)\
template<typename T1, typename T2>\
bool JOIN(NAME, _)(const t1_assert_info &info, T1 &&val, T2 &&expected)\
{\
    t1_tests::total_asserts++;\
\
    if (val OP expected)\
        return true;\
\
    t1_tests::total_asserts_failed++;\
    ASSERT_FAILED2(info, #NAME, val, expected, FAILDESC);\
    return false;\
}

DEFINE_ASSERT_OP2(assert_equal, ==, "does not equal expected value");
DEFINE_ASSERT_OP2(assert_not_equal, !=, "equals unexpected value");
DEFINE_ASSERT_OP2(assert_greater, >, "is not greater than");
DEFINE_ASSERT_OP2(assert_greater_or_equal, >=, "is not greater or equal to");
DEFINE_ASSERT_OP2(assert_less, <, "is not less than");
DEFINE_ASSERT_OP2(assert_less_or_equal, <=, "is not less or equal to");


// used internally
#define ASSERT_GENERIC2(ASRT, EXPR, EXPECTED) \
    {\
        if (! ASRT(t1_assert_info{t1_get_filename(__FILE__), __LINE__, #EXPR, #EXPECTED}, EXPR, EXPECTED) && t1_tests::stop_on_fail)\
            return;\
    }

#define assert_equal(EXPR, EXPECTED) ASSERT_GENERIC2(assert_equal_, EXPR, EXPECTED)
#define assert_not_equal(EXPR, EXPECTED) ASSERT_GENERIC2(assert_not_equal_, EXPR, EXPECTED)
#define assert_greater(EXPR, EXPECTED) ASSERT_GENERIC2(assert_greater_, EXPR, EXPECTED)
#define assert_greater_or_equal(EXPR, EXPECTED) ASSERT_GENERIC2(assert_greater_or_equal_, EXPR, EXPECTED)
#define assert_less(EXPR, EXPECTED) ASSERT_GENERIC2(assert_less_, EXPR, EXPECTED)
#define assert_less_or_equal(EXPR, EXPECTED) ASSERT_GENERIC2(assert_less_or_equal_, EXPR, EXPECTED)

static void t1_print_results(unsigned int failed, unsigned int total, const char *name)
{
    if (total == 0)
    {
        printf("%sno %s found%s\n", t1_COLOR_WARN, name, t1_COLOR_RESET);
        return;
    }

    float pct = ((float)failed / (float)total) * 100;

    printf("%s%u%s of %u (%s%.4f%%%s) %s failed\n",
           (failed > 0 ? t1_COLOR_FAILED : t1_COLOR_PASSED),
           failed, t1_COLOR_RESET, total,
           (failed >= total ? t1_COLOR_FAILED : t1_COLOR_PASSED),
           pct, t1_COLOR_RESET, name);
}

static void t1_print_summary()
{
    if (t1_tests::verbose && t1_tests::last_passed)
        puts("");

    printf("summary of %s%s%s\n", t1_COLOR_SOURCE, __FILE__, t1_COLOR_RESET);

    if (t1_tests::total_asserts > 0)
        t1_print_results(t1_tests::total_asserts_failed, t1_tests::total_asserts, "asserts");

    t1_print_results(t1_tests::total_units_failed, t1_tests::total_units, "units");
    printf("total time: %.12fs\n", t1_tests::total_seconds);

    if (t1_tests::unprintable_called)
    {
        printf("\n%sNOTE: One or more values could not be printed (%s<unprintable>%s), use\n"
                "%sdefine_t1_to_string(YourType x, \"%%d\", x.field, ...)%s to enable printing values\n"
                "of YourType. See %st1/tests/test5.cpp%s for an example.%s\n",
                t1_COLOR_WARN,
                t1_COLOR_FAILED, t1_COLOR_WARN,
                t1_COLOR_SOURCE, t1_COLOR_WARN,
                t1_COLOR_SOURCE, t1_COLOR_WARN,
                t1_COLOR_RESET
                );
    }
}

void t1_nop(){}

#define define_test_main(BEFORE_TESTS, AFTER_TESTS) \
int main(int argc, const char *argv[])\
{\
    for (int i = 1; i < argc; ++i)\
    {\
        const char *arg = argv[i];\
\
        if (strcmp(arg, "-b") == 0 || strcmp(arg, "--break") == 0)\
            t1_tests::stop_on_fail = true;\
        else if (strcmp(arg, "-v") == 0 || strcmp(arg, "--verbose") == 0)\
            t1_tests::verbose = true;\
    }\
\
    BEFORE_TESTS();\
    t1_tests::run();\
    AFTER_TESTS();\
\
    t1_print_summary();\
\
    free(&t1_tests::units);\
\
    if (t1_tests::total_units_failed > 0)\
        return 1;\
\
    return 0;\
}

#define define_default_test_main()\
    define_test_main(t1_nop, t1_nop)
