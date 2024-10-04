// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "shadercompiler/Token.hpp"

namespace cer::shadercompiler
{
auto token_type_to_string(TokenType type) -> std::string_view
{
    switch (type)
    {
        case TokenType::IntLiteral: return "<int>";
        case TokenType::UIntLiteral: return "<uint>";
        case TokenType::FloatLiteral: return "<float>";
        case TokenType::ScientificNumber: return "<scientific_number>";
        case TokenType::HexNumber: return "<hex_number>";
        case TokenType::ForwardSlash: return "/";
        case TokenType::Colon: return ":";
        case TokenType::Semicolon: return ";";
        case TokenType::LeftBrace: return "{";
        case TokenType::RightBrace: return "}";
        case TokenType::Comma: return ",";
        case TokenType::Dot: return ".";
        case TokenType::DotDot: return "..";
        case TokenType::LeftParen: return "(";
        case TokenType::RightParen: return ")";
        case TokenType::Hyphen: return "-";
        case TokenType::Identifier: return "<id>";
        case TokenType::LeftAngleBracket: return "<";
        case TokenType::RightAngleBracket: return ">";
        case TokenType::Asterisk: return "*";
        case TokenType::NumberSign: return "#";
        case TokenType::LeftBracket: return "[";
        case TokenType::RightBracket: return "]";
        case TokenType::At: return "@";
        case TokenType::Hat: return "^";
        case TokenType::Ampersand: return "&";
        case TokenType::Bar: return "|";
        case TokenType::Percent: return "%";
        case TokenType::ExclamationMark: return "!";
        case TokenType::Plus: return "+";
        case TokenType::Equal: return "=";
        case TokenType::DoubleQuote: return "\"";
        case TokenType::SingleQuote: return "\'";
        case TokenType::QuestionMark: return "?";
        case TokenType::LeftShift: return "<<";
        case TokenType::RightShift: return ">>";
        case TokenType::LessThanOrEqual: return "<=";
        case TokenType::GreaterThanOrEqual: return ">=";
        case TokenType::LogicalEqual: return "==";
        case TokenType::LogicalNotEqual: return "!=";
        case TokenType::LogicalAnd: return "&&";
        case TokenType::LogicalOr: return "||";
        case TokenType::CompoundAdd: return "+=";
        case TokenType::CompoundSubtract: return "-=";
        case TokenType::CompoundMultiply: return "*=";
        case TokenType::CompoundDivide: return "/=";
        case TokenType::RightArrow: return "->";
        case TokenType::Keyword: return "<keyword>";
        case TokenType::BeginningOfFile: return "<bof>";
        case TokenType::EndOfFile: return "<eof>";
    }
    return "<unknown>";
}
} // namespace cer::shadercompiler
