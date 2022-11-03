#include "jit.hpp"

#include <llvm/ExecutionEngine/Orc/CompileUtils.h>
#include <llvm/ExecutionEngine/Orc/Core.h>
#include <llvm/ExecutionEngine/Orc/ExecutionUtils.h>
#include <llvm/ExecutionEngine/Orc/IRCompileLayer.h>
#include <llvm/ExecutionEngine/Orc/JITTargetMachineBuilder.h>
#include <llvm/ExecutionEngine/Orc/Mangling.h>
#include <llvm/ExecutionEngine/Orc/RTDyldObjectLinkingLayer.h>
#include <llvm/ExecutionEngine/SectionMemoryManager.h>
#include <llvm/Support/TargetSelect.h>

namespace compiler {
namespace jit {

void initialize() {
	llvm::InitializeNativeTarget();
	llvm::InitializeNativeTargetAsmPrinter();
}

int invoke_module(std::unique_ptr<llvm::LLVMContext> ctx,
                  std::unique_ptr<llvm::Module> module) {
	llvm::orc::ExecutionSession execution_session(
	    llvm::cantFail(llvm::orc::SelfExecutorProcessControl::Create()));
	llvm::orc::JITTargetMachineBuilder jtmb(
	    execution_session.getExecutorProcessControl().getTargetTriple());
	llvm::DataLayout data_layer =
	    llvm::cantFail(jtmb.getDefaultDataLayoutForTarget());
	llvm::orc::MangleAndInterner mangle(execution_session, data_layer);
	llvm::orc::RTDyldObjectLinkingLayer link_layer(execution_session, []() {
		return std::make_unique<llvm::SectionMemoryManager>();
	});
	llvm::orc::IRCompileLayer compile_layer(
	    execution_session, link_layer,
	    std::make_unique<llvm::orc::ConcurrentIRCompiler>(
	        execution_session.getExecutorProcessControl().getTargetTriple()));
	llvm::orc::JITDylib &main_jd(
	    execution_session.createBareJITDylib("<main>"));
	main_jd.addGenerator(llvm::cantFail(
	    llvm::orc::DynamicLibrarySearchGenerator::GetForCurrentProcess(
	        data_layer.getGlobalPrefix())));
	if (jtmb.getTargetTriple().isOSBinFormatCOFF()) {
		link_layer.setOverrideObjectFlagsWithResponsibilityFlags(true);
		link_layer.setAutoClaimResponsibilityForObjectSymbols(true);
	}

	llvm::cantFail(compile_layer.add(
	    main_jd.getDefaultResourceTracker(),
	    llvm::orc::ThreadSafeModule(std::move(module), std::move(ctx))));

	llvm::JITEvaluatedSymbol symbol =
	    llvm::cantFail(execution_session.lookup({&main_jd}, mangle("main")));
	int (*mainFunc)() = (int (*)())(intptr_t)symbol.getAddress();
	int result = mainFunc();

	llvm::cantFail(execution_session.endSession());
	return result;
}

} // namespace jit
} // namespace compiler
