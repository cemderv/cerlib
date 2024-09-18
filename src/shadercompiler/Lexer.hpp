// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#include "shadercompiler/Token.hpp"
#include "util/InternalExport.hpp"

#include <array>
#include <vector>

namespace cer::shadercompiler
{
namespace keyword
{
constexpr std::string_view struct_{"struct"};
constexpr std::string_view return_{"return"};
constexpr std::string_view var{"var"};
constexpr std::string_view const_{"const"};
constexpr std::string_view for_{"for"};
constexpr std::string_view if_{"if"};
constexpr std::string_view in{"in"};
constexpr std::string_view else_{"else"};
constexpr std::string_view true_{"true"};
constexpr std::string_view false_{"false"};
constexpr std::string_view include{"include"};

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

CERLIB_API_INTERNAL void do_lexing(std::string_view    code,
                                   std::string_view    filename_hint,
                                   bool                do_post_processing,
                                   std::vector<Token>& tokens);

CERLIB_API_INTERNAL void assemble_tokens(std::string_view code, std::vector<Token>& tokens);

CERLIB_API_INTERNAL void remove_unnecessary_tokens(std::vector<Token>& tokens);
} // namespace cer::shadercompiler
