
#include <t1/t1.hpp>

struct mystruct
{
    int x;
};

bool operator==(mystruct l, mystruct r)
{
    return l.x == r.x;
}

t1_string t1_to_string(mystruct val)
{
    /*
    t1_string is a struct with two fields:

        struct t1_string
        {
            char *data;
            u64 size;
        };

    and t1_tprintf is a function which takes the same parameters as printf but
    returns a t1_string.
    */
    return t1_tprintf("mystruct{%d}", val.x);
}

struct otherstruct
{
    int y;
};

bool operator==(otherstruct l, otherstruct r)
{
    return l.y == r.y;
}

// alternative one-liner
define_t1_to_string(otherstruct val, "other{%d}", val.y);

define_test(a)
{
    assert_equal(1, 1);
    assert_equal(2, 2);

    mystruct a{5};
    mystruct b{5};
    mystruct c{20};
    assert_equal(a, b);
    // because t1_to_string is defined for mystruct, this shows more detail
    // than <unprintable>.
    assert_equal(a, c);

    otherstruct x{1};
    otherstruct y{2};
    assert_equal(x, y);
}

define_default_test_main();

