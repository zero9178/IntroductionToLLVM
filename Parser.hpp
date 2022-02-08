#pragma once

#include <unordered_map>

#include "Lexer.hpp"
#include "Syntax.hpp"

using Iterator = std::vector<Token>::const_iterator;

class Parser
{
    Iterator m_curr;
    Iterator m_end;
    Function* m_currentFunc;
    std::unordered_map<std::string_view, Function*> m_functions;
    std::unordered_map<std::string_view, VarDecl*> m_variables;

    void expect(Token::TokenType type);

    bool maybeConsume(Token::TokenType type)
    {
        if (m_curr != m_end && m_curr->tokenType == type)
        {
            m_curr++;
            return true;
        }
        return false;
    }

    std::string expectIdentifier();

    template <std::unique_ptr<Expression> (Parser::*parse)(), Token::TokenType... tokenTypes>
    std::unique_ptr<Expression> parseBinaryExpression();

public:
    Parser(Iterator begin, Iterator end) : m_curr(begin), m_end(end) {}

    File parseFile();

    Type parseType();

    std::unique_ptr<Function> parseFunction();

    Statement parseStatement();

    std::unique_ptr<Expression> parseExpression();

    std::unique_ptr<Expression> parseOrExpression();

    std::unique_ptr<Expression> parseAndExpression();

    std::unique_ptr<Expression> parseCmpExpression();

    std::unique_ptr<Expression> parseAddExpression();

    std::unique_ptr<Expression> parseMulExpression();

    std::unique_ptr<Expression> parseUnaryExpression();

    std::unique_ptr<Expression> parsePostfixExpression();

    std::unique_ptr<Expression> parseAtom();
};
