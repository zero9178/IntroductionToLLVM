
#pragma once

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "Lexer.hpp"

/// <type> ::= 'int' | 'double'
enum class Type
{
    Integer,
    Double,
};

struct Statement;

struct Expression;

struct VarDecl
{
    std::string identifier;
    Type type;
    std::unique_ptr<Expression> initializer; // NULLABLE

    VarDecl(std::string identifier, Type type, std::unique_ptr<Expression>&& initializer = {})
        : identifier(std::move(identifier)), type(type), initializer(std::move(initializer))
    {
    }
};

/// <function> ::= 'fun' IDENTIFIER '(' [ <param> { ',' <param> } ')' ':' <type> '{' { <statement> } '}'
///
/// <param> ::= IDENTIFIER ':' <type>
struct Function
{
    std::string identifier;
    std::vector<std::unique_ptr<VarDecl>> parameters;
    Type returnType;
    std::vector<Statement> body;

    Function(std::string identifier, std::vector<std::unique_ptr<VarDecl>> parameters, Type returnType)
        : identifier(std::move(identifier)), parameters(std::move(parameters)), returnType(returnType)
    {
    }
};

/// <statement> ::= 'if' <expression> '{' { <statement> } '}'
///               | 'while' <expression> '{' { <statement> } '}'
///               | 'return' <expression> ';'
///               | IDENTIFIER '=' <expression> ';'
///               | <expression> ';'
///               | 'var' IDENTIFIER [':' <type> ] [ '=' <expression> ] ';'
struct Statement
{
    struct IfStatement
    {
        std::unique_ptr<Expression> condition;
        std::vector<Statement> body;
    };

    struct ReturnStatement
    {
        std::unique_ptr<Expression> expression;
    };

    struct WhileStatement
    {
        std::unique_ptr<Expression> condition;
        std::vector<Statement> body;
    };

    struct Assignment
    {
        VarDecl* variable;
        std::unique_ptr<Expression> value;
    };

    std::variant<IfStatement, WhileStatement, ReturnStatement, Assignment, std::unique_ptr<Expression>,
                 std::unique_ptr<VarDecl>>
        variant;
};

/// <expression> ::= <or-expression> [ 'as' <type> ]
///
/// <or-expression> ::= <and-expression> { 'or' <and-expression> }
///
/// <and-expression> ::= <cmp-expression> { 'and' <cmp-expression> }
///
/// <cmp-expression> ::= <add-expression> { ('<' | '>' | '==' | '!=' | '<=' | '>=' ) <add-expression> }
///
/// <add-expression> ::= <mul-expression> { ('+' | '-') <mul-expression> }
///
/// <mul-expression> ::= <unary-expression> { ('*' | '/') <unary-expression> }
///
/// <unary-expression> ::= [ '-' ] <postfix-expression>
///
/// <postfix-expression> ::= <atom>
///                      | IDENTIFIER '(' [ <expression> { ',' <expression> } ] ')'
///
/// <atom> ::= INTEGER | DECIMAL | IDENTIFIER | '(' <expression> ')'
struct Expression
{
    Type type;

    virtual ~Expression() = default;

    explicit Expression(Type type) : type(type) {}
};

struct BinaryExpression : Expression
{
    std::unique_ptr<Expression> lhs;
    Token::TokenType operation;
    std::unique_ptr<Expression> rhs;

    BinaryExpression(Type type, std::unique_ptr<Expression>&& lhs, Token::TokenType operation,
                     std::unique_ptr<Expression>&& rhs)
        : Expression(type), lhs(std::move(lhs)), operation(operation), rhs(std::move(rhs))
    {
    }
};

struct NegateExpression : Expression
{
    std::unique_ptr<Expression> operand;

    NegateExpression(Type type, std::unique_ptr<Expression>&& operand) : Expression(type), operand(std::move(operand))
    {
    }
};

/// Implicit and explicit!
struct CastExpression : Expression
{
    std::unique_ptr<Expression> operand;

    CastExpression(Type type, std::unique_ptr<Expression>&& operand) : Expression(type), operand(std::move(operand)) {}
};

struct CallExpression : Expression
{
    Function* function;
    std::vector<std::unique_ptr<Expression>> arguments;

    CallExpression(Type type, Function* function, std::vector<std::unique_ptr<Expression>> arguments)
        : Expression(type), function(function), arguments(std::move(arguments))
    {
    }
};

struct Atom : Expression
{
    using Variant = std::variant<int, double, VarDecl*>;
    Variant valueOrVar;

    Atom(Type type, Variant variant) : Expression(type), valueOrVar(variant) {}
};

/// <file> ::= { <function> }
struct File
{
    std::vector<std::unique_ptr<Function>> functions;
};
