#include "DesignCompile/CompileHelper.h"
#include "Expression/ExprBuilder.h"
#include "gtest/gtest.h"
#include "gmock/gmock.h"


int foo(int i) {
  return 4;
}

struct CompileHelperTestStruct {
  fC file_tree;
  gUHDM::tf_call* expected;
}


TEST(TestCompileTfCall, FirstTest) {
  SURELOG::CompileHelper dut;
  ASSERT_EQ(foo(4), 4);
  ASSERT_EQ(foo(5), 5);
}

