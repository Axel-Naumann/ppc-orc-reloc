#include "/Users/axel/build/llvm/src/lib/ExecutionEngine/Orc/OrcMCJITReplacement.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/SectionMemoryManager.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/IR/LLVMContext.h"

using namespace llvm;

int main(int argc, const char* argv[]) {
  std::shared_ptr<MCJITMemoryManager> MemMgr{new SectionMemoryManager};
  std::shared_ptr<RuntimeDyld::SymbolResolver> ClientResolver;
  std::unique_ptr<TargetMachine> TM;
  TM.reset(EngineBuilder().selectTarget());
  orc::OrcMCJITReplacement Exe{MemMgr, ClientResolver, std::move(TM)};

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
    Exe.addModule(std::move(M));
  }
  Exe.runStaticConstructorsDestructors(/*isDtors*/ false);
  if (!FunMain) {
    llvm::errs() << "No main()\n";
    exit(1);
  }
  Exe.runFunctionAsMain(FunMain, std::vector<std::string>(), 0);
  return 0;
}