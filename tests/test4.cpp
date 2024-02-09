
#include <t1/t1.hpp>

struct mystruct
{
    int x;
};

bool operator==(mystruct l, mystruct r)
{
    return l.x == r.x;
}

define_test(a)
{
    assert_equal(1, 1);
    assert_equal(2, 2);

    mystruct a{5};
    mystruct b{5};
    mystruct c{20};
    assert_equal(a, b);
    assert_equal(a, c); // this will show <unprintable>
}

define_default_test_main();
