#include "aot.hpp"
#include "ast.hpp"
#include "codegen.hpp"
#include "error.hpp"
#include "jit.hpp"
#include "parser.hpp"
#include "tac.hpp"
#include <fstream>
#include <iomanip>
#include <iostream>
#include <llvm/Support/raw_os_ostream.h>

int main() {
	try {
		compiler::Tokenizer tokenizer(std::cin);
		compiler::Parser parser([&tokenizer] {
			auto token = tokenizer.next();
			std::cout << std::left << std::setw(5) << token.position
			          << std::left << std::setw(20) << to_string(token.type)
			          << token.str << "\n";
			return token;
		});
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
		{
			// write to program.o
			std::cout << "--- Writing to program.o ----\n";
			compiler::aot::compile_object_file(*module, "program.o");
			std::cout << "OK!\n\n";
		}
		{
			// write to program.s
			std::cout << "--- Writing to program.s ----\n";
			compiler::aot::compile_asm_file(*module, "program.s");
			std::cout << "OK!\n\n";
		}

		std::cout << "---- JIT Execution ----\n";
		compiler::jit::initialize();
		compiler::jit::invoke_module(std::move(llvm_ctx), std::move(module));
		std::cout << "\n";

	} catch (compiler::CompileException &ex) {
		std::cout << ex.what() << "\n";
	}
}
