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

static bool opt_help = false;
static bool opt_interactive = false;
static bool opt_optimize = false;
static bool opt_jit_run = false;
static bool opt_debug = false;
static std::string opt_infile = "in.txt";

static bool parse_commandline(int argc, char *argv[]) {
	int idx = 1;
	while (idx < argc) {
		std::string arg = argv[idx];

		if (arg == "-h" || arg == "--help") {
			opt_help = true;
			idx++;

		} else if (arg == "-i" || arg == "--interactive") {
			opt_interactive = true;
			idx++;

		} else if (arg == "-o" || arg == "--optimize") {
			opt_optimize = true;
			idx++;

		} else if (arg == "-j" || arg == "--jit-run") {
			opt_jit_run = true;
			idx++;

		} else if (arg == "-d" || arg == "--debug") {
			opt_debug = true;
			idx++;

		} else if (arg == "-f" || arg == "--infile") {
			if (idx + 1 < argc) {
				opt_infile = argv[idx + 1];
				idx += 2;
			} else {
				std::cout << "error: -f/--infile requires 1 argument\n";
				return false;
			}
		} else {
			std::cout << "error: unrecognized argument: " << arg << "\n";
			return false;
		}
	}
	return true;
}

static void print_help() {
	std::cout << R"(compiler - A demo compiler based on LLVM

Usage: compiler [options]

Options:
  -h/--help           prints this help text
  -i/--interactive    use interactive mode (see below)
  -f/--infile <path>  use specified source program (see below)
  -o/--optimize       turn on compilation optimization
  -j/--jit-run        run the program using JIT after compilation
  -d/--debug          compile the program in debug mode (print each assignment)

By default, the source program is read from "in.txt". The file path can be
changed using the -f/--infile argument. If -i/--interactive argument is
specified, the source program will be read from the standard input. In the
interactive mode, you can press Ctrl+D to compile and execute the program.

The compiler will output the following files:
  debug.txt             tokens, productions and TAC (three-address-code)
  out.txt               TAC (three-address-code)
  program_ast.json      AST in JSON format
  program.ll            unoptimized LLVM IR
  program_optimized.ll  optimized LLVM IR
                          (available only when -o/--optimize is turned on)
  program.o             compiled object file
  program.s             assembly code
  program               linked executable
                          (available only when "cc" is available)

Author: Haowei Wen <yushijinhun@gmail.com>

)";
}

static int run(std::istream &in) {
	try {

		std::vector<compiler::Token> tokens;
		compiler::Tokenizer tokenizer(in);
		tokenizer.set_token_callback(
		    [&tokens](const auto &it) { tokens.push_back(it); });

		std::vector<std::string> productions;
		compiler::Parser parser(tokenizer);
		parser.set_production_callback(
		    [&productions](const auto &it) { productions.push_back(it); });

		auto ast = parser.parse();
		auto tac = compiler::TAC(*ast);
		auto llvm_ctx = std::make_unique<llvm::LLVMContext>();
		auto module =
		    compiler::LLVMCodeGen::fromAST(*llvm_ctx, *ast, opt_debug);

		{
			std::cout
			    << "Writing tokens, productions and TAC to debug.txt ... ";
			std::cout.flush();
			std::ofstream out("debug.txt");
			if (!out.good()) {
				std::cout << "Failed!\n";
				return 1;
			}
			out << "---- Tokens ----\n";
			for (const auto &token : tokens) {
				out << token;
			}
			out << "\n";
			out << "---- Productions ----\n";
			for (const auto &production : productions) {
				out << production << "\n";
			}
			out << "\n";
			out << "---- TAC (three-address-code) ----\n";
			out << tac;
			std::cout << "OK\n";
		}

		{
			std::cout << "Writing TAC to out.txt ... ";
			std::cout.flush();
			std::ofstream out("out.txt");
			if (!out.good()) {
				std::cout << "Failed!\n";
				return 1;
			}
			out << tac;
			std::cout << "OK\n";
		}

		{
			std::cout << "Writing AST to program_ast.json ... ";
			std::cout.flush();
			std::ofstream out("program_ast.json");
			if (!out.good()) {
				std::cout << "Failed!\n";
				return 1;
			}
			out << *ast;
			std::cout << "OK\n";
		}

		{
			std::cout << "Writing LLVM IR to program.ll ... ";
			std::cout.flush();
			std::ofstream out("program.ll");
			if (!out.good()) {
				std::cout << "Failed!\n";
				return 1;
			}
			llvm::raw_os_ostream file_stream(out);
			module->print(file_stream, nullptr);
			std::cout << "OK\n";
		}

		compiler::aot::initialize();

		if (opt_optimize) {
			std::cout
			    << "Writing optimized LLVM IR to program_optimized.ll ... ";
			std::cout.flush();
			compiler::aot::optimize(*module);
			std::ofstream out("program_optimized.ll");
			if (!out.good()) {
				std::cout << "Failed!\n";
				return 1;
			}
			llvm::raw_os_ostream file_stream(out);
			module->print(file_stream, nullptr);
			std::cout << "OK\n";
		}

		{
			std::cout << "Writing object code to program.o ... ";
			std::cout.flush();
			compiler::aot::compile_object_file(*module, "program.o");
			std::cout << "OK\n";
		}
		{
			std::cout << "Writing ASM code to program.s ... ";
			std::cout.flush();
			compiler::aot::compile_asm_file(*module, "program.s");
			std::cout << "OK\n";
		}
		{
			std::cout << "Invoking cc to link executable ... ";
			std::cout.flush();
			int ret = std::system("cc program.o -o program");
			if (ret == 0) {
				std::cout << "OK\n";
			} else {
				std::cout << "warning: cc returns " << ret
				          << ", skipping linking\n";
			}
		}

		if (opt_jit_run) {
			std::cout << "\n---- JIT Execution ----\n";
			compiler::jit::initialize();
			compiler::jit::invoke_module(std::move(llvm_ctx),
			                             std::move(module));
			std::cout << "\n";
		}

		return 0;

	} catch (compiler::CompileException &ex) {
		std::cout << "error: " << ex.what() << "\n";
		return 1;
	}
}

int main(int argc, char *argv[]) {
	if (!parse_commandline(argc, argv))
		return 1;

	if (opt_help) {
		print_help();
		return 0;
	}

	if (opt_interactive) {
		return run(std::cin);
	} else {
		std::ifstream in(opt_infile);
		if (!in.good()) {
			std::cout << "error: cannot open " + opt_infile + "\n";
			return 1;
		}
		return run(in);
	}
}
