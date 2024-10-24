// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include "SourceLocation.hpp"
#include <cerlib/String.hpp>

namespace cer::shadercompiler
{
enum class TokenType
{
    BeginningOfFile = 0,
    IntLiteral,
    UIntLiteral,
    FloatLiteral,
    ScientificNumber,
    HexNumber,
    ForwardSlash,
    Colon,
    Semicolon,
    LeftBrace,
    RightBrace,
    Comma,
    Dot,
    DotDot,
    LeftParen,
    RightParen,
    Hyphen,
    Identifier,
    LeftAngleBracket,
    RightAngleBracket,
    Asterisk,
    NumberSign,
    LeftBracket,
    RightBracket,
    At,
    Hat,
    Ampersand,
    Bar,
    Percent,
    ExclamationMark,
    Plus,
    Equal,
    DoubleQuote,
    SingleQuote,
    QuestionMark,
    Keyword,
    LeftShift,
    RightShift,
    LessThanOrEqual,
    GreaterThanOrEqual,
    LogicalEqual,
    LogicalNotEqual,
    LogicalAnd,
    LogicalOr,
    CompoundAdd,
    CompoundSubtract,
    CompoundMultiply,
    CompoundDivide,
    RightArrow,
    EndOfFile,
};

struct Token
{
    constexpr Token(TokenType type, std::string_view value, const SourceLocation& location)
        : type(type)
        , value(value)
        , location(location)
    {
    }

    auto is(TokenType t) const -> bool
    {
        return this->type == t;
    }

    TokenType        type;
    std::string_view value;
    SourceLocation   location;
};

auto token_type_to_string(TokenType type) -> std::string_view;
} // namespace cer::shadercompiler
