// Copyright (C) 2023-2024 Cemalettin Dervis
// This file is part of cerlib.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "shadercompiler/Lexer.hpp"
#include <ostream>
#include <snitch/snitch.hpp>
#include <span>

using namespace cer;
using namespace cer::shadercompiler;

std::ostream& operator<<(std::ostream& os, TokenType tt)
{
    os << token_type_to_string(tt);
    return os;
}

static constexpr std::string_view filename = "SomeFile";

constexpr std::string_view mock_code = R"(
Id1_: _id1230 -+09>"P{}[]
<> ;'!!2345 01-9081!!}";'w
0123 abcd ":" ?
SV_Position() -> <- {
1.0 2.0 -3.0 // C!@#%^&*()_+-=
} 1.23 4.56 2.3283e-10 1.23e+10 0x5555 0x0F0F 0xAA 0x0 0xA 0xa 12u 34u
)";

static void check_tokens1(std::vector<Token>& tokens)
{
    REQUIRE(tokens.size() == 106u);
    REQUIRE(tokens.at(0).value == "Id1_");
    REQUIRE(tokens.at(1).value == ":");
    REQUIRE(tokens.at(2).value == "_id1230");
    REQUIRE(tokens.at(3).value == "-");
    REQUIRE(tokens.at(4).value == "+");
    REQUIRE(tokens.at(5).value == "09");
    REQUIRE(tokens.at(6).value == ">");
    REQUIRE(tokens.at(7).value == "\"");
    REQUIRE(tokens.at(8).value == "P");
    REQUIRE(tokens.at(9).value == "{");
    REQUIRE(tokens.at(10).value == "}");
    REQUIRE(tokens.at(11).value == "[");
    REQUIRE(tokens.at(12).value == "]");

    for (uint32_t i = 0; i <= 12; ++i)
    {
        REQUIRE(tokens[i].location.filename == filename);
        REQUIRE(tokens[i].location.line == 2);
    }
}

static void check_tokens2(std::vector<Token>& tokens)
{
    REQUIRE(tokens.at(13).value == "<");
    REQUIRE(tokens.at(14).value == ">");
    REQUIRE(tokens.at(15).value == ";");
    REQUIRE(tokens.at(16).value == "'");
    REQUIRE(tokens.at(17).value == "!");
    REQUIRE(tokens.at(18).value == "!");
    REQUIRE(tokens.at(19).value == "2345");
    REQUIRE(tokens.at(20).value == "01");
    REQUIRE(tokens.at(21).value == "-");
    REQUIRE(tokens.at(22).value == "9081");
    REQUIRE(tokens.at(23).value == "!");
    REQUIRE(tokens.at(24).value == "!");
    REQUIRE(tokens.at(25).value == "}");
    REQUIRE(tokens.at(26).value == "\"");
    REQUIRE(tokens.at(27).value == ";");
    REQUIRE(tokens.at(28).value == "'");
    REQUIRE(tokens.at(29).value == "w");

    for (uint32_t i = 13; i <= 29; ++i)
    {
        REQUIRE(tokens[i].location.filename == filename);
        REQUIRE(tokens[i].location.line == 3);
    }
}

static void check_tokens3(std::vector<Token>& tokens)
{
    REQUIRE(tokens.at(30).value == "0123");
    REQUIRE(tokens.at(31).value == "abcd");
    REQUIRE(tokens.at(32).value == "\"");
    REQUIRE(tokens.at(33).value == ":");
    REQUIRE(tokens.at(34).value == "\"");
    REQUIRE(tokens.at(35).value == "?");

    for (uint32_t i = 30; i <= 35; ++i)
    {
        REQUIRE(tokens[i].location.filename == filename);
        REQUIRE(tokens[i].location.line == 4);
    }

    REQUIRE(tokens.at(36).value == "SV_Position");
    REQUIRE(tokens.at(37).value == "(");
    REQUIRE(tokens.at(38).value == ")");
    REQUIRE(tokens.at(39).value == "-");
    REQUIRE(tokens.at(40).value == ">");
    REQUIRE(tokens.at(41).value == "<");
    REQUIRE(tokens.at(42).value == "-");
    REQUIRE(tokens.at(43).value == "{");
}

static void check_tokens4(std::vector<Token>& tokens)
{
    for (uint32_t i = 36; i <= 43; ++i)
    {
        REQUIRE(tokens[i].location.filename == filename);
        REQUIRE(tokens[i].location.line == 5);
    }

    REQUIRE(tokens.at(44).value == "1");
    REQUIRE(tokens.at(45).value == ".");
    REQUIRE(tokens.at(46).value == "0");
    REQUIRE(tokens.at(47).value == "2");
    REQUIRE(tokens.at(48).value == ".");
    REQUIRE(tokens.at(49).value == "0");
    REQUIRE(tokens.at(50).value == "-");
    REQUIRE(tokens.at(51).value == "3");
    REQUIRE(tokens.at(52).value == ".");
    REQUIRE(tokens.at(53).value == "0");
    REQUIRE(tokens.at(54).value == "/");
    REQUIRE(tokens.at(55).value == "/");
    REQUIRE(tokens.at(56).value == "C");
    REQUIRE(tokens.at(57).value == "!");
    REQUIRE(tokens.at(58).value == "@");
    REQUIRE(tokens.at(59).value == "#");
    REQUIRE(tokens.at(60).value == "%");
    REQUIRE(tokens.at(61).value == "^");
    REQUIRE(tokens.at(62).value == "&");
    REQUIRE(tokens.at(63).value == "*");
    REQUIRE(tokens.at(64).value == "(");
    REQUIRE(tokens.at(65).value == ")");
    REQUIRE(tokens.at(66).value == "_");
    REQUIRE(tokens.at(67).value == "+");
    REQUIRE(tokens.at(68).value == "-");
    REQUIRE(tokens.at(69).value == "=");
}

static void check_tokens5(std::vector<Token>& tokens)
{
    for (uint32_t i = 44; i <= 69; ++i)
    {
        REQUIRE(tokens[i].location.filename == filename);
        REQUIRE(tokens[i].location.line == 6);
    }

    REQUIRE(tokens.at(70).value == "}");
    REQUIRE(tokens.at(70).location.filename == filename);
    REQUIRE(tokens.at(70).location.line == 7);

    REQUIRE(tokens.at(77).value == "2");
    REQUIRE(tokens.at(78).value == ".");
    REQUIRE(tokens.at(79).value == "3283");
    REQUIRE(tokens.at(80).value == "e");
    REQUIRE(tokens.at(81).value == "-");
    REQUIRE(tokens.at(82).value == "10");
    REQUIRE(tokens.at(83).value == "1");
    REQUIRE(tokens.at(84).value == ".");
    REQUIRE(tokens.at(85).value == "23");
    REQUIRE(tokens.at(86).value == "e");
    REQUIRE(tokens.at(87).value == "+");
    REQUIRE(tokens.at(88).value == "10");
    REQUIRE(tokens.at(89).value == "0");
    REQUIRE(tokens.at(90).value == "x5555");
    REQUIRE(tokens.at(91).value == "0");
    REQUIRE(tokens.at(92).value == "x0F0F");
    REQUIRE(tokens.at(93).value == "0");
    REQUIRE(tokens.at(94).value == "xAA");
    REQUIRE(tokens.at(95).value == "0");
    REQUIRE(tokens.at(96).value == "x0");
    REQUIRE(tokens.at(97).value == "0");
    REQUIRE(tokens.at(98).value == "xA");
    REQUIRE(tokens.at(99).value == "0");
    REQUIRE(tokens.at(100).value == "xa");
    REQUIRE(tokens.at(101).value == "12");
    REQUIRE(tokens.at(102).value == "u");
    REQUIRE(tokens.at(103).value == "34");
    REQUIRE(tokens.at(104).value == "u");
}

static void check_tokens6(std::vector<Token>& tokens)
{
    REQUIRE(tokens.back().type == TokenType::EndOfFile);
    REQUIRE(tokens.back().value.empty());

    // Verify columns + start indices
    REQUIRE(tokens.at(0).location.column == 1); // Id1_
    REQUIRE(tokens.at(0).location.start_index == 1);
    REQUIRE(tokens.at(1).location.column == 5); // :
    REQUIRE(tokens.at(1).location.start_index == 5);
    REQUIRE(tokens.at(2).location.column == 7); // _id1230
    REQUIRE(tokens.at(2).location.start_index == 7);
    REQUIRE(tokens.at(3).location.column == 15); // -
    REQUIRE(tokens.at(3).location.start_index == 15);
    REQUIRE(tokens.at(4).location.column == 16); // +
    REQUIRE(tokens.at(4).location.start_index == 16);
    REQUIRE(tokens.at(5).location.column == 17); // 09
    REQUIRE(tokens.at(5).location.start_index == 17);
    REQUIRE(tokens.at(19).location.column == 8); // 2345
    REQUIRE(tokens.at(19).location.start_index == 34);
    REQUIRE(tokens.at(21).location.column == 15); // -
    REQUIRE(tokens.at(21).location.start_index == 41);
    REQUIRE(tokens.at(29).location.column == 26); // w
    REQUIRE(tokens.at(29).location.start_index == 52);
    REQUIRE(tokens.at(30).location.column == 1); // 0123
    REQUIRE(tokens.at(30).location.start_index == 54);
    REQUIRE(tokens.at(31).location.column == 6); // abcd
    REQUIRE(tokens.at(31).location.start_index == 59);
    REQUIRE(tokens.at(32).location.column == 11); // "
    REQUIRE(tokens.at(32).location.start_index == 64);
    REQUIRE(tokens.at(33).location.column == 12); // :
    REQUIRE(tokens.at(33).location.start_index == 65);
    REQUIRE(tokens.at(34).location.column == 13); // "
    REQUIRE(tokens.at(34).location.start_index == 66);
    REQUIRE(tokens.at(35).location.column == 15); // ?
    REQUIRE(tokens.at(35).location.start_index == 68);
    REQUIRE(tokens.at(36).location.column == 1); // SV_Position
    REQUIRE(tokens.at(36).location.start_index == 70);
    REQUIRE(tokens.at(37).location.column == 12); // (
    REQUIRE(tokens.at(37).location.start_index == 81);
    REQUIRE(tokens.at(38).location.column == 13); // )
    REQUIRE(tokens.at(38).location.start_index == 82);
    REQUIRE(tokens.at(39).location.column == 15); // -
    REQUIRE(tokens.at(39).location.start_index == 84);
    REQUIRE(tokens.at(40).location.column == 16); // >
    REQUIRE(tokens.at(40).location.start_index == 85);
    REQUIRE(tokens.at(41).location.column == 18); // <
    REQUIRE(tokens.at(41).location.start_index == 87);
    REQUIRE(tokens.at(42).location.column == 19); // -
    REQUIRE(tokens.at(42).location.start_index == 88);
    REQUIRE(tokens.at(43).location.column == 21); // {
    REQUIRE(tokens.at(43).location.start_index == 90);
    REQUIRE(tokens.at(44).location.column == 1); // 1
    REQUIRE(tokens.at(44).location.start_index == 92);
    REQUIRE(tokens.at(45).location.column == 2); // .
    REQUIRE(tokens.at(45).location.start_index == 93);
    REQUIRE(tokens.at(46).location.column == 3); // 0
    REQUIRE(tokens.at(46).location.start_index == 94);
    REQUIRE(tokens.at(47).location.column == 5); // 2
    REQUIRE(tokens.at(47).location.start_index == 96);
    REQUIRE(tokens.at(54).location.column == 14); // /
    REQUIRE(tokens.at(54).location.start_index == 105);
    REQUIRE(tokens.at(55).location.column == 15); // /
    REQUIRE(tokens.at(55).location.start_index == 106);
    REQUIRE(tokens.at(57).location.column == 18); // !
    REQUIRE(tokens.at(57).location.start_index == 109);
    REQUIRE(tokens.at(58).location.column == 19); // @
    REQUIRE(tokens.at(58).location.start_index == 110);
    REQUIRE(tokens.at(59).location.column == 20); // #
    REQUIRE(tokens.at(59).location.start_index == 111);
    REQUIRE(tokens.at(60).location.column == 21); // %
    REQUIRE(tokens.at(60).location.start_index == 112);
    REQUIRE(tokens.at(61).location.column == 22); // ^
    REQUIRE(tokens.at(61).location.start_index == 113);
    REQUIRE(tokens.at(64).location.column == 25); // (
    REQUIRE(tokens.at(64).location.start_index == 116);
    REQUIRE(tokens.at(69).location.column == 30); // =
    REQUIRE(tokens.at(69).location.start_index == 121);
    REQUIRE(tokens.at(70).location.column == 1); // }
    REQUIRE(tokens.at(70).location.start_index == 123);
}

static void check_tokens7(std::vector<Token>& tokens)
{
    // Now assemble the single tokens into special token types.
    // E.g. tokens '-' and '>' become '->' (ArrowRight).
    // Or '1', '.' and '0' become '1.0' (IntLiteral)
    assemble_tokens(mock_code, tokens);

    REQUIRE_FALSE(tokens.empty());
    REQUIRE(tokens.back().is(TokenType::EndOfFile));
    REQUIRE(tokens.size() == 76u);

    REQUIRE(tokens.at(39).type == TokenType::RightArrow);
    REQUIRE(tokens.at(39).value == "->");
    REQUIRE(tokens.at(39).location.column == 15);
    REQUIRE(tokens.at(39).location.start_index == 84);

    REQUIRE(tokens.at(40).type == TokenType::LeftAngleBracket);
    REQUIRE(tokens.at(40).value == "<");
    REQUIRE(tokens.at(40).location.column == 18);
    REQUIRE(tokens.at(40).location.start_index == 87);

    REQUIRE(tokens.at(41).type == TokenType::Hyphen);
    REQUIRE(tokens.at(41).value == "-");
    REQUIRE(tokens.at(41).location.column == 19);
    REQUIRE(tokens.at(41).location.start_index == 88);

    REQUIRE(tokens.at(42).type == TokenType::LeftBrace);
    REQUIRE(tokens.at(42).value == "{");
    REQUIRE(tokens.at(42).location.column == 21);
    REQUIRE(tokens.at(42).location.start_index == 90);

    // 1.0
    REQUIRE(tokens.at(43).type == TokenType::FloatLiteral);
    REQUIRE(tokens.at(43).value == "1.0");
    REQUIRE(tokens.at(43).location.column == 1);
    REQUIRE(tokens.at(43).location.start_index == 92);

    // 2.0
    REQUIRE(tokens.at(44).type == TokenType::FloatLiteral);
    REQUIRE(tokens.at(44).value == "2.0");
    REQUIRE(tokens.at(44).location.column == 5);
    REQUIRE(tokens.at(44).location.start_index == 96);

    // 3.0
    REQUIRE(tokens.at(46).type == TokenType::FloatLiteral);
    REQUIRE(tokens.at(46).value == "3.0");
    REQUIRE(tokens.at(46).location.column == 10);
    REQUIRE(tokens.at(46).location.start_index == 101);

    // 1.23
    REQUIRE(tokens.at(63).type == TokenType::FloatLiteral);
    REQUIRE(tokens.at(63).value == "1.23");
    REQUIRE(tokens.at(63).location.column == 3);
    REQUIRE(tokens.at(63).location.start_index == 125);

    // 4.56
    REQUIRE(tokens.at(64).type == TokenType::FloatLiteral);
    REQUIRE(tokens.at(64).value == "4.56");
    REQUIRE(tokens.at(64).location.column == 8);
    REQUIRE(tokens.at(64).location.start_index == 130);
}

static void check_tokens8(std::vector<Token>& tokens)
{
    REQUIRE(tokens.at(65).type == TokenType::ScientificNumber);
    REQUIRE(tokens.at(65).value == "2.3283e-10");

    REQUIRE(tokens.at(66).type == TokenType::ScientificNumber);
    REQUIRE(tokens.at(66).value == "1.23e+10");

    REQUIRE(tokens.at(67).type == TokenType::HexNumber);
    REQUIRE(tokens.at(67).value == "0x5555");

    REQUIRE(tokens.at(68).type == TokenType::HexNumber);
    REQUIRE(tokens.at(68).value == "0x0F0F");

    REQUIRE(tokens.at(69).type == TokenType::HexNumber);
    REQUIRE(tokens.at(69).value == "0xAA");

    REQUIRE(tokens.at(70).type == TokenType::HexNumber);
    REQUIRE(tokens.at(70).value == "0x0");

    REQUIRE(tokens.at(71).type == TokenType::HexNumber);
    REQUIRE(tokens.at(71).value == "0xA");

    REQUIRE(tokens.at(72).type == TokenType::HexNumber);
    REQUIRE(tokens.at(72).value == "0xa");

    REQUIRE(tokens.at(73).type == TokenType::UIntLiteral);
    REQUIRE(tokens.at(73).value == "12u");

    REQUIRE(tokens.at(74).type == TokenType::UIntLiteral);
    REQUIRE(tokens.at(74).value == "34u");
}

static void check_tokens9(std::vector<Token>& tokens)
{
    // Remove unnecessary tokens such as comments.
    remove_unnecessary_tokens(tokens);

    REQUIRE(tokens.size() == 61u);
    REQUIRE(tokens.at(46).value == "3.0");
    REQUIRE(tokens.at(47).value == "}");
    REQUIRE(tokens.at(47).location.line == 7);
    REQUIRE(tokens.at(47).location.column == 1);
    REQUIRE(tokens.at(47).location.start_index == 123);
    REQUIRE(tokens.back().type == TokenType::EndOfFile);
}

TEST_CASE("Shader lexer", "[shaderc]")
{
    std::vector<Token> tokens;

    REQUIRE_THROWS_AS(do_lexing("", "", false, tokens), std::invalid_argument);

    {
        tokens.clear();
        do_lexing(mock_code, filename, false, tokens);

        check_tokens1(tokens);
        check_tokens2(tokens);
        check_tokens3(tokens);
        check_tokens4(tokens);
        check_tokens5(tokens);
        check_tokens6(tokens);
        check_tokens7(tokens);
        check_tokens8(tokens);
        check_tokens9(tokens);
    }
}
