
#include <t1/t1.hpp>

define_test(a)
{
    assert_equal(1, 1);
    assert_equal(2, 2);
    float *f = new float;
}

define_default_test_main();
