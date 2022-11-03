#pragma once

#include <llvm/IR/Module.h>
#include <llvm/Target/TargetMachine.h>

namespace compiler {
namespace aot {

void initialize();

std::unique_ptr<llvm::TargetMachine> create_target_machine();

void optimize(llvm::Module &module);

void compile(llvm::Module &module, llvm::CodeGenFileType type,
             llvm::raw_pwrite_stream &out);

void compile_object_file(llvm::Module &module, const std::string &output_file);

void compile_asm_file(llvm::Module &module, const std::string &output_file);

} // namespace aot
} // namespace compiler
