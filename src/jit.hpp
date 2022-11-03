#pragma once

#include <llvm/IR/Module.h>

namespace compiler {
namespace jit {

void initialize();
int invoke_module(std::unique_ptr<llvm::LLVMContext> ctx,
                  std::unique_ptr<llvm::Module> module);

} // namespace jit
} // namespace compiler
