/*
 Copyright 2019 Alain Dargelas

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
 */

/*
 * File:   CompileProgram.cpp
 * Author: alain
 *
 * Created on June 6, 2018, 10:43 PM
 */
#include "SourceCompile/VObjectTypes.h"
#include "Design/VObject.h"
#include "Library/Library.h"
#include "Design/Signal.h"
#include "Design/FileContent.h"
#include "Design/ClockingBlock.h"
#include "Testbench/ClassDefinition.h"
#include "SourceCompile/SymbolTable.h"
#include "ErrorReporting/Error.h"
#include "ErrorReporting/Location.h"
#include "ErrorReporting/Error.h"
#include "CommandLine/CommandLineParser.h"
#include "ErrorReporting/ErrorDefinition.h"
#include "ErrorReporting/ErrorContainer.h"
#include "SourceCompile/CompilationUnit.h"
#include "SourceCompile/PreprocessFile.h"
#include "SourceCompile/CompileSourceFile.h"
#include "SourceCompile/ParseFile.h"
#include "SourceCompile/Compiler.h"
#include "DesignCompile/CompileHelper.h"
#include "DesignCompile/CompileDesign.h"
#include "DesignCompile/CompileProgram.h"

using namespace SURELOG;

CompileProgram::~CompileProgram() {}

int FunctorCompileProgram::operator()() const {
  CompileProgram* instance = new CompileProgram(m_compileDesign, m_program,
                                                m_design, m_symbols, m_errors);
  instance->compile();
  delete instance;
  return true;
}

bool CompileProgram::compile() {
  FileContent* fC = m_program->m_fileContents[0];
  NodeId nodeId = m_program->m_nodeIds[0];

  Location loc(m_symbols->registerSymbol(fC->getFileName(nodeId)),
               fC->Line(nodeId), 0,
               m_symbols->registerSymbol(m_program->getName()));

  Error err1(ErrorDefinition::COMP_COMPILE_PROGRAM, loc);
  ErrorContainer* errors = new ErrorContainer(m_symbols);
  errors->regiterCmdLine(
      m_compileDesign->getCompiler()->getCommandLineParser());
  errors->addError(err1);
  errors->printMessage(
      err1,
      m_compileDesign->getCompiler()->getCommandLineParser()->muteStdout());
  delete errors;

  Error err2(ErrorDefinition::COMP_PROGRAM_OBSOLETE_USAGE, loc);
  m_errors->addError(err2);

  std::vector<VObjectType> stopPoints = {
      VObjectType::slClass_declaration,
  };

  if (fC->getSize() == 0) return true;
  VObject current = fC->Object(nodeId);
  NodeId id = current.m_child;
  if (!id) id = current.m_sibling;
  if (!id) return false;

  // Package imports
  std::vector<FileCNodeId> pack_imports;
  // - Local file imports
  for (auto import : fC->getObjects(VObjectType::slPackage_import_item)) {
    pack_imports.push_back(import);
  }

  for (auto pack_import : pack_imports) {
    FileContent* pack_fC = pack_import.fC;
    NodeId pack_id = pack_import.nodeId;
    m_helper.importPackage(m_program, m_design, pack_fC, pack_id);
  }

  std::stack<NodeId> stack;
  stack.push(id);
  VObjectType port_direction = VObjectType::slNoType;
  while (stack.size()) {
    id = stack.top();
    stack.pop();
    current = fC->Object(id);
    VObjectType type = fC->Type(id);
    switch (type) {
    case VObjectType::slPackage_import_item:
    {
      m_helper.importPackage(m_program, m_design, fC, id);
      break;
    }
    case VObjectType::slAnsi_port_declaration:
    {
      m_helper.compileAnsiPortDeclaration(m_program, fC, id, port_direction);
      break;
    }
    case VObjectType::slPort:
    {
      m_helper.compilePortDeclaration(m_program, fC, id, port_direction);
      break;
    }
    case VObjectType::slInput_declaration:
    case VObjectType::slOutput_declaration:
    case VObjectType::slInout_declaration:
    {
      m_helper.compilePortDeclaration(m_program, fC, id, port_direction);
      break;
    }
    case VObjectType::slPort_declaration:
    {
      m_helper.compilePortDeclaration(m_program, fC, id, port_direction);
      break;
    }
    case VObjectType::slContinuous_assign:
    {
      m_helper.compileContinuousAssignment(m_program, fC, id, m_compileDesign);
      break;
    }
    case VObjectType::slClass_declaration:
    {
      NodeId nameId = fC->Child(id);
      std::string name = fC->SymName(nameId);
      FileCNodeId fnid(fC, nameId);
      m_program->addObject(type, fnid);

      std::string completeName = m_program->getName() + "::" + name;

      DesignComponent* comp = fC->getComponentDefinition(completeName);

      m_program->addNamedObject(name, fnid, comp);
      break;
    }
    case VObjectType::slNet_declaration:
    {
      m_helper.compileNetDeclaration(m_program, fC, id, false);
      break;
    }
    case VObjectType::slData_declaration:
    {
      m_helper.compileDataDeclaration(m_program, m_program, fC, id, false);
      break;
    }
    default:
      break;
    }

    if (current.m_sibling) stack.push(current.m_sibling);
    if (current.m_child) {
      if (stopPoints.size()) {
        bool stop = false;
        for (auto t : stopPoints) {
          if (t == current.m_type) {
            stop = true;
            break;
          }
        }
        if (!stop)
          if (current.m_child) stack.push(current.m_child);
      } else {
        if (current.m_child) stack.push(current.m_child);
      }
    }
  }
  return true;
}
