#include "resp_parser.h"

#include <gtest/gtest.h>
#include <sstream>

TEST(Resp, SimpleStringHappyPath) {
    std::istringstream input("+OK\r\n");
    const auto output = resp::decode(input);
    EXPECT_EQ("OK", std::get<resp::SimpleString>(*output).value);
}

TEST(Resp, SimpleStringEmptyInvalidString) {
    std::istringstream input("");
    const auto output = resp::decode(input);
    EXPECT_EQ(std::nullopt, output);
}

TEST(Resp, SimpleStringNoCrlf) {
    std::istringstream input("+");
    const auto output = resp::decode(input);
    EXPECT_EQ(std::nullopt, output);
}

TEST(Resp, SimpleStringCrNoLf) {
    std::istringstream input("+\r");
    const auto output = resp::decode(input);
    EXPECT_EQ(std::nullopt, output);
}

TEST(Resp, SimpleStringEmptyValidString) {
    std::istringstream input("+\r\n");
    const auto output = resp::decode(input);
    EXPECT_EQ("", std::get<resp::SimpleString>(*output).value);
}

TEST(Resp, SimpleStringTrailingInput) {
    std::istringstream input("+\r\nTRAILING");
    const auto output = resp::decode(input);
    EXPECT_EQ("", std::get<resp::SimpleString>(*output).value);
}

TEST(Resp, SimpleStringUnexpectedPrefix) {
    std::istringstream input("UNEXPECTED+ERR\r\n");
    const auto output = resp::decode(input);
    EXPECT_EQ(std::nullopt, output);
}

TEST(Resp, SimpleStringMultipleMessagesNotAllowed) {
    std::istringstream input("+OK\r\n+MORE\r\n");
    const auto a = resp::decode(input);
    const auto b = resp::decode(input);
    EXPECT_EQ("OK", std::get<resp::SimpleString>(*a).value);
    EXPECT_EQ("MORE", std::get<resp::SimpleString>(*b).value);
}

TEST(Resp, SimpleErrorEmpty) {
    std::istringstream input("-\r\n");
    const auto output = resp::decode(input);
    const auto value = std::get<resp::SimpleError>(*output).value;
    EXPECT_EQ("", value);
}

TEST(Resp, SimpleErrorEmptyPrefix) {
    std::istringstream input("- value\r\n");
    const auto output = resp::decode(input);
    const auto value = std::get<resp::SimpleError>(*output).value;
    EXPECT_EQ(" value", value);
}

TEST(Resp, SimpleErrorEmptyValue) {
    std::istringstream input("-PREFIX\r\n");
    const auto output = resp::decode(input);
    const auto value = std::get<resp::SimpleError>(*output).value;
    EXPECT_EQ("PREFIX", value);
}

TEST(Resp, SimpleErrorHappyPath) {
    std::istringstream input("-PREFIX value\r\n");
    const auto output = resp::decode(input);
    const auto value = std::get<resp::SimpleError>(*output).value;
    EXPECT_EQ("PREFIX value", value);
}

TEST(Resp, SimpleErrorEmptyValueWithSpaceAfterPrefix) {
    std::istringstream input("-PREFIX \r\n");
    const auto output = resp::decode(input);
    const auto value = std::get<resp::SimpleError>(*output).value;
    EXPECT_EQ("PREFIX ", value);
}

TEST(Resp, SimpleErrorNoCrlf) {
    std::istringstream input("-PREFIX");
    const auto output = resp::decode(input);
    EXPECT_EQ(std::nullopt, output);
}

TEST(Resp, SimpleErrorCrNoLf) {
    std::istringstream input("-PREFIX\r");
    const auto output = resp::decode(input);
    EXPECT_EQ(std::nullopt, output);
}

TEST(Resp, IntegerNoSign) {
    std::istringstream input(":0\r\n");
    const auto output = resp::decode(input);
    EXPECT_EQ(0, std::get<resp::Integer>(*output).value);
}

TEST(Resp, IntegerWithPlus) {
    std::istringstream input(":+1\r\n");
    const auto output = resp::decode(input);
    EXPECT_EQ(1, std::get<resp::Integer>(*output).value);
}

TEST(Resp, IntegerWithMinus) {
    std::istringstream input(":-1\r\n");
    const auto output = resp::decode(input);
    EXPECT_EQ(-1, std::get<resp::Integer>(*output).value);
}

TEST(Resp, IntegerEmptyWithPlus) {
    std::istringstream input(":+\r\n");
    const auto output = resp::decode(input);
    EXPECT_EQ(std::nullopt, output);
}

TEST(Resp, IntegerEmptyWithMinus) {
    std::istringstream input(":-\r\n");
    const auto output = resp::decode(input);
    EXPECT_EQ(std::nullopt, output);
}

TEST(Resp, IntegerEmpty) {
    std::istringstream input(":\r\n");
    const auto output = resp::decode(input);
    EXPECT_EQ(std::nullopt, output);
}

TEST(Resp, BulkStringHappyPath) {
    std::istringstream input("$5\r\nhello\r\n");
    const auto output = resp::decode(input);
    EXPECT_EQ("hello", std::get<resp::BulkString>(*output).value.value());
}

TEST(Resp, BulkStringNull) {
    std::istringstream input("$-1\r\n");
    const auto output = resp::decode(input);
    EXPECT_EQ(std::nullopt, std::get<resp::BulkString>(*output).value);
}

TEST(Resp, BulkStringEmpty) {
    std::istringstream input("$0\r\n\r\n");
    const auto output = resp::decode(input);
    EXPECT_EQ("", std::get<resp::BulkString>(*output).value);
}

TEST(Resp, BulkStringNoCrlf) {
    std::istringstream input("$5\r\nhello");
    const auto output = resp::decode(input);
    EXPECT_EQ(std::nullopt, output);
}

TEST(Resp, ArrayEmpty) {
    std::istringstream input("*0\r\n");
    const auto output = resp::decode(input);
    EXPECT_EQ(std::optional{std::vector<resp::Value>{}},
              std::get<resp::Array>(*output).value);
}

TEST(Resp, ArrayNull) {
    std::istringstream input("*-1\r\n");
    const auto output = resp::decode(input);
    EXPECT_EQ(std::nullopt, std::get<resp::Array>(*output).value);
}

TEST(Resp, ArraySimpleStrings) {
    std::istringstream input("*2\r\n+OK\r\n+PONG\r\n");
    const auto output = resp::decode(input);
    const auto expected = resp::Array{
        std::vector<resp::Value>{
            resp::SimpleString{"OK"},
            resp::SimpleString{"PONG"}
        }
    };

    EXPECT_EQ(
        expected, std::get<resp::Array>(*output)
    );
}

TEST(Resp, ArraySimpleErrors) {
    std::istringstream input("*2\r\n-ERR one\r\n-ERR two\r\n");
    const auto output = resp::decode(input);
    const auto expected = resp::Array{
        std::vector<resp::Value>{
            resp::SimpleError{"ERR one"},
            resp::SimpleError{"ERR two"},
        }
    };

    EXPECT_EQ(
        expected, std::get<resp::Array>(*output)
    );
}

TEST(Resp, ArrayIntegers) {
    std::istringstream input("*3\r\n:1\r\n:42\r\n:-5\r\n");
    const auto output = resp::decode(input);
    const auto expected = resp::Array{
        std::vector<resp::Value>{
            resp::Integer{1},
            resp::Integer{42},
            resp::Integer{-5},
        }
    };

    EXPECT_EQ(
        expected, std::get<resp::Array>(*output)
    );
}

TEST(Resp, ArrayBulkStrings) {
    std::istringstream input("*2\r\n$3\r\nfoo\r\n$3\r\nbar\r\n");
    const auto output = resp::decode(input);
    const auto expected = resp::Array{
        std::vector<resp::Value>{
            resp::BulkString{"foo"},
            resp::BulkString{"bar"},
        }
    };

    EXPECT_EQ(
        expected, std::get<resp::Array>(*output)
    );
}

TEST(Resp, ArrayNestedArrays) {
    std::istringstream input("*2\r\n*2\r\n+Hello\r\n+World\r\n*1\r\n:100\r\n");
    const auto output = resp::decode(input);
    const auto expected = resp::Array{
        std::vector<resp::Value>{
            resp::Array{
                std::vector<resp::Value>{
                    resp::SimpleString{"Hello"},
                    resp::SimpleString{"World"},
                }
            },
            resp::Array{
                std::vector<resp::Value>{
                    resp::Integer{100},
                }
            },
        }
    };

    EXPECT_EQ(
        expected, std::get<resp::Array>(*output)
    );
}

TEST(Resp, ArrayMixedVariants) {
    std::istringstream input("*4\r\n+Hi\r\n:123\r\n$5\r\nhello\r\n-Oops\r\n");
    const auto output = resp::decode(input);
    const auto expected = resp::Array{
        std::vector<resp::Value>{
            resp::SimpleString{"Hi"},
            resp::Integer{123},
            resp::BulkString{"hello"},
            resp::SimpleError{"Oops"},
        }
    };

    EXPECT_EQ(
        expected, std::get<resp::Array>(*output)
    );
}

TEST(Resp, ArrayInvalidVariant) {
    std::istringstream input("*4\r\n+Hi\r\n:123\r\n");
    const auto output = resp::decode(input);

    EXPECT_EQ(
        std::nullopt, output
    );
}
