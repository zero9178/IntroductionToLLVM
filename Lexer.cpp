#include "Lexer.hpp"

#include <iostream>

std::vector<Token> tokenize(std::string_view source)
{
    std::vector<Token> result;
    auto curr = source.begin();
    while (curr != source.end())
    {
        auto character = *curr;
        curr++;
        switch (character)
        {
            case ',': result.emplace_back(Token::Comma); break;
            case ':': result.emplace_back(Token::Colon); break;
            case ';': result.emplace_back(Token::SemiColon); break;
            case '+': result.emplace_back(Token::Plus); break;
            case '-': result.emplace_back(Token::Minus); break;
            case '*': result.emplace_back(Token::Times); break;
            case '/': result.emplace_back(Token::Divide); break;
            case '(': result.emplace_back(Token::OpenParen); break;
            case ')': result.emplace_back(Token::CloseParen); break;
            case '{': result.emplace_back(Token::OpenBrace); break;
            case '}': result.emplace_back(Token::CloseBrace); break;
            case '!':
            {
                if (curr != source.end() && *curr == '=')
                {
                    curr++;
                    result.emplace_back(Token::NotEqual);
                    break;
                }
                std::cerr << "Unknown token: !";
                std::abort();
            }
            case '<':
            {
                if (curr != source.end() && *curr == '=')
                {
                    curr++;
                    result.emplace_back(Token::LessEqual);
                    break;
                }
                result.emplace_back(Token::Less);
                break;
            }
            case '>':
            {
                if (curr != source.end() && *curr == '=')
                {
                    curr++;
                    result.emplace_back(Token::GreaterEqual);
                    break;
                }
                result.emplace_back(Token::Greater);
                break;
            }
            case '=':
            {
                if (curr != source.end() && *curr == '=')
                {
                    curr++;
                    result.emplace_back(Token::Equal);
                    break;
                }
                result.emplace_back(Token::Assignment);
                break;
            }
            case ' ':
            case '\n':
            case '\t':
            case '\r': break;
            default:
            {
                if (character >= '0' && character <= '9')
                {
                    std::string value;
                    value += character;
                    for (; curr != source.end() && *curr >= '0' && *curr <= '9'; curr++)
                    {
                        value += *curr;
                    }
                    if (curr == source.end() || *curr != '.')
                    {
                        result.emplace_back(Token::Number, std::stoi(value));
                        break;
                    }
                    value += '.';
                    curr++;
                    for (; curr != source.end() && *curr >= '0' && *curr <= '9'; curr++)
                    {
                        value += *curr;
                    }
                    result.emplace_back(Token::Number, std::stod(value));
                    break;
                }
                if ((character >= 'a' && character <= 'z') || (character >= 'A' && character <= 'Z'))
                {
                    std::string value;
                    value += character;
                    for (; curr != source.end() && (*curr >= 'a' && *curr <= 'z') || (*curr >= 'A' && *curr <= 'Z');
                         curr++)
                    {
                        value += *curr;
                    }
                    if (value == "int")
                    {
                        result.emplace_back(Token::IntKeyword);
                    }
                    else if (value == "double")
                    {
                        result.emplace_back(Token::DoubleKeyword);
                    }
                    else if (value == "fun")
                    {
                        result.emplace_back(Token::FunKeyword);
                    }
                    else if (value == "if")
                    {
                        result.emplace_back(Token::IfKeyword);
                    }
                    else if (value == "for")
                    {
                        result.emplace_back(Token::ForKeyword);
                    }
                    else if (value == "while")
                    {
                        result.emplace_back(Token::WhileKeyword);
                    }
                    else if (value == "var")
                    {
                        result.emplace_back(Token::VarKeyword);
                    }
                    else if (value == "as")
                    {
                        result.emplace_back(Token::AsKeyword);
                    }
                    else if (value == "or")
                    {
                        result.emplace_back(Token::OrKeyword);
                    }
                    else if (value == "and")
                    {
                        result.emplace_back(Token::AndKeyword);
                    }
                    else if (value == "return")
                    {
                        result.emplace_back(Token::ReturnKeyword);
                    }
                    else
                    {
                        result.emplace_back(Token::Identifier, std::move(value));
                    }
                    break;
                }
                std::cerr << "Unexpected character: " << character;
                std::abort();
            }
        }
    }
    return result;
}
