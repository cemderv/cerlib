// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include "shadercompiler/Token.hpp"
#include <array>
#include <cerlib/List.hpp>

namespace cer::shadercompiler
{
namespace keyword
{
constexpr auto struct_ = std::string_view{"struct"};
constexpr auto return_ = std::string_view{"return"};
constexpr auto var     = std::string_view{"var"};
constexpr auto const_  = std::string_view{"const"};
constexpr auto for_    = std::string_view{"for"};
constexpr auto if_     = std::string_view{"if"};
constexpr auto in      = std::string_view{"in"};
constexpr auto else_   = std::string_view{"else"};
constexpr auto true_   = std::string_view{"true"};
constexpr auto false_  = std::string_view{"false"};
constexpr auto include = std::string_view{"include"};

static constexpr auto list = std::array{
    struct_,
    return_,
    var,
    const_,
    for_,
    if_,
    in,
    else_,
    true_,
    false_,
    include,
};
} // namespace keyword

void do_lexing(std::string_view code,
               std::string_view filename_hint,
               bool             do_post_processing,
               List<Token>&     tokens);

void assemble_tokens(std::string_view code, List<Token>& tokens);

void remove_unnecessary_tokens(List<Token>& tokens);
} // namespace cer::shadercompiler
