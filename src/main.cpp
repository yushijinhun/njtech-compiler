#include "aot.hpp"
#include "ast.hpp"
#include "codegen.hpp"
#include "error.hpp"
#include "jit.hpp"
#include "parser.hpp"
#include "tac.hpp"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <llvm/Support/raw_os_ostream.h>

int main() {
	try {
		compiler::Tokenizer tokenizer(std::cin);
		tokenizer.set_print_token_to(std::cout);
		compiler::Parser parser(tokenizer);
		parser.set_print_production_to(std::cout);
		auto ast = parser.parse();
		std::cout << "\n";
		std::cout << "---- TAC ----\n" << compiler::TAC(*ast) << "\n";

		{
			// write program_ast.json
			std::cout << "--- Writing AST to program_ast.json ----\n";
			std::ofstream fout("program_ast.json");
			fout << *ast;
			std::cout << "OK!\n\n";
		}

		auto llvm_ctx = std::make_unique<llvm::LLVMContext>();
		auto module = compiler::LLVMCodeGen::fromAST(*llvm_ctx, *ast);

		{
			// write to program.ll
			std::cout << "--- Writing LLVM IR to program.ll ----\n";
			std::ofstream fout("program.ll");
			llvm::raw_os_ostream file_stream(fout);
			module->print(file_stream, nullptr);
			std::cout << "OK!\n\n";
		}

		compiler::aot::initialize();
		/* Optimization is disabled because it would increase code size dramatically
		{
			// write to program_optimized.ll
			std::cout << "--- Writing optimized LLVM IR to "
			             "program_optimized.ll ----\n";
			compiler::aot::optimize(*module);
			std::ofstream fout("program_optimized.ll");
			llvm::raw_os_ostream file_stream(fout);
			module->print(file_stream, nullptr);
			std::cout << "OK!\n\n";
		}
		*/
		{
			// write to program.o
			std::cout << "--- Writing object code to program.o ----\n";
			compiler::aot::compile_object_file(*module, "program.o");
			std::cout << "OK!\n\n";
		}
		{
			// write to program.s
			std::cout << "--- Writing ASM code to program.s ----\n";
			compiler::aot::compile_asm_file(*module, "program.s");
			std::cout << "OK!\n\n";
		}
		{
			// invoke cc to link executable
			std::cout << "--- Invoking cc to link executable ----\n";
			int ret = std::system("cc program.o -o program");
			if (ret == 0) {
				std::cout << "OK!\n\n";
			} else {
				std::cout << "Error! Return code " << ret << "\n\n";
			}
		}

		std::cout << "---- JIT Execution ----\n";
		compiler::jit::initialize();
		compiler::jit::invoke_module(std::move(llvm_ctx), std::move(module));
		std::cout << "\n";

	} catch (compiler::CompileException &ex) {
		std::cout << ex.what() << "\n";
	}
}
