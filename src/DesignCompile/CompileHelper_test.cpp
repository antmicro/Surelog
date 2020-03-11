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

struct CompileHelperTestStruct {
  std::vector<SURELOG::VObject> fc;
  UHDM::tf_call* expected;
  CompileHelperTestStruct(std::vector<SURELOG::VObject> file_content,
                          UHDM::tf_call* output) : fc(file_content),
                                                   expected(output)
                         {}
};

CompileHelperTestStruct testCases[] = {
  {
    // Vector of VObjects
    //{
    /*
     "", u<93> t<Seq_block> p<94> c<54> l<4>
     n<> u<94> t<Statement_item> p<95> c<93> l<4>
     n<> u<95> t<Statement> p<96> c<94> l<4>
     n<> u<96> t<Statement_or_null> p<97> c<95> l<4>
     n<> u<97> t<Initial_construct> p<98> c<96> l<4>
    */

    //},
	  {},0 //expected UHDM
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
                                                0,
                                                &cd);
    test_case.expected = funcCall;
    ASSERT_EQ(returned, test_case.expected);
  }
}

