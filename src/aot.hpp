#pragma once

#include <llvm/IR/Module.h>

namespace compiler {
namespace aot {

void initialize();

void compile(llvm::Module &module, llvm::CodeGenFileType type,
             llvm::raw_pwrite_stream &out);

void compile_object_file(llvm::Module &module, const std::string &output_file);

void compile_asm_file(llvm::Module &module, const std::string &output_file);

} // namespace aot
} // namespace compiler
