// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "shadercompiler/Writer.hpp"
#include <snitch/snitch.hpp>

using namespace cer;
using namespace cer::shadercompiler;

TEST_CASE("Shader writer", "[shaderc]")
{
    SECTION("Mixed writing")
    {
        auto writer = Writer{};
        writer << "Hello";
        REQUIRE(writer.buffer() == "Hello");
        writer << ' ';
        REQUIRE(writer.buffer() == "Hello ");
        writer << "World" << WNewline;
        REQUIRE(writer.buffer() == "Hello World\n");
        writer << 1 << 'x' << true << ' ' << false << 2u;
        REQUIRE(writer.buffer() == "Hello World\n1xtrue false2");
    }

    SECTION("Braces")
    {
        auto w = Writer{};
        w << "func Test() ";
        w.open_brace();
        w << "Hello" << WNewline;
        w.close_brace();

        REQUIRE(w.buffer() == "func Test() {\n  Hello\n}");
    }

    SECTION("Clearing")
    {
        auto w = Writer{};
        w << "Hello World";
        REQUIRE(w.buffer() == "Hello World");
        w.clear();
        REQUIRE(w.buffer() == "");
    }
}
