#include "aot.hpp"
#include "error.hpp"
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Host.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>

namespace compiler {
namespace aot {

void initialize() {
	llvm::InitializeNativeTarget();
	llvm::InitializeNativeTargetAsmPrinter();
	llvm::InitializeNativeTargetAsmParser();
}

void compile(llvm::Module &module, llvm::CodeGenFileType type,
             llvm::raw_pwrite_stream &out) {
	auto target_triple = llvm::sys::getDefaultTargetTriple();
	std::string error;
	auto *target = llvm::TargetRegistry::lookupTarget(target_triple, error);
	if (!target) {
		throw CompileException(-1, error);
	}

	llvm::TargetOptions opt;

	std::unique_ptr<llvm::LLVMTargetMachine> target_machine(
	    dynamic_cast<llvm::LLVMTargetMachine *>(target->createTargetMachine(
	        target_triple, "generic", "", opt, llvm::Reloc::Model::PIC_)));

	llvm::legacy::PassManager pass;

	if (target_machine->addPassesToEmitFile(pass, out, nullptr, type))
		throw CompileException(-1,
		                       "Target machine can't emit code of given type");

	pass.run(module);
}

void compile_object_file(llvm::Module &module, const std::string &output_file) {
	std::error_code ec;
	llvm::raw_fd_ostream dest(output_file, ec);
	if (ec)
		throw CompileException(-1, "Could not open file: " + ec.message());
	compile(module, llvm::CodeGenFileType::CGFT_ObjectFile, dest);
}

void compile_asm_file(llvm::Module &module, const std::string &output_file) {
	std::error_code ec;
	llvm::raw_fd_ostream dest(output_file, ec);
	if (ec)
		throw CompileException(-1, "Could not open file: " + ec.message());
	compile(module, llvm::CodeGenFileType::CGFT_AssemblyFile, dest);
}

} // namespace aot
} // namespace compiler
