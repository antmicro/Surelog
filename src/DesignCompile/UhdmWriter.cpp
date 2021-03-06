/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   UhdmWriter.cpp
 * Author: alain
 * 
 * Created on January 17, 2020, 9:13 PM
 */
#include <map>
#include "uhdm.h"
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
#include "DesignCompile/UVMElaboration.h"
#include "DesignCompile/CompilePackage.h"
#include "DesignCompile/CompileModule.h"
#include "DesignCompile/CompileFileContent.h"
#include "DesignCompile/CompileProgram.h"
#include "DesignCompile/CompileClass.h"
#include "DesignCompile/Builtin.h"
#include "DesignCompile/PackageAndRootElaboration.h"
#include "surelog.h"
#include "UhdmWriter.h"
#include "vpi_visitor.h"
#include "Serializer.h"
#include "module.h"

using namespace SURELOG;
using namespace UHDM;

typedef std::map<ModPort*, modport*> ModPortMap;
typedef std::map<DesignComponent*, BaseClass*> ComponentMap;
typedef std::map<Signal*, BaseClass*> SignalBaseClassMap;
typedef std::map<std::string, Signal*> SignalMap;

UhdmWriter::~UhdmWriter()
{
}

unsigned int getVpiDirection(VObjectType type)
{
  unsigned int direction = vpiInput;
  if (type == VObjectType::slPortDir_Inp)
    direction = vpiInput;
  else if (type == VObjectType::slPortDir_Out)
    direction = vpiOutput;
  else if (type == VObjectType::slPortDir_Inout)
    direction = vpiInout;
  return direction;
}

unsigned int getVpiNetType(VObjectType type)
{
  unsigned int nettype = 0;
  if (type == VObjectType::slNetType_Wire)
    nettype = vpiWire;
  else if (type == VObjectType::slIntVec_TypeReg)
    nettype = vpiReg;
  else if (type == VObjectType::slNetType_Supply0)
    nettype = vpiSupply0;
  else if (type == VObjectType::slNetType_Supply1)
    nettype = vpiSupply1;
  // TODO
  return nettype;
}

void writePorts(std::vector<Signal*>& orig_ports, BaseClass* parent, 
        VectorOfport* dest_ports, Serializer& s, ComponentMap& componentMap,
        ModPortMap& modPortMap, SignalBaseClassMap& signalBaseMap, 
        SignalMap& signalMap) {
  for (Signal* orig_port : orig_ports ) {
    port* dest_port = s.MakePort();
    signalBaseMap.insert(std::make_pair(orig_port, dest_port));
    signalMap.insert(std::make_pair(orig_port->getName(), orig_port));
    dest_port->VpiName(orig_port->getName());
    unsigned int direction = getVpiDirection(orig_port->getDirection());
    dest_port->VpiDirection(direction);
    dest_port->VpiLineNo(orig_port->getFileContent()->Line(orig_port->getNodeId()));
    dest_port->VpiFile(orig_port->getFileContent()->getFileName());
    dest_port->VpiParent(parent);
    if (ModPort* orig_modport = orig_port->getModPort()) {
      ref_obj* ref = s.MakeRef_obj();
      dest_port->Low_conn(ref);
      std::map<ModPort*, modport*>::iterator itr = modPortMap.find(orig_modport);
      if (itr != modPortMap.end()) {
        ref->Actual_group((*itr).second);
      }
    } else if (ModuleDefinition* orig_interf = orig_port->getInterfaceDef()) {
      ref_obj* ref = s.MakeRef_obj();
      dest_port->Low_conn(ref);
      std::map<DesignComponent*, BaseClass*>::iterator itr = 
                                                componentMap.find(orig_interf);
      if (itr != componentMap.end()) {
        ref->Actual_group((*itr).second);
      }
    }
    dest_ports->push_back(dest_port);
  }
}

   
void writeNets(std::vector<Signal*>& orig_nets, BaseClass* parent, 
        VectorOfnet* dest_nets, Serializer& s, SignalBaseClassMap& signalBaseMap, 
        SignalMap& signalMap) {
  for (auto& orig_net : orig_nets ) {
    logic_net* dest_net = s.MakeLogic_net();
    signalBaseMap.insert(std::make_pair(orig_net, dest_net));
    signalMap.insert(std::make_pair(orig_net->getName(), orig_net));
    dest_net->VpiName(orig_net->getName());
    dest_net->VpiLineNo(orig_net->getFileContent()->Line(orig_net->getNodeId()));
    dest_net->VpiFile(orig_net->getFileContent()->getFileName());
    dest_net->VpiNetType(getVpiNetType(orig_net->getType()));
    dest_net->VpiParent(parent);
    dest_nets->push_back(dest_net);
  }
}

void mapLowConns(std::vector<Signal*>& orig_ports, Serializer& s,
        SignalBaseClassMap& signalBaseMap) {
   for (Signal* orig_port : orig_ports ) {
     if (Signal* lowconn = orig_port->getLowConn()) {
       std::map<Signal*, BaseClass*>::iterator itrlow = signalBaseMap.find(lowconn);
       std::map<Signal*, BaseClass*>::iterator itrport = signalBaseMap.find(orig_port);
       ref_obj* ref = s.MakeRef_obj();
       ((port*)(*itrport).second)->Low_conn(ref);
       ref->Actual_group((*itrlow).second);
     }
   }
}

void writeClasses(ClassNameClassDefinitionMultiMap& orig_classes, 
        VectorOfclass_defn* dest_classes, Serializer& s, 
        ComponentMap& componentMap) {
  for (auto& orig_class : orig_classes ) {
    ClassDefinition* orig_def = orig_class.second;
    std::map<DesignComponent*, BaseClass*>::iterator itr = 
                componentMap.find(orig_def);
    if (itr != componentMap.end()) {
      dest_classes->push_back((class_defn*) (*itr).second);
    }
  }
}

void writeVariables(DesignComponent::VariableMap& orig_vars, BaseClass* parent,
        VectorOfvariables* dest_vars, Serializer& s, ComponentMap& componentMap) {
  for (auto& orig_var : orig_vars) {
    Variable* var = orig_var.second;
    DataType* dtype = var->getDataType();
    ClassDefinition* classdef = dynamic_cast<ClassDefinition*> (dtype);
    if (classdef) {
      class_var* cvar = s.MakeClass_var();
      cvar->VpiName(var->getName());
      cvar->VpiLineNo(var->getFileContent()->Line(var->getNodeId()));
      cvar->VpiFile(var->getFileContent()->getFileName());
      cvar->VpiParent(parent);
      std::map<DesignComponent*, BaseClass*>::iterator itr =
              componentMap.find(classdef);
      if (itr != componentMap.end()) {
        //TODO: Bind Class type,
        // class_var -> class_typespec -> class_defn
      }
      dest_vars->push_back(cvar);
    }
  }
}

void bindExpr(expr* ex, ComponentMap& componentMap,
        ModPortMap& modPortMap, SignalBaseClassMap& signalBaseMap, 
        SignalMap& signalMap)
{
  switch (ex->UhdmType()) {
  case UHDM_OBJECT_TYPE::uhdmref_obj:
  {
    ref_obj* ref = (ref_obj*) ex;
    const std::string& name = ref->VpiName();
    SignalMap::iterator sigitr = signalMap.find(name);
    if (sigitr != signalMap.end()) {
      Signal* sig = (*sigitr).second;
      SignalBaseClassMap::iterator sigbaseitr = signalBaseMap.find(sig);
      if (sigbaseitr != signalBaseMap.end()) {
        BaseClass* baseclass = (*sigbaseitr).second;
        ref->Actual_group(baseclass);
        
      }
    }
    break;
  }
  default:
    break;
  }
}

void writeContAssigns(std::vector<cont_assign*>* orig_cont_assigns,
        module* parent, Serializer& s, ComponentMap& componentMap,
        ModPortMap& modPortMap, SignalBaseClassMap& signalBaseMap, 
        SignalMap& signalMap) {
  if (orig_cont_assigns == nullptr)
    return;
  for (cont_assign* cassign : *orig_cont_assigns) {
    expr* lexpr = (expr*) cassign->Lhs();
    bindExpr(lexpr, componentMap, modPortMap, signalBaseMap, signalMap);
    expr* rexpr = (expr*) cassign->Rhs();
    bindExpr(rexpr, componentMap, modPortMap, signalBaseMap, signalMap);
  }
}

void writeModule(ModuleDefinition* mod, module* m, Serializer& s, 
        ComponentMap& componentMap,
        ModPortMap& modPortMap) {
  SignalBaseClassMap signalBaseMap;
  SignalMap portMap;
  SignalMap netMap;
  // Ports
  std::vector<Signal*>& orig_ports = mod->getPorts();
  VectorOfport* dest_ports = s.MakePortVec();
  writePorts(orig_ports, m, dest_ports, s, componentMap,
        modPortMap, signalBaseMap, portMap);
  m->Ports(dest_ports);
  // Nets
  std::vector<Signal*>& orig_nets = mod->getSignals();
  VectorOfnet* dest_nets = s.MakeNetVec();
  writeNets(orig_nets, m, dest_nets, s, signalBaseMap, netMap);
  m->Nets(dest_nets);
  mapLowConns(orig_ports, s, signalBaseMap);
  // Classes
  ClassNameClassDefinitionMultiMap& orig_classes = mod->getClassDefinitions();
  VectorOfclass_defn* dest_classes = s.MakeClass_defnVec();
  writeClasses(orig_classes, dest_classes, s, componentMap);
  m->Class_defns(dest_classes);
  // Variables
  DesignComponent::VariableMap& orig_vars = mod->getVariables();
  VectorOfvariables* dest_vars = s.MakeVariablesVec();
  writeVariables(orig_vars, m, dest_vars, s, componentMap);
  m->Variables(dest_vars);
  // Cont assigns
  std::vector<cont_assign*>* orig_cont_assigns = mod->getContAssigns();
  writeContAssigns(orig_cont_assigns, m, s, componentMap, modPortMap, 
          signalBaseMap, netMap);
  m->Cont_assigns(orig_cont_assigns);
}

void writeInterface(ModuleDefinition* mod, interface* m, Serializer& s,
        ComponentMap& componentMap,
        ModPortMap& modPortMap) {
  SignalBaseClassMap signalBaseMap;
  SignalMap signalMap;
  // Ports
  std::vector<Signal*>& orig_ports = mod->getPorts();
  VectorOfport* dest_ports = s.MakePortVec();
  writePorts(orig_ports, m, dest_ports, s, componentMap,
        modPortMap, signalBaseMap, signalMap);
  m->Ports(dest_ports);
  // Modports
  ModuleDefinition::ModPortSignalMap& orig_modports = mod->getModPortSignalMap();
  VectorOfmodport* dest_modports = s.MakeModportVec();
  for (auto& orig_modport : orig_modports ) {
    modport* dest_modport = s.MakeModport();
    modPortMap.insert(std::make_pair(&orig_modport.second, dest_modport));
    dest_modport->VpiName(orig_modport.first);
    VectorOfio_decl* ios = s.MakeIo_declVec();
    for (auto& sig : orig_modport.second.getPorts()) {
      io_decl* io = s.MakeIo_decl();
      io->VpiName(sig.getName());
      unsigned int direction = getVpiDirection(sig.getDirection());
      io->VpiDirection(direction);
      ios->push_back(io);
    }
    dest_modport->Io_decls(ios);
    dest_modports->push_back(dest_modport);
  }
  m->Modports(dest_modports);
}

void writeProgram(Program* mod, program* m, Serializer& s,
        ComponentMap& componentMap,
        ModPortMap& modPortMap) {
  SignalBaseClassMap signalBaseMap;
  SignalMap portMap;
  SignalMap netMap;
  // Ports
  std::vector<Signal*>& orig_ports = mod->getPorts();
  VectorOfport* dest_ports = s.MakePortVec();
  writePorts(orig_ports, m, dest_ports, s, componentMap,
        modPortMap, signalBaseMap, portMap);
  m->Ports(dest_ports);
   // Nets
  std::vector<Signal*>& orig_nets = mod->getSignals();
  VectorOfnet* dest_nets = s.MakeNetVec();
  writeNets(orig_nets, m, dest_nets, s, signalBaseMap, netMap);
  m->Nets(dest_nets);
  mapLowConns(orig_ports, s, signalBaseMap);
  // Classes
  ClassNameClassDefinitionMultiMap& orig_classes = mod->getClassDefinitions();
  VectorOfclass_defn* dest_classes = s.MakeClass_defnVec();
  writeClasses(orig_classes, dest_classes, s, componentMap);
  m->Class_defns(dest_classes);
  // Variables
  DesignComponent::VariableMap& orig_vars = mod->getVariables();
  VectorOfvariables* dest_vars = s.MakeVariablesVec();
  writeVariables(orig_vars, m, dest_vars, s, componentMap);
  m->Variables(dest_vars);
}

bool UhdmWriter::write(std::string uhdmFile) {
  ComponentMap componentMap;
  ModPortMap modPortMap;
  Serializer& s = m_compileDesign->getSerializer();
  if (m_design) {
    design* d = s.MakeDesign();
    std::string designName = "unnamed";
    auto topLevelModules = m_design->getTopLevelModuleInstances();
    for (auto inst : topLevelModules) {
      designName = inst->getModuleName();
      break;
    }
    d->VpiName(designName);
     
    // Packages
    auto packages = m_design->getPackageDefinitions();
    VectorOfpackage* v2 = s.MakePackageVec();
    for (auto packNamePair : packages) {
      Package* pack = packNamePair.second;
      if (pack->getFileContents().size() && 
          pack->getType() == VObjectType::slPackage_declaration) {
        FileContent* fC = pack->getFileContents()[0];
        package* p = s.MakePackage();
        componentMap.insert(std::make_pair(pack, p));
        p->VpiParent(d);
        p->VpiName(pack->getName());
        if (fC) {
          // Builtin package has no file 
          p->VpiFile(fC->getFileName());
          p->VpiLineNo(fC->Line(pack->getNodeIds()[0]));
        }
        v2->push_back(p);      
      }
    }
    d->AllPackages(v2);
    
    // Programs
    auto programs = m_design->getProgramDefinitions();
    VectorOfprogram* uhdm_programs = s.MakeProgramVec();
    for (auto progNamePair : programs) {
      Program* prog = progNamePair.second;
      if (prog->getFileContents().size() && 
          prog->getType() == VObjectType::slProgram_declaration) {
        FileContent* fC = prog->getFileContents()[0];
        program* p = s.MakeProgram();
        componentMap.insert(std::make_pair(prog, p)); 
        p->VpiParent(d);
        p->VpiName(prog->getName());    
        p->VpiFile(fC->getFileName());
        p->VpiLineNo(fC->Line(prog->getNodeIds()[0]));
        writeProgram(prog, p, s, componentMap,modPortMap);
        uhdm_programs->push_back(p);      
      }
    }
    d->AllPrograms(uhdm_programs);
    
    // Classes
    auto classes = m_design->getClassDefinitions();
    VectorOfclass_defn* v4 = s.MakeClass_defnVec();
    for (auto classNamePair : classes) {
      ClassDefinition* classDef = classNamePair.second;
      if (classDef->getFileContents().size() && 
          classDef->getType() == VObjectType::slClass_declaration) {
        FileContent* fC = classDef->getFileContents()[0];
        class_defn* c = s.MakeClass_defn();
        componentMap.insert(std::make_pair(classDef, c));  
        c->VpiParent(d);
        c->VpiName(classDef->getName());
        if (fC) {
          // Builtin classes have no file
          c->VpiFile(fC->getFileName());
          c->VpiLineNo(fC->Line(classDef->getNodeIds()[0]));
        }
        v4->push_back(c);      
      }
    }
    d->AllClasses(v4);

    // Interfaces
    auto modules = m_design->getModuleDefinitions();
    VectorOfinterface* uhdm_interfaces = s.MakeInterfaceVec();
    for (auto modNamePair : modules) {
      ModuleDefinition* mod = modNamePair.second;
      if (mod->getFileContents().size() == 0) {
        // Built-in primitive
      } else if (mod->getType() == VObjectType::slInterface_declaration) {
        FileContent* fC = mod->getFileContents()[0];
        interface* m = s.MakeInterface();
        componentMap.insert(std::make_pair(mod, m));
        m->VpiParent(d);
        m->VpiName(mod->getName());    
        m->VpiFile(fC->getFileName());
        m->VpiLineNo(fC->Line(mod->getNodeIds()[0]));
        uhdm_interfaces->push_back(m); 
        writeInterface(mod, m, s, componentMap, modPortMap);
      }
    }
    d->AllInterfaces(uhdm_interfaces);
    
    // Modules
    VectorOfmodule* uhdm_modules = s.MakeModuleVec();
    for (auto modNamePair : modules) {
      ModuleDefinition* mod = modNamePair.second;
      if (mod->getFileContents().size() == 0) {
        // Built-in primitive
      } else if (mod->getType() == VObjectType::slModule_declaration) {
        FileContent* fC = mod->getFileContents()[0];
        module* m = s.MakeModule();
        componentMap.insert(std::make_pair(mod, m));
        m->VpiParent(d);
        m->VpiName(mod->getName());    
        m->VpiFile(fC->getFileName());
        m->VpiLineNo(fC->Line(mod->getNodeIds()[0]));
        uhdm_modules->push_back(m); 
        writeModule(mod, m, s, componentMap, modPortMap);
      }
    }
    d->AllModules(uhdm_modules);
    
    // Repair parent relationship
    for (auto classNamePair : classes) {
      ClassDefinition* classDef = classNamePair.second;
      DesignComponent* parent = classDef->getContainer();
      std::map<DesignComponent*, BaseClass*>::iterator itr =
              componentMap.find(parent);
      if (itr != componentMap.end()) {
        std::map<DesignComponent*, BaseClass*>::iterator itr2 =
                componentMap.find(classDef);
        (*itr2).second->VpiParent((*itr).second);
      }
    }     
  }
  s.Save(uhdmFile);
  
  if (m_compileDesign->getCompiler()->getCommandLineParser()->getDebugUhdm()) {
    std::cout << "====== UHDM =======\n";
    const std::vector<vpiHandle>& restoredDesigns = s.Restore(uhdmFile);
    std::string restored = visit_designs(restoredDesigns);
    std::cout << restored;
    std::cout << "===================\n";

  }
  
  return true;
}
 
