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

TEST_F(LispTest, testEmpty1)
{
    value_t res = cl_eval_string("");
    EXPECT_TRUE(cf_isNIL(res));
}
TEST_F(LispTest, testEmpty2)
{
    value_t res = cl_eval_string("\n");
    EXPECT_TRUE(cf_isNIL(res));
}

TEST_F(LispTest, testAdd1)
{
    value_t res = cl_eval_string("(+ 1)");
   EXPECT_EQ(cf_num_val(res), 1);
}
TEST_F(LispTest, testAdd2)
{
    value_t res = cl_eval_string("(+ 1 2)");
    EXPECT_EQ(cf_num_val(res), 3);
}
TEST_F(LispTest, testAdd3)
{
    value_t res = cl_eval_string("\n(+ 1 2 3)");
    EXPECT_EQ(cf_num_val(res), 6);
}

TEST_F(LispTest, testSub1)
{
    value_t res = cl_eval_string("(- 1)");
    EXPECT_EQ(cf_num_val(res), -1);
}
TEST_F(LispTest, testSub2)
{
    value_t res = cl_eval_string("(- 1 2)");
    EXPECT_EQ(cf_num_val(res), -1);
}
TEST_F(LispTest, testSub3)
{
    value_t res = cl_eval_string("(- 1 2 3)");
    EXPECT_EQ(cf_num_val(res), -4);
}

TEST_F(LispTest, testMul1)
{
    value_t res = cl_eval_string("(* 2)");
    EXPECT_EQ(cf_num_val(res), 2);
}
TEST_F(LispTest, testMul2)
{
    value_t res = cl_eval_string("(* (- 2) 3)");
    EXPECT_EQ(cf_num_val(res), -6);
}
TEST_F(LispTest, testMul3)
{
    value_t res = cl_eval_string("(* (- 2) 3 (- 4))");
    EXPECT_EQ(cf_num_val(res), 24);
}

TEST_F(LispTest, testDiv1)
{
    value_t res = cl_eval_string("(/ 2)");
    // NOTE: 这是否合理呢？ [陈智鹏@2026-7-5]
    EXPECT_EQ(cf_num_val(res), 2);
}
TEST_F(LispTest, testDiv2)
{
    value_t res = cl_eval_string("(/ 4 2)");
    EXPECT_EQ(cf_num_val(res), 2);
}
TEST_F(LispTest, testDiv2_2)
{
    value_t res = cl_eval_string("(/ 3 2)");
    EXPECT_EQ(cf_num_val(res), 1);
}

TEST_F(LispTest, testLT_1)
{
    value_t res = cl_eval_string("(< 2 3)");
    EXPECT_FALSE(cf_isNIL(res));
}
TEST_F(LispTest, testLT_2)
{
    value_t res = cl_eval_string("(< 3 2)");
    EXPECT_TRUE(cf_isNIL(res));
}
TEST_F(LispTest, testLT_3)
{
    value_t res = cl_eval_string("(< 3 3)");
    EXPECT_TRUE(cf_isNIL(res));
}

TEST_F(LispTest, testGT_1)
{
    value_t res = cl_eval_string("(> 2 3)");
    EXPECT_TRUE(cf_isNIL(res));
}
TEST_F(LispTest, testGT_2)
{
    value_t res = cl_eval_string("(> 3 2)");
    EXPECT_FALSE(cf_isNIL(res));
}
TEST_F(LispTest, testGT_3)
{
    value_t res = cl_eval_string("(> 3 3)");
    EXPECT_TRUE(cf_isNIL(res));
}

TEST_F(LispTest, testSymbol_1)
{
    value_t res = cl_eval_string("(symbol? 1)");
    EXPECT_TRUE(cf_isNIL(res));
}
TEST_F(LispTest, testSymbol_2)
{
    value_t res = cl_eval_string("(symbol? def)");
    EXPECT_TRUE(cf_isNIL(res));
}
TEST_F(LispTest, testSymbol_3)
{
    value_t res = cl_eval_string("(def x 1)\n(symbol? x)");
    EXPECT_TRUE(cf_isNIL(res));
}
TEST_F(LispTest, testSymbol_4)
{
    value_t res = cl_eval_string("(def x 1)\n(symbol? 'x)");
    EXPECT_FALSE(cf_isNIL(res));
}

TEST_F(LispTest, testNumber_1)
{
    value_t res = cl_eval_string("(number? 1)");
    EXPECT_FALSE(cf_isNIL(res));
}
TEST_F(LispTest, testNumber_2)
{
    value_t res = cl_eval_string("(number? def)");
    EXPECT_TRUE(cf_isNIL(res));
}
TEST_F(LispTest, testNumber_3)
{
    value_t res = cl_eval_string("(def x 1)\n(number? x)");
    EXPECT_FALSE(cf_isNIL(res));
}
TEST_F(LispTest, testNumber_4)
{
    value_t res = cl_eval_string("(def x 1)\n(number? 'x)");
    EXPECT_TRUE(cf_isNIL(res));
}

TEST_F(LispTest, testBuiltin_1)
{
    value_t res = cl_eval_string("(builtin? 1)");
    EXPECT_TRUE(cf_isNIL(res));
}
TEST_F(LispTest, testBuiltin_2)
{
    value_t res = cl_eval_string("(builtin? def)");
    EXPECT_FALSE(cf_isNIL(res));
}
TEST_F(LispTest, testBuiltin_3)
{
    value_t res = cl_eval_string("(def x 1)\n(builtin? x)");
    EXPECT_TRUE(cf_isNIL(res));
}

