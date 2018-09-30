#include <benchmark/benchmark.h>

#include <llvm/Bitcode/BitcodeReader.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/Orc/LLJIT.h>
#include <llvm/IR/ModuleSummaryIndex.h>
#include <llvm/Support/TargetSelect.h>

#include "common.h"

using namespace llvm;
using namespace llvm::orc;

std::vector<InputFile> InputFiles;
llvm::ExitOnError ExitOnFail("Abort benchmark due to error\n");

void PrepareFrontloadBench() {
  std::string DirName = BITCODE_DIR "/";
  DIR *Dir = opendir(DirName.c_str());
  if (!Dir) {
    llvm::errs() << "Cannot find benchmark inputs in: " << DirName << "\n";
    exit(1);
  }

  struct dirent *Entry;
  while ((Entry = readdir(Dir))) {
    std::string FullFileName = DirName + Entry->d_name;
    if (!llvm::sys::fs::is_regular_file(FullFileName))
      continue;

    InputFile Input(FullFileName);
    ExitOnFail(Input.Load());

    InputFiles.push_back(std::move(Input));
  }

  llvm::outs() << "Collected " << InputFiles.size() << " input files\n\n";
}

void BM_FrontloadEager(benchmark::State &state) {
  TargetMachine *TM = EngineBuilder().selectTarget();
  JITTargetMachineBuilder JTMB(TM->getTargetTriple());
  DataLayout DL = TM->createDataLayout();

  std::unique_ptr<LLJIT> Jit = ExitOnFail(LLJIT::Create(std::move(JTMB), DL));

  ExecutionSession &ES = Jit->getExecutionSession();
  MangleAndInterner MandI(ES, DL);

  while (state.KeepRunning()) {
    for (const InputFile &I : InputFiles) {
      JITDylib &JD = ES.createJITDylib(random_string(80));
      MemoryBufferRef BufferRef = I.Buffer->getMemBufferRef();

      // Parse and add full module to build symbol table.
      auto C = std::make_unique<LLVMContext>();
      auto M = ExitOnFail(parseBitcodeFile(BufferRef, *C));
      ThreadSafeModule TSM(std::move(M), std::move(C));
      ExitOnFail(Jit->addIRModule(JD, std::move(TSM)));

      SymbolStringPtr Mangled = MandI(I.FirstFunctionName);
      SymbolFlagsMap FlagsMap = JD.lookupFlags({Mangled});

      auto FlagsIt = FlagsMap.find(Mangled);
      assert(FlagsIt != FlagsMap.end() && "Invalid benchmark");

      JITSymbolFlags F = FlagsIt->second;
      benchmark::DoNotOptimize(F);
    }
  }
}

void BM_FrontloadLazy(benchmark::State &state) {
  LLVMContext C;
  TargetMachine *TM = EngineBuilder().selectTarget();
  JITTargetMachineBuilder JTMB(TM->getTargetTriple());
  DataLayout DL = TM->createDataLayout();

  std::unique_ptr<LLLazyJIT> Jit =
      ExitOnFail(LLLazyJIT::Create(std::move(JTMB), DL));
  ExecutionSession &ES = Jit->getExecutionSession();
  MangleAndInterner MandI(ES, DL);

  while (state.KeepRunning()) {
    for (const InputFile &I : InputFiles) {
      JITDylib &JD = ES.createJITDylib(random_string(80));
      MemoryBufferRef BufferRef = I.Buffer->getMemBufferRef();

      // Parse and add "lazy" module to build symbol table.
      auto C = std::make_unique<LLVMContext>();
      auto M = ExitOnFail(getLazyBitcodeModule(BufferRef, *C));
      ThreadSafeModule TSM(std::move(M), std::move(C));
      ExitOnFail(Jit->addLazyIRModule(JD, std::move(TSM)));

      SymbolStringPtr Mangled = MandI(I.FirstFunctionName);
      SymbolFlagsMap FlagsMap = JD.lookupFlags({Mangled});

      auto FlagsIt = FlagsMap.find(Mangled);
      assert(FlagsIt != FlagsMap.end() && "Invalid benchmark");

      JITSymbolFlags F = FlagsIt->second;
      benchmark::DoNotOptimize(F);
    }
  }
}

static JITSymbolFlags TransformFlags(const GlobalValueSummary &S,
                                     GlobalValue::GUID Guid) {
  JITSymbolFlags Flags = JITSymbolFlags::None;

  auto Linkage = S.flags().Linkage;
  if (Linkage == GlobalValue::WeakAnyLinkage ||
      Linkage == GlobalValue::WeakODRLinkage)
    Flags |= JITSymbolFlags::Weak;

  if (Linkage == GlobalValue::CommonLinkage)
    Flags |= JITSymbolFlags::Common;

  // TODO Is this correct? Consider hidden if we had to import with new name.
  bool hasHiddenVisibility = (Guid != S.getOriginalName());
  if (Linkage != GlobalValue::InternalLinkage &&
      Linkage != GlobalValue::PrivateLinkage && !hasHiddenVisibility)
    Flags |= JITSymbolFlags::Exported;

  if (S.getSummaryKind() == GlobalValueSummary::FunctionKind)
    Flags |= JITSymbolFlags::Callable;

  return Flags;
}

void BM_FrontloadSummaryBased(benchmark::State &state) {
  LLVMContext C;
  bool HaveGVs = false;

  while (state.KeepRunning()) {
    uint64_t ModuleId = 0;
    ModuleSummaryIndex CombinedIndex(HaveGVs);

    for (const InputFile &I : InputFiles) {
      MemoryBufferRef BufferRef = I.Buffer->getMemBufferRef();
      ExitOnFail(readModuleSummaryIndex(BufferRef, CombinedIndex, ++ModuleId));

      auto Guid = GlobalValue::getGUID(I.FirstFunctionName);
      GlobalValueSummary *S =
          CombinedIndex.findSummaryInModule(Guid, I.FullFileName);

      assert(S && "Invalid benchmark");

      JITSymbolFlags F = TransformFlags(*S, Guid);
      benchmark::DoNotOptimize(F);
    }
  }
}

BENCHMARK(BM_FrontloadEager);
BENCHMARK(BM_FrontloadLazy);
BENCHMARK(BM_FrontloadSummaryBased);
