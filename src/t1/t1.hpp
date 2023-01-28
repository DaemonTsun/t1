// test header, inspired by vpp bugged

#pragma once

#include <vector>
#include <sstream>
#include <string>
#include <chrono>

namespace t1
{
namespace internal
{
template<typename T, typename C = void>
struct printable
{
    static std::string call(const T&)
    {
        static auto txt = std::string("<not printable : ") + typeid(T).name() + std::string(">");
        return txt;
    }
};

const char *get_filename(const char *path)
{
    if (path == nullptr)
        return nullptr;

    const char *last_slash = "";

    while (*path != '\0')
    {
        if (*path == '/')
            last_slash = path;
        
        path++;
    }

    return last_slash + 1;
}

template<typename T>
struct printable<T, std::void_t<decltype(std::declval<std::ostream&>() << std::declval<T>())>>
{
    static const std::string call(const T& obj)
    {
        std::stringstream ss;
        ss << obj;
        return ss.str();
    }
};

template<typename E>
struct exception_assert
{
	template<typename F>
	static bool call(const F& func, std::string& msg)
	{
		try
        {
			func();
		}
        catch(const E&)
        {
			return true;
		}
        catch(const std::exception& err)
        {
			msg = "std::exception: ";
			msg += err.what();
		}
        catch(...)
        {
			msg = "<not std::exception>";
		}

		return false;
	}
};

template<>
struct exception_assert<std::exception>
{
	template<typename F>
	static bool call(const F& func, std::string& msg)
	{
		try
        {
			func();
		}
        catch(const std::exception&)
        {
			return true;
		}
        catch(...)
        {
			msg = "<not std::exception>";
		}

		return false;
	}
};
}

template<typename T>
auto printable(const T& obj) -> decltype(::t1::internal::printable<T>::call(obj))
    { return ::t1::internal::printable<T>::call(obj); }

struct assert_info;
void exception_failed(const assert_info& info, const char *error, const std::string &other);

#define GENERIC_ERROR(fmt, ...)\
{\
    printf("\n");\
    printf(fmt __VA_OPT__(,) __VA_ARGS__);\
}

#define UNEXPECETED_EXCEPTION(fmt, ...)\
{\
    printf("\n[%s%s:%d%s %s%s%s] %sunexpected exception:%s\n  ",\
                  escape::source, ::t1::tests::current_unit->file, ::t1::tests::current_unit->line,\
                  escape::reset, escape::test_name, ::t1::tests::current_unit->name,\
                  escape::reset, \
                  escape::exception, escape::reset\
                  );\
    printf(fmt __VA_OPT__(,) __VA_ARGS__);\
}

#define ASSERT_FAILED2(INFO, ASRT, VALUE, EXPECTED, DESC)\
{\
    printf("\n[%s%s:%d%s %s%s%s] %sassert failed:%s\n  " ASRT "(%s, %s)\n  %s%s%s " DESC " %s%s%s\n",\
                  escape::source, INFO.file, info.line,\
                  escape::reset, escape::test_name, ::t1::tests::current_unit->name,\
                  escape::reset,\
                  escape::exception, escape::reset,\
                  INFO.str1, INFO.str2,\
                  escape::check_actual, printable(VALUE).c_str(), escape::reset,\
                  escape::check_expected, printable(EXPECTED).c_str(),\
                  escape::reset);\
\
    ::t1::tests::current_unit_failed = true;\
}

#define JOIN(X, Y) X##Y
#define JOIN3(X, Y, Z) X##Y##Z

#define define_test(NAME) \
    static void JOIN3(test_, NAME, _f)();\
    namespace { static const auto JOIN(test_, NAME) = ::t1::tests::add(\
            ::t1::unit{#NAME, JOIN3(test_, NAME, _f), t1::internal::get_filename(__FILE__), __LINE__}); } \
    static void JOIN3(test_, NAME, _f)()

// used internally
#define ASSERT_GENERIC2(ASRT, EXPR, EXPECTED) \
    {\
        if (! ASRT(::t1::assert_info{t1::internal::get_filename(__FILE__), __LINE__, #EXPR, #EXPECTED}, EXPR, EXPECTED) && ::t1::tests::stop_on_fail)\
            return;\
    }

// use these instead
#define assert_equal(EXPR, EXPECTED) \
    ASSERT_GENERIC2(::t1::assert_equal_, EXPR, EXPECTED)

#define assert_not_equal(EXPR, EXPECTED) \
    ASSERT_GENERIC2(::t1::assert_not_equal_, EXPR, EXPECTED)

#define assert_greater(EXPR, EXPECTED) \
    ASSERT_GENERIC2(::t1::assert_greater_, EXPR, EXPECTED)

#define assert_greater_or_equal(EXPR, EXPECTED) \
    ASSERT_GENERIC2(::t1::assert_greater_or_equal_, EXPR, EXPECTED)

#define assert_less(EXPR, EXPECTED) \
    ASSERT_GENERIC2(::t1::assert_less_, EXPR, EXPECTED)

#define assert_less_or_equal(EXPR, EXPECTED) \
    ASSERT_GENERIC2(::t1::assert_less_or_equal_, EXPR, EXPECTED)

#define assert_throws(EXPR, EXCEPT) \
{\
    ::t1::tests::total_asserts++;\
	std::string TEST_altMsg{};\
	if (!::t1::internal::exception_assert<EXCEPT>::call([&]{ EXPR; }, TEST_altMsg))\
    {\
        ::t1::tests::total_asserts_failed++;\
        ::t1::tests::current_unit_failed = true;\
        ::t1::exception_failed(::t1::assert_info{t1::internal::get_filename(__FILE__), __LINE__, #EXPR, #EXCEPT},\
                               #EXCEPT, TEST_altMsg.c_str());\
    }\
}

struct assert_info
{
    const char *file;
    int line;
    const char *str1; // the actual expression string
    const char *str2; // the expected value string, if any
};

struct unit
{
    using FuncPtr = void(*)();
    const char *name;
    FuncPtr func;
    const char *file;
    unsigned int line;
};

namespace escape
{
#if defined(__unix__) || defined(__linux__) || defined(__APPLE__)
    static const char* test_name = "\033[33m";
    static const char* check_expected = "\033[32m";
    static const char* check_actual = "\033[31m";
    static const char* error_expected = "\033[32m";
    static const char* exception = "\033[31m";
    static const char* source = "\033[36m";
    static const char* passed = "\033[32m";
    static const char* failed = "\033[31m";
    static const char* warn = "\033[33m";
    static const char* reset = "\033[0m";
#else
    static const char* test_name = "";
    static const char* check_expected = "";
    static const char* check_actual = "";
    static const char* error_expected = "";
    static const char* exception = "";
    static const char* source = "";
    static const char* passed = "";
    static const char* failed = "";
    static const char* warn = "";
    static const char* reset = "";
#endif
}

struct tests
{
  public:

    static std::vector<unit> units;
    static bool stop_on_fail;
    static bool last_passed;
    static bool verbose;
    static bool current_unit_failed;
    static unsigned int total_units_failed;
    static unsigned int total_units;
    static unsigned int total_asserts_failed;
    static unsigned int total_asserts;
    static unit *current_unit;

    static int add(const unit &u)
    {
        units.push_back(u);
        return 0;
    }

    static void run()
    {
        for (auto &unit : units)
        {
            auto start = std::chrono::high_resolution_clock::now();
            total_units++;
            current_unit_failed = false;
            current_unit = &unit;
            auto thrown = false;

            if (::t1::tests::verbose)
                printf("%s %s %s...", escape::test_name, unit.name, escape::reset);

            try
            {
                unit.func();
            }
            catch (const std::exception& exception)
            {
                thrown = true;
                UNEXPECETED_EXCEPTION("%s std::exception: %s%s\n", escape::exception, exception.what(), escape::reset);
            }
            catch (...)
            {
                thrown = true;
                UNEXPECETED_EXCEPTION("%s<Not a std::exception object>%s\n", escape::exception, escape::reset);
            }

            auto end = std::chrono::high_resolution_clock::now();

            last_passed = !(thrown || current_unit_failed);

            if (last_passed)
            {
                std::chrono::duration<double> diff = end-start;
                
                if (::t1::tests::verbose)
                    printf(" %spasses%s (%.5fs)", escape::passed, escape::reset, diff.count());
            }
            else
                total_units_failed++;
        }
    }
};

std::vector<unit> tests::units{};
bool tests::stop_on_fail = false;
bool tests::last_passed = false;
bool tests::verbose = false;
bool tests::current_unit_failed = false;
unsigned int tests::total_units_failed = 0;
unsigned int tests::total_units = 0;
unsigned int tests::total_asserts_failed = 0;
unsigned int tests::total_asserts = 0;
unit* tests::current_unit = 0;

#define DEFINE_ASSERT_OP2(NAME, OP, FAILDESC)\
template<typename T1, typename T2>\
bool JOIN(NAME, _)(const assert_info &info, T1 &&val, T2 &&expected)\
{\
    tests::total_asserts++;\
\
    if (val OP expected)\
        return true;\
\
    tests::total_asserts_failed++;\
    ASSERT_FAILED2(info, #NAME, val, expected, FAILDESC);\
    return false;\
}

DEFINE_ASSERT_OP2(assert_equal, ==, "does not equal expected value");
DEFINE_ASSERT_OP2(assert_not_equal, !=, "equals unexpected value");
DEFINE_ASSERT_OP2(assert_greater, >, "is not greater than");
DEFINE_ASSERT_OP2(assert_greater_or_equal, >=, "is not greater or equal to");
DEFINE_ASSERT_OP2(assert_less, <, "is not less than");
DEFINE_ASSERT_OP2(assert_less_or_equal, <=, "is not less or equal to");

void exception_failed(const assert_info& info, const char *error, const std::string &other)
{
    GENERIC_ERROR("[%s%s:%d%s %s%s%s] %s assert failed:%s\n  assert_throws(%s, %s)\n",
                  escape::source, info.file, info.line,
                  escape::reset, escape::test_name, ::t1::tests::current_unit->name,
                  escape::reset,
                  escape::exception, escape::reset,
                  info.str1, info.str2);

	if (!other.empty())
    {
		printf("  got unexpected exception: \n  %s%s%s\n\n",
			 	 escape::exception, other.c_str(), escape::reset);
	}
    else
		printf("  %sno exception was thrown%s\n\n",
			 	 escape::exception, escape::reset);
}

void print_results(unsigned int failed, unsigned int total, const char *name)
{
    if (total == 0)
    {
        printf("%sno %s found%s\n", escape::warn, name, escape::reset);
        return;
    }

    float pct = ((float)failed / total) * 100;

    printf("%s%u%s of %u (%s%.4f%%%s) %s failed\n",
           (failed > 0 ? escape::failed : escape::passed),
           failed, escape::reset, total,
           (failed >= total ? escape::failed : escape::passed),
           pct, escape::reset, name);
}

#define print_summary()\
{\
    if (::t1::tests::verbose && ::t1::tests::last_passed)\
        puts("");\
    printf("summary of %s%s%s\n", ::t1::escape::source, __FILE__, ::t1::escape::reset);\
\
    if (::t1::tests::total_asserts > 0)\
        ::t1::print_results(::t1::tests::total_asserts_failed, t1::tests::total_asserts, "asserts");\
    ::t1::print_results(::t1::tests::total_units_failed, t1::tests::total_units, "units");\
}

// use if needed
#define define_default_test_main() \
int main(int argc, const char *argv[])\
{\
    for (int i = 1; i < argc; ++i)\
    {\
        std::string arg = argv[i];\
\
        if (arg == "-b" || arg == "--break")\
            ::t1::tests::stop_on_fail = true;\
        else if (arg == "-v" || arg == "--verbose")\
            ::t1::tests::verbose = true;\
        \
    }\
\
    ::t1::tests::run();\
\
    print_summary();\
\
    if (::t1::tests::total_units_failed > 0)\
        return 1;\
\
    return 0;\
}

#define define_test_main(BEFORE_TESTS, AFTER_TESTS) \
int main(int argc, const char *argv[])\
{\
    for (int i = 1; i < argc; ++i)\
    {\
        std::string arg = argv[i];\
\
        if (arg == "-b" || arg == "--break")\
            ::t1::tests::stop_on_fail = true;\
        else if (arg == "-v" || arg == "--verbose")\
            ::t1::tests::verbose = true;\
        \
    }\
\
    BEFORE_TESTS;\
    ::t1::tests::run();\
    AFTER_TESTS;\
\
    print_summary();\
\
    if (::t1::tests::total_units_failed > 0)\
        return 1;\
\
    return 0;\
}

#undef DEFINE_ASSERT_OP2
#undef UNEXPECETED_EXCEPTION
#undef ASSERT_FAILED2
}
