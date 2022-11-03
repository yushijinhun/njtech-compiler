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

void generate_object(llvm::Module &module, const std::string &filename) {
	auto target_triple = llvm::sys::getDefaultTargetTriple();
	std::string error;
	auto *target = llvm::TargetRegistry::lookupTarget(target_triple, error);
	if (!target) {
		throw CompileException(-1, error);
	}

	llvm::TargetOptions opt;

	auto *target_machine = target->createTargetMachine(
	    target_triple, "generic", "", opt, llvm::Reloc::Model::PIC_);

	std::error_code ec;
	llvm::raw_fd_ostream dest(filename, ec, llvm::sys::fs::OF_None);
	if (ec) {
		throw CompileException(-1, "Could not open file: " + ec.message());
	}

	llvm::legacy::PassManager pass;
	auto file_type = llvm::CGFT_ObjectFile;

	if (target_machine->addPassesToEmitFile(pass, dest, nullptr, file_type)) {
		throw CompileException(-1,
		                       "Target machine can't emit a file of this type");
	}

	pass.run(module);
	dest.flush();
}

} // namespace aot
} // namespace compiler
