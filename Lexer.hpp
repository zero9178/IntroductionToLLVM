#pragma once

#include <string>
#include <string_view>
#include <variant>
#include <vector>

struct Token
{
    enum TokenType
    {
        IntKeyword,
        DoubleKeyword,
        FunKeyword,
        ReturnKeyword,
        IfKeyword,
        ForKeyword,
        WhileKeyword,
        VarKeyword,
        AsKeyword,
        OrKeyword,
        AndKeyword,
        OpenParen,
        CloseParen,
        OpenBrace,
        CloseBrace,
        Comma,
        SemiColon,
        Colon,
        Assignment,
        Less,
        Greater,
        LessEqual,
        GreaterEqual,
        Equal,
        NotEqual,
        Plus,
        Minus,
        Times,
        Divide,
        Identifier,
        Decimal,
        Number
    };
    TokenType tokenType;
    using Variant = std::variant<std::monostate, double, int, std::string>;
    Variant variant;

    explicit Token(TokenType tokenType, Variant variant = {}) : tokenType(tokenType), variant(variant) {}
};

std::vector<Token> tokenize(std::string_view source);
