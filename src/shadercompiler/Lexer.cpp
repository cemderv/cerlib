// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "shadercompiler/Lexer.hpp"
#include "shadercompiler/Error.hpp"
#include <algorithm>
#include <cassert>
#include <cerlib/InternalError.hpp>
#include <optional>
#include <span>

namespace cer::shadercompiler
{
using TokenIterator = List<Token>::iterator;

enum class CharClassification
{
    Digit,
    Letter,
    Symbol,
};

static auto is_digit(char ch) -> bool
{
    return ch >= '0' && ch <= '9';
}

static auto is_letter(char ch) -> bool
{
    return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || ch == '_';
}

static auto is_symbol(char ch) -> bool
{
    return !is_digit(ch) && !is_letter(ch);
}

static auto is_identifier(std::string_view str) -> bool
{
    if (const char first = str.front(); is_digit(first) || !is_letter(first))
    {
        return false;
    }

    for (size_t i = 1, size = str.size(); i < size; ++i)
    {
        if (is_symbol(str[i]))
        {
            return false;
        }
    }

    return true;
}

static auto is_keyword(std::string_view str) -> bool
{
    return std::ranges::find(keyword::list, str) != keyword::list.cend();
}

static auto get_single_char_token_type(char ch) -> std::optional<TokenType>
{
    if (is_digit(ch))
    {
        return TokenType::IntLiteral;
    }

    switch (ch)
    {
        case '/': return TokenType::ForwardSlash;
        case ':': return TokenType::Colon;
        case ';': return TokenType::Semicolon;
        case '{': return TokenType::LeftBrace;
        case '}': return TokenType::RightBrace;
        case ',': return TokenType::Comma;
        case '.': return TokenType::Dot;
        case '(': return TokenType::LeftParen;
        case ')': return TokenType::RightParen;
        case '-': return TokenType::Hyphen;
        case '<': return TokenType::LeftAngleBracket;
        case '>': return TokenType::RightAngleBracket;
        case '*': return TokenType::Asterisk;
        case '#': return TokenType::NumberSign;
        case '[': return TokenType::LeftBracket;
        case ']': return TokenType::RightBracket;
        case '@': return TokenType::At;
        case '^': return TokenType::Hat;
        case '&': return TokenType::Ampersand;
        case '|': return TokenType::Bar;
        case '%': return TokenType::Percent;
        case '!': return TokenType::ExclamationMark;
        case '+': return TokenType::Plus;
        case '=': return TokenType::Equal;
        case '\"': return TokenType::DoubleQuote;
        case '\'': return TokenType::SingleQuote;
        case '?': return TokenType::QuestionMark;
        default: return {};
    }
}

static auto get_char_classification(char ch) -> CharClassification
{
    if (is_digit(ch))
    {
        return CharClassification::Digit;
    }

    if (is_letter(ch))
    {
        return CharClassification::Letter;
    }

    return CharClassification::Symbol;
}

static auto should_ignore_char(char ch) -> bool
{
    return ch == '\r' || ch == '\t';
}

static auto is_int(std::string_view str) -> bool
{
    return std::ranges::all_of(str, is_digit);
}

static auto get_trimmed(std::string_view str) -> std::string_view
{
    const auto should_remove = [](char ch) {
        return ch == ' ' || ch == '\r' || ch == '\n' || ch == '\t';
    };

    while (!str.empty() && should_remove(str.back()))
    {
        str.remove_suffix(1);
    }

    while (!str.empty() && should_remove(str.front()))
    {
        str.remove_prefix(1);
    }

    // ReSharper disable once CppDFALocalValueEscapesFunction
    return str;
}

static auto determine_token_type(const SourceLocation& location, std::string_view value)
    -> TokenType
{
    auto type = std::optional<TokenType>{};

    if (value.size() == 1)
    {
        type = get_single_char_token_type(value.front());

        if (!type)
        {
            if (is_identifier(value))
            {
                type = TokenType::Identifier;
            }
            else if (is_keyword(value))
            {
                type = TokenType::Keyword;
            }
        }
    }
    else if (is_keyword(value))
    {
        type = TokenType::Keyword;
    }
    else if (is_identifier(value))
    {
        type = TokenType::Identifier;
    }
    else if (is_int(value))
    {
        type = TokenType::IntLiteral;
    }

    if (!type)
    {
        throw Error{location, "invalid token '{}'", value};
    }

    return *type;
}

void do_lexing(std::string_view code,
               std::string_view filename_hint,
               bool             do_post_processing,
               List<Token>&     tokens)
{
    if (code.empty())
    {
        CER_THROW_INVALID_ARG_STR("No source code provided.");
    }

    tokens.clear();
    tokens.emplace_back(TokenType::BeginningOfFile, std::string_view(), SourceLocation());

    auto previous_token_index  = size_t(0);
    auto previous_token_column = size_t(1);

    auto line   = 1u;
    auto column = 1u;

    auto previous_classification = get_char_classification(code.front());

    auto is_currently_in_identifier_token =
        previous_classification == CharClassification::Letter || code.front() == '_';

    for (size_t i = 0, size = code.size(); i < size; ++i)
    {
        const auto ch             = code[i];
        const auto classification = get_char_classification(ch);
        auto       should_cut     = classification != previous_classification;

        if (ch != '_' && classification == CharClassification::Symbol)
        {
            is_currently_in_identifier_token = false;
            should_cut                       = true;
        }

        if (should_cut && is_currently_in_identifier_token)
        {
            should_cut = false;
        }

        if (i > 0 && should_cut && !should_ignore_char(ch))
        {
            const auto value =
                get_trimmed(code.substr(previous_token_index, i - previous_token_index));

            if (value.size() == 1 && value[0] == '\0')
            {
                break;
            }

            if (!value.empty())
            {
                const auto location = SourceLocation{filename_hint,
                                                     uint16_t(line),
                                                     uint16_t(previous_token_column),
                                                     uint16_t(previous_token_index)};

                tokens.emplace_back(determine_token_type(location, value), value, location);
            }

            previous_token_index  = i;
            previous_token_column = column;
            is_currently_in_identifier_token =
                classification == CharClassification::Letter || ch == '_';
        }

        if (ch == '\n')
        {
            ++line;
            column = 0;
        }

        previous_classification = classification;
        ++column;
    }

    if (do_post_processing)
    {
        assemble_tokens(code, tokens);
        remove_unnecessary_tokens(tokens);
    }

    tokens.erase(tokens.begin());

    tokens.emplace_back(TokenType::EndOfFile, std::string_view(), SourceLocation());
}

static auto are_tokens_neighbors(std::span<const TokenIterator> tokens) -> bool
{
    assert(tokens.size() > 1);

    for (size_t i = 1; i < tokens.size(); ++i)
    {
        const auto& prev_token    = tokens[i - 1];
        const auto& current_token = tokens[i];

        if (prev_token->location.line != current_token->location.line)
        {
            return false;
        }

        if (current_token->location.start_index !=
            prev_token->location.start_index + prev_token->value.size())
        {
            return false;
        }
    }

    return true;
}

// Checks whether a string represents a valid hexadecimal suffix (the part that follows
// '0x').
static auto is_hex_suffix(std::string_view str) -> bool
{
    auto len = str.size();

    if (len == 9)
    {
        if (str[8] != 'u')
        {
            return false;
        }

        --len;
    }
    else if (len > 8)
    {
        return false;
    }

    const auto is_valid = [](char ch) {
        return (ch >= 'a' && ch <= 'f') || (ch >= '0' && ch <= '9') || (ch >= 'A' && ch <= 'F');
    };

    for (size_t i = 0; i < len; ++i)
    {
        if (!is_valid(str[i]))
        {
            return false;
        }
    }

    return true;
}

static auto merge_tokens(std::string_view code,
                         List<Token>&     tokens,
                         TokenIterator    first,
                         TokenIterator    last,
                         TokenType        result_type) -> TokenIterator
{
    assert(first < last);

    const auto& first_location = first->location;
    const auto& last_location  = last->location;

    // Verify that both tokens are in the same translation unit.
    assert(first_location.filename == last_location.filename);

    const auto start_index = first_location.start_index;
    const auto end_index   = last_location.start_index + last->value.size();

    first->type  = result_type;
    first->value = code.substr(start_index, end_index - start_index);

    return tokens.erase(first + 1, last + 1);
}

static void assemble_int_literals(std::string_view code, List<Token>& tokens)
{
    for (auto tk0 = tokens.begin(); tokens.size() >= 3 && tk0 < tokens.end() - 2; ++tk0)
    {
        if (const auto tk1 = tk0 + 1; tk0->is(TokenType::IntLiteral) && tk1->is(TokenType::Dot))
        {
            auto tk2         = tk0 + 2;
            auto tk_previous = tk1;
            auto tk_next     = tk2;
            auto tk_last     = tk_previous;

            while (tk_next->is(TokenType::IntLiteral) &&
                   tk_next->location.start_index ==
                       tk_previous->location.start_index + tk_previous->value.size())
            {
                ++tk2;
                tk_previous = tk_next;
                tk_last     = tk_next;

                if (tk2 >= tokens.end())
                {
                    break;
                }

                tk_next = tk2;
            }

            if (tk_last != tk1)
            {
                // Got a legit number.
                merge_tokens(code, tokens, tk0, tk_last, TokenType::FloatLiteral);
            }
        }
    }
}

static void assemble_uint_literals(std::string_view code, List<Token>& tokens)
{
    for (auto tk0 = tokens.begin(); tokens.size() >= 2 && tk0 < tokens.end() - 2; ++tk0)
    {
        const auto tk1 = tk0 + 1;

        if (!are_tokens_neighbors({{tk0, tk1}}))
        {
            continue;
        }

        if (tk0->is(TokenType::IntLiteral) && tk1->value == "u")
        {
            merge_tokens(code, tokens, tk0, tk1, TokenType::UIntLiteral);
        }
    }
}

static void assemble_scientific_numbers(std::string_view code, List<Token>& tokens)
{
    // format: (<float>|<int>)'e'('+'|'-')<int>

    for (auto tk0 = tokens.begin(); tokens.size() >= 4 && tk0 < tokens.end() - 4; ++tk0)
    {
        const auto tk1 = tk0 + 1; // 'e'
        const auto tk2 = tk0 + 2; // '+'|'-'
        const auto tk3 = tk0 + 3; // <int>

        if (!are_tokens_neighbors({{tk0, tk1, tk2, tk3}}))
        {
            continue;
        }

        if ((tk0->is(TokenType::FloatLiteral) || tk0->is(TokenType::IntLiteral)) &&
            tk1->value == "e" && (tk2->value == "+" || tk2->value == "-") &&
            tk3->is(TokenType::IntLiteral))
        {
            merge_tokens(code, tokens, tk0, tk3, TokenType::ScientificNumber);
        }
    }
}

static void assemble_hex_numbers(std::string_view code, List<Token>& tokens)
{
    for (auto tk0 = tokens.begin(); tokens.size() >= 2 && tk0 < tokens.end() - 2; ++tk0)
    {
        const auto tk1 = tk0 + 1;

        if (!are_tokens_neighbors({{tk0, tk1}}))
        {
            continue;
        }

        if (tk0->value == "0" && tk1->is(TokenType::Identifier) && tk1->value[0] == 'x')
        {
            // Verify that the 'x...' part represents a valid hexadecimal number.
            if (const auto suffix = tk1->value.substr(1); !is_hex_suffix(suffix))
            {
                throw Error{tk0->location, "expected a valid hexadecimal number"};
            }

            merge_tokens(code, tokens, tk0, tk1, TokenType::HexNumber);
        }
    }
}

/**
 * \brief Assembles single-char tokens to multi-char tokens, e.g. '<' and '=' become
 * '<=' (less_than_or_equal).
 */
static void assemble_multi_char_tokens(std::string_view code, List<Token>& tokens)
{
    struct TokenTransform
    {
        TokenType first;
        TokenType second;
        TokenType result;
    };

    static constexpr auto s_transformations = std::array{
        TokenTransform{.first  = TokenType::LeftAngleBracket,
                       .second = TokenType::LeftAngleBracket,
                       .result = TokenType::LeftShift}, // <<
        TokenTransform{.first  = TokenType::RightAngleBracket,
                       .second = TokenType::RightAngleBracket,
                       .result = TokenType::RightShift}, // >>
        TokenTransform{.first  = TokenType::LeftAngleBracket,
                       .second = TokenType::Equal,
                       .result = TokenType::LessThanOrEqual}, // <=
        TokenTransform{.first  = TokenType::RightAngleBracket,
                       .second = TokenType::Equal,
                       .result = TokenType::GreaterThanOrEqual}, // >=
        TokenTransform{.first  = TokenType::Equal,
                       .second = TokenType::Equal,
                       .result = TokenType::LogicalEqual}, // ==
        TokenTransform{.first  = TokenType::ExclamationMark,
                       .second = TokenType::Equal,
                       .result = TokenType::LogicalNotEqual}, // !=
        TokenTransform{.first  = TokenType::Ampersand,
                       .second = TokenType::Ampersand,
                       .result = TokenType::LogicalAnd}, // &&
        TokenTransform{.first  = TokenType::Bar,
                       .second = TokenType::Bar,
                       .result = TokenType::LogicalOr}, // ||
        TokenTransform{.first  = TokenType::Plus,
                       .second = TokenType::Equal,
                       .result = TokenType::CompoundAdd}, // +=
        TokenTransform{.first  = TokenType::Hyphen,
                       .second = TokenType::Equal,
                       .result = TokenType::CompoundSubtract}, // -=
        TokenTransform{.first  = TokenType::Asterisk,
                       .second = TokenType::Equal,
                       .result = TokenType::CompoundMultiply}, // *=
        TokenTransform{.first  = TokenType::ForwardSlash,
                       .second = TokenType::Equal,
                       .result = TokenType::CompoundDivide}, // /=
        TokenTransform{.first  = TokenType::Dot,
                       .second = TokenType::Dot,
                       .result = TokenType::DotDot}, // ..
        TokenTransform{.first  = TokenType::Hyphen,
                       .second = TokenType::RightAngleBracket,
                       .result = TokenType::RightArrow}, // ->
    };

    if (tokens.empty())
    {
        return;
    }

    for (auto tk0 = tokens.begin(); tk0 < tokens.end() - 1; ++tk0)
    {
        const auto tk1 = tk0 + 1;

        if (const auto it = std::ranges::find_if(s_transformations,
                                                 [&](const auto& transform) {
                                                     return tk0->type == transform.first &&
                                                            tk1->type == transform.second;
                                                 });
            it != s_transformations.cend())
        {
            tk0 = merge_tokens(code, tokens, tk0, tk1, it->result);
            if (tk0 > tokens.begin())
            {
                --tk0;
            }
        }
    }
}

void assemble_tokens(std::string_view code, List<Token>& tokens)
{
    assemble_multi_char_tokens(code, tokens);
    assemble_int_literals(code, tokens);
    assemble_uint_literals(code, tokens);
    assemble_scientific_numbers(code, tokens);
    assemble_hex_numbers(code, tokens);
}

void remove_unnecessary_tokens(List<Token>& tokens)
{
    if (tokens.empty())
    {
        return;
    }

    for (auto tk0 = tokens.begin(); tokens.size() >= 2 && tk0 < tokens.end() - 2; ++tk0)
    {
        const auto tk1 = tk0 + 1;

        if (!are_tokens_neighbors({{tk0, tk1}}))
        {
            continue;
        }

        if (tk0->is(TokenType::ForwardSlash) && tk1->is(TokenType::ForwardSlash))
        {
            // Got a '//'. Remove everything that follows, until a new line begins.
            auto last_tk = tk1;

            while (last_tk < tokens.end() && last_tk->location.line == tk0->location.line)
            {
                ++last_tk;
            }

            tk0 = tokens.erase(tk0, last_tk);

            if (tk0 > tokens.begin())
            {
                --tk0;
            }
        }
    }
}
} // namespace cer::shadercompiler
