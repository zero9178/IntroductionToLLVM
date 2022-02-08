#include "Codegen.hpp"

#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>

Codegen::Codegen(llvm::LLVMContext& context) : m_builder(context)
{
    m_module = std::make_unique<llvm::Module>("", context);
}

llvm::Type* Codegen::visit(const Type& type)
{
    switch (type)
    {
        case Type::Integer: return llvm::IntegerType::get(m_module->getContext(), sizeof(int) * __CHAR_BIT__);
        case Type::Double: return llvm::Type::getDoubleTy(m_module->getContext());
        default: assert(false);
    }
}

void Codegen::visit(const Function& function)
{
    auto returnType = visit(function.returnType);
    std::vector<llvm::Type*> argumentTypes;
    for (auto& iter : function.parameters)
    {
        argumentTypes.push_back(visit(iter->type));
    }
    auto* functionType = llvm::FunctionType::get(returnType, argumentTypes, false);
    m_currentFunc = llvm::Function::Create(functionType, llvm::GlobalValue::ExternalLinkage, 0, function.identifier,
                                           m_module.get());
    m_functionMap[&function] = m_currentFunc;
    m_builder.SetInsertPoint(llvm::BasicBlock::Create(m_module->getContext(), "entry", m_currentFunc));
    for (std::size_t i = 0; i < function.parameters.size(); i++)
    {
        auto* alloca = m_builder.CreateAlloca(visit(function.parameters[i]->type));
        m_variableMap[function.parameters[i].get()] = alloca;
        m_builder.CreateStore(m_currentFunc->getArg(i), alloca);
    }
    for (auto& iter : function.body)
    {
        visit(iter);
    }
    if (m_builder.GetInsertBlock() && !m_builder.GetInsertBlock()->back().isTerminator())
    {
        m_builder.CreateUnreachable();
    }
}

llvm::Value* Codegen::boolean(llvm::Value* value)
{
    if (value->getType()->isIntegerTy())
    {
        return m_builder.CreateCmp(llvm::CmpInst::ICMP_NE, value, llvm::ConstantInt::get(value->getType(), 0));
    }
    return m_builder.CreateFCmp(llvm::CmpInst::FCMP_UNE, value, llvm::ConstantFP::get(value->getType(), 0));
}

void Codegen::visit(const Statement& statement)
{
    if (auto* ret = std::get_if<Statement::ReturnStatement>(&statement.variant))
    {
        llvm::Value* value = visit(*ret->expression);
        m_builder.CreateRet(value);
        m_builder.ClearInsertionPoint();
        return;
    }
    if (auto* expr = std::get_if<std::unique_ptr<Expression>>(&statement.variant))
    {
        visit(**expr);
        return;
    }
    if (auto* varDecl = std::get_if<std::unique_ptr<VarDecl>>(&statement.variant))
    {
        llvm::IRBuilder<> temp(&m_currentFunc->getEntryBlock());
        auto* alloca = temp.CreateAlloca(visit((*varDecl)->type));
        if ((*varDecl)->initializer)
        {
            llvm::Value* value = visit(*(*varDecl)->initializer);
            m_builder.CreateStore(value, alloca);
        }
        m_variableMap[varDecl->get()] = alloca;
        return;
    }
    if (auto* assignment = std::get_if<Statement::Assignment>(&statement.variant))
    {
        llvm::Value* value = visit(*assignment->value);
        auto result = m_variableMap.find(assignment->variable);
        assert(result != m_variableMap.end());
        m_builder.CreateStore(value, result->second);
        return;
    }
    if (auto* ifStmt = std::get_if<Statement::IfStatement>(&statement.variant))
    {
        auto trueBranch = llvm::BasicBlock::Create(m_module->getContext());
        auto continueBranch = llvm::BasicBlock::Create(m_module->getContext());
        llvm::Value* condition = boolean(visit(*ifStmt->condition));
        m_builder.CreateCondBr(condition, trueBranch, continueBranch);

        trueBranch->insertInto(m_currentFunc);
        m_builder.SetInsertPoint(trueBranch);
        for (auto& iter : ifStmt->body)
        {
            visit(iter);
        }
        if (m_builder.GetInsertBlock() && !m_builder.GetInsertBlock()->getTerminator())
        {
            m_builder.CreateBr(continueBranch);
        }

        continueBranch->insertInto(m_currentFunc);
        m_builder.SetInsertPoint(continueBranch);
        return;
    }
    if (auto* whileStmt = std::get_if<Statement::WhileStatement>(&statement.variant))
    {
        auto conditionBlock = llvm::BasicBlock::Create(m_module->getContext(), "", m_currentFunc);
        m_builder.CreateBr(conditionBlock);

        m_builder.SetInsertPoint(conditionBlock);
        auto body = llvm::BasicBlock::Create(m_module->getContext());
        auto continueBranch = llvm::BasicBlock::Create(m_module->getContext());
        llvm::Value* condition = boolean(visit(*whileStmt->condition));
        m_builder.CreateCondBr(condition, body, continueBranch);

        body->insertInto(m_currentFunc);
        m_builder.SetInsertPoint(body);
        for (auto& iter : whileStmt->body)
        {
            visit(iter);
        }
        if (m_builder.GetInsertBlock() && !m_builder.GetInsertBlock()->getTerminator())
        {
            m_builder.CreateBr(conditionBlock);
        }

        continueBranch->insertInto(m_currentFunc);
        m_builder.SetInsertPoint(continueBranch);
        return;
    }
}

llvm::Value* Codegen::visit(const Expression& expression)
{
    if (auto* atom = dynamic_cast<const Atom*>(&expression))
    {
        if (const int* integer = std::get_if<int>(&atom->valueOrVar))
        {
            return llvm::ConstantInt::get(visit(expression.type), *integer);
        }
        if (const double* floating = std::get_if<double>(&atom->valueOrVar))
        {
            return llvm::ConstantFP::get(visit(expression.type), *floating);
        }
        VarDecl* decl = std::get<VarDecl*>(atom->valueOrVar);
        auto result = m_variableMap.find(decl);
        assert(result != m_variableMap.end());
        return m_builder.CreateLoad(visit(decl->type), result->second);
    }
    if (auto* cast = dynamic_cast<const CastExpression*>(&expression))
    {
        llvm::Value* value = visit(*cast->operand);
        if (cast->type == Type::Integer && cast->operand->type == Type::Double)
        {
            return m_builder.CreateFPToSI(value, visit(cast->type));
        }
        if (cast->type == Type::Double && cast->operand->type == Type::Integer)
        {
            return m_builder.CreateSIToFP(value, visit(cast->type));
        }
        return value;
    }
    if (auto* negate = dynamic_cast<const NegateExpression*>(&expression))
    {
        llvm::Value* value = visit(*negate->operand);
        if (negate->type == Type::Double)
        {
            return m_builder.CreateFNeg(value);
        }
        return m_builder.CreateNeg(value);
    }
    if (auto* call = dynamic_cast<const CallExpression*>(&expression))
    {
        auto result = m_functionMap.find(call->function);
        assert(result != m_functionMap.end());
        std::vector<llvm::Value*> arguments;
        for (auto& iter : call->arguments)
        {
            arguments.push_back(visit(*iter));
        }
        return m_builder.CreateCall(result->second, arguments);
    }
    auto& binary = dynamic_cast<const BinaryExpression&>(expression);
    llvm::Value* lhs = visit(*binary.lhs);
    llvm::Value* rhs = visit(*binary.rhs);
    switch (binary.operation)
    {
        case Token::OrKeyword:
        {
            lhs = boolean(lhs);
            rhs = boolean(rhs);
            auto result = m_builder.CreateOr(lhs, rhs);
            return m_builder.CreateZExt(result, visit(binary.type));
        }
        case Token::AndKeyword:
        {
            lhs = boolean(lhs);
            rhs = boolean(rhs);
            auto result = m_builder.CreateOr(lhs, rhs);
            return m_builder.CreateZExt(result, visit(binary.type));
        }
        case Token::Less:
        case Token::LessEqual:
        case Token::Greater:
        case Token::GreaterEqual:
        case Token::Equal:
        case Token::NotEqual:
        {
            llvm::CmpInst::Predicate predicate;
            if (binary.lhs->type == Type::Integer)
            {
                switch (binary.operation)
                {
                    case Token::Less: predicate = llvm::CmpInst::ICMP_SLT; break;
                    case Token::LessEqual: predicate = llvm::CmpInst::ICMP_SLE; break;
                    case Token::Greater: predicate = llvm::CmpInst::ICMP_SGT; break;
                    case Token::GreaterEqual: predicate = llvm::CmpInst::ICMP_SGE; break;
                    case Token::Equal: predicate = llvm::CmpInst::ICMP_EQ; break;
                    case Token::NotEqual: predicate = llvm::CmpInst::ICMP_NE; break;
                    default: __builtin_unreachable();
                }
            }
            else
            {
                switch (binary.operation)
                {
                    case Token::Less: predicate = llvm::CmpInst::FCMP_ULT; break;
                    case Token::LessEqual: predicate = llvm::CmpInst::FCMP_ULE; break;
                    case Token::Greater: predicate = llvm::CmpInst::FCMP_UGT; break;
                    case Token::GreaterEqual: predicate = llvm::CmpInst::FCMP_UGE; break;
                    case Token::Equal: predicate = llvm::CmpInst::FCMP_UEQ; break;
                    case Token::NotEqual: predicate = llvm::CmpInst::FCMP_UNE; break;
                    default: __builtin_unreachable();
                }
            }
            auto cmp = m_builder.CreateCmp(predicate, lhs, rhs);
            return m_builder.CreateZExt(cmp, visit(binary.type));
        }
        case Token::Plus:
        {
            if (binary.type == Type::Integer)
            {
                return m_builder.CreateAdd(lhs, rhs);
            }
            else
            {
                return m_builder.CreateFAdd(lhs, rhs);
            }
        }
        case Token::Minus:
        {
            if (binary.type == Type::Integer)
            {
                return m_builder.CreateSub(lhs, rhs);
            }
            else
            {
                return m_builder.CreateFSub(lhs, rhs);
            }
        }
        case Token::Times:
        {
            if (binary.type == Type::Integer)
            {
                return m_builder.CreateMul(lhs, rhs);
            }
            else
            {
                return m_builder.CreateFMul(lhs, rhs);
            }
        }
        case Token::Divide:
        {
            if (binary.type == Type::Integer)
            {
                return m_builder.CreateSDiv(lhs, rhs);
            }
            else
            {
                return m_builder.CreateFDiv(lhs, rhs);
            }
        }
        default: __builtin_unreachable();
    }
    __builtin_unreachable();
}
