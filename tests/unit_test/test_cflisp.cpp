#include "gtest/gtest.h"
#include "test_utils.h"
#include "stdio.h"
#include <stdlib.h>

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
    value_t res = cf_eval_string("");
    EXPECT_TRUE(cf_isNIL(res));
}
TEST_F(LispTest, testEmpty2)
{
    value_t res = cf_eval_string("\n");
    EXPECT_TRUE(cf_isNIL(res));
}

TEST_F(LispTest, testComments_1)
{
    value_t res = cf_eval_string("; (print 123)\n(print 456)");
    EXPECT_TRUE(cf_isNIL(res));
}
TEST_F(LispTest, testComments_2)
{
    value_t res = cf_eval_string("; (print 123);\n(+ 1 2)");
    EXPECT_EQ(cf_num_val(res), 3);
}

TEST_F(LispTest, testAdd1)
{
    value_t res = cf_eval_string("(+ 1)");
   EXPECT_EQ(cf_num_val(res), 1);
}
TEST_F(LispTest, testAdd2)
{
    value_t res = cf_eval_string("(+ 1 2)");
    EXPECT_EQ(cf_num_val(res), 3);
}
TEST_F(LispTest, testAdd3)
{
    value_t res = cf_eval_string("\n(+ 1 2 3)");
    EXPECT_EQ(cf_num_val(res), 6);
}

TEST_F(LispTest, testSub1)
{
    value_t res = cf_eval_string("(- 1)");
    EXPECT_EQ(cf_num_val(res), -1);
}
TEST_F(LispTest, testSub2)
{
    value_t res = cf_eval_string("(- 1 2)");
    EXPECT_EQ(cf_num_val(res), -1);
}
TEST_F(LispTest, testSub3)
{
    value_t res = cf_eval_string("(- 1 2 3)");
    EXPECT_EQ(cf_num_val(res), -4);
}

TEST_F(LispTest, testMul1)
{
    value_t res = cf_eval_string("(* 2)");
    EXPECT_EQ(cf_num_val(res), 2);
}
TEST_F(LispTest, testMul2)
{
    value_t res = cf_eval_string("(* (- 2) 3)");
    EXPECT_EQ(cf_num_val(res), -6);
}
TEST_F(LispTest, testMul3)
{
    value_t res = cf_eval_string("(* (- 2) 3 (- 4))");
    EXPECT_EQ(cf_num_val(res), 24);
}

TEST_F(LispTest, testDiv1)
{
    value_t res = cf_eval_string("(/ 2)");
    // NOTE: 这是否合理呢？ [陈智鹏@2026-7-5]
    EXPECT_EQ(cf_num_val(res), 2);
}
TEST_F(LispTest, testDiv2)
{
    value_t res = cf_eval_string("(/ 4 2)");
    EXPECT_EQ(cf_num_val(res), 2);
}
TEST_F(LispTest, testDiv2_2)
{
    value_t res = cf_eval_string("(/ 3 2)");
    EXPECT_EQ(cf_num_val(res), 1);
}

TEST_F(LispTest, testLT_1)
{
    value_t res = cf_eval_string("(< 2 3)");
    EXPECT_FALSE(cf_isNIL(res));
}
TEST_F(LispTest, testLT_2)
{
    value_t res = cf_eval_string("(< 3 2)");
    EXPECT_TRUE(cf_isNIL(res));
}
TEST_F(LispTest, testLT_3)
{
    value_t res = cf_eval_string("(< 3 3)");
    EXPECT_TRUE(cf_isNIL(res));
}

TEST_F(LispTest, testGT_1)
{
    value_t res = cf_eval_string("(> 2 3)");
    EXPECT_TRUE(cf_isNIL(res));
}
TEST_F(LispTest, testGT_2)
{
    value_t res = cf_eval_string("(> 3 2)");
    EXPECT_FALSE(cf_isNIL(res));
}
TEST_F(LispTest, testGT_3)
{
    value_t res = cf_eval_string("(> 3 3)");
    EXPECT_TRUE(cf_isNIL(res));
}

TEST_F(LispTest, testSymbol_1)
{
    value_t res = cf_eval_string("(symbol? 1)");
    EXPECT_TRUE(cf_isNIL(res));
}
TEST_F(LispTest, testSymbol_2)
{
    value_t res = cf_eval_string("(symbol? def)");
    EXPECT_TRUE(cf_isNIL(res));
}
TEST_F(LispTest, testSymbol_3)
{
    value_t res = cf_eval_string("(def x 1)\n(symbol? x)");
    EXPECT_TRUE(cf_isNIL(res));
}
TEST_F(LispTest, testSymbol_4)
{
    value_t res = cf_eval_string("(def x 1)\n(symbol? 'x)");
    EXPECT_FALSE(cf_isNIL(res));
}

TEST_F(LispTest, testNumber_1)
{
    value_t res = cf_eval_string("(number? 1)");
    EXPECT_FALSE(cf_isNIL(res));
}
TEST_F(LispTest, testNumber_2)
{
    value_t res = cf_eval_string("(number? def)");
    EXPECT_TRUE(cf_isNIL(res));
}
TEST_F(LispTest, testNumber_3)
{
    value_t res = cf_eval_string("(def x 1)\n(number? x)");
    EXPECT_FALSE(cf_isNIL(res));
}
TEST_F(LispTest, testNumber_4)
{
    value_t res = cf_eval_string("(def x 1)\n(number? 'x)");
    EXPECT_TRUE(cf_isNIL(res));
}

TEST_F(LispTest, testBuiltin_1)
{
    value_t res = cf_eval_string("(builtin? 1)");
    EXPECT_TRUE(cf_isNIL(res));
}
TEST_F(LispTest, testBuiltin_2)
{
    value_t res = cf_eval_string("(builtin? def)");
    EXPECT_FALSE(cf_isNIL(res));
}
TEST_F(LispTest, testBuiltin_3)
{
    value_t res = cf_eval_string("(def x 1)\n(builtin? x)");
    EXPECT_TRUE(cf_isNIL(res));
}

TEST_F(LispTest, testNot_1)
{
    value_t res = cf_eval_string("(not (< 1 2))");
    EXPECT_TRUE(cf_isNIL(res));
}
TEST_F(LispTest, testNot_2)
{
    value_t res = cf_eval_string("(not (> 1 2))");
    EXPECT_FALSE(cf_isNIL(res));
}
TEST_F(LispTest, testNot_3)
{
    value_t res = cf_eval_string("(not `#t)");
    EXPECT_TRUE(cf_isNIL(res));
}
TEST_F(LispTest, testNot_4)
{
    value_t res = cf_eval_string("(not `())");
    EXPECT_FALSE(cf_isNIL(res));
}

TEST_F(LispTest, testAnd_1)
{
    value_t res = cf_eval_string("(and (< 1 2) (< 2 3) (< 3 4))");
    EXPECT_FALSE(cf_isNIL(res));
}
TEST_F(LispTest, testAnd_2)
{
    value_t res = cf_eval_string("(and (< 1 2) (< 2 1))");
    EXPECT_TRUE(cf_isNIL(res));
}
TEST_F(LispTest, testAnd_3)
{
    value_t res = cf_eval_string("(and (> 1 2) (> 2 3))");
    EXPECT_TRUE(cf_isNIL(res));
}

TEST_F(LispTest, testOr_1)
{
    value_t res = cf_eval_string("(or (< 1 2) (< 2 3) (< 3 4))");
    EXPECT_FALSE(cf_isNIL(res));
}
TEST_F(LispTest, testOR_2)
{
    value_t res = cf_eval_string("(or (< 1 2) (< 2 1))");
    EXPECT_FALSE(cf_isNIL(res));
}
TEST_F(LispTest, testOR_3)
{
    value_t res = cf_eval_string("(or (> 1 2) (> 2 3))");
    EXPECT_TRUE(cf_isNIL(res));
}

#define PRINT_ALL(x)                        \
    do {                                    \
        value_t res = cf_eval_string(x);    \
        cf_println(res);                    \
        cf_mprint(res);                     \
    } while (0)

TEST_F(LispTest, testPrint)
{
    PRINT_ALL("");
    PRINT_ALL("`()");
    PRINT_ALL("'(1 '(1 2) 2)");
    PRINT_ALL("'(1 `(1 2) 2)");
    PRINT_ALL("'(1 ,(+ 1 2) 2)");
    PRINT_ALL("'(1 ,@(+ 1 2) 2)");
    PRINT_ALL("`(1 2 3 4)");

    PRINT_ALL("def");
    PRINT_ALL("print");
    PRINT_ALL("(fn (x) (+ x 1))");

    // NOTE: list 内部函数有注释的情况 [陈智鹏@2026-7-5]
    PRINT_ALL("`(1 2 ;3 \n4)");
}
TEST_F(LispTest, testPrintUnbound)
{
    value_t unbound = cf_read_file("");
    cf_println(unbound);
    cf_mprint(unbound);
}

TEST_F(LispTest, testEval)
{
    value_t res = cf_eval_string("(eval `(+ 1 2 3 4))");
    EXPECT_EQ(cf_num_val(res), 10);
}

TEST_F(LispTest, testApply)
{
    value_t res = cf_eval_string("(apply + `(1 2 3 4))");
    EXPECT_EQ(cf_num_val(res), 10);
}

TEST_F(LispTest, testDo)
{
    // NOTE: flisp 的 do 和 Common Lisp 中的不一致 [陈智鹏@2026-7-5]
    value_t res = cf_eval_string("(do (print 1) (print 2) (+ 1 2))");
    EXPECT_EQ(cf_num_val(res), 3);
}

TEST_F(LispTest, testCons_1)
{
    value_t res = cf_eval_string("(= (cons 1 (list 2 3 4)) (list 1 2 3 4))");
    EXPECT_FALSE(cf_isNIL(res));
}
TEST_F(LispTest, testCons_2)
{
    value_t res = cf_eval_string("(= (cons 1 (list 3 4)) (list 1 2 4))");
    EXPECT_TRUE(cf_isNIL(res));
}

TEST_F(LispTest, testUnquote)
{
    value_t res = cf_eval_string("(def x '(1 2 3))\n`(= (list ,x) (list (1 2 3)))");
    EXPECT_FALSE(cf_isNIL(res));
}
TEST_F(LispTest, testUnquoteSplicing)
{
    value_t res = cf_eval_string("(def x '(1 2 3))\n`(= (list ,@x) (list 1 2 3))");
    EXPECT_FALSE(cf_isNIL(res));
}

TEST_F(LispTest, testNoTailEval)
{
    value_t res = cf_eval_string("((fn (x) (+ x 1) (* x 2)) 5)");
    EXPECT_FALSE(cf_isNIL(res));
}

TEST_F(LispTest, testGc)
{
    const char* pChar =
        "(defun bigList (n)"
        "  (if (= n 0) '()"
        "  (cons n (bigList(- n 1)))))\n"
        "(def l (bigList 6000))\n"
        "(head l)\n"
        "(defun doubleGc (n)"
        "  (if (= n 0) '()"
        "    (let ((inner (bigList 4000)))"
        "      (cons inner (doubleGc (- n 1))))))\n"
        "(doubleGc 5)";
    value_t res = cf_eval_string(pChar);
    cf_println(res);
    EXPECT_FALSE(cf_isNIL(res));
}

TEST_F(LispTest, testExample)
{
    value_t sexp = cf_read_file("examples/example.lsp");
    value_t res = cf_eval_toplevel(sexp);
    EXPECT_TRUE(cf_isNIL(res));
}

static void testGc1()
{
    const char* pChar =
        "(defun bigList (n)"
        "  (if (= n 0) '()"
        "    (cons n (bigList(- n 1)))))\n"
        "(def l (bigList 6000))\n"
        "(head l)\n"
        "(defun doubleGc (n)"
        "  (if (= n 0) '()"
        "    (let ((inner (bigList 4000)))"
        "      (cons inner (doubleGc (- n 1))))))\n"
        "(doubleGc 5)";
    value_t res = cf_eval_string(pChar);
    cf_println(res);
    EXPECT_FALSE(cf_isNIL(res));
    exit(0);
}
TEST_F(LispTest, HandleGC_Exit_1)
{
    // 单独运行避免对其它测例带来影响
    EXPECT_EXIT(testGc1(), ::testing::ExitedWithCode(0), "");
}

static void testGc2()
{
    const char* pChar1 =
        "(defun bigList (n)\n"
          "(if (= n 0) '()\n"
            "(cons n (bigList(- n 1)))))\n"
        "(def l (bigList 6000))\n"
        "(head l)\n"
        "(defun doubleGc (n)\n"
          "(if (= n 0) '()\n"
            "(let ((inner (bigList 4000)))\n"
              "(cons inner (doubleGc (- n 1))))))\n"
        "(doubleGc 5)\n"
        "(defun range (n)\n"
          "(if (= n 0) '() (cons n (range (- n 1)))))\n"
        "(= (head (range 2000)) 2000)";
    value_t res1 = cf_eval_string(pChar1);
    cf_println(res1);
    EXPECT_FALSE(cf_isNIL(res1));

    const char* pChar2 =
        "(defun assert (expected actual)"
        "  (if (= expected actual) 'pass"
        "    (do (print 'FAIL) (print '-expected) (print expected)"
        "        (print '-actual) (print actual) 'fail)))\n"
        "(defun check (label val expected)"
        "  (print label) (assert expected val))\n"
        "(defun range (n)"
        "  (if (= n 0) '() (cons n (range (- n 1)))))\n"
        "(check 'gc (head (range 2000)) 2000)";
    value_t res2 = cf_eval_string(pChar2);
    cf_println(res2);
    EXPECT_FALSE(cf_isNIL(res2));

    exit(0);
}
TEST_F(LispTest, HandleGC_Exit_2)
{
    // 单独运行避免对其它测例带来影响
    EXPECT_EXIT(testGc2(), ::testing::ExitedWithCode(0), "");
}

#define MAX_NAME 256
static void symbolExitMaxName()
{
    char a[MAX_NAME + 2];
    memset(a, 'a', sizeof(a));
    char buf[MAX_NAME + 20];
    // NOTE: symbol 符号超过最大限制 [陈智鹏@2026-7-5]
    sprintf(buf, "(def %s 1)", a);
    cf_eval_string(buf);
}
TEST_F(LispTest, HandleExit_1)
{
    EXPECT_EXIT(symbolExitMaxName(), ::testing::ExitedWithCode(1), "");
}

static void unmatchedClosingParentesis()
{
    cf_eval_string(")");
}
TEST_F(LispTest, HandleExit_2)
{
    EXPECT_EXIT(unmatchedClosingParentesis(), ::testing::ExitedWithCode(1), "");
}

static void applyNotAFunction()
{
    cf_eval_string("(1 2)");
}
TEST_F(LispTest, HandleExit_3)
{
    EXPECT_EXIT(applyNotAFunction(), ::testing::ExitedWithCode(1), "");
}

#define STACK_SIZE (160 * 1024)
static void stackOverflow()
{
    char* a = (char*)malloc(STACK_SIZE * 2 + 4);
    a[0] = '(';
    a[1] = '+';
    a[2] = ' ';
    for (int i = 0; i < STACK_SIZE; ++i) {
        a[i * 2 + 3] = '1';
        a[i * 2 + 4] = ' ';
    }
    a[STACK_SIZE * 2 + 2] = ')';
    a[STACK_SIZE * 2 + 3] = '\0';
    cf_eval_string(a);
}
TEST_F(LispTest, HandleExit_4)
{
    EXPECT_EXIT(stackOverflow(), ::testing::ExitedWithCode(1), "");
}

static void envStackOverflow()
{
    cf_eval_string("(defun f (x) (f x) (f x))\n(f 0)");
}
TEST_F(LispTest, HandleExit_5)
{
    EXPECT_EXIT(envStackOverflow(), ::testing::ExitedWithCode(1), "");
}
