#include "gtest/gtest.h"

TEST(example, add)
{
    double res;
    res = 1.0 + 2.0;
    ASSERT_NEAR(res, 3.0, 1.0e-11);
}
