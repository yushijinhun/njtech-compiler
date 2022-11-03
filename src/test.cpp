#include <fstream>
#include <iostream>
#include <llvm/IR/Constants.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Value.h>
#include <llvm/Support/raw_os_ostream.h>

#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/JITSymbol.h>
#include <llvm/ExecutionEngine/Orc/CompileUtils.h>
#include <llvm/ExecutionEngine/Orc/ExecutionUtils.h>
#include <llvm/ExecutionEngine/Orc/IRCompileLayer.h>
#include <llvm/ExecutionEngine/Orc/RTDyldObjectLinkingLayer.h>
#include <llvm/ExecutionEngine/SectionMemoryManager.h>
#include <llvm/Support/TargetSelect.h>

std::unique_ptr<llvm::Module> createModule(llvm::LLVMContext &ctx) {
	llvm::IRBuilder<> builder(ctx);

	auto module = std::make_unique<llvm::Module>("program", ctx);
	llvm::Function *mainFunc = llvm::Function::Create(
	    llvm::FunctionType::get(builder.getInt32Ty(), false),
	    llvm::Function::ExternalLinkage, "main", *module);
	llvm::BasicBlock *entry =
	    llvm::BasicBlock::Create(ctx, "entrypoint", mainFunc);
	builder.SetInsertPoint(entry);

	llvm::Value *helloWorldStr =
	    builder.CreateGlobalStringPtr("hello, world\n");

	llvm::FunctionCallee putsFunc = module->getOrInsertFunction(
	    "puts",
	    llvm::FunctionType::get(
	        builder.getInt32Ty(),
	        llvm::ArrayRef<llvm::Type *>({builder.getInt8Ty()->getPointerTo()}),
	        false));
	builder.CreateCall(putsFunc, helloWorldStr);
	builder.CreateRet(llvm::ConstantInt::get(ctx, llvm::APInt(32, 0)));
	return module;
}

int executeModule(std::unique_ptr<llvm::LLVMContext> ctx,
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

int main() {
	LLVMInitializeNativeTarget();
	llvm::InitializeNativeTargetAsmPrinter();

	auto ctx = std::make_unique<llvm::LLVMContext>();
	auto module = createModule(*ctx);

	std::cout << "---- LLVM IR ----\n";
	llvm::raw_os_ostream file_stream(std::cout);
	module->print(file_stream, nullptr);

	std::cout << "\n\n---- JIT Execution ----\n";
	executeModule(std::move(ctx), std::move(module));
}
