#pragma once

#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>

#include <unordered_map>

#include "Syntax.hpp"

class Codegen
{
    std::unique_ptr<llvm::Module> m_module;
    llvm::Function* m_currentFunc{};
    std::unordered_map<const Function*, llvm::Function*> m_functionMap;
    std::unordered_map<const VarDecl*, llvm::AllocaInst*> m_variableMap;
    llvm::IRBuilder<> m_builder;

    llvm::Value* boolean(llvm::Value* value);

public:
    [[nodiscard]] llvm::Module* getModule() const
    {
        return m_module.get();
    }

    explicit Codegen(llvm::LLVMContext& context);

    llvm::Type* visit(const Type& type);

    void visit(const File& file)
    {
        for (auto& iter : file.functions)
        {
            visit(*iter);
        }
    }

    void visit(const Function& function);

    void visit(const Statement& statement);

    llvm::Value* visit(const Expression& expression);
};
