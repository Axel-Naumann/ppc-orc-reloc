#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/OrcMCJITReplacement.h"
#include "llvm/ExecutionEngine/SectionMemoryManager.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/TargetSelect.h"

using namespace llvm;

std::unique_ptr<ExecutionEngine> createJIT(std::unique_ptr<Module> M) {
  EngineBuilder Builder(std::move(M));
  Builder.setUseOrcMCJITReplacement(true);
  std::unique_ptr<RTDyldMemoryManager> MemMgr{new SectionMemoryManager};
  Builder.setMCJITMemoryManager(std::move(MemMgr));
  return std::unique_ptr<ExecutionEngine>(Builder.create());
}

int main(int argc, const char* argv[]) {
  InitializeNativeTarget();
  InitializeNativeTargetAsmPrinter();
  InitializeNativeTargetAsmParser();

  std::unique_ptr<ExecutionEngine> Exe;
  SMDiagnostic Diag;
  Function* FunMain = 0;
  for (int i = 1; i < argc; ++i) {
    std::unique_ptr<Module> M{parseIRFile(argv[i], Diag, getGlobalContext())};
    if (!M) {
      Diag.print(argv[0], llvm::errs());
      exit(1);
    }
    if (!FunMain)
      FunMain = M->getFunction("main");
    if (!Exe)
      Exe = std::move(createJIT(std::move(M)));
    else
      Exe->addModule(std::move(M));
  }
  if (!Exe)
    return 0;

  Exe->runStaticConstructorsDestructors(/*isDtors*/ false);
  if (!FunMain) {
    llvm::errs() << "No main()\n";
    exit(1);
  }
  Exe->runFunctionAsMain(FunMain, std::vector<std::string>(), 0);
  return 0;
}
