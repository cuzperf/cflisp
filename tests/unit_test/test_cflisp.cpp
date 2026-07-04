#include "gtest/gtest.h"
#include "test_utils.h"


class LispTest : public ::testing::Test {
 protected:
  void SetUp() override {
    cf_lisp_init();
    // NOTE: 工作路径必须是 system.lsp 所在路径 [陈智鹏@2026-6-27]
    value_t sexp = cf_read_file("system.lsp");
    value_t res = cf_eval_toplevel(sexp);
    EXPECT_TRUE(cf_isNIL(res));
  }
};

TEST_F(LispTest, testEmpty)
{
    value_t res = cl_eval_string("");
    EXPECT_TRUE(cf_isNIL(res));
}

TEST_F(LispTest, testAdd)
{
    value_t res = cl_eval_string("(+ 1 2)");
    EXPECT_EQ(num_val(res), 3);
}

TEST_F(LispTest, testAdd2)
{
    value_t res = cl_eval_string("(+ 1 2)\n(- 5)");
    EXPECT_EQ(num_val(res), -5);
}
