#include "gtest/gtest.h"
#include "test_utils.h"
#include "stdio.h"

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

TEST_F(LispTest, testComments_1)
{
    value_t res = cl_eval_string("; (print 123)\n(print 456)");
    EXPECT_TRUE(cf_isNIL(res));
}
TEST_F(LispTest, testComments_2)
{
    value_t res = cl_eval_string("; (print 123);\n(+ 1 2)");
    EXPECT_EQ(cf_num_val(res), 3);
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

TEST_F(LispTest, testNot_1)
{
    value_t res = cl_eval_string("(not (< 1 2))");
    EXPECT_TRUE(cf_isNIL(res));
}
TEST_F(LispTest, testNot_2)
{
    value_t res = cl_eval_string("(not (> 1 2))");
    EXPECT_FALSE(cf_isNIL(res));
}

TEST_F(LispTest, testAnd_1)
{
    value_t res = cl_eval_string("(and (< 1 2) (< 2 3) (< 3 4))");
    EXPECT_FALSE(cf_isNIL(res));
}
TEST_F(LispTest, testAnd_2)
{
    value_t res = cl_eval_string("(and (< 1 2) (< 2 1))");
    EXPECT_TRUE(cf_isNIL(res));
}
TEST_F(LispTest, testAnd_3)
{
    value_t res = cl_eval_string("(and (> 1 2) (> 2 3))");
    EXPECT_TRUE(cf_isNIL(res));
}

TEST_F(LispTest, testOr_1)
{
    value_t res = cl_eval_string("(or (< 1 2) (< 2 3) (< 3 4))");
    EXPECT_FALSE(cf_isNIL(res));
}
TEST_F(LispTest, testOR_2)
{
    value_t res = cl_eval_string("(or (< 1 2) (< 2 1))");
    EXPECT_FALSE(cf_isNIL(res));
}
TEST_F(LispTest, testOR_3)
{
    value_t res = cl_eval_string("(or (> 1 2) (> 2 3))");
    EXPECT_TRUE(cf_isNIL(res));
}

#define PRINT_ALL(x)                        \
    do {                                    \
        value_t res = cl_eval_string(x);    \
        cf_print(res);                      \
        cf_smprint(res);                    \
    } while (0)

TEST_F(LispTest, testPrint)
{
    PRINT_ALL("");
    PRINT_ALL("(print `()");
    PRINT_ALL("(print '(1 '(1 2) 2))");
    PRINT_ALL("(print '(1 `(1 2) 2))");
    PRINT_ALL("(print '(1 ,(+ 1 2) 2))");
    PRINT_ALL("(print '(1 ,@(+ 1 2) 2))");
    PRINT_ALL("print `(1 2 3 4)");

    PRINT_ALL("def");
    PRINT_ALL("print");
    PRINT_ALL("(fn (x) (+ x 1))");

    // NOTE: list 内部函数有注释的情况 [陈智鹏@2026-7-5]
    PRINT_ALL("(print `(1 2 ;3 \n4)");
}
TEST_F(LispTest, testPrintUnbound)
{
    value_t unbound = cf_read_file("");
    cf_print(unbound);
    cf_smprint(unbound);
}

TEST_F(LispTest, testEval)
{
    value_t res = cl_eval_string("(eval `(+ 1 2 3 4))");
    EXPECT_EQ(cf_num_val(res), 10);
}

TEST_F(LispTest, testApply)
{
    value_t res = cl_eval_string("(apply + `(1 2 3 4))");
    EXPECT_EQ(cf_num_val(res), 10);
}

TEST_F(LispTest, testDo)
{
    // NOTE: flisp 的 do 和 Common Lisp 中的不一致 [陈智鹏@2026-7-5]
    value_t res = cl_eval_string("(do (print 1) (print 2) (+ 1 2))");
    EXPECT_EQ(cf_num_val(res), 3);
}

TEST_F(LispTest, testCons_1)
{
    value_t res = cl_eval_string("(= (cons 1 (list 2 3 4)) (list 1 2 3 4))");
    EXPECT_FALSE(cf_isNIL(res));
}
TEST_F(LispTest, testCons_2)
{
    value_t res = cl_eval_string("(= (cons 1 (list 3 4)) (list 1 2 4))");
    EXPECT_TRUE(cf_isNIL(res));
}

TEST_F(LispTest, testUnquote)
{
    value_t res = cl_eval_string("(def x '(1 2 3))\n`(= (list ,x) (list (1 2 3)))");
    EXPECT_FALSE(cf_isNIL(res));
}
TEST_F(LispTest, testUnquoteSplicing)
{
    value_t res = cl_eval_string("(def x '(1 2 3))\n`(= (list ,@x) (list 1 2 3))");
    EXPECT_FALSE(cf_isNIL(res));
}

#define MAX_NAME 256
static void symbolExitMaxName()
{
    char a[MAX_NAME + 2];
    memset(a, 'a', sizeof(a));
    char buf[MAX_NAME + 20];
    // NOTE: symbol 符号超过最大限制 [陈智鹏@2026-7-5]
    sprintf(buf, "(def %s 1)", a);
    cl_eval_string(buf);
}
TEST_F(LispTest, HandleExit_1)
{
    EXPECT_EXIT(symbolExitMaxName(), ::testing::ExitedWithCode(1), "");
}

static void unmatchedClosingParentesis()
{
    cl_eval_string(")");
}
TEST_F(LispTest, HandleExit_2)
{
    EXPECT_EXIT(unmatchedClosingParentesis(), ::testing::ExitedWithCode(1), "");
}
