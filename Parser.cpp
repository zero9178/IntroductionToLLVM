
#include "Parser.hpp"

#include <iostream>

File Parser::parseFile()
{
    std::vector<std::unique_ptr<Function>> functions;
    while (m_curr != m_end)
    {
        functions.push_back(parseFunction());
    }
    return {std::move(functions)};
}

namespace
{
struct Stream
{
    [[noreturn]] ~Stream()
    {
        std::abort();
    }

    Stream& operator<<(Token::TokenType type)
    {
        switch (type)
        {
            case Token::IntKeyword: std::cerr << "'int'"; break;
            case Token::DoubleKeyword: std::cerr << "'double'"; break;
            case Token::FunKeyword: std::cerr << "'fun'"; break;
            case Token::IfKeyword: std::cerr << "'if'"; break;
            case Token::ForKeyword: std::cerr << "'for'"; break;
            case Token::WhileKeyword: std::cerr << "'while'"; break;
            case Token::ReturnKeyword: std::cerr << "'return'"; break;
            case Token::VarKeyword: std::cerr << "'var'"; break;
            case Token::AsKeyword: std::cerr << "'as'"; break;
            case Token::OrKeyword: std::cerr << "'or'"; break;
            case Token::AndKeyword: std::cerr << "'and'"; break;
            case Token::OpenParen: std::cerr << "'('"; break;
            case Token::CloseParen: std::cerr << "')'"; break;
            case Token::OpenBrace: std::cerr << "'{'"; break;
            case Token::CloseBrace: std::cerr << "'}'"; break;
            case Token::Comma: std::cerr << "','"; break;
            case Token::Colon: std::cerr << "':'"; break;
            case Token::SemiColon: std::cerr << "';'"; break;
            case Token::Assignment: std::cerr << "'='"; break;
            case Token::Less: std::cerr << "'<'"; break;
            case Token::Greater: std::cerr << "'>'"; break;
            case Token::LessEqual: std::cerr << "'<='"; break;
            case Token::GreaterEqual: std::cerr << "'>='"; break;
            case Token::Equal: std::cerr << "'=='"; break;
            case Token::NotEqual: std::cerr << "'!='"; break;
            case Token::Plus: std::cerr << "'+'"; break;
            case Token::Minus: std::cerr << "'-'"; break;
            case Token::Times: std::cerr << "'*'"; break;
            case Token::Divide: std::cerr << "'/'"; break;
            case Token::Identifier: std::cerr << "identifier"; break;
            case Token::Decimal: std::cerr << "decimal"; break;
            case Token::Number: std::cerr << "number"; break;
        }
        return *this;
    }

    template <class T>
    Stream& operator<<(const T& value)
    {
        std::cerr << value;
        return *this;
    }
};

Stream error(std::string_view text)
{
    std::cerr << "error: " << text;
    return {};
}

} // namespace

void Parser::expect(Token::TokenType type)
{
    if (m_curr == m_end)
    {
        error("Expected ") << type;
    }
    if (m_curr->tokenType != type)
    {
        error("Expected ") << type << " instead of " << m_curr->tokenType;
    }
    m_curr++;
}

std::string Parser::expectIdentifier()
{
    if (m_curr == m_end)
    {
        error("Expected ") << Token::Identifier;
    }
    if (m_curr->tokenType != Token::Identifier)
    {
        error("Expected ") << Token::Identifier << " instead of " << m_curr->tokenType;
    }
    std::string result = std::get<std::string>(m_curr->variant);
    m_curr++;
    return result;
}

Type Parser::parseType()
{
    if (m_curr == m_end)
    {
        error("Expected 'int' or 'double'");
    }
    switch (m_curr->tokenType)
    {
        case Token::DoubleKeyword: m_curr++; return Type::Double;
        case Token::IntKeyword: m_curr++; return Type::Integer;
        default: error("Expected 'int' or 'double' instead of ") << m_curr->tokenType;
    }
}

std::unique_ptr<Function> Parser::parseFunction()
{
    expect(Token::FunKeyword);
    auto name = expectIdentifier();
    expect(Token::OpenParen);
    std::vector<std::unique_ptr<VarDecl>> parameters;
    auto parseParen = [&]
    {
        auto name = expectIdentifier();
        expect(Token::Colon);
        return std::make_unique<VarDecl>(std::move(name), parseType());
    };

    if (m_curr != m_end && m_curr->tokenType == Token::Identifier)
    {
        parameters.push_back(parseParen());
        while (maybeConsume(Token::Comma))
        {
            parameters.push_back(parseParen());
        }
    }
    expect(Token::CloseParen);
    expect(Token::Colon);
    auto type = parseType();
    auto function = std::make_unique<Function>(std::move(name), std::move(parameters), type);
    m_functions[function->identifier] = function.get();
    expect(Token::OpenBrace);
    m_variables.clear();
    m_currentFunc = function.get();
    for (auto& iter : function->parameters)
    {
        m_variables[iter->identifier] = iter.get();
    }
    std::vector<Statement> statements;
    while (m_curr != m_end && m_curr->tokenType != Token::CloseBrace)
    {
        statements.push_back(parseStatement());
    }
    expect(Token::CloseBrace);
    function->body = std::move(statements);
    return function;
}

Statement Parser::parseStatement()
{
    switch (m_curr->tokenType)
    {
        case Token::VarKeyword:
        {
            m_curr++;
            auto name = expectIdentifier();
            std::optional<Type> type;
            if (maybeConsume(Token::Colon))
            {
                type = parseType();
            }
            std::unique_ptr<Expression> initializer;
            if (maybeConsume(Token::Assignment))
            {
                initializer = parseExpression();
            }
            expect(Token::SemiColon);
            if (!type && !initializer)
            {
                error("variable ") << name << " declared without a type";
            }
            if (!type)
            {
                type = initializer->type;
            }
            if (initializer && initializer->type != *type)
            {
                initializer = std::make_unique<CastExpression>(*type, std::move(initializer));
            }
            auto var = std::make_unique<VarDecl>(std::move(name), *type, std::move(initializer));
            m_variables[var->identifier] = var.get();
            return {std::move(var)};
        }
        case Token::ReturnKeyword:
        {
            m_curr++;
            auto expression = parseExpression();
            expect(Token::SemiColon);
            if (m_currentFunc->returnType != expression->type)
            {
                expression = std::make_unique<CastExpression>(m_currentFunc->returnType, std::move(expression));
            }
            return {Statement::ReturnStatement{std::move(expression)}};
        }
        case Token::IfKeyword:
        {
            m_curr++;
            auto condition = parseExpression();
            expect(Token::OpenBrace);
            std::vector<Statement> statements;
            while (m_curr != m_end && m_curr->tokenType != Token::CloseBrace)
            {
                statements.push_back(parseStatement());
            }
            expect(Token::CloseBrace);
            return {Statement::IfStatement{std::move(condition), std::move(statements)}};
        }
        case Token::WhileKeyword:
        {
            m_curr++;
            auto condition = parseExpression();
            expect(Token::OpenBrace);
            std::vector<Statement> statements;
            while (m_curr != m_end && m_curr->tokenType != Token::CloseBrace)
            {
                statements.push_back(parseStatement());
            }
            expect(Token::CloseBrace);
            return {Statement::WhileStatement{std::move(condition), std::move(statements)}};
        }
        case Token::Identifier:
        {
            if (std::next(m_curr) != m_end && std::next(m_curr)->tokenType == Token::Assignment)
            {
                auto identifier = expectIdentifier();
                m_curr++;
                auto expression = parseExpression();
                expect(Token::SemiColon);
                auto result = m_variables.find(identifier);
                if (result == m_variables.end())
                {
                    error("Could not assign to unknown variable ") << identifier;
                }
                if (expression->type != result->second->type)
                {
                    expression = std::make_unique<CastExpression>(result->second->type, std::move(expression));
                }
                return {Statement::Assignment{result->second, std::move(expression)}};
            }
            [[fallthrough]];
        }
        default:
        {
            auto expression = parseExpression();
            expect(Token::SemiColon);
            return {std::move(expression)};
        }
    }
}

std::unique_ptr<Expression> Parser::parseExpression()
{
    auto expression = parseOrExpression();
    if (!maybeConsume(Token::AsKeyword))
    {
        return expression;
    }
    auto type = parseType();
    return std::make_unique<CastExpression>(type, std::move(expression));
}

namespace
{
Type commonType(std::unique_ptr<Expression>& lhs, std::unique_ptr<Expression>& rhs)
{
    if (lhs->type == Type::Double && rhs->type != Type::Double)
    {
        rhs = std::make_unique<CastExpression>(Type::Double, std::move(rhs));
        return Type::Double;
    }
    else if (rhs->type == Type::Double && lhs->type != Type::Double)
    {
        lhs = std::make_unique<CastExpression>(Type::Double, std::move(lhs));
        return Type::Double;
    }
    return Type::Integer;
}

} // namespace

template <std::unique_ptr<Expression> (Parser::*parse)(), Token::TokenType... tokenTypes>
std::unique_ptr<Expression> Parser::parseBinaryExpression()
{
    auto lhs = (this->*parse)();
    while (m_curr != m_end && ((m_curr->tokenType == tokenTypes) || ...))
    {
        auto op = m_curr->tokenType;
        m_curr++;
        auto rhs = (this->*parse)();
        Type type;
        if (op == Token::AndKeyword || op == Token::OrKeyword)
        {
            type = Type::Integer;
        }
        else if (op == Token::Less || op == Token::Greater || op == Token::LessEqual || op == Token::GreaterEqual
                 || op == Token::Equal || op == Token::NotEqual)
        {
            type = Type::Integer;
            commonType(lhs, rhs);
        }
        else
        {
            type = commonType(lhs, rhs);
        }
        lhs = std::make_unique<BinaryExpression>(type, std::move(lhs), op, std::move(rhs));
    }
    return lhs;
}

std::unique_ptr<Expression> Parser::parseOrExpression()
{
    return parseBinaryExpression<&Parser::parseAndExpression, Token::AndKeyword>();
}

std::unique_ptr<Expression> Parser::parseAndExpression()
{
    return parseBinaryExpression<&Parser::parseCmpExpression, Token::OrKeyword>();
}

std::unique_ptr<Expression> Parser::parseCmpExpression()
{
    return parseBinaryExpression<&Parser::parseAddExpression, Token::Less, Token::LessEqual, Token::Greater,
                                 Token::GreaterEqual, Token::Equal, Token::NotEqual>();
}

std::unique_ptr<Expression> Parser::parseAddExpression()
{
    return parseBinaryExpression<&Parser::parseMulExpression, Token::Plus, Token::Minus>();
}

std::unique_ptr<Expression> Parser::parseMulExpression()
{
    return parseBinaryExpression<&Parser::parseUnaryExpression, Token::Times, Token::Divide>();
}

std::unique_ptr<Expression> Parser::parseUnaryExpression()
{
    if (maybeConsume(Token::Minus))
    {
        auto expression = parsePostfixExpression();
        return std::make_unique<NegateExpression>(expression->type, std::move(expression));
    }
    return parsePostfixExpression();
}

std::unique_ptr<Expression> Parser::parsePostfixExpression()
{
    if (m_curr != m_end && m_curr->tokenType == Token::Identifier && std::next(m_curr) != m_end
        && std::next(m_curr)->tokenType == Token::OpenParen)
    {
        auto functionName = expectIdentifier();
        expect(Token::OpenParen);
        std::vector<std::unique_ptr<Expression>> arguments;
        arguments.push_back(parseExpression());
        while (maybeConsume(Token::Colon))
        {
            arguments.push_back(parseExpression());
        }
        expect(Token::CloseParen);
        auto result = m_functions.find(functionName);
        if (result == m_functions.end())
        {
            error("Cannot call unknown function ") << functionName;
        }
        auto* function = result->second;
        if (function->parameters.size() != arguments.size())
        {
            error("Too many arguments given for call to ") << functionName;
        }
        for (std::size_t i = 0; i < arguments.size(); i++)
        {
            if (arguments[i]->type != function->parameters[i]->type)
            {
                arguments[i] = std::make_unique<CastExpression>(function->parameters[i]->type, std::move(arguments[i]));
            }
        }
        return std::make_unique<CallExpression>(function->returnType, function, std::move(arguments));
    }
    return parseAtom();
}

std::unique_ptr<Expression> Parser::parseAtom()
{
    if (m_curr == m_end)
    {
        error("Expected number, decimal or '('");
    }
    switch (m_curr->tokenType)
    {
        case Token::Number:
        {
            auto number = std::make_unique<Atom>(Type::Integer, std::get<int>(m_curr->variant));
            m_curr++;
            return number;
        }
        case Token::Decimal:
        {
            auto number = std::make_unique<Atom>(Type::Double, std::get<double>(m_curr->variant));
            m_curr++;
            return number;
        }
        case Token::Identifier:
        {
            auto identifier = expectIdentifier();
            auto result = m_variables.find(identifier);
            if (result == m_variables.end())
            {
                error("Could not read from unknown variable ") << identifier;
            }
            return std::make_unique<Atom>(result->second->type, result->second);
        }
        default: error("Expected number, decimal or '(' instead of ") << m_curr->tokenType;
    }
}
