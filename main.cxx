#include "llvm/ExecutionEngine/SectionMemoryManager.h"
#include "unittests/ExecutionEngine/MCJIT/MCJITTestBase.h"
#include "llvm/ExecutionEngine/Orc/IRCompileLayer.h"
#include "llvm/ExecutionEngine/Orc/ObjectLinkingLayer.h"
#include "llvm/ExecutionEngine/Orc/LazyEmittingLayer.h"

#include <set>
#include <vector>

using namespace llvm;

class IncrementalJIT;

class NotifyObjectLoadedT {
public:
  typedef std::vector<std::unique_ptr<llvm::object::ObjectFile>> ObjListT;
  typedef std::vector<std::unique_ptr<llvm::RuntimeDyld::LoadedObjectInfo>>
  LoadedObjInfoListT;

  NotifyObjectLoadedT(IncrementalJIT &jit) : m_JIT(jit) {}

  void operator()(ObjectLinkingLayerBase::ObjSetHandleT H,
                  const ObjListT &Objects,
                  const LoadedObjInfoListT &Infos) const;
private:
  IncrementalJIT &m_JIT;
};

class NotifyFinalizedT {
public:
  NotifyFinalizedT(IncrementalJIT &jit) : m_JIT(jit) {}
  void operator()(ObjectLinkingLayerBase::ObjSetHandleT H);
private:
  IncrementalJIT &m_JIT;
};


class IncrementalJIT: public MCJITTestBase {
public:
  typedef orc::ObjectLinkingLayer<NotifyObjectLoadedT> ObjectLayerT;
  typedef orc::IRCompileLayer<ObjectLayerT> CompileLayerT;
  typedef orc::LazyEmittingLayer<CompileLayerT> LazyEmitLayerT;
  typedef LazyEmitLayerT::ModuleSetHandleT ModuleSetHandleT;


  ObjectLayerT m_ObjectLayer{ObjectLayerT::CreateRTDyldMMFtor(),
    m_NotifyObjectLoaded,
    m_NotifyFinalized};
  CompileLayerT m_CompileLayer{m_ObjectLayer, SimpleCompiler{TM}};
  LazyEmitLayerT m_LazyEmitLayer{m_CompileLayer};


  std::unique_ptr<RTDyldMemoryManager> m_MemMgr;

  NotifyObjectLoadedT m_NotifyObjectLoaded;
  NotifyFinalizedT m_NotifyFinalized;

  // We need to store ObjLayerT::ObjSetHandles for each of the object sets
  // that have been emitted but not yet finalized so that we can forward the
  // mapSectionAddress calls appropriately.
  typedef std::set<const void *> SectionAddrSet;
  struct ObjSetHandleCompare {
    bool operator()(ObjectLayerT::ObjSetHandleT H1,
                    ObjectLayerT::ObjSetHandleT H2) const {
      return &*H1 < &*H2;
    }
  };

  SectionAddrSet m_SectionsAllocatedSinceLastLoad;
  std::map<ObjectLayerT::ObjSetHandleT, SectionAddrSet, ObjSetHandleCompare>
  m_UnfinalizedSections;

  size_t addModules(std::vector<llvm::Module*>&& modules) {
    // If this module doesn't have a DataLayout attached then attach the
    // default.
    for (auto&& mod: modules) {
      if (!mod->getDataLayout())
        mod->setDataLayout(m_TM->getDataLayout());
    }

    ModuleSetHandleT MSHandle
    = m_LazyEmitLayer.addModuleSet(std::move(modules),
                                   llvm::make_unique<Azog>(*this));
    m_UnloadPoints.push_back(MSHandle);
    return m_UnloadPoints.size() - 1;
  };
};


void NotifyObjectLoadedT::operator()(orc::ObjectLinkingLayerBase::ObjSetHandleT H,
                const ObjListT &Objects,
                const LoadedObjInfoListT &Infos) const {
  m_JIT.m_UnfinalizedSections[H]
  = std::move(m_JIT.m_SectionsAllocatedSinceLastLoad);
  m_JIT.m_SectionsAllocatedSinceLastLoad = SectionAddrSet();
  assert(Objects.size() == Infos.size() &&
         "Incorrect number of Infos for Objects.");
  for (size_t I = 0, N = Objects.size(); I < N; ++I)
    m_JIT.m_GDBListener->NotifyObjectEmitted(*Objects[I], *Infos[I]);
};

private:
IncrementalJIT &m_JIT;
};

void NotifyFinalizedT::operator()(orc::ObjectLinkingLayerBase::ObjSetHandleT H) {
  m_JIT.m_UnfinalizedSections.erase(H);
}

private:
  IncrementalJIT &m_JIT;
};


int main() {
  OrcJITRelocTester Tester;
  Tester.m_LazyEmitLayer.getSymbolAddress(Name, false))
  return 0;
}