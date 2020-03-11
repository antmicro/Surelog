#include "SourceCompile/SymbolTable.h"
#include "Library/Library.h"
#include "Design/FileContent.h"
#include "ErrorReporting/Error.h"
#include "ErrorReporting/Location.h"
#include "ErrorReporting/Error.h"
#include "ErrorReporting/ErrorDefinition.h"
#include "ErrorReporting/ErrorContainer.h"
#include "SourceCompile/CompilationUnit.h"
#include "SourceCompile/PreprocessFile.h"
#include "SourceCompile/CompileSourceFile.h"
#include "CommandLine/CommandLineParser.h"
#include "SourceCompile/ParseFile.h"
#include "Testbench/ClassDefinition.h"
#include "SourceCompile/Compiler.h"
#include "DesignCompile/CompileDesign.h"
#include "DesignCompile/ResolveSymbols.h"
#include "DesignCompile/DesignElaboration.h"
#include "DesignCompile/NetlistElaboration.h"
#include "DesignCompile/UVMElaboration.h"
#include "DesignCompile/CompilePackage.h"
#include "DesignCompile/CompileModule.h"
#include "DesignCompile/CompileFileContent.h"
#include "DesignCompile/CompileProgram.h"
#include "DesignCompile/CompileClass.h"
#include "DesignCompile/Builtin.h"
#include "DesignCompile/PackageAndRootElaboration.h"
#include "DesignCompile/UhdmWriter.h"

#include "Design/VObject.h"
#include "SourceCompile/PreprocessFile.h"
#include "SourceCompile/CompileSourceFile.h"
#include "SourceCompile/Compiler.h"
#include "DesignCompile/CompileDesign.h"
#include "Design/FileContent.h"
#include "Expression/ExprBuilder.h"

#include "DesignCompile/CompileHelper.h"
#include "gtest/gtest.h"
#include "gmock/gmock.h"

class MockCompileDesign : public SURELOG::CompileDesign {
  public:
    MockCompileDesign() : CompileDesign(nullptr) {};
};

class MockFileContent : public SURELOG::FileContent {
  public:
    MockFileContent() : FileContent(0, nullptr, nullptr, nullptr, nullptr, 0){};
    void set_objects(std::vector<SURELOG::VObject> objs){m_objects = objs;}
};

using SURELOG::VObject;
struct CompileHelperTestStruct {
  std::vector<VObject> fc;
  UHDM::tf_call* expected;
  CompileHelperTestStruct(std::vector<SURELOG::VObject> file_content,
                          UHDM::tf_call* output) : fc(file_content),
                                                   expected(output)
                         {}
};

CompileHelperTestStruct testCases[] = {
  {
    // Simplest case: foo();
    {
      // Vector of VObjects
      // n<>    u<19> t<Subroutine_call>   p<20> c<17>       l<3>
      // n<foo> u<17> t<StringConst>       p<19>       s<18> l<3>
      // n<>    u<18> t<List_of_arguments> p<19>             l<3>
      //
      // Constructor call:
      // (nameId, fileId, type, line, parent, definition, child, sibling)
     { 0, 0, VObjectType::slSubroutine_call,   3, 20, 1,  17, 0},
     { 1/*"foo"*/, 0, VObjectType::slStringConst,       3, 19, 17, 0,  18},
     { 0, 0, VObjectType::slList_of_arguments, 3, 19, 18, 0,  0}
    },
    0 //expected UHDM
  }
};

using ::testing::NiceMock;

TEST(TestCompileTfCall, FirstTest) {
  SURELOG::CompileHelper dut;
  NiceMock<MockCompileDesign> cd;
  MockFileContent fc;
  UHDM::Serializer& s = cd.getSerializer();

  UHDM::tf_call* funcCall = s.MakeFunc_call();//new UHDM::func_call();
  funcCall->VpiName("foo");
  for (auto test_case : testCases) {
    fc.set_objects(test_case.fc);
    UHDM::tf_call* returned = dut.compileTfCall(&fc,
                                                1,
                                                &cd);
    test_case.expected = funcCall;
    ASSERT_EQ(returned, test_case.expected);
  }
}

