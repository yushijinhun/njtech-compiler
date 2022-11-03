#pragma once

#include <llvm/IR/Module.h>

namespace compiler {
namespace aot {

void initialize();

void generate_object(llvm::Module &module, const std::string &filename);

} // namespace aot
} // namespace compiler
