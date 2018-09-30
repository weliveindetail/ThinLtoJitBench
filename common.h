#include <llvm/Bitcode/BitcodeReader.h>
#include <llvm/IR/ModuleSummaryIndex.h>
#include <llvm/Support/Error.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Path.h>

#include "dirent.h"

using namespace llvm;

inline std::string random_string(size_t length) {
  auto randchar = []() -> char {
    const char charset[] = "0123456789"
                           "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                           "abcdefghijklmnopqrstuvwxyz";
    const size_t max_index = (sizeof(charset) - 1);
    return charset[rand() % max_index];
  };
  std::string str(length, 0);
  std::generate_n(str.begin(), length, randchar);
  return str;
}

struct InputFile {
  std::string FullFileName;
  std::string FirstFunctionName;
  std::unique_ptr<MemoryBuffer> Buffer;

  InputFile(std::string FullFileName) : FullFileName(std::move(FullFileName)) {}

  Error Load() {
    auto BufferOrErr = MemoryBuffer::getFile(FullFileName);
    if (!BufferOrErr)
      return error("Failed to read file: " + FullFileName);

    Buffer = std::move(*BufferOrErr);
    LLVMContext C;

    auto ModOrErr = parseBitcodeFile(Buffer->getMemBufferRef(), C);
    if (!ModOrErr)
      return ModOrErr.takeError();

    auto ModSumOrErr = getModuleSummaryIndexForFile(FullFileName, false);
    if (!ModSumOrErr)
      return ModSumOrErr.takeError();

    std::unique_ptr<Module> M = std::move(*ModOrErr);
    std::unique_ptr<ModuleSummaryIndex> I = std::move(*ModSumOrErr);

    for (const Function &F : M->functions()) {
      StringRef Name = F.getName();
      if (Name.empty() || !F.hasExternalLinkage())
        continue;

      auto Guid = GlobalValue::getGUID(Name);
      GlobalValueSummary *S = I->findSummaryInModule(Guid, FullFileName);
      if (!S || S->getSummaryKind() != GlobalValueSummary::FunctionKind)
        continue;

      FirstFunctionName = Name;
      return Error::success();
    }

    return error("No named function definition with external linkage");
  }

private:
  Error error(std::string Message) {
    return llvm::make_error<StringError>(std::move(Message),
                                         inconvertibleErrorCode());
  }
};
