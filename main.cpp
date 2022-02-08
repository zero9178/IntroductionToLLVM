#include <llvm/Support/TargetSelect.h>

#include "Codegen.hpp"
#include "Parser.hpp"

int main()
{
    auto tokens = tokenize(R"(

fun fib(x: int): int {
    if x <= 1 {
        return 1;
    }
    return fib(x - 2) + fib(x - 1);
}

)");
    auto file = Parser(tokens.begin(), tokens.end()).parseFile();

    llvm::InitializeAllTargetInfos();
    llvm::InitializeAllTargets();
    llvm::InitializeAllTargetMCs();
    llvm::InitializeAllAsmPrinters();
    llvm::InitializeAllAsmParsers();
    llvm::LLVMContext context;
    Codegen codegen(context);
    codegen.visit(file);
    auto* llvmModule = codegen.getModule();
    llvmModule->print(llvm::outs(), nullptr);
}
