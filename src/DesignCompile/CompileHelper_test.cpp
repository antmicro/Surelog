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
#include "vpi_visitor.h"

class MockFileContent : public SURELOG::FileContent {
  public:
    MockFileContent() : FileContent(0, nullptr, nullptr, nullptr, nullptr, 0){};
    void set_objects(std::vector<SURELOG::VObject> objs){m_objects = objs;}
};

using SURELOG::VObject;
struct CompileHelperTestStruct {
  std::vector<VObject> objects;
  //std::initializer_list<UHDM::func_call> expected;
  UHDM::func_call* expected;
  SURELOG::SymbolTable symbols;
  CompileHelperTestStruct(std::vector<VObject> file_content,
                          std::vector<std::string> strings,
                          int type, UHDM::function* ptr,
                          std::vector<UHDM::any*> args
                          )
                           : objects(file_content)
                         {
                           for (auto s : strings)
                             symbols.registerSymbol(s);
                           expected = new UHDM::func_call{type, ptr};

                           VectorOfany arguments;
                           for (auto a : args)
                             arguments.push_back(a);

                           expected->Tf_call_args(&arguments);
                         }
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
     { 0, 0, VObjectType::slSubroutine_call,   3, 20, 1,  1, 0},
     { 1, 0, VObjectType::slStringConst,       3, 0, 2, 0,  2},
     { 0, 0, VObjectType::slList_of_arguments, 3, 0, 3, 0,  0}
    },
    // Symbol table
    {"foo"},
    // UHDM func_call initializers
    1, nullptr,
    // Argument vector initializer list
    {},
  },
  {
    // dsp("%d",clk);
    {
      // Vector of VObjects
      // n<> u<0> t<Subroutine_call> p<35> c<1> l<4>
      //    n<dsp> u<1> t<StringConst> p<0> s<2> l<4>
      //    n<> u<2> t<List_of_arguments> p<0> c<3> l<3>
      //        n<> u<3> t<Expression> p<2> c<4> s<7> l<3>
      //            n<> u<4> t<Primary> p<3> c<5> l<3>
      //                n<> u<5> t<Primary_literal> p<4> c<6> l<3>
      //                    n<"%d"> u<6> t<StringLiteral> p<5> l<4>
      //        n<> u<7> t<Expression> p<2> c<8> l<4>
      //            n<> u<8> t<Primary> p<7> c<9> l<4>
      //                n<> u<9> t<Primary_literal> p<8> c<10> l<4>
      //                    n<clk> u<10> t<StringConst> p<9> l<4>
      // Constructor call:
      // (nameId, fileId, type, line, parent, definition, child, sibling)
     { 0, 0, VObjectType::slSubroutine_call,         4, 35,  0,  1, 0},
       { 1, 0, VObjectType::slStringConst,           4,  0,  1,  0, 2},
       { 0, 0, VObjectType::slList_of_arguments,     4,  0,  2,  3, 0},
         { 0, 0, VObjectType::slExpression,          4,  2,  3,  4, 7},
           { 0, 0, VObjectType::slPrimary,           4,  3,  4,  5, 0},
             { 0, 0, VObjectType::slPrimary_literal, 4,  4,  5,  6, 0},
               { 2, 0, VObjectType::slStringLiteral, 4,  5,  6,  0, 0},
         { 0, 0, VObjectType::slExpression,          4,  2,  7,  8, 0},
           { 0, 0, VObjectType::slPrimary,           4,  7,  8,  9, 0},
             { 0, 0, VObjectType::slPrimary_literal, 4,  8,  9, 10, 0},
               { 3, 0, VObjectType::slStringConst,   4,  9, 10,  0, 0},
    },
    // Symbol table
    {"dsp", "%d", "clk"},
    // UHDM func_call initializers
    1, new UHDM::function(),
    // Argument vector initializer list
    {new UHDM::constant(nullptr, 0, 0, 0 ,0, false)},
  }
};

TEST(TestCompileTfCall, FirstTest) {
  SURELOG::CompileHelper dut;
  SURELOG::CompileDesign cd(nullptr);
  MockFileContent fc;
  UHDM::Serializer& s = cd.getSerializer();

  for (auto test_case : testCases) {
    fc.set_objects(test_case.objects);
    fc.setSymbolTable(&test_case.symbols);
    UHDM::tf_call* returned = dut.compileTfCall(&fc,
                                                0,
                                                &cd);

    UHDM::func_call* funcCall = test_case.expected;
    funcCall->SetSerializer(&s);  // Needed for vpiName
    funcCall->VpiName(test_case.symbols.getSymbol(1));  // Symbol table starts at 1

    std::string parsed = visit_designs({s.MakeUhdmHandle(uhdmtf_call, returned)});
    std::string expected = visit_designs({s.MakeUhdmHandle(uhdmtf_call, funcCall)});
    ASSERT_EQ(parsed, expected);
    delete funcCall;
  }
}

