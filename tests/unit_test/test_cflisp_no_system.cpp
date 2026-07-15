#include "gtest/gtest.h"
#include "test_utils.h"
#include "stdio.h"
#include <stdlib.h>

class LispTestNoSystem : public ::testing::Test {
 protected:
  void SetUp() override {
    cf_lisp_init();
  }
};

TEST_F(LispTestNoSystem, test1)
{
    value_t res = cf_eval_string("((fn () (+ 1 2)))");
    EXPECT_EQ(cf_num_val(res), 3);
}

static void noEnoughArgs()
{
    cf_eval_string("((fn (x y) (+ x y)) 1)");
}
TEST_F(LispTestNoSystem, HandleExit_1)
{
    EXPECT_EXIT(noEnoughArgs(), ::testing::ExitedWithCode(1), "");
}

static void evalEmptyList()
{
    cf_eval_string("()");
}
TEST_F(LispTestNoSystem, HandleExit_2)
{
    EXPECT_EXIT(evalEmptyList(), ::testing::ExitedWithCode(1), "");
}
