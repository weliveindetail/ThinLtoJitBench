#include <benchmark/benchmark.h>

#include <llvm/Support/Error.h>
#include <llvm/Support/TargetSelect.h>

void PrepareFrontloadBench();

int main(int argc, char **argv) {
  llvm::InitializeNativeTarget();
  llvm::InitializeNativeTargetAsmPrinter();
  llvm::InitializeNativeTargetAsmParser();

  PrepareFrontloadBench();

  benchmark::Initialize(&argc, argv);
  benchmark::RunSpecifiedBenchmarks();
}
