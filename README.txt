作业说明
====================

---- 目录结构 ----
  source/               程序源代码
    ...
  compiler              编译器可执行文件 (ELF 格式, 需要 Linux 下运行)
  in.txt                示例源程序文件
  out.txt               输出的四元式
  debug.txt             输出的二元式、产生式、四元式
  program_ast.json      输出的 JSON 格式的 AST (抽象语法树)
  program.ll            输出的 LLVM IR 中间代码 (未优化)
  program_optimized.ll  输出的优化后的 LLVM IR 中间代码
  program.o             输出的目标代码文件 (ELF 格式)
  program.s             输出的汇编代码
  program               输出的可执行文件 (ELF 格式, 需要 Linux 下运行)


---- 运行说明 ----
本编译器需要在 Linux 下运行, 编译使用的环境为 Ubuntu 22.10.
运行时可加上 -h 参数查看帮助:
$ ./compiler -h
compiler - A demo compiler based on LLVM

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


---- 编译说明 ----
编译需要 CMake 3.10 或以上版本, LLVM 16.0 (目前还是开发版本).
Ubuntu 下安装 LLVM 的方法可参考 <https://apt.llvm.org/>, 至少需要安装 clang lld llvm-dev 这三个包.

在 source/ 目录下运行以下命令:
$ cmake -B build
$ cd build
$ make -j

编译完成后, 编译器可执行文件位于 source/build/compiler.


---- 使用示例 ----

1. 从当前目录 in.txt 读入文件并编译
$ ./compiler
Writing tokens, productions and TAC to debug.txt ... OK
Writing TAC to out.txt ... OK
Writing AST to program_ast.json ... OK
Writing LLVM IR to program.ll ... OK
Writing object code to program.o ... OK
Writing ASM code to program.s ... OK
Invoking cc to link executable ... OK


2. 读入指定源文件 (如 samples/in.txt) 并编译
$ ./compiler -f samples/in.txt
Writing tokens, productions and TAC to debug.txt ... OK
Writing TAC to out.txt ... OK
Writing AST to program_ast.json ... OK
Writing LLVM IR to program.ll ... OK
Writing object code to program.o ... OK
Writing ASM code to program.s ... OK
Invoking cc to link executable ... OK


3. 从当前目录 in.txt 读入文件并编译, 开启编译优化
$ ./compiler -o
Writing tokens, productions and TAC to debug.txt ... OK
Writing TAC to out.txt ... OK
Writing AST to program_ast.json ... OK
Writing LLVM IR to program.ll ... OK
Writing optimized LLVM IR to program_optimized.ll ... OK
Writing object code to program.o ... OK
Writing ASM code to program.s ... OK
Invoking cc to link executable ... OK


4. 从当前目录 in.txt 读入文件并编译, 并使用 JIT 运行代码
$ ./compiler -j
Writing tokens, productions and TAC to debug.txt ... OK
Writing TAC to out.txt ... OK
Writing AST to program_ast.json ... OK
Writing LLVM IR to program.ll ... OK
Writing object code to program.o ... OK
Writing ASM code to program.s ... OK
Invoking cc to link executable ... OK

---- JIT Execution ----
a = aaa
b = jmkkkkkk
c = acacdacacacdacdacacacdacacacdacdacdacacacdacacacdacdacacacdacacacdacdacdacdacacacdacacacdacdacacacdacacacdacdacdacacacdacacacdacdacacacdacacacdacdacdacdacdacacacdacacacdacdacacacdacacacdacdacdacacacdacacacdacdacacacdacacacdacdacdacdacacacdacacacdacdacacacdacacacdacdacdacacacdacacacdacdacacacdacacacdacdacdacdacdacd


5. 从标准输入读入代码 (直接在终端输入源程序, 然后按 Ctrl+D 结束), 编译, 打开调试 (目标程序展示每一次变量赋值)，并使用 JIT 运行代码
$ ./compiler -i -j -d
string a,b,c;
a="aaa";
b="jm";
c="";
do 
start
  b=b+"k";
  c=(c+"ac")*2+"d";
end
while (b<a*2+"mm");

Writing tokens, productions and TAC to debug.txt ... OK
Writing TAC to out.txt ... OK
Writing AST to program_ast.json ... OK
Writing LLVM IR to program.ll ... OK
Writing object code to program.o ... OK
Writing ASM code to program.s ... OK
Invoking cc to link executable ... OK

---- JIT Execution ----
a := aaa
b := jm
c := 
b := jmk
c := acacd
b := jmkk
c := acacdacacacdacd
b := jmkkk
c := acacdacacacdacdacacacdacacacdacdacd
b := jmkkkk
c := acacdacacacdacdacacacdacacacdacdacdacacacdacacacdacdacacacdacacacdacdacdacd
b := jmkkkkk
c := acacdacacacdacdacacacdacacacdacdacdacacacdacacacdacdacacacdacacacdacdacdacdacacacdacacacdacdacacacdacacacdacdacdacacacdacacacdacdacacacdacacacdacdacdacdacd
b := jmkkkkkk
c := acacdacacacdacdacacacdacacacdacdacdacacacdacacacdacdacacacdacacacdacdacdacdacacacdacacacdacdacacacdacacacdacdacdacacacdacacacdacdacacacdacacacdacdacdacdacdacacacdacacacdacdacacacdacacacdacdacdacacacdacacacdacdacacacdacacacdacdacdacdacacacdacacacdacdacacacdacacacdacdacdacacacdacacacdacdacacacdacacacdacdacdacdacdacd
a = aaa
b = jmkkkkkk
c = acacdacacacdacdacacacdacacacdacdacdacacacdacacacdacdacacacdacacacdacdacdacdacacacdacacacdacdacacacdacacacdacdacdacacacdacacacdacdacacacdacacacdacdacdacdacdacacacdacacacdacdacacacdacacacdacdacdacacacdacacacdacdacacacdacacacdacdacdacdacacacdacacacdacdacacacdacacacdacdacdacacacdacacacdacdacacacdacacacdacdacdacdacdacd


---- 版权说明 ----
MIT License

Copyright (c) 2022 Haowei Wen <yushijinhun@gmail.com>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
