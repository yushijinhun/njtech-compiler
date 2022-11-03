#include "aot.hpp"
#include "error.hpp"
#include <llvm/Analysis/TargetLibraryInfo.h>
#include <llvm/Analysis/TargetTransformInfo.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Verifier.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Host.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/Transforms/IPO.h>
#include <llvm/Transforms/IPO/PassManagerBuilder.h>

namespace compiler {
namespace aot {

void initialize() {
	llvm::InitializeNativeTarget();
	llvm::InitializeNativeTargetAsmPrinter();
	llvm::InitializeNativeTargetAsmParser();
}

std::unique_ptr<llvm::TargetMachine> create_target_machine() {
	auto target_triple = llvm::sys::getDefaultTargetTriple();
	std::string error;
	auto *target = llvm::TargetRegistry::lookupTarget(target_triple, error);
	if (!target)
		throw CompileException(-1, error);
	std::unique_ptr<llvm::TargetMachine> target_machine(
	    target->createTargetMachine(target_triple, "generic", "",
	                                llvm::TargetOptions(),
	                                llvm::Reloc::Model::PIC_, llvm::None,
	                                llvm::CodeGenOpt::Aggressive));
	if (target_machine == nullptr)
		throw CompileException(-1, "Can't create target machine");
	return target_machine;
}

void optimize(llvm::Module &module) {
	auto target_machine = create_target_machine();
	module.setTargetTriple(target_machine->getTargetTriple().str());
	module.setDataLayout(target_machine->createDataLayout());

	llvm::legacy::PassManager passes;
	passes.add(new llvm::TargetLibraryInfoWrapperPass(
	    target_machine->getTargetTriple()));
	passes.add(llvm::createTargetTransformInfoWrapperPass(
	    target_machine->getTargetIRAnalysis()));

	llvm::legacy::FunctionPassManager fnPasses(&module);
	fnPasses.add(llvm::createTargetTransformInfoWrapperPass(
	    target_machine->getTargetIRAnalysis()));

	llvm::PassManagerBuilder pmb;
	pmb.OptLevel = 3;
	pmb.SizeLevel = 0;
	pmb.Inliner = llvm::createFunctionInliningPass(3, 0, false);
	pmb.LoopVectorize = true;
	pmb.SLPVectorize = true;
	target_machine->adjustPassManager(pmb);
	pmb.populateFunctionPassManager(fnPasses);
	pmb.populateModulePassManager(passes);

	fnPasses.doInitialization();
	for (auto &func : module) {
		fnPasses.run(func);
	}
	fnPasses.doFinalization();

	passes.add(llvm::createVerifierPass());
	passes.run(module);
}

void compile(llvm::Module &module, llvm::CodeGenFileType type,
             llvm::raw_pwrite_stream &out) {
	auto target_machine = create_target_machine();
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
